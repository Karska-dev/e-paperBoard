// Board description: which GPIO does what on the XIAO ePaper Display Board
// (EE04) with the XIAO ESP32-S3 Plus, as shipped in the Seeed TRMNL 7.5"
// (OG) DIY kit.
//
// LAYER: hal, but this file is plain data with no hardware headers, so the
// host tests can include it and check the table.
//
// PATTERN: single source of truth.
// Every pin number in the firmware comes from this file. If the wiring ever
// changes, or the firmware moves to another board, this is the one place to
// edit. No other file contains a raw GPIO number.
#pragma once

#include <cstddef>
#include <cstdint>

#include "pure/buttons.h"

namespace epb::board {

// The three user keys. Active low: pressed connects the pin to ground, so a
// pressed key reads 0. A pull-up resistor holds the pin at 1 otherwise.
// Pin mapping confirmed on our unit on 2026-10-05.
inline constexpr uint8_t kPinKey1 = 2;  // XIAO pin D1
inline constexpr uint8_t kPinKey2 = 3;  // XIAO pin D2
inline constexpr uint8_t kPinKey3 = 5;  // XIAO pin D4

// Battery measurement. GPIO6 HIGH connects the voltage divider to the
// battery; it is kept LOW the rest of the time so the divider does not
// slowly drain the cell.
inline constexpr uint8_t kPinBatteryAdc = 1;
inline constexpr uint8_t kPinBatteryEnable = 6;

// What each key means (see "table-driven mapping" in pure/buttons.h).
//
// Looking at the screen, the keys are, from left to right:
//   key 1 = HOME, key 2 = NEXT, key 3 = PREV
// Any labels on the case have to follow this order. If the layout ever
// changes, this table is the only thing to edit.
//
// Row order is also the priority when two keys are down at once: Home wins.
inline constexpr ButtonPin kButtons[] = {
    {kPinKey1, NavAction::Home},  // left
    {kPinKey2, NavAction::Next},  // middle
    {kPinKey3, NavAction::Prev},  // right
};
inline constexpr size_t kButtonCount = sizeof(kButtons) / sizeof(kButtons[0]);

}  // namespace epb::board
