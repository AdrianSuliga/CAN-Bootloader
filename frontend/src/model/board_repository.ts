import { BoardNode } from "./board_node";

export class BoardRepository {

    async getBoards(): Promise<BoardNode[]> {
        //Temp data
        return [
            {
                type: "system",
                name: "Boaty Mc'floaty",
                children: [
                    {
                        type: "board",
                        name: "stm32 #1",
                        controlId: 0,
                        firmwareId: 0
                    },
                    {
                        type: "board",
                        name: "stm32 #2",
                        controlId: 0,
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
                name: "Test System 1",
                children: [
                    {
                        type: "board",
                        name: "Nucleo",
                        controlId: 1024,
                        firmwareId: 1025
                    }
                ],
                mqttBroker: "broker.hivemq.com",
                mqttPort: 1883,
                mqttUploadTopic: "system/gateway_board/subscribe/new_app",
                mqttDebugTopic: "system/gateway_board/publish/state"
            }
        ];
    }
}