// How long to sleep until the next wake.
//
// LAYER: pure.
#pragma once

#include <cstdint>

namespace epb {

struct SleepConfig {
    // Used when the server gives no hint. Placeholder value: the real
    // schedule will come from the server (X-Next-Wake header).
    uint32_t defaultSeconds = 30 * 60;
    // The server's hint is clamped into [min, max] so a bug on the server
    // can neither keep the device awake nor put it to sleep for a month.
    uint32_t minSeconds = 60;
    uint32_t maxSeconds = 24 * 60 * 60;
    // Failure handling, see planSleepSeconds().
    uint32_t backoffBaseSeconds = 60;
    uint32_t backoffMaxSeconds = 60 * 60;
};

// Decides the sleep time after one wake cycle.
//
// Success: use the server's hint (0 means "no hint"), clamped to the allowed
// range, or the default interval.
//
// ALGORITHM: exponential backoff.
// After a failure (no Wi-Fi, server down) retrying at full speed would
// drain the battery against a problem that is probably still there: the
// Wi-Fi radio is the most power-hungry part of the board. So each failure
// in a row DOUBLES the wait: 1 min, 2 min, 4 min, 8 min ... up to a cap of
// one hour. A short hiccup is retried quickly, a long outage costs little.
// `consecutiveFailures` is the count INCLUDING the failure that just happened.
uint32_t planSleepSeconds(bool cycleSucceeded, uint8_t consecutiveFailures, uint32_t serverHintSeconds,
                          const SleepConfig& config);

}  // namespace epb
