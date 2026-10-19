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

    ctrId: number;
    firmwareId: number;
}
