// The list of screens and how prev / home / next move through it.
//
// LAYER: pure.
#pragma once

#include <cstdint>

#include "pure/buttons.h"

namespace epb {

// Screen ids as the server knows them (the `id=` part of the request).
// The order here is the order the buttons flip through. Index 0 is Home.
// Only lowercase letters, digits and '-' so the ids can go into a URL
// without any escaping.
inline constexpr const char* kScreenIds[] = {
    "home",         // weather, today, to-do
    "time-left",    // day / week / month / year
    "year-dots",    // year in dots
    "night-sky",    // constellations
    "family-week",  // family calendar
    "weather",      // 12 h + 7 days
    "word",         // word of the day
};
inline constexpr uint8_t kScreenCount = sizeof(kScreenIds) / sizeof(kScreenIds[0]);
inline constexpr uint8_t kHomeScreen = 0;

// ALGORITHM: ring navigation with modular arithmetic.
// The screens form a circle: Next on the last screen goes to the first,
// Prev on the first goes to the last. The remainder operator (%) does the
// wrap-around: (current + 1) % count.
// Going backwards uses (current + count - 1) % count. Adding `count` first
// keeps the value positive; with unsigned numbers "0 - 1" would wrap to 255.
uint8_t navigate(uint8_t current, NavAction action, uint8_t count);

// Returns the id for an index. An out-of-range index gives Home, so a
// corrupted index can never produce an invalid request.
const char* screenId(uint8_t index);

}  // namespace epb
