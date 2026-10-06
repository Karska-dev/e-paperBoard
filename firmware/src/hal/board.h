// Board description: which GPIO does what on the XIAO ePaper Display Board
// EE04 (Seeed TRMNL 7.5" OG DIY kit).
// LAYER: hal, but plain data without hardware headers, so tests can use it.
//
// PATTERN: single source of truth. Every pin number in the firmware comes
// from this file; no other file contains a raw GPIO number.
#pragma once

#include <cstddef>
#include <cstdint>

#include "pure/buttons.h"

namespace epb::board {

// The three keys. Active low: a pressed key connects the pin to ground and
// reads 0; a pull-up holds it at 1 otherwise. Confirmed on our unit.
inline constexpr uint8_t kPinKey1 = 2;  // XIAO pin D1
inline constexpr uint8_t kPinKey2 = 3;  // XIAO pin D2
inline constexpr uint8_t kPinKey3 = 5;  // XIAO pin D4

// Battery measurement. GPIO6 HIGH connects the voltage divider; it stays LOW
// otherwise, so the divider does not drain the cell.
inline constexpr uint8_t kPinBatteryAdc = 1;
inline constexpr uint8_t kPinBatteryEnable = 6;

// What each key means (PATTERN: table-driven mapping, see pure/buttons.h).
//
//   looking at the screen:   [ key 1 ]   [ key 2 ]   [ key 3 ]
//                              HOME        NEXT        PREV
//
// Row order is also the priority when two keys are down at once: Home wins.
inline constexpr ButtonPin kButtons[] = {
    {kPinKey1, NavAction::Home},  // left
    {kPinKey2, NavAction::Next},  // middle
    {kPinKey3, NavAction::Prev},  // right
};
inline constexpr size_t kButtonCount = sizeof(kButtons) / sizeof(kButtons[0]);

}  // namespace epb::board
