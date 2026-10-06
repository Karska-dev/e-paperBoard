#include "hal/wall_clock.h"

#include <sys/time.h>

namespace epb {

void WallClock::set(int64_t epochSeconds) {
    // settimeofday() is the standard POSIX call. The ESP32 keeps counting
    // through deep sleep, so one sync serves later wakes too. v0.1 does not
    // use the time yet (planned: quiet hours, "updated HH:MM").
    timeval now = {};
    now.tv_sec = static_cast<time_t>(epochSeconds);
    settimeofday(&now, nullptr);
}

}  // namespace epb
