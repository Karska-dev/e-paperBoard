#include "hal/wall_clock.h"

#include <sys/time.h>

namespace epb {

void WallClock::set(int64_t epochSeconds) {
    // settimeofday() is the standard POSIX call. On the ESP32 the time keeps
    // counting through deep sleep (driven by the RTC timer), so after one
    // successful sync the device knows the time on later wakes even before
    // it reaches the server. v0.1 does not use the time yet; it is kept
    // correct for the features that will (quiet hours, "updated HH:MM").
    timeval now = {};
    now.tv_sec = static_cast<time_t>(epochSeconds);
    settimeofday(&now, nullptr);
}

}  // namespace epb
