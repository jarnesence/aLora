#include <Arduino.h>
#include <cstring>
#include "config/AppBuildConfig.h"
#include "ui/Ui.h"

static const char* kCharset = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?-_/@";

Ui::Ui(IDisplay* display, RotaryInput* input, MeshRadio* radio, ChatLog* log)
: _d(display), _in(input), _radio(radio), _log(log) {}

void Ui::begin() {
  _lastDrawMs = 0;
  if (_draft[0] == '\0') std::strcpy(_draft, "ALORA READY");
  syncCharIndexToDraft();
}

void Ui::onIncoming(uint16_t src, const WireChatPacket& pkt) {
  if (!_log) return;
  _log->add(src, false, pkt.text, (uint32_t)(millis()/1000));
}

void Ui::handleInput() {
  if (!_in) return;
  _in->tick();

  bool click = _in->wasClicked();
  int32_t delta = _in->readDelta();

  if (click) handleClick();
  if (delta != 0) handleDelta(delta);
}

void Ui::handleClick() {
  if (_screen == Screen::Compose) {
    handleComposeClick();
    return;
  }

  if (_screen == Screen::Chat) _screen = Screen::Compose;
  else if (_screen == Screen::Compose) _screen = Screen::Status;
  else if (_screen == Screen::Status) _screen = Screen::Settings;
  else _screen = Screen::Chat;
}

void Ui::handleDelta(int32_t delta) {
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
      handleComposeDelta(delta);
      break;
    }
    case Screen::Status:
    case Screen::Settings:
    default:
      break;
  }
}

void Ui::handleComposeClick() {
  switch (_focus) {
    case ComposeFocus::Destination:
      _focus = ComposeFocus::Cursor;
      break;
    case ComposeFocus::Cursor:
      _focus = ComposeFocus::Character;
      break;
    case ComposeFocus::Character:
      _focus = ComposeFocus::Send;
      break;
    case ComposeFocus::Send:
      sendDraft();
      _focus = ComposeFocus::Destination;
      _screen = Screen::Chat;
      break;
  }
}

void Ui::handleComposeDelta(int32_t delta) {
  switch (_focus) {
    case ComposeFocus::Destination: {
      int32_t nd = (int32_t)_dst + delta;
      if (nd < 1) nd = 1;
      if (nd > 65535) nd = 65535;
      _dst = (uint16_t)nd;
      break;
    }
    case ComposeFocus::Cursor:
      advanceCursor(delta);
      syncCharIndexToDraft();
      break;
    case ComposeFocus::Character: {
      size_t n = std::strlen(kCharset);
      int32_t idx = (int32_t)_charIndex + delta;
      while (idx < 0) idx += (int32_t)n;
      idx %= (int32_t)n;
      _charIndex = (uint8_t)idx;

      _draft[_cursor] = kCharset[_charIndex];
      _draft[sizeof(_draft)-1] = '\0';
      break;
    }
    case ComposeFocus::Send:
      // Keep steady; clicks handle send.
      break;
  }
}

void Ui::advanceCursor(int32_t delta) {
  int32_t next = (int32_t)_cursor + delta;
  int32_t maxIdx = (int32_t)sizeof(_draft) - 2; // leave room for null
  if (next < 0) next = 0;
  if (next > maxIdx) next = maxIdx;
  _cursor = (uint8_t)next;

  if (_draft[_cursor] == '\0') {
    _draft[_cursor] = ' ';
    _draft[_cursor + 1] = '\0';
  }
}

void Ui::syncCharIndexToDraft() {
  char c = _draft[_cursor];
  if (c == '\0') c = ' ';

  const char* pos = std::strchr(kCharset, c);
  if (pos) {
    _charIndex = (uint8_t)(pos - kCharset);
  } else {
    _charIndex = 0;
  }
}

void Ui::sendDraft() {
  if (!_radio) return;

  WireChatPacket pkt{};
  pkt.msgId = _nextMsgId++;
  pkt.to = _dst;
  pkt.from = _radio->localAddress();
  pkt.ts = (uint32_t)(millis() / 1000);
  std::strncpy(pkt.text, _draft, sizeof(pkt.text) - 1);
  pkt.text[sizeof(pkt.text) - 1] = '\0';

  _radio->sendDm(_dst, pkt);

  if (_log) {
    _log->add(_dst, true, pkt.text, pkt.ts);
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
  _d->drawStr(2, 12, "COMPOSE (click=cycle)");
  _d->drawHLine(0, 14, w);

  _d->setFontSmall();

  char line1[24];
  snprintf(line1, sizeof(line1), "To: %u", (unsigned)_dst);
  _d->drawStr(2, 28, line1);

  const char* focusLabel = "DST";
  if (_focus == ComposeFocus::Cursor) focusLabel = "CUR";
  else if (_focus == ComposeFocus::Character) focusLabel = "CHAR";
  else if (_focus == ComposeFocus::Send) focusLabel = "SEND";

  char focusLine[28];
  snprintf(focusLine, sizeof(focusLine), "Focus:%s", focusLabel);
  _d->drawStr(2, 40, focusLine);

  char cursorLine[28];
  snprintf(cursorLine, sizeof(cursorLine), "Cursor:%u", (unsigned)_cursor);
  _d->drawStr(2, 52, cursorLine);

  _d->drawStr(2, 68, "Text:");
  _d->drawFrame(0, 72, w, 32);

  // Render the draft (one line for now)
  char shown[18];
  std::strncpy(shown, _draft, sizeof(shown)-1);
  shown[sizeof(shown)-1] = '\0';
  _d->drawStr(2, 86, shown);

  _d->drawStr(2, 108, "Click: focus/send");
  _d->drawStr(2, 120, "Rotate: move/edit");
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
