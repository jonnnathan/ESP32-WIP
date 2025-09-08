#ifndef OLED_H
#define OLED_H

#include <Arduino.h>
#include <Wire.h>

// Pin definitions for Heltec ESP32 LoRa v3
#define VEXT_PIN 36
#define SDA_PIN 17
#define SCL_PIN 18
#define RST_PIN 21
#define I2C_ADDR 0x3C

// Display dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define BUFFER_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 8)

// SSD1306 Commands
#define SSD1306_DISPLAYOFF                  0xAE
#define SSD1306_SETDISPLAYCLOCKDIV          0xD5
#define SSD1306_SETMULTIPLEX                0xA8
#define SSD1306_SETDISPLAYOFFSET            0xD3
#define SSD1306_SETSTARTLINE                0x40
#define SSD1306_CHARGEPUMP                  0x8D
#define SSD1306_MEMORYMODE                  0x20
#define SSD1306_SEGREMAP                    0xA0
#define SSD1306_COMSCANDEC                  0xC8
#define SSD1306_SETCOMPINS                  0xDA
#define SSD1306_SETCONTRAST                 0x81
#define SSD1306_SETPRECHARGE                0xD9
#define SSD1306_SETVCOMDETECT               0xDB
#define SSD1306_DISPLAYALLON_RESUME         0xA4
#define SSD1306_NORMALDISPLAY               0xA6
#define SSD1306_DISPLAYON                   0xAF
#define SSD1306_COLUMNADDR                  0x21
#define SSD1306_PAGEADDR                    0x22

class OLED {
public:
    // Constructor
    OLED();
    
    // Initialization and display control
    bool init();
    void clearDisplay();
    void updateDisplay();
    void displayOn();
    void displayOff();
    
    // Drawing functions
    void setPixel(int x, int y, bool white = true);
    void drawLine(int x0, int y0, int x1, int y1);
    void drawRect(int x, int y, int width, int height, bool fill = false);
    void drawCircle(int x, int y, int radius, bool fill = false);
    void drawString(int x, int y, const char* str);
    void drawString(int x, int y, String str);

private:
    uint8_t displayBuffer[BUFFER_SIZE];
    
    // Low-level functions
    void sendCommand(uint8_t cmd);
    void sendData(uint8_t data);
    void resetDisplay();
    
    // Internal drawing helpers
    void drawChar(int x, int y, char c);
    void drawHorizontalLine(int x, int y, int width);
    void drawVerticalLine(int x, int y, int height);
    void fillRect(int x, int y, int width, int height);
};

#endif // OLED_H