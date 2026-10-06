// The list of screens and how prev / home / next move through it.
// LAYER: pure.
#pragma once

#include <cstdint>

#include "pure/buttons.h"

namespace epb {

// Screen ids as the server knows them, in button order. Index 0 is Home.
// Only a-z, 0-9 and '-', so they need no escaping in a URL.
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

// ALGORITHM: ring navigation with the remainder operator (%).
//
//   home -> time-left -> year-dots -> ... -> word --+
//     ^---------------------------------------------+
//
//   next: (current + 1) % count
//   prev: (current + count - 1) % count   "+ count" avoids 0 - 1 = 255
uint8_t navigate(uint8_t current, NavAction action, uint8_t count);

// Id for an index. Out of range gives Home, so a corrupted index can never
// produce an invalid request.
const char* screenId(uint8_t index);

}  // namespace epb
