#include "storage/SettingsStore.h"
#include <Preferences.h>

static const char* NS = "loradm";
static const char* KEY_VER = "ver";
static const uint32_t CUR_VER = 1;

bool SettingsStore::begin() {
  // Preferences doesn't need explicit begin here; we do it per call.
  _ok = true;
  return _ok;
}

static void loadRadio(Preferences& p, RadioSettings& r) {
  r.frequencyHz = p.getULong("freq", r.frequencyHz);
  r.bandwidthKhz = p.getFloat("bw", r.bandwidthKhz);
  r.spreadingFactor = p.getUChar("sf", r.spreadingFactor);
  r.syncWord = p.getUChar("sw", r.syncWord);
  r.txPowerDbm = (int8_t)p.getChar("pwr", r.txPowerDbm);
  r.preambleLen = p.getUShort("pre", r.preambleLen);
}

static void saveRadio(Preferences& p, const RadioSettings& r) {
  p.putULong("freq", r.frequencyHz);
  p.putFloat("bw", r.bandwidthKhz);
  p.putUChar("sf", r.spreadingFactor);
  p.putUChar("sw", r.syncWord);
  p.putChar("pwr", (int8_t)r.txPowerDbm);
  p.putUShort("pre", r.preambleLen);
}

void SettingsStore::load(AppSettings& out) {
  if (!_ok) return;

  Preferences p;
  p.begin(NS, true);
  const uint32_t ver = p.getULong(KEY_VER, 0);
  if (ver == CUR_VER) {
    loadRadio(p, out.radio);
    out.ackReliability = p.getBool("ack", out.ackReliability);
    out.maxRetries = p.getUChar("rtry", out.maxRetries);
    out.ackTimeoutMs = p.getUShort("atmo", out.ackTimeoutMs);
    out.ui.portrait = p.getBool("port", out.ui.portrait);
  }
  p.end();
}

void SettingsStore::save(const AppSettings& s) {
  if (!_ok) return;

  Preferences p;
  p.begin(NS, false);
  p.putULong(KEY_VER, CUR_VER);
  saveRadio(p, s.radio);
  p.putBool("ack", s.ackReliability);
  p.putUChar("rtry", s.maxRetries);
  p.putUShort("atmo", s.ackTimeoutMs);
  p.putBool("port", s.ui.portrait);
  p.end();
}
