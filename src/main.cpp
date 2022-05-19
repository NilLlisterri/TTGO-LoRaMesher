#include <Arduino.h>
#include "loramesher.h"
#include "display.cpp"
#include "WString.h"
#include "SoftwareSerial.h"

SoftwareSerial portentaSerial(13, 12); // RX, TX
LoraMesher& radio = LoraMesher::getInstance();

struct dataPacket {
    uint16_t size;
    byte data[];
};

Display display = Display();

void processDataPacket(LoraMesher::userPacket<dataPacket>* packet) {
    Log.trace(F("Packet arrived from %d with size %d bytes" CR), packet->src, packet->payloadSize);

    dataPacket* dPacket = packet->payload;

    Serial.println("---- Payload ---- ");
    Serial.println("Packet size: " + String(dPacket->size));
    portentaSerial.write('r');
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
            LoraMesher::userPacket<dataPacket>* packet = radio.getNextUserPacket<dataPacket>();
            processDataPacket(packet);
            radio.deletePacket(packet);
        }
    }
}


void printRoutingTable() {
    if (radio.routingTableSize() == 0) {
        Serial.println("Empty routing table");
    } else {
        for (int i = 0; i < radio.routingTableSize(); i++) {
            LoraMesher::networkNode node = radio.routingTable[i].networkNode;
            Serial.println("|" + String(node.address) + "(" +String(node.metric)+")->"+String(radio.routingTable[i].via));
        }
    }
}

void setup() {
    Serial.begin(115200);
    portentaSerial.begin(4800);

    display.initDisplay();

    radio.init(processReceivedPackets);
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
                byte ref[2];
                portentaSerial.readBytes(ref, 2);
                uint16_t bytesAmount;
                memcpy(&bytesAmount, ref, sizeof(uint16_t));
                Serial.println(bytesAmount);
                /*portentaSerial.readBytes((byte*) bytesAmount, 2);*/

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
                LoraMesher::networkNode node = radio.routingTable[0].networkNode;
                
                radio.sendReliablePacket(node.address, (uint8_t*)p, payloadSize);
                // radio.sendReliable(node.address, p, payloadSize);
            }
        } else if(c == 'r') { // Print the routing table
            portentaSerial.println(radio.routingTableSize());
            for (int i = 0; i < radio.routingTableSize(); i++) {
                LoraMesher::networkNode node = radio.routingTable[i].networkNode;
                portentaSerial.println(node.address);
            }
        } else {
            portentaSerial.println("Command '" + String(c) + "' not recognized");
        }
    }
}
