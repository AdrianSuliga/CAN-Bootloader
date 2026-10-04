export type BoardNode =
    | Board
    | System;


export interface System {
    type: "system";
    name: string;
    children: Board[];
}

export interface Board {
    type: "board";
    name: string;
}
