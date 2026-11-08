export type BoardNode =
    | Board
    | System;


export interface System {
    type: "system";
    name: string;
    children: Board[];

    mqttBroker: string;
    mqttPort: number;
    mqttUploadTopic: string;
    mqttDebugTopic: string;
}

export interface Board {
    type: "board";
    name: string;

    controlId: number;
    firmwareId: number;
}

export function isSystem(
    node: BoardNode | undefined | null
): node is System {
    return true;
}

export function isBoard(
    node: BoardNode | undefined | null
): node is Board {
    return true;
}