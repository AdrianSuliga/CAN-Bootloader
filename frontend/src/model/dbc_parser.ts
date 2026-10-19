import { Board } from "./board_node";
import * as vscode from 'vscode';

export async function parseDBC(filePath: string, ctr: string, firmware: string): Promise<Board[]> {
    const uri = vscode.Uri.file(filePath);

    const document = await vscode.workspace.openTextDocument(uri);
    const text = document.getText().split('\n');

    let boards: { [id: string]: Board } = {};
    for (let line of text) {
        let line_processed = line.split(' ');
        if (line_processed[0] === "BO_") {
            if (line_processed[2].includes(ctr)) {
                let name = line_processed[2].split(ctr)[0];
                if (name in boards) {
                    boards[name].ctrId = Number(line_processed[1]);
                } else {
                    boards[name] = {
                        type: "board",
                        name: name,
                        ctrId: Number(line_processed[1]),
                        firmwareId: -1
                    };
                }
            } else if (line_processed[2].includes(firmware)) {
                let name = line_processed[2].split(firmware)[0];
                if (name in boards) {
                    boards[name].firmwareId = Number(line_processed[1]);
                } else {
                    boards[name] = {
                        type: "board",
                        name: name,
                        ctrId: -1,
                        firmwareId: Number(line_processed[1])
                    };
                }
            }
        }
    }

    return Object.values(boards);
}