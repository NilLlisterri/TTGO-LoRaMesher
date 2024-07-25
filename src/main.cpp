#include <Arduino.h>
#include "loramesher.h"
#include "display.cpp"
#include "WString.h"
#include "SoftwareSerial.h"

SoftwareSerial portentaSerial(13, 12); // RX, TX
LoraMesher& radio = LoraMesher::getInstance();

bool debug = true;

struct dataPacket {
    uint16_t size;
    byte data[];
};

Display display = Display();

SemaphoreHandle_t portentaSerialSemaphore;

void takeSemaphore() {
    while (xSemaphoreTake(portentaSerialSemaphore, ( TickType_t ) 100) != pdTRUE) {
        if (debug) Serial.println("Waiting for semaphore...");
    }
}

void giveSemaphore() {
    xSemaphoreGive(portentaSerialSemaphore);
}

void processDataPacket(AppPacket<dataPacket>* packet) {
    dataPacket* dPacket = packet->payload;

    if (debug) Serial.println("Received packet, size: " + String(dPacket->size));

    takeSemaphore();

    portentaSerial.write('r');

    vTaskDelay(5); // Don't delete this line, otherwise a bit will be randomly switched
    portentaSerial.write((uint8_t*) &packet->src, 2);
    if (debug) Serial.println("Sent src " + String(packet->src));

    vTaskDelay(5); // Don't delete this line, otherwise a bit will be randomly switched
    portentaSerial.write((uint8_t*) &dPacket->size, 2);
    if (debug) Serial.println("Sent size " + String(dPacket->size));

    for(int j = 0; j < dPacket->size; j++) {
        portentaSerial.write(dPacket->data[j]);
        if (debug) Serial.println("Sent byte " + String(j) + " ("+ String(dPacket->data[j]) +")");
        vTaskDelay(5);
    }
    
    if (debug) Serial.println("Done!");

    giveSemaphore();
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

void updateDisplay(void * pvParameters) {
    for(;;) {
        uint8_t size = radio.routingTableSize();
        LM_LinkedList<RouteNode>* routingTableList = radio.routingTableListCopy();
        String string = String(size) + " Node(s): ";
        for (int i = 0; i < size; i++) {
            RouteNode* rNode = (*routingTableList)[i];
            NetworkNode node = rNode->networkNode;
            string += String(node.address) + " ";
        }
        delete routingTableList;
        display.print(string, 4);
        display.print("Free heap (Kb): " + String(ESP.getFreeHeap()/1024), 3);
        // if (debug) Serial.println("Free heap (Kb): " + String(ESP.getFreeHeap()/1024));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

TaskHandle_t receiveMessageHandle = NULL;
void createReceiveMessages() {
    int res = xTaskCreate(processReceivedPackets, "Receive App Task", 4096, (void*) 1, 2, &receiveMessageHandle);
    if (res != pdPASS) {
        Serial.printf("Error: Receive App Task creation gave error: %d\n", res);
    }
}


void setup() {
    Serial.begin(115200);
    portentaSerial.begin(4800);

    portentaSerialSemaphore = xSemaphoreCreateMutex();
    if (portentaSerialSemaphore == NULL) {
        while(true) {
            Serial.println("The semaphore could not be created!");
            delay(1000);
        }
    }
    xSemaphoreGive(portentaSerialSemaphore);

    display.initDisplay();

    LoraMesher::LoraMesherConfig config = LoraMesher::LoraMesherConfig();

    //Set the configuration of the LoRaMesher (TTGO T-BEAM v1.1)
    config.loraCs = 18;
    config.loraRst = 23;
    config.loraIrq = 26;
    config.loraIo1 = 33;

    config.module = LoraMesher::LoraModules::SX1276_MOD;

    radio.begin(config);
    display.print("Local address: " + String(radio.getLocalAddress()), 0);

    createReceiveMessages();

    radio.setReceiveAppDataTaskHandle(receiveMessageHandle);

    radio.start();

    TaskHandle_t xHandle = NULL;
    xTaskCreate(updateDisplay, "Update display", 2048, ( void * ) 1, tskIDLE_PRIORITY, &xHandle);
}



void loop() {
    if (portentaSerial.available()) {
        if (debug) Serial.println("Serial available");
        takeSemaphore();
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
                    p->data[i] = portentaSerial.read();
                    if (debug) Serial.println("Byte #" + String(i) + " (" + String(p->data[i]) + ") read");
                    portentaSerial.write(p->data[i]);
                }

                portentaSerial.write((uint8_t*) &bytesAmount, 2);

                if (debug) Serial.println("Sending a packet of size " + String(payloadSize));
                
                radio.createPacketAndSend(recipientAddress, (uint8_t*)p, payloadSize);
                // radio.sendReliable(recipientAddress, (uint8_t*)p, payloadSize);
            }
        } else if(c == 'r') { // Print the routing table
            uint8_t size = radio.routingTableSize();
            if (debug) Serial.println("Nodes: " + String(size));
            portentaSerial.write(size);
            LM_LinkedList<RouteNode>* routingTableList = radio.routingTableListCopy();
            for (int i = 0; i < size; i++) {
                RouteNode* rNode = (*routingTableList)[i];
                NetworkNode node = rNode->networkNode;
                portentaSerial.write((uint8_t*) &node.address, 2);
            }
            delete routingTableList;
        } else {
            String error = "ERROR: CMD '" + String(c) + "' unrecognized. Rest: ";
            while (portentaSerial.available()) {
                error += portentaSerial.read();
            }
            // portentaSerial.println(error);
            display.print(error, 2);
            while(true) {
                delay(1000);
                Serial.println(error);
            }
        }
        giveSemaphore();
    }
}
