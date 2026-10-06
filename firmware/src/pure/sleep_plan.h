// How long to sleep until the next wake.
// LAYER: pure.
#pragma once

#include <cstdint>

namespace epb {

struct SleepConfig {
    // Used when the server gives no hint. A placeholder until the server sends
    // its schedule (X-Next-Wake header).
    uint32_t defaultSeconds = 30 * 60;
    // The hint is clamped into [min, max]: a server bug can neither keep the
    // device awake nor put it to sleep for a month.
    uint32_t minSeconds = 60;
    uint32_t maxSeconds = 24 * 60 * 60;
    // Failure handling, see planSleepSeconds().
    uint32_t backoffBaseSeconds = 60;
    uint32_t backoffMaxSeconds = 60 * 60;
};

// Sleep time after one wake cycle.
// Success: the server's hint (0 = none) clamped to [min, max], or the default.
//
// ALGORITHM: exponential backoff. Each failure in a row doubles the wait, up
// to a cap. A hiccup is retried quickly; a long outage costs little battery
// (the radio is the hungriest part of the board).
//
//   failures in a row:  1   2   3   4   5    6    7+
//   wait in minutes:    1   2   4   8   16   32   60
//
// `consecutiveFailures` includes the failure that just happened.
uint32_t planSleepSeconds(bool cycleSucceeded, uint8_t consecutiveFailures, uint32_t serverHintSeconds,
                          const SleepConfig& config);

}  // namespace epb
