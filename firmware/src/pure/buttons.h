// Buttons: from "which pin woke the chip" to "what did the user mean".
//
// LAYER: pure (no hardware headers), so the decoding is unit tested on a
// laptop. The actual pin numbers live in src/hal/board.h.
#pragma once

#include <cstddef>
#include <cstdint>

namespace epb {

// What a button press means to the application. The rest of the firmware
// only ever sees these values, never GPIO numbers.
enum class NavAction : uint8_t {
    None,  // Not a button wake (timer or power-on).
    Prev,
    Home,
    Next,
};

// PATTERN: table-driven mapping.
// One row per physical button: which GPIO it is wired to and what it means.
// Instead of scattering "if pin == 3" checks through the code, the whole
// assignment of keys to actions is one small table (see kButtons in
// src/hal/board.h). Changing the layout means editing one line of data, not
// logic.
struct ButtonPin {
    uint8_t gpio;
    NavAction action;
};

// ALGORITHM: bit mask.
// The chip reports "which pins woke me" as a 64-bit number in which bit N
// stands for GPIO N. Example: GPIO 3 pressed gives the mask 0b1000 (1 << 3).
// This builds the mask that has one bit set for each button in the table,
// which is what the sleep code hands to the chip as "wake me on any of these".
uint64_t wakeMaskFor(const ButtonPin* table, size_t count);

// Translates the wake mask back into an action. If two buttons were down at
// once, the one that comes FIRST in the table wins, so the table order is
// also the priority order.
NavAction decodeWakeMask(uint64_t wakeMask, const ButtonPin* table, size_t count);

// Short lowercase name for logs and for the request sent to the server.
const char* navActionName(NavAction action);

}  // namespace epb
