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
                        name: "stm32 #1",
                        ctrId: 0,
                        firmwareId: 0
                    },
                    {
                        type: "board",
                        name: "stm32 #2",
                        ctrId: 0,
                        firmwareId: 0
                    },
                ],
                mqttBroker: "test.example.none",
                mqttPort: 1234,
                mqttUploadTopic: "none",
                mqttDebugTopic: "none"
            },
            {
                type: "system",
                name: "test system 2",
                children: [
                    {
                        type: "board",
                        name: "steering wheel",
                        ctrId: 0,
                        firmwareId: 0
                    },
                    {
                        type: "board",
                        name: "engine",
                        ctrId: 0,
                        firmwareId: 0
                    },
                    {
                        type: "board",
                        name: "battery",
                        ctrId: 0,
                        firmwareId: 0
                    },
                ],
                mqttBroker: "test.example.none",
                mqttPort: 1234,
                mqttUploadTopic: "none",
                mqttDebugTopic: "none"
            }
        ];
    }
}