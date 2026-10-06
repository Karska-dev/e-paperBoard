// Battery maths: noisy ADC samples in, "3940 mV, 68 %, not low" out.
// LAYER: pure. No hardware headers, so it is unit tested on a laptop.
#pragma once

#include <cstddef>
#include <cstdint>

namespace epb {

// One reading = 11 samples; the 2 highest and 2 lowest are dropped.
inline constexpr size_t kBatterySampleCount = 11;
inline constexpr size_t kBatteryTrimEachSide = 2;

// Two thresholds on purpose, see updateLowBatteryFlag().
inline constexpr uint8_t kLowBatteryOnPercent = 8;
inline constexpr uint8_t kLowBatteryOffPercent = 12;

// ALGORITHM: trimmed mean. Sort, drop `trimEachSide` values at both ends,
// average the rest. An outlier (an ADC glitch) is ignored, noise still
// averages out. Sorts `samples` in place; returns 0 if nothing is left.
uint16_t trimmedMean(uint16_t* samples, size_t count, size_t trimEachSide);

// 12-bit ADC reading (0..4095) -> battery millivolts.
//
//   raw / 4095 * 3.6 V * 2 * 0.968
//                |       |   '-- calibration (check with a multimeter)
//                |       '------ the board halves the voltage (divider)
//                '-------------- full-scale voltage of the ADC
uint16_t adcRawToMillivolts(uint16_t raw);

// ALGORITHM: lookup table + linear interpolation. A Li-ion cell discharges
// along a curve, not a line, so we keep measured (voltage, percent) points
// and draw straight lines between neighbours.
uint8_t millivoltsToPercent(uint16_t millivolts);

// PATTERN: hysteresis. Two thresholds stop the flag from flickering when the
// battery hovers around one value. In between, it keeps its previous value,
// which is why `wasLow` is an input.
//
//   percent:  0 ...... 8 | 9  10  11 | 12 ...... 100
//   flag:        low     |   keep    |    not low
bool updateLowBatteryFlag(bool wasLow, uint8_t percent);

}  // namespace epb
