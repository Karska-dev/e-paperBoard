// Sleeping and waking: the hardware side of the wake cycle.
#pragma once

#include <cstdint>

#include "app/wake_cycle.h"

namespace epb::power {

// Why did the chip start? Call once, early in setup().
WakeInfo readWakeInfo();

// Hands the button pins back from the sleep circuitry to normal use.
// Call once after readWakeInfo().
void releaseButtonPins();

// PATTERN: dead man's switch (a software watchdog for the whole cycle).
// Starts a timer that forces the device into deep sleep after `maxAwake`
// seconds no matter what the main code is doing. Every individual wait in
// the firmware already has its own timeout, but this is the safety net for
// the bug nobody thought of: a hang in a library, an endless loop. Without
// it such a bug keeps the chip awake until the battery is flat; with it the
// damage is one long cycle, and the next wake gets a fresh start.
void armAwakeLimit(uint32_t maxAwakeSeconds, uint32_t thenSleepSeconds);

// Powers down until the timer expires or a button is pressed. Never returns:
// the next thing that runs is setup(), from the top.
[[noreturn]] void deepSleepFor(uint32_t seconds);

}  // namespace epb::power
