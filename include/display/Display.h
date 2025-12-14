#pragma once
#include <stdint.h>

class IDisplay {
public:
  virtual ~IDisplay() = default;
  virtual bool begin() = 0;
  virtual void clear() = 0;
  virtual void send() = 0;

  virtual int width() const = 0;
  virtual int height() const = 0;

  virtual void setFontUi() = 0;
  virtual void setFontSmall() = 0;

  virtual void drawStr(int x, int y, const char* s) = 0;
  virtual void drawBox(int x, int y, int w, int h) = 0;
  virtual void drawFrame(int x, int y, int w, int h) = 0;
  virtual void drawHLine(int x, int y, int w) = 0;
  virtual void drawVLine(int x, int y, int h) = 0;
  virtual void drawGlyph(int x, int y, uint16_t encoding) = 0;

  virtual void setDrawColor(uint8_t c) = 0;
};
