# LoRaDM (LoRaMesher-based Direct Messaging)

This is an early, DIY-focused firmware skeleton:
- **LoRaMesher** for mesh routing + reliable DM packets
- **U8g2** for OLED UI (portrait-first)
- **AiEsp32RotaryEncoder** for EC11 rotary input

## Current status
- OLED init now prints a boot banner + frame test.
- UI screens: Chat / Compose / Status / Settings (minimal scaffolding).
- Radio: LoRaMesher reliable send + receive task.

## OLED troubleshooting (SSD1315 / SSD1306)
If you get **no image**:
1. Try forcing the I2C address:
   - `-D APP_OLED_I2C_ADDR=0x3C` or `0x3D`
2. Try forcing the controller:
   - `-D APP_OLED_CTRL=1315` (SSD1315) or `1306` (SSD1306)
3. Confirm pins in `include/pinmaps/*` match your wiring.
4. Confirm the OLED RESET line (if present) is connected and matches `APP_OLED_RESET`.

The firmware prints OLED config to Serial on boot:
`[OLED] ctrl=... addr=... SDA=... SCL=...`

## Feature selection
All selection is done **per PlatformIO environment** (platformio.ini build_flags):
- MCU env: `esp32s3`, `esp32c3`, ...
- Pinmap: `APP_PINMAP_SUPERMINI_S3`, ...
- Display: `APP_DISPLAY_OLED` / `APP_DISPLAY_NONE`
- Input: `APP_ENABLE_ROTARY`
- Radio module: `APP_LORA_MODULE_SX1278` etc.

## Next steps (planned)
- Proper rotary text input (focus switching, backspace, send action).
- Settings editing and persistence (LittleFS).
- Real RSSI/SNR + airtime stats (RadioLib integration).
- TFT UI plugin (SPI TFT) as an optional build component.
