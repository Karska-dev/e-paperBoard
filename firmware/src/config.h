// Tunable numbers of the firmware, in one place.
// PATTERN: named constants, no magic numbers. The name says what a number
// means; the comment says why it has that value.
#pragma once

#include <cstdint>

namespace epb::config {

// Wi-Fi. A connect with a hint takes well under a second. If it has not
// finished after 4 s the hint is stale, and we fall back to a full scan.
inline constexpr uint32_t kWifiFastConnectMs = 4000;
inline constexpr uint32_t kWifiFullConnectMs = 12000;

// HTTP. The second one is a STALL timeout: the longest time without a new
// byte, not a limit on the whole download.
inline constexpr uint32_t kHttpConnectTimeoutMs = 5000;
inline constexpr uint32_t kHttpStallTimeoutMs = 5000;

// Awake limit, see armAwakeLimit() in hal/power.h. A healthy cycle takes
// about 10 s, so 90 s is far beyond normal.
inline constexpr uint32_t kMaxAwakeSeconds = 90;
inline constexpr uint32_t kAwakeLimitSleepSeconds = 15 * 60;

// How long to wait for the user to let go of a key before sleeping.
inline constexpr uint32_t kButtonReleaseWaitMs = 3000;

inline constexpr uint32_t kSerialBaud = 115200;

}  // namespace epb::config
