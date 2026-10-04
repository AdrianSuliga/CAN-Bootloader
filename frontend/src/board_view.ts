import * as vscode from 'vscode';
import { BoardNode } from './model/board_node';
import { BoardRepository } from './model/board_repository';

const boardRepository: BoardRepository = new BoardRepository;

export class BoardTreeProvider implements vscode.TreeDataProvider<BoardNode> {

    getTreeItem(element: BoardNode): BoardTreeItem {
        return new BoardTreeItem(element);
    }

    getChildren(element?: BoardNode): vscode.ProviderResult<BoardNode[]> {
        if (!element) {
            return boardRepository.getBoards();
        }

        switch (element.type) {
            case "system":
                return element.children;
            case "board":
                return [];
        }
    }

}

class BoardTreeItem extends vscode.TreeItem {
    constructor(public node: BoardNode) {
        super(
            node.name,
            getCollapsibleState(node)
        );
        this.contextValue = node.type;
        this.iconPath = getIcon(node);
    }
}

function getCollapsibleState(node: BoardNode): vscode.TreeItemCollapsibleState {
    if (node.type === "system") {
        return vscode.TreeItemCollapsibleState.Collapsed;
    }
    return vscode.TreeItemCollapsibleState.None;
}

function getIcon(node: BoardNode): vscode.ThemeIcon {
    switch (node.type) {
        case "system":
            return new vscode.ThemeIcon("package");
        case "board":
            return new vscode.ThemeIcon("circuit-board");
    }
}