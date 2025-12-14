#include "util/TimeOnAir.h"
#include <math.h>

// Coding rate in Semtech formula uses CR = 1..4 representing 4/5..4/8.
// We accept denom 5..8 and convert.
static int crToSemtech(uint8_t cr_denom) {
  if (cr_denom <= 5) return 1;
  if (cr_denom == 6) return 2;
  if (cr_denom == 7) return 3;
  return 4;
}

uint32_t loraTimeOnAirUs(uint32_t bw_hz, uint8_t sf, uint8_t cr_denom, uint16_t preamble_len, uint16_t payload_bytes) {
  const double bw = (double)bw_hz;
  const double sf_d = (double)sf;
  const int cr = crToSemtech(cr_denom);

  const double ts = pow(2.0, sf_d) / bw; // symbol time
  const bool lowDataRateOptimize = (ts > 0.016); // ~16ms

  const int crcOn = 1;
  const int ih = 0; // explicit header
  const int de = lowDataRateOptimize ? 1 : 0;

  const double tpreamble = (preamble_len + 4.25) * ts;

  const double payloadSymbNb = 8 + fmax(
    ceil(( (double)(8*payload_bytes) - 4*sf_d + 28 + 16*crcOn - 20*ih ) / (4*(sf_d - 2*de))) * (cr + 4),
    0.0
  );

  const double tpayload = payloadSymbNb * ts;
  const double tpacket = tpreamble + tpayload;
  const uint32_t us = (uint32_t)llround(tpacket * 1e6);
  return us;
}
