#include "Display_SSD1306.h"

Display_SSD1306::Display_SSD1306(int w, int h, int sda, int scl, uint8_t addr)
    : width(w), height(h), sda_pin(sda), scl_pin(scl), address(addr) {
    oled = new Adafruit_SSD1306(width, height, &Wire, -1);
}

Display_SSD1306::~Display_SSD1306() {
    delete oled;
}

void Display_SSD1306::init() {
    Wire.begin(sda_pin, scl_pin);
    if (!oled->begin(SSD1306_SWITCHCAPVCC, address)) {
        // Handle error, maybe serial print?
        // Serial.println(F("SSD1306 allocation failed"));
    }
    oled->clearDisplay();
    oled->display();
}

void Display_SSD1306::clear() {
    oled->clearDisplay();
}

void Display_SSD1306::display() {
    oled->display();
}

void Display_SSD1306::drawPixel(int x, int y, uint16_t color) {
    oled->drawPixel(x, y, color);
}

void Display_SSD1306::drawRect(int x, int y, int w, int h, uint16_t color) {
    oled->drawRect(x, y, w, h, color);
}

void Display_SSD1306::fillRect(int x, int y, int w, int h, uint16_t color) {
    oled->fillRect(x, y, w, h, color);
}

void Display_SSD1306::setCursor(int x, int y) {
    oled->setCursor(x, y);
}

void Display_SSD1306::setTextSize(int size) {
    oled->setTextSize(size);
}

void Display_SSD1306::setTextColor(uint16_t color) {
    oled->setTextColor(color);
}

void Display_SSD1306::setTextColor(uint16_t color, uint16_t bg) {
    oled->setTextColor(color, bg);
}

void Display_SSD1306::print(const char* text) {
    oled->print(text);
}

void Display_SSD1306::print(int number) {
    oled->print(number);
}

void Display_SSD1306::println(const char* text) {
    oled->println(text);
}

int Display_SSD1306::getWidth() {
    return width;
}

int Display_SSD1306::getHeight() {
    return height;
}
