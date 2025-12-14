#pragma once
#include <stdint.h>

// Time-on-air estimation for LoRa (rough, but useful for UI).
// Assumes explicit header, CRC enabled, low data rate optimize when needed.
// Reference formula is standard Semtech ToA model.
//
// Returns microseconds.
uint32_t loraTimeOnAirUs(uint32_t bw_hz, uint8_t sf, uint8_t cr_denom /*5..8*/, uint16_t preamble_len, uint16_t payload_bytes);
