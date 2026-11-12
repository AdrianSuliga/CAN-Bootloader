import mqtt from "mqtt";
import vscode from "vscode";
import path from "path";
import * as converter from "../util/convert_elf";

const LAST_PICKED_DIRECTORY = "WIRELESS_UPLOADER_LAST_PICKED_DIRECTORY";

export async function upload(
    brokerUrl: string,
    brokerPort: number,
    uploadTopic: string,
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

function cleanup(file: converter.BinFile) {
    if (file.delete) {
        vscode.workspace.fs.delete(vscode.Uri.file(file.filePath));
    }
}
