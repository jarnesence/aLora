#include <Arduino.h>

#include "config/AppBuildConfig.h"
#include "net/MeshRadio.h"
#include "storage/ChatLog.h"
#include "input/Rotary.h"
#include "ui/Ui.h"

#if defined(APP_DISPLAY_OLED)
  #include "display/DisplayOledU8g2.h"
#elif defined(APP_DISPLAY_TFT)
  #include "display/DisplayTftStub.h"
#endif

static MeshRadio g_radio;
static ChatLog g_log;
static RotaryInput g_rotary;

#if defined(APP_DISPLAY_OLED)
  static DisplayOledU8g2 g_display;
  static Ui g_ui(&g_display, &g_rotary, &g_radio, &g_log);
#elif defined(APP_DISPLAY_TFT)
  static DisplayTftStub g_display;
  static Ui g_ui(&g_display, &g_rotary, &g_radio, &g_log);
#else
  static Ui g_ui(nullptr, &g_rotary, &g_radio, &g_log);
#endif

static void onRadioRx(uint16_t src, const WireChatPacket& pkt, int16_t rssi, float snr) {
  (void)rssi; (void)snr;
  g_ui.onIncoming(src, pkt);
}

void setup() {
  Serial.begin(APP_LOG_BAUD);
  delay(200);
  Serial.println();
  Serial.println("aLora starting...");

#if defined(APP_DISPLAY_OLED) || defined(APP_DISPLAY_TFT)
  g_display.begin();
#endif

#if (APP_ENABLE_ROTARY == 1)
  g_rotary.begin();
#endif

  g_radio.setRxCallback(onRadioRx);
  g_radio.begin();

  g_ui.begin();

#if defined(APP_DISPLAY_OLED)
  // Add a local boot message to chat log to confirm UI is alive.
  g_log.add(0, true, "aLora boot", (uint32_t)(millis()/1000));
#endif
}

void loop() {
  g_ui.tick();
  delay(5);
}
