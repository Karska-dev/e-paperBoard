// Buttons: from "which pin woke the chip" to "what did the user mean".
// LAYER: pure. The pin numbers live in hal/board.h.
#pragma once

#include <cstddef>
#include <cstdint>

namespace epb {

// What a press means. The rest of the firmware sees these, never GPIO numbers.
enum class NavAction : uint8_t {
    None,  // Not a button wake (timer or power-on).
    Prev,
    Home,
    Next,
};

// PATTERN: table-driven mapping. One row per key (GPIO, meaning) replaces
// "if pin == 3" checks scattered through the code. A new layout is a data
// edit in hal/board.h, not a logic change.
struct ButtonPin {
    uint8_t gpio;
    NavAction action;
};

// ALGORITHM: bit mask. A set of pins in one number: bit N stands for GPIO N,
// so GPIO 3 is 0b1000 (1 << 3). Returns the mask of all keys in the table,
// which the chip takes as "wake me on any of these".
uint64_t wakeMaskFor(const ButtonPin* table, size_t count);

// Mask -> action. If several keys are down, the first table row wins, so
// table order is priority order.
NavAction decodeWakeMask(uint64_t wakeMask, const ButtonPin* table, size_t count);

// Lowercase name for logs and for the server request.
const char* navActionName(NavAction action);

}  // namespace epb
