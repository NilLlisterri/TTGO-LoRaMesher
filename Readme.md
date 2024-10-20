# TTGO LoRaMesher

<p align="center">
Application-agnostic LoRaMesher modem
</p>

Turn a LoRa-enabled microcontroller into a LoRaMesher modem and communicate via UART with the application.

## Requirements
* Visual Studio Code
* PlaformIO extension

## Usage
Firstly, connect to the pins 13 and 12 for UART's RX and TX respecively.

To interact with the modem, a single character (command) must be sent. Use:
* `r`: To obtain the routing table. An unsigned, 1-byte integer node count will be returned, followed by the 2-byte node addresses.
* `s`: Send a message. Write the 2-byte node address of the recipient, followed by a 2-byte integer with the payload size and the payload bytes.

When a message from another node is received, the modem will write the `r` (received) command. Then, the 2-byte sender node address, the 2-byte integer with the payload size and finally the payload bytes.


## Execution
To build and flash the program to the device, run the following command for each device port:

```pio run --target upload -e ttgo-lora32-v1 --upload-port PORT```