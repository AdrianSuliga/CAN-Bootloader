import mqtt from "mqtt";
import vscode from "vscode";
import path, { resolve } from "path";
import * as converter from "../util/convert_elf";
import { rejects } from "assert";

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

    const context = await vscode.workspace.fs.readFile(vscode.Uri.file(binFile.filePath));

    const client = mqtt.connect({
        host: brokerUrl,
        port: brokerPort,
        protocol: 'mqtt'
    });

    client.on('connect', () => {
        client.subscribe(subscribeTopic);
    });

    startProgressWindow(client, boardName);

    const payload = addHeader(context, controlId, firmwareId);

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

function startProgressWindow(client: mqtt.MqttClient, boardName: string) {
    vscode.window.withProgress(
        {
            location: vscode.ProgressLocation.Notification,
            title: "Flashing [" + boardName + "]",
            cancellable: false
        },
        (progress, token) => {
            return new Promise<void>((resolve) => {
                let currentProgress = 0;
                progress.report({ message: "Waiting for the gateway board" });

                client.on('message', (topic, payload) => {
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
                            resolve();
                            break;
                        default:
                            if (!message.includes("App transmission progres")) {
                                vscode.window.showErrorMessage("Received unknown message: ", message);
                                break;
                            }
                            let starts = "App transmission progres".length;
                            let parts = message.substring(starts + 3, message.length - 1).split('/');

                            let newProgress = Number(parts[0]) / Number(parts[1]) * 100;
                            newProgress = Math.floor(newProgress);

                            progress.report({ increment: newProgress - currentProgress, message: "Transferred " + message.substring(starts + 2, message.length) });

                            currentProgress = newProgress;
                            break;
                    }
                });
            });
        }
    );



}

function cleanup(file: converter.BinFile) {
    if (file.delete) {
        vscode.workspace.fs.delete(vscode.Uri.file(file.filePath));
    }
}
