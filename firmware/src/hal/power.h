// Sleeping and waking: the hardware side of the wake cycle.
#pragma once

#include <cstdint>

#include "app/wake_cycle.h"

namespace epb::power {

// Why did the chip start? Call once, early in setup().
WakeInfo readWakeInfo();

// Hands the key pins back from the sleep circuitry. Call once, after
// readWakeInfo().
void releaseButtonPins();

// PATTERN: dead man's switch (a watchdog for the whole cycle). A timer forces
// deep sleep after `maxAwakeSeconds`, whatever the main code is doing. Every
// wait already has its own timeout; this catches the hang nobody thought of,
// so the damage is one long cycle and not a flat battery.
void armAwakeLimit(uint32_t maxAwakeSeconds, uint32_t thenSleepSeconds);

// Sleeps until the timer expires or a key is pressed. Never returns: the next
// thing that runs is setup(), from the top.
[[noreturn]] void deepSleepFor(uint32_t seconds);

}  // namespace epb::power
