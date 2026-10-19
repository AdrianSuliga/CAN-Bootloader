// The module 'vscode' contains the VS Code extensibility API
// Import the module and reference it with the alias vscode in your code below
import * as vscode from 'vscode';
import { BoardTreeProvider } from './board_view';
import { parseDBC } from './model/dbc_parser';

// This method is called when your extension is activated
// Your extension is activated the very first time the command is executed
export function activate(context: vscode.ExtensionContext) {

	// Use the console to output diagnostic information (console.log) and errors (console.error)
	// This line of code will only be executed once when your extension is activated
	console.log('Congratulations, your extension "wireless-uploader" is now active!');

	const provider = new BoardTreeProvider();

	const treeView = vscode.window.createTreeView(
		"wirelessUploaderBoardView",
		{
			treeDataProvider: provider
		}
	);

	context.subscriptions.push(treeView);

	const debug = vscode.commands.registerCommand(
		'wirelessUploader.debug',
		async () => {
			const file = await vscode.window.showOpenDialog({
				canSelectFiles: true,
				canSelectFolders: false,
				canSelectMany: false
			});

			if (!file?.[0]) { return; }

			let boards = await parseDBC(file[0].fsPath, "_BOOTLOADER_CTRL", "_BOOTLOADER_FIRMWARE");
			console.log(boards);
		}
	);
}

// This method is called when your extension is deactivated
export function deactivate() { }
