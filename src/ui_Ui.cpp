#include <Arduino.h>
#include <cstring>
#include "config/AppBuildConfig.h"
#include "ui/Ui.h"

static const char* kCharset = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?-_/@";

Ui::Ui(IDisplay* display, RotaryInput* input, MeshRadio* radio, ChatLog* log)
: _d(display), _in(input), _radio(radio), _log(log) {}

void Ui::begin() {
  _lastDrawMs = 0;
  if (_draft[0] == '\0') std::strcpy(_draft, "HELLO");
}

void Ui::onIncoming(uint16_t src, const WireChatPacket& pkt) {
  if (!_log) return;
  _log->add(src, false, pkt.text, (uint32_t)(millis()/1000));
}

void Ui::handleInput() {
  if (!_in) return;
  _in->tick();

  int32_t delta = _in->readDelta();
  bool click = _in->wasClicked();

  if (click) {
    // Cycle screens
    if (_screen == Screen::Chat) _screen = Screen::Compose;
    else if (_screen == Screen::Compose) _screen = Screen::Status;
    else if (_screen == Screen::Status) _screen = Screen::Settings;
    else _screen = Screen::Chat;
  }

  if (delta == 0) return;

  switch (_screen) {
    case Screen::Chat: {
      _chatScroll += (delta > 0 ? 1 : -1);
      if (_chatScroll < 0) _chatScroll = 0;
      int maxScroll = (int)_log->size() - 1;
      if (maxScroll < 0) maxScroll = 0;
      if (_chatScroll > maxScroll) _chatScroll = maxScroll;
      break;
    }
    case Screen::Compose: {
      // Very simple editor:
      // - Rotate: adjust destination when focus=dst
      // - Rotate: adjust character when focus=text
      // - Click already switches screens (temporary). We'll later add long-press for send/edit.
      if (_focus == 0) {
        int32_t nd = (int32_t)_dst + delta;
        if (nd < 1) nd = 1;
        if (nd > 65535) nd = 65535;
        _dst = (uint16_t)nd;
      } else if (_focus == 1) {
        // adjust character at cursor
        size_t n = std::strlen(kCharset);
        int32_t idx = (int32_t)_charIndex + delta;
        while (idx < 0) idx += (int32_t)n;
        idx %= (int32_t)n;
        _charIndex = (uint8_t)idx;

        _draft[_cursor] = kCharset[_charIndex];
      }
      break;
    }
    case Screen::Status:
    case Screen::Settings:
    default:
      break;
  }
}

void Ui::tick() {
  handleInput();

  // Draw at ~10 FPS
  uint32_t now = millis();
  if (now - _lastDrawMs < 100) return;
  _lastDrawMs = now;

  if (!_d) return;
  _d->clear();

  switch (_screen) {
    case Screen::Chat: drawChat(); break;
    case Screen::Compose: drawCompose(); break;
    case Screen::Status: drawStatus(); break;
    case Screen::Settings: drawSettings(); break;
  }

  _d->send();
}

void Ui::drawChat() {
  int w = _d->width();
  int h = _d->height();

  _d->setFontUi();
  _d->drawStr(2, 12, "CHAT (click=next)");
  _d->drawHLine(0, 14, w);

  _d->setFontSmall();

  int y = 28;
  int lineH = 12;

  int total = (int)_log->size();
  int idxNewest = total - 1 - _chatScroll;

  for (int lines = 0; lines < 8 && y < h; lines++) {
    int idx = idxNewest - lines;
    if (idx < 0) break;

    const ChatMsg& m = _log->at((size_t)idx);

    char header[18];
    snprintf(header, sizeof(header), "%c%u:", m.outgoing ? '>' : '<', (unsigned)m.src);
    _d->drawStr(2, y, header);

    // Show a compact snippet; for portrait width it's very limited.
    char snippet[18];
    std::strncpy(snippet, m.text, sizeof(snippet)-1);
    snippet[sizeof(snippet)-1] = '\0';

    _d->drawStr(2, y + 10, snippet);

    y += (lineH * 2);
  }
}

void Ui::drawCompose() {
  int w = _d->width();

  _d->setFontUi();
  _d->drawStr(2, 12, "COMPOSE (click=next)");
  _d->drawHLine(0, 14, w);

  _d->setFontSmall();

  char line1[24];
  snprintf(line1, sizeof(line1), "To: %u", (unsigned)_dst);
  _d->drawStr(2, 28, line1);

  _d->drawStr(2, 44, "Text:");
  _d->drawFrame(0, 48, w, 40);

  // Render the draft (one line for now)
  char shown[18];
  std::strncpy(shown, _draft, sizeof(shown)-1);
  shown[sizeof(shown)-1] = '\0';
  _d->drawStr(2, 62, shown);

  _d->drawStr(2, 98, "Rotate: edit");
  _d->drawStr(2, 110, "Tip: set APP_OLED_ROT");
}

void Ui::drawStatus() {
  int w = _d->width();

  _d->setFontUi();
  _d->drawStr(2, 12, "STATUS (click=next)");
  _d->drawHLine(0, 14, w);

  _d->setFontSmall();

  char line[26];

  snprintf(line, sizeof(line), "RX:%lu", (unsigned long)_radio->rxCount());
  _d->drawStr(2, 30, line);

  snprintf(line, sizeof(line), "TX:%lu", (unsigned long)_radio->txCount());
  _d->drawStr(2, 42, line);

  snprintf(line, sizeof(line), "Uptime:%lus", (unsigned long)(millis()/1000));
  _d->drawStr(2, 54, line);

  _d->drawStr(2, 70, "OLED should show now.");
  _d->drawStr(2, 82, "If blank: try 0x3C/0x3D.");
}

void Ui::drawSettings() {
  int w = _d->width();

  _d->setFontUi();
  _d->drawStr(2, 12, "SETTINGS (click=next)");
  _d->drawHLine(0, 14, w);

  _d->setFontSmall();

  char line[28];

  snprintf(line, sizeof(line), "Freq:%lu", (unsigned long)APP_LORA_FREQ_HZ);
  _d->drawStr(2, 30, line);

  snprintf(line, sizeof(line), "BW:%d kHz", (int)APP_LORA_BW_KHZ);
  _d->drawStr(2, 42, line);

  snprintf(line, sizeof(line), "SF:%d", (int)APP_LORA_SF);
  _d->drawStr(2, 54, line);

  snprintf(line, sizeof(line), "TX:%d dBm", (int)APP_LORA_TX_DBM);
  _d->drawStr(2, 66, line);

  _d->drawStr(2, 84, "Later: edit via rotary.");
}
