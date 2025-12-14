#pragma once
#include <type_traits>

// C++11-friendly compatibility layer for LoRaMesherConfig field name changes across releases.
// Avoids if constexpr / C++17-only features.

namespace compat {

// Helper: try first overload (int tag), then fall back (long tag).
// Each overload only participates if the member exists.

template <typename Cfg>
auto set_lora_cs_impl(Cfg& cfg, int pin, int) -> decltype((void)(cfg.loraCs = pin), void()) { cfg.loraCs = pin; }
template <typename Cfg>
auto set_lora_cs_impl(Cfg& cfg, int pin, long) -> decltype((void)(cfg.loraCS = pin), void()) { cfg.loraCS = pin; }
template <typename Cfg>
inline void set_lora_cs(Cfg& cfg, int pin) { set_lora_cs_impl(cfg, pin, 0); }

template <typename Cfg>
auto set_lora_irq_impl(Cfg& cfg, int pin, int) -> decltype((void)(cfg.loraIrq = pin), void()) { cfg.loraIrq = pin; }
template <typename Cfg>
auto set_lora_irq_impl(Cfg& cfg, int pin, long) -> decltype((void)(cfg.loraIRQ = pin), void()) { cfg.loraIRQ = pin; }
template <typename Cfg>
inline void set_lora_irq(Cfg& cfg, int pin) { set_lora_irq_impl(cfg, pin, 0); }

template <typename Cfg>
auto set_lora_rst_impl(Cfg& cfg, int pin, int) -> decltype((void)(cfg.loraRst = pin), void()) { cfg.loraRst = pin; }
template <typename Cfg>
auto set_lora_rst_impl(Cfg& cfg, int pin, long) -> decltype((void)(cfg.loraRST = pin), void()) { cfg.loraRST = pin; }
template <typename Cfg>
inline void set_lora_rst(Cfg& cfg, int pin) { set_lora_rst_impl(cfg, pin, 0); }

template <typename Cfg>
auto set_lora_io1_impl(Cfg& cfg, int pin, int) -> decltype((void)(cfg.loraIo1 = pin), void()) { cfg.loraIo1 = pin; }
template <typename Cfg>
auto set_lora_io1_impl(Cfg& cfg, int pin, long) -> decltype((void)(cfg.loraI01 = pin), void()) { cfg.loraI01 = pin; }
template <typename Cfg>
inline void set_lora_io1(Cfg& cfg, int pin) { set_lora_io1_impl(cfg, pin, 0); }

template <typename Cfg>
auto set_freq_mhz_impl(Cfg& cfg, float mhz, int) -> decltype((void)(cfg.freq = mhz), void()) { cfg.freq = mhz; }
template <typename Cfg>
auto set_freq_mhz_impl(Cfg& cfg, float mhz, long) -> decltype((void)(cfg.frequency = mhz), void()) { cfg.frequency = mhz; }
template <typename Cfg>
inline void set_freq_mhz(Cfg& cfg, float mhz) { set_freq_mhz_impl(cfg, mhz, 0); }

template <typename Cfg>
auto set_bw_khz_impl(Cfg& cfg, float khz, int) -> decltype((void)(cfg.bw = khz), void()) { cfg.bw = khz; }
template <typename Cfg>
auto set_bw_khz_impl(Cfg& cfg, float khz, long) -> decltype((void)(cfg.band = khz), void()) { cfg.band = khz; }
template <typename Cfg>
inline void set_bw_khz(Cfg& cfg, float khz) { set_bw_khz_impl(cfg, khz, 0); }

template <typename Cfg>
auto set_sf_impl(Cfg& cfg, int sf, int) -> decltype((void)(cfg.sf = sf), void()) { cfg.sf = sf; }
template <typename Cfg>
auto set_sf_impl(Cfg& cfg, int sf, long) -> decltype((void)(cfg.spreadingFactor = sf), void()) { cfg.spreadingFactor = sf; }
template <typename Cfg>
inline void set_sf(Cfg& cfg, int sf) { set_sf_impl(cfg, sf, 0); }

template <typename Cfg>
auto set_syncword_impl(Cfg& cfg, int sw, int) -> decltype((void)(cfg.syncWord = sw), void()) { cfg.syncWord = sw; }
template <typename Cfg>
auto set_syncword_impl(Cfg& cfg, int sw, long) -> decltype((void)(cfg.synchronizationWord = sw), void()) { cfg.synchronizationWord = sw; }
template <typename Cfg>
inline void set_syncword(Cfg& cfg, int sw) { set_syncword_impl(cfg, sw, 0); }

template <typename Cfg>
auto set_power_dbm_impl(Cfg& cfg, int dbm, int) -> decltype((void)(cfg.power = dbm), void()) { cfg.power = dbm; }
template <typename Cfg>
auto set_power_dbm_impl(Cfg& cfg, int dbm, long) -> decltype((void)(cfg.txPower = dbm), void()) { cfg.txPower = dbm; }
template <typename Cfg>
inline void set_power_dbm(Cfg& cfg, int dbm) { set_power_dbm_impl(cfg, dbm, 0); }

template <typename Cfg>
auto set_preamble_impl(Cfg& cfg, int pre, int) -> decltype((void)(cfg.preambleLength = pre), void()) { cfg.preambleLength = pre; }
template <typename Cfg>
auto set_preamble_impl(Cfg& cfg, int pre, long) -> decltype((void)(cfg.preamble = pre), void()) { cfg.preamble = pre; }
template <typename Cfg>
inline void set_preamble(Cfg& cfg, int pre) { set_preamble_impl(cfg, pre, 0); }

// Optional fields (SPI class, module enum) – ignore if not present.
template <typename Cfg, typename SpiT>
auto set_spi_impl(Cfg& cfg, SpiT* spi, int) -> decltype((void)(cfg.spi = spi), void()) { cfg.spi = spi; }
template <typename Cfg, typename SpiT>
inline void set_spi_impl(Cfg&, SpiT*, long) {}
template <typename Cfg, typename SpiT>
inline void set_spi(Cfg& cfg, SpiT* spi) { set_spi_impl(cfg, spi, 0); }

template <typename Cfg, typename ModT>
auto set_module_impl(Cfg& cfg, ModT mod, int) -> decltype((void)(cfg.module = mod), void()) { cfg.module = mod; }
template <typename Cfg, typename ModT>
inline void set_module_impl(Cfg&, ModT, long) {}
template <typename Cfg, typename ModT>
inline void set_module(Cfg& cfg, ModT mod) { set_module_impl(cfg, mod, 0); }

} // namespace compat
