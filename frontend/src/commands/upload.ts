import mqtt from "mqtt";
import vscode from "vscode";
import path from "path";
import * as converter from "../util/convert_elf";

const LAST_PICKED_DIRECTORY = "WIRELESS_UPLOADER_LAST_PICKED_DIRECTORY";

export async function upload(
    brokerUrl: string,
    brokerPort: number,
    uploadTopic: string,
    subscribeTopic: string,
    boardName: string,
    controlId: number,
    firmwareId: number,
    extensionContext: vscode.ExtensionContext
) {
    const previousDirectory = extensionContext.workspaceState.get<string>(LAST_PICKED_DIRECTORY);

    const file = await vscode.window.showOpenDialog({
        canSelectFiles: true,
        canSelectFolders: false,
        canSelectMany: false,
        defaultUri: previousDirectory ? vscode.Uri.file(previousDirectory) : undefined,
        openLabel: "Pick a binary file",
        filters: {
            "Firmware files": ["bin", "elf"]
        }
    });

    if (!file || file.length === 0) {
        return;
    }

    const directory = path.dirname(file[0].path);

    extensionContext.workspaceState.update(LAST_PICKED_DIRECTORY, directory);

    let binFile;
    try {
        binFile = await converter.getBinaryFile(file[0].path);
    } catch (e: any) {
        vscode.window.showErrorMessage(e.message);
        return;
    }

    const openedFile = await vscode.workspace.fs.readFile(vscode.Uri.file(binFile.filePath));

    const client = mqtt.connect({
        host: brokerUrl,
        port: brokerPort,
        protocol: 'mqtt'
    });

    client.on('connect', () => {
        client.subscribe(subscribeTopic);
    });

    const extensionConfig = vscode.workspace.getConfiguration("wirelessUploader");

    startProgressWindow(
        client,
        boardName,
        extensionConfig.get<number>("gatewayFirstConnectionTimeout", 10000),
        extensionConfig.get<number>("gatewaySubsequentTimeout", 30000)
    );

    const payload = addHeader(openedFile, controlId, firmwareId);

    client.publish(uploadTopic, Buffer.from(payload));

    cleanup(binFile);
}

function addHeader(payload: Uint8Array, controlId: number, firmwareId: number): Uint8Array {
    let buffer = new ArrayBuffer(4);
    let view = new DataView(buffer);

    view.setUint16(0, controlId, true);
    view.setUint16(2, firmwareId, true);

    let newPayload = new Uint8Array(payload.length + 4);

    newPayload.set(new Uint8Array(buffer), 0);
    newPayload.set(payload, 4);

    return newPayload;
}

function startProgressWindow(client: mqtt.MqttClient, boardName: string, timeout1Ms: number, timeout2Ms: number) {
    vscode.window.withProgress(
        {
            location: vscode.ProgressLocation.Notification,
            title: "Flashing [" + boardName + "]",
            cancellable: false
        },
        (progress, token) => {
            return new Promise<void>((resolve) => {
                // First connection timeout;
                let timeout = setTimeout(() => {
                    vscode.window.showErrorMessage("Couldn't connect to the gateway board");
                    finish();
                }, timeout1Ms);

                //Timeout if something went wrong during upload
                const resetTimeout = () => {
                    clearTimeout(timeout);
                    timeout = setTimeout(() => {
                        vscode.window.showErrorMessage("Gateway board became unresponsive");
                        finish();
                    }, timeout2Ms);
                };

                // Cleanly finish the function 
                let finished = false;
                const finish = () => {
                    finished = true;
                    clearTimeout(timeout);
                    client.end();
                    resolve();
                };

                let currentProgress = 0;
                progress.report({ message: "Waiting for the gateway board" });

                client.on('message', (topic, payload) => {
                    if (finished) {
                        return;
                    }
                    resetTimeout();

                    let message = payload.toString();
                    switch (message) {
                        case "Bootloader successfully started":
                            progress.report({ message: "Bootloader successfully started" });
                            break;

                        case "Flash setup completed":
                            progress.report({ increment: 0, message: "Flash setup completed" });
                            break;

                        case "App transmitted successfully":
                            vscode.window.showInformationMessage("Application flashed successfully");
                            finish();
                            break;

                        default:
                            if (!message.includes("App transmission progress")) {
                                vscode.window.showErrorMessage("Received unknown message: ", message);
                                break;
                            }
                            let starts = "App transmission progress".length;
                            let parts = message.substring(starts + 2, message.length - 1).split('/');

                            let newProgress = Number(parts[0]) / Number(parts[1]) * 100;
                            newProgress = Math.floor(newProgress);

                            progress.report({ increment: newProgress - currentProgress, message: "Transferred " + message.substring(starts + 1, message.length) });

                            currentProgress = newProgress;
                            break;
                    }
                });
            });
        }
    );



}

function cleanup(file: converter.BinFile) {
    // If .elf was converted to .bin
    if (file.delete) {
        vscode.workspace.fs.delete(vscode.Uri.file(file.filePath));
    }
}
