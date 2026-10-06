// The state that must survive deep sleep.
// LAYER: pure. Describes the data and how to check it; hal/rtc_store.cpp
// stores it.
//
// Deep sleep powers down the CPU and normal RAM, so every wake is a reboot.
// Two kinds of memory survive:
//
//                 survives sleep   survives power loss   writing
//   RTC RAM            yes                 no            free
//   flash (NVS)        yes                 yes           wears the flash out
//
// So per-wake state lives here (RTC RAM) and rare settings live in NVS.
#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace epb {

// Longest ETag we keep, including the terminating zero.
inline constexpr size_t kEtagCapacity = 48;

// Channel and BSSID (the access point's MAC address) of the last connect.
// With them the next connect skips the channel scan, its slowest part.
struct WifiHint {
    uint8_t valid;  // 0 or 1
    uint8_t channel;
    uint8_t bssid[6];
};

// PATTERN: validated persistent record. Memory that outlives a reboot can be
// corrupt (brown-out) or stale (written by another layout), so it carries:
//   magic    a constant that random memory will not contain
//   version  bumped whenever the layout changes
//   crc      a checksum over all the other bytes
// If any check fails we start from clean defaults. hal/rtc_store.cpp explains
// how much of this is needed today.
//
// 76 bytes, no padding (padding bytes would make the checksum unreliable):
//
//   offset:  0      4        6           12         16    24        72
//   field:   magic  version  6 x 1 byte  wakeCount  wifi  etag[48]  crc
struct RtcState {
    uint32_t magic;
    uint16_t version;
    uint8_t shownScreen;          // Index of the screen on the panel.
    uint8_t hasFrame;             // 1 once a server image has been drawn.
    uint8_t lowBattery;           // Hysteresis memory, see battery_math.h.
    uint8_t consecutiveFailures;  // For the backoff in sleep_plan.h.
    uint8_t partialsSinceFull;    // See refresh_policy.h.
    uint8_t setupNoticeShown;     // 1 while the "Setup needed" notice is shown.
    uint32_t wakeCount;           // Wakes since power-on, for the log.
    WifiHint wifi;
    char etag[kEtagCapacity];  // ETag of the image on the panel, "" if none.
    uint32_t crc;              // Must stay the LAST member.
};

inline constexpr uint32_t kRtcMagic = 0x45504231;  // The ASCII bytes "EPB1".
inline constexpr uint16_t kRtcVersion = 1;

static_assert(std::is_trivially_copyable<RtcState>::value, "must be plain bytes");
static_assert(std::has_unique_object_representations<RtcState>::value, "no padding allowed");
static_assert(offsetof(RtcState, crc) == sizeof(RtcState) - sizeof(uint32_t), "crc must be last");

// ALGORITHM: CRC-32, the checksum of zip files and Ethernet. The data is
// treated as one long binary number; the checksum is the remainder of dividing
// it by a fixed constant. Any flipped bit changes the remainder. This is the
// bit-by-bit version: slower than a table, fine for 76 bytes per wake.
uint32_t crc32(const uint8_t* data, size_t length);

// A clean state with valid magic, version and checksum.
RtcState makeDefaultRtcState();

// Recomputes the checksum. Call after changing any field, before storing.
void sealRtcState(RtcState* state);

// True if magic, version and checksum are all correct.
bool isRtcStateValid(const RtcState& state);

}  // namespace epb
