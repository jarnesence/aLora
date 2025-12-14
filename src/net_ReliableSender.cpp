#include "net/ReliableSender.h"
#include "net/MeshRadio.h"
#include "storage/ChatLog.h"
#include <Arduino.h>

static uint32_t rand32() {
#if defined(ESP32)
  return (uint32_t)esp_random();
#else
  return (uint32_t)random(0x7fffffff);
#endif
}

void ReliableSender::begin(MeshRadio* r, ChatLog* log, const AppSettings* s) {
  _r = r; _log = log; _s = s;
  for (auto &e : _q) e.used = false;
}

uint32_t ReliableSender::nowMs() const { return millis(); }

bool ReliableSender::sendChat(uint16_t dst, const char* text) {
  if (!_r || !_s || !text) return false;

  // Find empty slot
  int slot = -1;
  for (int i = 0; i < (int)(sizeof(_q)/sizeof(_q[0])); i++) {
    if (!_q[i].used) { slot = i; break; }
  }
  if (slot < 0) return false;

  PendingMsg& pm = _q[slot];
  pm.used = true;
  pm.dst = dst;
  pm.msgId = rand32() ^ ((uint32_t)millis() << 1);
  pm.tries = 0;
  pm.nextSendAtMs = nowMs(); // send immediately
  strncpy(pm.text, text, sizeof(pm.text) - 1);
  pm.text[sizeof(pm.text)-1] = 0;

  return true;
}

void ReliableSender::onAck(uint16_t from, uint32_t msgId) {
  (void)from;
  for (auto &pm : _q) {
    if (pm.used && pm.msgId == msgId) {
      pm.used = false;
    }
  }
}

void ReliableSender::tick() {
  if (!_r || !_s) return;

  const uint32_t now = nowMs();

  for (auto &pm : _q) {
    if (!pm.used) continue;
    if ((int32_t)(now - pm.nextSendAtMs) < 0) continue;

    if (_s->ackReliability && pm.tries >= _s->maxRetries) {
      // Give up
      pm.used = false;
      continue;
    }

    // Build packet for the next transmit attempt
    WireChatPacket pkt;
    pkt.type = (uint8_t)AppMsgType::ChatMsg;
    pkt.msgId = pm.msgId;
    pkt.tsMs = now;
    pkt.textLen = (uint8_t)strnlen(pm.text, sizeof(pkt.text));
    memcpy(pkt.text, pm.text, pkt.textLen);
    pkt.text[pkt.textLen] = 0;

    _r->sendDm(pm.dst, pkt);

    // Log TX only on first attempt (avoids duplicate lines on retries)
    if (_log && pm.tries == 0) {
      ChatLine line;
      line.isTx = true;
      line.peer = pm.dst;
      line.millis = now;
      line.msgId = pm.msgId;
      strncpy(line.text, pm.text, sizeof(line.text)-1);
      _log->append(line);
    }

    pm.tries++;

    // Backoff with jitter to reduce collision probability
    uint32_t base = _s->ackReliability ? _s->ackTimeoutMs : 600;
    uint32_t jitter = (rand32() % 400);
    pm.nextSendAtMs = now + base + jitter + (pm.tries * 300);
  }
}
