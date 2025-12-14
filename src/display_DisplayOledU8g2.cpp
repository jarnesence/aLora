#include <Arduino.h>
#include <Wire.h>

#include "config/AppBuildConfig.h"
#include "display/DisplayOledU8g2.h"

#ifndef APP_DISPLAY_OLED
  #error "DisplayOledU8g2 compiled, but APP_DISPLAY_OLED is not enabled."
#endif

// Map rotation macro to U8g2 rotation callback
#if (APP_OLED_ROT == 0)
  #define APP_U8G2_ROT U8G2_R0
#elif (APP_OLED_ROT == 1)
  #define APP_U8G2_ROT U8G2_R1
#elif (APP_OLED_ROT == 2)
  #define APP_U8G2_ROT U8G2_R2
#elif (APP_OLED_ROT == 3)
  #define APP_U8G2_ROT U8G2_R3
#else
  #define APP_U8G2_ROT U8G2_R1
#endif

// Reset pin handling: if you don't have a reset line, set APP_OLED_RESET=U8X8_PIN_NONE in pinmap/env
#ifndef APP_OLED_RESET
  #define APP_OLED_RESET U8X8_PIN_NONE
#endif

// Two controller variants, selectable at runtime via APP_OLED_CTRL (and optionally auto)
static U8G2_SSD1306_128X64_NONAME_F_HW_I2C g_u8g2_1306(APP_U8G2_ROT, APP_OLED_RESET);
static U8G2_SSD1315_128X64_NONAME_F_HW_I2C g_u8g2_1315(APP_U8G2_ROT, APP_OLED_RESET);

DisplayOledU8g2::DisplayOledU8g2() {}

static bool i2cProbe(uint8_t addr7) {
  Wire.beginTransmission(addr7);
  return (Wire.endTransmission() == 0);
}

uint8_t DisplayOledU8g2::detectI2cAddress() {
  // If env specifies address, honor it.
  if (APP_OLED_I2C_ADDR == 0x3C || APP_OLED_I2C_ADDR == 0x3D) {
    return (uint8_t)APP_OLED_I2C_ADDR;
  }

  // Auto: probe the two common OLED addresses.
  if (i2cProbe(0x3C)) return 0x3C;
  if (i2cProbe(0x3D)) return 0x3D;

  // Fallback: most modules are 0x3C
  return 0x3C;
}

U8G2* DisplayOledU8g2::selectController() {
  // 0 = auto. Prefer SSD1315 when available because SSD1315 modules are common
  // and SSD1306 modules often still work with SSD1315 init (but not always).
  if (APP_OLED_CTRL == 1306) return &g_u8g2_1306;
  if (APP_OLED_CTRL == 1315) return &g_u8g2_1315;

  // Auto: try SSD1315 first.
  return &g_u8g2_1315;
}

void DisplayOledU8g2::hardResetIfPresent() {
  if ((int)APP_OLED_RESET < 0 || APP_OLED_RESET == U8X8_PIN_NONE) return;

  pinMode(APP_OLED_RESET, OUTPUT);
  digitalWrite(APP_OLED_RESET, HIGH);
  delay(1);
  digitalWrite(APP_OLED_RESET, LOW);
  delay(10);
  digitalWrite(APP_OLED_RESET, HIGH);
  delay(10);
}

void DisplayOledU8g2::drawBootBanner(const char* line1, const char* line2) {
  if (!_u8) return;
  _u8->clearBuffer();
  _u8->setFont(u8g2_font_6x12_tr);
  _u8->drawStr(0, 12, line1);
  _u8->drawStr(0, 28, line2);
  _u8->drawStr(0, 44, "If blank: check I2C addr");
  _u8->sendBuffer();
}

bool DisplayOledU8g2::begin() {
  // Ensure I2C is initialized on the selected pins.
  Wire.begin(APP_I2C_SDA, APP_I2C_SCL);
  Wire.setClock((uint32_t)APP_I2C_CLOCK_HZ);

  Wire.setClock((uint32_t)APP_I2C_CLOCK_HZ);

  hardResetIfPresent();
  delay(50);


  uint8_t addr7 = detectI2cAddress();
  _u8 = selectController();
  if (_u8) _u8->setBusClock((uint32_t)APP_I2C_CLOCK_HZ);


  // U8g2 expects 8-bit address (7-bit << 1)
  _u8->setI2CAddress((uint8_t)(addr7 << 1));

  // Quick serial diagnostics (optional but useful)
  Serial.printf("[OLED] ctrl=%d addr=0x%02X SDA=%d SCL=%d RST=%d rot=%d clk=%lu\n",
                (int)APP_OLED_CTRL, addr7, (int)APP_I2C_SDA, (int)APP_I2C_SCL, (int)APP_OLED_RESET,
                (int)APP_OLED_ROT, (unsigned long)APP_I2C_CLOCK_HZ);

  drawBootBanner("aLora boot...", "OLED init...");

  _u8->begin();
  _u8->setPowerSave(0);

  // Test pattern to confirm pixels are being updated
  _u8->clearBuffer();
  _u8->setFont(u8g2_font_6x12_tr);
  _u8->drawFrame(0, 0, _u8->getDisplayWidth(), _u8->getDisplayHeight());
  _u8->drawStr(2, 14, "aLora OLED OK");
  char buf[48];
  snprintf(buf, sizeof(buf), "I2C 0x%02X ctrl %d", addr7, (int)APP_OLED_CTRL);
  _u8->drawStr(2, 30, buf);
  _u8->drawStr(2, 46, "Rotate: APP_OLED_ROT");
  _u8->sendBuffer();

  return true;
}

void DisplayOledU8g2::clear() {
  if (_u8) _u8->clearBuffer();
}

void DisplayOledU8g2::send() {
  if (_u8) _u8->sendBuffer();
}

int DisplayOledU8g2::width() const {
  return _u8 ? _u8->getDisplayWidth() : 0;
}

int DisplayOledU8g2::height() const {
  return _u8 ? _u8->getDisplayHeight() : 0;
}

void DisplayOledU8g2::setFontUi() {
  if (_u8) _u8->setFont(u8g2_font_6x12_tr);
}

void DisplayOledU8g2::setFontSmall() {
  if (_u8) _u8->setFont(u8g2_font_5x8_tr);
}

void DisplayOledU8g2::drawStr(int x, int y, const char* s) {
  if (_u8) _u8->drawStr(x, y, s);
}

void DisplayOledU8g2::drawBox(int x, int y, int w, int h) {
  if (_u8) _u8->drawBox(x, y, w, h);
}

void DisplayOledU8g2::drawFrame(int x, int y, int w, int h) {
  if (_u8) _u8->drawFrame(x, y, w, h);
}

void DisplayOledU8g2::drawHLine(int x, int y, int w) {
  if (_u8) _u8->drawHLine(x, y, w);
}

void DisplayOledU8g2::drawVLine(int x, int y, int h) {
  if (_u8) _u8->drawVLine(x, y, h);
}

void DisplayOledU8g2::drawGlyph(int x, int y, uint16_t encoding) {
  if (_u8) _u8->drawGlyph(x, y, encoding);
}

void DisplayOledU8g2::setDrawColor(uint8_t c) {
  if (_u8) _u8->setDrawColor(c);
}
