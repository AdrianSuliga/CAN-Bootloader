import { BoardNode } from "./board_node";

export class BoardRepository {

    async getBoards(): Promise<BoardNode[]> {
        //Temp data
        return [
            {
                type: "system",
                name: "test system 1",
                children: [
                    {
                        type: "board",
                        name: "stm32 #1"
                    },
                    {
                        type: "board",
                        name: "stm32 #2"
                    },
                ]
            },
            {
                type: "system",
                name: "test system 2",
                children: [
                    {
                        type: "board",
                        name: "steering wheel"
                    },
                    {
                        type: "board",
                        name: "engine"
                    },
                    {
                        type: "board",
                        name: "battery"
                    },
                ]
            }
        ];
    }
}