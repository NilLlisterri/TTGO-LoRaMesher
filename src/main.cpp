//#define LORA_IO1 (uint8_t)255U

#include <Arduino.h>
#include "LoraMesher.h"
#include "display.cpp"
#include "SoftwareSerial.h"

SoftwareSerial portentaSerial(13, 12); // RX, TX
LoraMesher& radio = LoraMesher::getInstance();

struct dataPacket {
    uint16_t size;
    byte data[];
};

Display display = Display();

void processDataPacket(AppPacket<dataPacket>* packet) {
    // Log.trace(F("Packet arrived from %d with size %d bytes" CR), packet->src, packet->payloadSize);

    dataPacket* dPacket = packet->payload;

    Serial.println("---- Payload ---- ");
    Serial.println("Packet size: " + String(dPacket->size));
    portentaSerial.write('r');
    portentaSerial.write(static_cast<byte*>(static_cast<void*>(&packet->src)), 2);
    portentaSerial.write(static_cast<byte*>(static_cast<void*>(&dPacket->size)), 2);
    for(int j = 0; j < dPacket->size; j++) {
        Serial.println("Byte: " + String(j+1) + ": " + String(dPacket->data[j]));
        portentaSerial.write(dPacket->data[j]);
        delay(5);
    }
    Serial.println("---- Payload Done ---- ");
}


void processReceivedPackets(void*) {
    for (;;) {
        /* Wait for the notification of processReceivedPackets and enter blocking */
        ulTaskNotifyTake(pdPASS, portMAX_DELAY);

        //Iterate through all the packets inside the Received User Packets FiFo
        while (radio.getReceivedQueueSize() > 0) {
            Log.trace(F("Received %d packets!" CR), radio.getReceivedQueueSize());
            //Get the first element inside the Received User Packets FiFo
            AppPacket<dataPacket>* packet = radio.getNextAppPacket<dataPacket>();
            processDataPacket(packet);
            radio.deletePacket(packet);
        }
    }
}


void printRoutingTable() {
    if (radio.routingTableSize() == 0) {
        Serial.println("Empty routing table");
    } else {
        LM_LinkedList<RouteNode>* routingTableList = radio.routingTableList();
        for (int i = 0; i < radio.routingTableSize(); i++) {
            RouteNode* rNode = (*routingTableList)[i];
            NetworkNode node = rNode->networkNode;
            Serial.println("|" + String(node.address) + "(" +String(node.metric)+")->"+String(rNode->via));
        }
        /*for (int i = 0; i < radio.routingTableSize(); i++) {
            LoraMesher::networkNode node = radio->routingTable[i].networkNode;
            Serial.println("|" + String(node.address) + "(" +String(node.metric)+")->"+String(radio->routingTable[i].via));
        }*/
    }
}

void setup() {
    Serial.begin(115200);
    portentaSerial.begin(4800);

    // radio = new LoraMesher(processReceivedPackets);

    display.initDisplay();
    radio.init(processReceivedPackets);

    // radio.init(processReceivedPackets);
    display.print("Id: " + String(radio.getLocalAddress()), 0);
}

void loop() {
    if (portentaSerial.available()) {
        char c = portentaSerial.read();
        Serial.println("Received command '" + String(c) + "'");
        if (c == 's') { // Send a message
            if (radio.routingTableSize() == 0) {
                Serial.println("The routing table is empty!");
            } else {
                while(portentaSerial.available() < 2) { Serial.println("Waiting for size"); delay(100); }
                
                byte recipientBytes[2];
                portentaSerial.readBytes(recipientBytes, 2);
                uint16_t recipientAddress;
                memcpy(&recipientAddress, recipientBytes, sizeof(uint16_t));
                Serial.println("Recipient: " + String(recipientAddress));

                byte amountBytes[2];
                portentaSerial.readBytes(amountBytes, 2);
                uint16_t bytesAmount;
                memcpy(&bytesAmount, amountBytes, sizeof(uint16_t));
                Serial.println("Amount of bytes: " + String(bytesAmount));

                Serial.println("Waiting for " + String(bytesAmount) + " bytes");
                uint32_t payloadSize = bytesAmount * sizeof(byte) + sizeof(dataPacket);

                dataPacket* p = (dataPacket*) malloc(payloadSize);
                p->size = bytesAmount;

                for(int i = 0; i < bytesAmount; i++) {
                    while(!portentaSerial.available()) {}
                    Serial.println("Byte " + String(i+1) + " read");
                    p->data[i] = portentaSerial.read();
                }

                Serial.println("Sending a packet of size " + String(payloadSize));
                // LoraMesher::networkNode node = radio->routingTable[0].networkNode;
                
                //radio.createPacketAndSend(recipientAddress, (uint8_t*)p, payloadSize);
                
                
                radio.sendReliable(recipientAddress, p, payloadSize);
                free(p);
            }
        } else if(c == 'r') { // Print the routing table
            /*uint8_t size = radio.routingTableSize();
            Serial.println("Size: " + String(size));
            portentaSerial.write(size);
            for (int i = 0; i < size; i++) {
                LoraMesher::networkNode node = radio->routingTable[i].networkNode;
                portentaSerial.write(static_cast<byte*>(static_cast<void*>(&node.address)), 2);
            }*/
            
            LM_LinkedList<RouteNode>* routingTableList = radio.routingTableList();
            Serial.println("Size: " + String(radio.routingTableSize()));
            portentaSerial.write(radio.routingTableSize());
            for (int i = 0; i < radio.routingTableSize(); i++) {
                RouteNode* rNode = (*routingTableList)[i];
                NetworkNode node = rNode->networkNode;
                portentaSerial.write(static_cast<byte*>(static_cast<void*>(&node.address)), 2);
            }
        } else {
            portentaSerial.println("Command '" + String(c) + "' not recognized");
        }
    }
}
