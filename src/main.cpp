#include <Arduino.h>
#include "loramesher.h"
#include "display.cpp"
#include "WString.h"
#include "SoftwareSerial.h"

SoftwareSerial portentaSerial(13, 12); // RX, TX
LoraMesher& radio = LoraMesher::getInstance();

bool debug = false;

struct dataPacket {
    uint16_t size;
    byte data[];
};

Display display = Display();

void processDataPacket(AppPacket<dataPacket>* packet) {
    dataPacket* dPacket = packet->payload;

    if (debug) Serial.println("---- Payload ---- ");
    if (debug) Serial.println("Packet size: " + String(dPacket->size));
    portentaSerial.write('r');
    portentaSerial.write(static_cast<byte*>(static_cast<void*>(&packet->src)), 2);
    portentaSerial.write(static_cast<byte*>(static_cast<void*>(&dPacket->size)), 2);
    for(int j = 0; j < dPacket->size; j++) {
        portentaSerial.write(dPacket->data[j]);
        vTaskDelay(5);
    }
    if (debug) Serial.println("---- Payload Done ---- ");
}


void processReceivedPackets(void*) {
    for (;;) {
        /* Wait for the notification of processReceivedPackets and enter blocking */
        ulTaskNotifyTake(pdPASS, portMAX_DELAY);

        //Iterate through all the packets inside the Received User Packets FiFo
        while (radio.getReceivedQueueSize() > 0) {
            AppPacket<dataPacket>* packet = radio.getNextAppPacket<dataPacket>();
            processDataPacket(packet);
            radio.deletePacket(packet);
        }
    }
}

void displayRountingTable(void * pvParameters) {
    for(;;) {
        uint8_t size = radio.routingTableSize();
        LM_LinkedList<RouteNode>* routingTableList = radio.routingTableListCopy();
        String string = String(size) + " Node(s): ";
        for (int i = 0; i < size; i++) {
            RouteNode* rNode = (*routingTableList)[i];
            NetworkNode node = rNode->networkNode;
            string += String(node.address) + " ";
        }
        display.print(string, 4);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

TaskHandle_t receiveLoRaMessage_Handle = NULL;
void createReceiveMessages() {
    int res = xTaskCreate(processReceivedPackets, "Receive App Task", 4096, (void*) 1, 2, &receiveLoRaMessage_Handle);
    if (res != pdPASS) {
        Serial.printf("Error: Receive App Task creation gave error: %d\n", res);
    }
}


void setup() {
    Serial.begin(115200);
    portentaSerial.begin(4800);

    display.initDisplay();

    LoraMesher::LoraMesherConfig config = LoraMesher::LoraMesherConfig();

    //Set the configuration of the LoRaMesher (TTGO T-BEAM v1.1)
    config.loraCs = 18;
    config.loraRst = 23;
    config.loraIrq = 26;
    config.loraIo1 = 33;

    config.module = LoraMesher::LoraModules::SX1276_MOD;

    //Init the loramesher with a configuration
    radio.begin(config);
    display.print("Local address: " + String(radio.getLocalAddress()), 0);

    createReceiveMessages();

    radio.setReceiveAppDataTaskHandle(receiveLoRaMessage_Handle);
    //Start LoRaMesher
    radio.start();

    TaskHandle_t xHandle = NULL;
    xTaskCreate(displayRountingTable, "Update display", 2048, ( void * ) 1, tskIDLE_PRIORITY, &xHandle);
}



void loop() {
    if (portentaSerial.available()) {
        char c = portentaSerial.read();
        if (debug) Serial.println("Received command '" + String(c) + "'");
        if (c == 's') { // Send a message
            if (radio.routingTableSize() == 0) {
                if (debug) Serial.println("The routing table is empty!");
            } else {
                while(portentaSerial.available() < 2) {
                    if (debug) Serial.println("Waiting for size");
                    delay(100); 
                }
                
                byte recipientBytes[2];
                portentaSerial.readBytes(recipientBytes, 2);
                uint16_t recipientAddress;
                memcpy(&recipientAddress, recipientBytes, sizeof(uint16_t));
                if (debug) Serial.println("Recipient: " + String(recipientAddress));

                byte amountBytes[2];
                portentaSerial.readBytes(amountBytes, 2);
                uint16_t bytesAmount;
                memcpy(&bytesAmount, amountBytes, sizeof(uint16_t));
                if (debug) Serial.println("Amount of bytes: " + String(bytesAmount));

                if (debug) Serial.println("Waiting for " + String(bytesAmount) + " bytes");
                uint32_t payloadSize = bytesAmount * sizeof(byte) + sizeof(dataPacket);

                dataPacket* p = (dataPacket*) malloc(payloadSize);
                p->size = bytesAmount;

                for(int i = 0; i < bytesAmount; i++) {
                    while(!portentaSerial.available()) {}
                    if (debug) Serial.println("Byte " + String(i+1) + " read");
                    p->data[i] = portentaSerial.read();
                }

                if (debug) Serial.println("Sending a packet of size " + String(payloadSize));
                
                radio.createPacketAndSend(recipientAddress, (uint8_t*)p, payloadSize);
                // radio.sendReliable(recipientAddress, p, payloadSize);
            }
        } else if(c == 'r') { // Print the routing table
            uint8_t size = radio.routingTableSize();
            if (debug) Serial.println("Nodes: " + String(size));
            portentaSerial.write(size);
            LM_LinkedList<RouteNode>* routingTableList = radio.routingTableListCopy();
            for (int i = 0; i < size; i++) {
                RouteNode* rNode = (*routingTableList)[i];
                NetworkNode node = rNode->networkNode;
                portentaSerial.write(static_cast<byte*>(static_cast<void*>(&node.address)), 2);
            }
        } else {
            String error = "ERROR: Command '" + String(c) + "' not recognized";
            portentaSerial.println(error);
            display.print(error, 2);
        }
    }
}
