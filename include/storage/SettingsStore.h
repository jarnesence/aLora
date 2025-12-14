#pragma once
#include "storage/Settings.h"

// Non-volatile settings (Preferences).
class SettingsStore {
public:
  bool begin();
  void load(AppSettings& out);
  void save(const AppSettings& s);

private:
  bool _ok = false;
};
