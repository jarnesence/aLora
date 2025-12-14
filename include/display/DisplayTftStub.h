#pragma once
#include "display/Display.h"

// TFT support will be implemented later.
// This stub exists so the build system can already expose a TFT option.
class DisplayTftStub : public IDisplay {
public:
  bool begin() override { return true; }
  void clear() override {}
  void send() override {}
  int width() const override { return 0; }
  int height() const override { return 0; }
  void setFontUi() override {}
  void setFontSmall() override {}
  void drawStr(int, int, const char*) override {}
  void drawBox(int, int, int, int) override {}
  void drawFrame(int, int, int, int) override {}
  void drawHLine(int, int, int) override {}
  void drawVLine(int, int, int) override {}
  void drawGlyph(int, int, uint16_t) override {}
  void setDrawColor(uint8_t) override {}
};
