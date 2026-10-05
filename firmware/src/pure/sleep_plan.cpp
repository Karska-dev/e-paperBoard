#include "pure/sleep_plan.h"

namespace epb {

namespace {

uint32_t clampTo(uint32_t value, uint32_t low, uint32_t high) {
    if (value < low) {
        return low;
    }
    return value > high ? high : value;
}

}  // namespace

uint32_t planSleepSeconds(bool cycleSucceeded, uint8_t consecutiveFailures, uint32_t serverHintSeconds,
                          const SleepConfig& config) {
    if (cycleSucceeded) {
        const uint32_t wanted = serverHintSeconds != 0 ? serverHintSeconds : config.defaultSeconds;
        return clampTo(wanted, config.minSeconds, config.maxSeconds);
    }

    // base * 2^(failures - 1), computed by doubling in a loop that stops as
    // soon as the cap is reached. Stopping early matters: doubling a 32-bit
    // number 40 times would overflow and wrap around to a tiny value, which
    // would turn "wait longer" into "retry immediately".
    uint32_t wait = config.backoffBaseSeconds;
    for (uint8_t i = 1; i < consecutiveFailures && wait < config.backoffMaxSeconds; ++i) {
        wait *= 2;
    }
    return wait > config.backoffMaxSeconds ? config.backoffMaxSeconds : wait;
}

}  // namespace epb
