// Tunable numbers of the firmware, in one place.
//
// PATTERN: named constants instead of magic numbers.
// A bare `4000` in the middle of the Wi-Fi code says nothing. A name says
// what the number means, the comment says why it has that value, and
// collecting them here shows every knob that can be turned.
#pragma once

#include <cstdint>

namespace epb::config {

// Wi-Fi. A connect with a channel/BSSID hint normally completes in well
// under a second, so if it has not after 4 s the hint is stale (the router
// changed channel) and we fall back to a full scan.
inline constexpr uint32_t kWifiFastConnectMs = 4000;
inline constexpr uint32_t kWifiFullConnectMs = 12000;

// HTTP. The read timeout is a STALL timeout: it is the longest time without
// any new byte, not a limit on the whole download.
inline constexpr uint32_t kHttpConnectTimeoutMs = 5000;
inline constexpr uint32_t kHttpStallTimeoutMs = 5000;

// Awake limit, see armAwakeLimit() in hal/power.h. A healthy cycle takes
// about 10 s including the panel refresh; 90 s is far beyond anything normal.
inline constexpr uint32_t kMaxAwakeSeconds = 90;
inline constexpr uint32_t kAwakeLimitSleepSeconds = 15 * 60;

// How long to wait for the user to let go of a key before sleeping.
inline constexpr uint32_t kButtonReleaseWaitMs = 3000;

inline constexpr uint32_t kSerialBaud = 115200;

}  // namespace epb::config
