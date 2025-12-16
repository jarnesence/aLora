#ifndef DISPLAY_SSD1306_H
#define DISPLAY_SSD1306_H

#include "DisplayDriver.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

class Display_SSD1306 : public DisplayDriver {
private:
    Adafruit_SSD1306* oled;
    int width;
    int height;
    int sda_pin;
    int scl_pin;
    uint8_t address;

public:
    Display_SSD1306(int w, int h, int sda, int scl, uint8_t addr);
    ~Display_SSD1306();

    void init() override;
    void clear() override;
    void display() override;

    void drawPixel(int x, int y, uint16_t color) override;
    void drawRect(int x, int y, int w, int h, uint16_t color) override;
    void fillRect(int x, int y, int w, int h, uint16_t color) override;

    void setCursor(int x, int y) override;
    void setTextSize(int size) override;
    void setTextColor(uint16_t color) override;
    void setTextColor(uint16_t color, uint16_t bg) override;
    void print(const char* text) override;
    void print(int number) override;
    void println(const char* text) override;

    int getWidth() override;
    int getHeight() override;
};

#endif // DISPLAY_SSD1306_H
