#pragma once
#include "display/Display.h"
#include <U8g2lib.h>

// OLED display implementation using U8g2.
// Supports SSD1306 and SSD1315 over HW I2C.
// I2C address (0x3C/0x3D) can be auto-detected.

class DisplayOledU8g2 : public IDisplay {
public:
  DisplayOledU8g2();
  bool begin() override;

  void clear() override;
  void send() override;

  int width() const override;
  int height() const override;

  void setFontUi() override;
  void setFontSmall() override;

  void drawStr(int x, int y, const char* s) override;
  void drawBox(int x, int y, int w, int h) override;
  void drawFrame(int x, int y, int w, int h) override;
  void drawHLine(int x, int y, int w) override;
  void drawVLine(int x, int y, int h) override;
  void drawGlyph(int x, int y, uint16_t encoding) override;
  void setDrawColor(uint8_t c) override;

private:
  U8G2* _u8 = nullptr;

  uint8_t detectI2cAddress();
  U8G2* selectController();
  const u8g2_cb_t* rotationToU8g2(uint8_t rot);

  void hardResetIfPresent();
  void drawBootBanner(const char* line1, const char* line2);
};
