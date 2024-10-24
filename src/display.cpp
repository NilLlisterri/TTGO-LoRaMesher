#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "config.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define LINE_COUNT 6

class Display {
public:
    String lines[LINE_COUNT] = {"", "", "", "", "", ""};
    Adafruit_SSD1306 display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, DISPLAY_RST);
    Display() {}

    void initDisplay() {
        pinMode(DISPLAY_RST, OUTPUT);
        digitalWrite(DISPLAY_RST, LOW);
        delay(20);
        digitalWrite(DISPLAY_RST, HIGH);

        Wire.begin(DISPLAY_SDA, DISPLAY_SCL);

        if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
            Serial.println(F("SSD1306 allocation failed"));
            for(;;); // Don't proceed, loop forever
        }

        display.clearDisplay();
        display.setTextColor(WHITE);
        display.setTextSize(1);
        display.setCursor(0,0);
        display.display();
    }

    void print(String string, int line) {
        lines[line] = string;
        display.clearDisplay();
        for (uint i = 0; i < LINE_COUNT; i++) {
            display.setCursor(0, i*10);
            display.print(lines[i]);
        }
        display.display();
    }

    void clear() {
        display.clearDisplay();
    }

};
