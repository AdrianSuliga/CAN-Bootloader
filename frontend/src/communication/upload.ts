import mqtt from "mqtt";
import vscode from "vscode";

export async function upload(
    brokerUrl: string,
    brokerPort: number,
    uploadTopic: string,
    controlId: number,
    firmwareId: number
) {
    const file = await vscode.window.showOpenDialog({
        canSelectFiles: true,
        canSelectFolders: false,
        canSelectMany: false,
        openLabel: "Pick a binary file",
        filters: {
            "Binary files": ["bin"]
        }
    });

    if (!file || file.length === 0) {
        return;
    }

    const context = await vscode.workspace.fs.readFile(file[0]);

    const client = mqtt.connect({
        host: brokerUrl,
        port: brokerPort,
        protocol: 'mqtt'
    });

    const payload = addHeader(context, controlId, firmwareId);

    client.publish(uploadTopic, Buffer.from(payload));
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