#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <Arduino.h>

class DisplayDriver {
public:
    virtual ~DisplayDriver() {}

    virtual void init() = 0;
    virtual void clear() = 0;
    virtual void display() = 0;

    // Graphics primitives
    virtual void drawPixel(int x, int y, uint16_t color) = 0;
    virtual void drawRect(int x, int y, int w, int h, uint16_t color) = 0;
    virtual void fillRect(int x, int y, int w, int h, uint16_t color) = 0;

    // Text handling
    virtual void setCursor(int x, int y) = 0;
    virtual void setTextSize(int size) = 0;
    virtual void setTextColor(uint16_t color) = 0;
    virtual void setTextColor(uint16_t color, uint16_t bg) = 0;
    virtual void print(const char* text) = 0;
    virtual void print(int number) = 0;
    virtual void println(const char* text) = 0;

    // Specific helpers that might be useful
    virtual int getWidth() = 0;
    virtual int getHeight() = 0;
};

#endif // DISPLAY_DRIVER_H
