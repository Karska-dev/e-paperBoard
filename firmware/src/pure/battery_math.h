// Battery maths: turning noisy ADC samples into "3.94 V, 78 %, not low".
//
// LAYER: pure. Nothing in src/pure/ may include Arduino or ESP-IDF headers.
// That single rule is what lets these files compile on a laptop and be unit
// tested without a board (see tests/host/). The hardware part of the battery
// reading (switching the divider on, calling analogRead) lives in
// src/hal/battery_adc.cpp and is kept as thin as possible.
#pragma once

#include <cstddef>
#include <cstdint>

namespace epb {

// How many ADC samples one battery reading takes, and how many of the
// highest and lowest are thrown away before averaging. 11 samples with 2
// trimmed on each side leaves the middle 7.
inline constexpr size_t kBatterySampleCount = 11;
inline constexpr size_t kBatteryTrimEachSide = 2;

// Low-battery thresholds, in percent. Two different numbers on purpose:
// see updateLowBatteryFlag().
inline constexpr uint8_t kLowBatteryOnPercent = 8;
inline constexpr uint8_t kLowBatteryOffPercent = 12;

// ALGORITHM: trimmed mean.
// Sort the samples, drop `trimEachSide` values from both ends, average the
// rest. A plain average is pulled around by a single wild sample (an ADC
// glitch while the Wi-Fi radio fires, for example). A median ignores
// outliers but throws away most of the data. The trimmed mean sits between
// the two: outliers are discarded, the remaining samples still average
// their noise away.
//
// Sorts `samples` in place. Returns 0 if there is nothing left to average.
uint16_t trimmedMean(uint16_t* samples, size_t count, size_t trimEachSide);

// Converts a 12-bit ADC reading (0..4095) to battery millivolts.
// Formula for this board: raw / 4095 * 3.6 V * 2 * 0.968
//   3.6 V  full-scale voltage of the ADC at the attenuation Arduino uses
//   2      the board halves the battery voltage with a resistor divider
//   0.968  calibration factor (starting value, check with a multimeter)
uint16_t adcRawToMillivolts(uint16_t raw);

// ALGORITHM: lookup table with linear interpolation.
// A Li-ion cell does not discharge in a straight line: it sits near 3.7-3.8 V
// for most of its life and then falls off a cliff. So instead of one formula
// we keep a table of (voltage, percent) points measured on a real discharge
// curve and draw straight lines between neighbouring points.
uint8_t millivoltsToPercent(uint16_t millivolts);

// PATTERN: hysteresis (the software version of a Schmitt trigger).
// With a single threshold, a battery hovering around it would flip the
// "low" flag on and off on every reading. With two thresholds the flag
// turns ON at or below 8 % and only turns OFF again at or above 12 %.
// In between, it keeps whatever value it had. That is why this function
// needs the previous value as an input.
bool updateLowBatteryFlag(bool wasLow, uint8_t percent);

}  // namespace epb
