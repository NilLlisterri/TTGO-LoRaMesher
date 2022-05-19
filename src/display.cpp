#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_SDA 4
#define OLED_SCL 15 
#define OLED_RST 16


class Display {
public:
    String lines[5] = {"", "", "", "", ""};
    Adafruit_SSD1306 display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);
    Display() {}

    void initDisplay() {
        pinMode(OLED_RST, OUTPUT);
        digitalWrite(OLED_RST, LOW);
        delay(20);
        digitalWrite(OLED_RST, HIGH);

        Wire.begin(OLED_SDA, OLED_SCL);

        if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
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
        for(uint i = 0; i < 5; i++) {
            display.setCursor(0, i*10);
            display.print(lines[i]);
        }
        display.display();
    }

    void clear() {
        display.clearDisplay();
    }
    
private:

};



