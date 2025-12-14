#pragma once
#include <type_traits>
#include <utility>

// Small compile-time compatibility helpers to smooth API differences between library versions.
// Use SFINAE to detect members and methods without breaking compilation.

namespace compat {

// ---- member detection ----
template <typename, typename = void> struct has_loraCs : std::false_type {};
template <typename T> struct has_loraCs<T, std::void_t<decltype(std::declval<T>().loraCs)>> : std::true_type {};

template <typename, typename = void> struct has_loraCS : std::false_type {};
template <typename T> struct has_loraCS<T, std::void_t<decltype(std::declval<T>().loraCS)>> : std::true_type {};

template <typename, typename = void> struct has_loraIrq : std::false_type {};
template <typename T> struct has_loraIrq<T, std::void_t<decltype(std::declval<T>().loraIrq)>> : std::true_type {};

template <typename, typename = void> struct has_loraIRQ : std::false_type {};
template <typename T> struct has_loraIRQ<T, std::void_t<decltype(std::declval<T>().loraIRQ)>> : std::true_type {};

template <typename, typename = void> struct has_loraRst : std::false_type {};
template <typename T> struct has_loraRst<T, std::void_t<decltype(std::declval<T>().loraRst)>> : std::true_type {};

template <typename, typename = void> struct has_loraRST : std::false_type {};
template <typename T> struct has_loraRST<T, std::void_t<decltype(std::declval<T>().loraRST)>> : std::true_type {};

template <typename, typename = void> struct has_loraIo1 : std::false_type {};
template <typename T> struct has_loraIo1<T, std::void_t<decltype(std::declval<T>().loraIo1)>> : std::true_type {};

template <typename, typename = void> struct has_loraI01 : std::false_type {};
template <typename T> struct has_loraI01<T, std::void_t<decltype(std::declval<T>().loraI01)>> : std::true_type {};

template <typename, typename = void> struct has_module : std::false_type {};
template <typename T> struct has_module<T, std::void_t<decltype(std::declval<T>().module)>> : std::true_type {};

template <typename, typename = void> struct has_freq : std::false_type {};
template <typename T> struct has_freq<T, std::void_t<decltype(std::declval<T>().freq)>> : std::true_type {};

template <typename, typename = void> struct has_frequency : std::false_type {};
template <typename T> struct has_frequency<T, std::void_t<decltype(std::declval<T>().frequency)>> : std::true_type {};

template <typename, typename = void> struct has_bw : std::false_type {};
template <typename T> struct has_bw<T, std::void_t<decltype(std::declval<T>().bw)>> : std::true_type {};

template <typename, typename = void> struct has_band : std::false_type {};
template <typename T> struct has_band<T, std::void_t<decltype(std::declval<T>().band)>> : std::true_type {};

template <typename, typename = void> struct has_sf : std::false_type {};
template <typename T> struct has_sf<T, std::void_t<decltype(std::declval<T>().sf)>> : std::true_type {};

template <typename, typename = void> struct has_spreadingFactor : std::false_type {};
template <typename T> struct has_spreadingFactor<T, std::void_t<decltype(std::declval<T>().spreadingFactor)>> : std::true_type {};

template <typename, typename = void> struct has_power : std::false_type {};
template <typename T> struct has_power<T, std::void_t<decltype(std::declval<T>().power)>> : std::true_type {};

template <typename, typename = void> struct has_preambleLength : std::false_type {};
template <typename T> struct has_preambleLength<T, std::void_t<decltype(std::declval<T>().preambleLength)>> : std::true_type {};

template <typename, typename = void> struct has_preamble : std::false_type {};
template <typename T> struct has_preamble<T, std::void_t<decltype(std::declval<T>().preamble)>> : std::true_type {};

template <typename, typename = void> struct has_syncWord : std::false_type {};
template <typename T> struct has_syncWord<T, std::void_t<decltype(std::declval<T>().syncWord)>> : std::true_type {};

// ---- helper setters ----
template <typename Cfg>
inline void set_lora_cs(Cfg& cfg, int pin) {
  if constexpr (has_loraCs<Cfg>::value) cfg.loraCs = pin;
  else if constexpr (has_loraCS<Cfg>::value) cfg.loraCS = pin;
}

template <typename Cfg>
inline void set_lora_irq(Cfg& cfg, int pin) {
  if constexpr (has_loraIrq<Cfg>::value) cfg.loraIrq = pin;
  else if constexpr (has_loraIRQ<Cfg>::value) cfg.loraIRQ = pin;
}

template <typename Cfg>
inline void set_lora_rst(Cfg& cfg, int pin) {
  if constexpr (has_loraRst<Cfg>::value) cfg.loraRst = pin;
  else if constexpr (has_loraRST<Cfg>::value) cfg.loraRST = pin;
}

template <typename Cfg>
inline void set_lora_io1(Cfg& cfg, int pin) {
  if constexpr (has_loraIo1<Cfg>::value) cfg.loraIo1 = pin;
  else if constexpr (has_loraI01<Cfg>::value) cfg.loraI01 = pin;
}

template <typename Cfg>
inline void set_freq_mhz(Cfg& cfg, float mhz) {
  if constexpr (has_freq<Cfg>::value) cfg.freq = mhz;
  else if constexpr (has_frequency<Cfg>::value) cfg.frequency = mhz;
}

template <typename Cfg>
inline void set_bw_khz(Cfg& cfg, float khz) {
  if constexpr (has_bw<Cfg>::value) cfg.bw = khz;
  else if constexpr (has_band<Cfg>::value) cfg.band = khz;
}

template <typename Cfg>
inline void set_sf(Cfg& cfg, int sf) {
  if constexpr (has_sf<Cfg>::value) cfg.sf = sf;
  else if constexpr (has_spreadingFactor<Cfg>::value) cfg.spreadingFactor = sf;
}

template <typename Cfg>
inline void set_power_dbm(Cfg& cfg, int dbm) {
  if constexpr (has_power<Cfg>::value) cfg.power = dbm;
}

template <typename Cfg>
inline void set_preamble(Cfg& cfg, int pre) {
  if constexpr (has_preambleLength<Cfg>::value) cfg.preambleLength = pre;
  else if constexpr (has_preamble<Cfg>::value) cfg.preamble = pre;
}

template <typename Cfg>
inline void set_syncword(Cfg& cfg, int sw) {
  if constexpr (has_syncWord<Cfg>::value) cfg.syncWord = sw;
}

} // namespace compat
