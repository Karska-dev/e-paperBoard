// The small amount of state that must survive deep sleep.
//
// LAYER: pure. This file only describes the data and how to check it.
// Where it is physically stored is the business of src/hal/rtc_store.cpp.
//
// BACKGROUND: deep sleep on the ESP32 powers down the CPU and normal RAM.
// Waking up is a reboot: setup() runs from the top and every ordinary
// variable is back at its initial value. Two kinds of memory survive:
//   - RTC RAM: a few kilobytes that stay powered during deep sleep. Free to
//     write as often as we like, but lost when the battery is removed.
//   - Flash (NVS): survives power loss, but flash cells wear out after
//     roughly 100,000 writes. Fine for settings, wrong for a counter that
//     changes on every wake.
// So: per-wake state goes into RTC RAM (this struct), rarely-changing
// settings go into NVS (src/storage/).
#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace epb {

// Longest ETag we remember, including the terminating zero byte.
inline constexpr size_t kEtagCapacity = 48;

// What we learned about the access point on the last successful connect.
// Handing channel + BSSID (the access point's MAC address) to the next
// connect lets the radio skip scanning all channels, which is the slowest
// part of joining Wi-Fi.
struct WifiHint {
    uint8_t valid;  // 0 or 1
    uint8_t channel;
    uint8_t bssid[6];
};

// PATTERN: validated persistent record (magic number + version + checksum).
// Memory that outlives a reboot must not be trusted blindly: bits can flip
// when the battery sags during sleep (brown-out), and data written by a
// firmware with a different layout would be misread. Before using it we
// check three things:
//   magic    a fixed constant. Random memory will not contain it.
//   version  bumped whenever the layout below changes. Old data is dropped.
//   crc      a checksum over all the other bytes. Detects corruption.
// If any check fails we start from clean defaults.
// (How much of this is needed today is explained in src/hal/rtc_store.cpp.)
//
// All members are fixed-size integers and are ordered so that the compiler
// inserts no padding bytes between them (padding has undefined content and
// would make the checksum unreliable). The static_asserts below enforce it.
struct RtcState {
    uint32_t magic;
    uint16_t version;
    uint8_t shownScreen;          // Index of the screen currently on the panel.
    uint8_t hasFrame;             // 1 once a server image has been drawn.
    uint8_t lowBattery;           // Hysteresis memory, see battery_math.h.
    uint8_t consecutiveFailures;  // For the backoff in sleep_plan.h.
    uint8_t partialsSinceFull;    // See refresh_policy.h.
    uint8_t setupNoticeShown;     // 1 while the "setup needed" notice is on the panel.
    uint32_t wakeCount;           // Wakes since power-on. Handy in logs.
    WifiHint wifi;
    char etag[kEtagCapacity];  // ETag of the image on the panel, "" if none.
    uint32_t crc;              // Must stay the LAST member.
};

inline constexpr uint32_t kRtcMagic = 0x45504231;  // The ASCII bytes "EPB1".
inline constexpr uint16_t kRtcVersion = 1;

static_assert(std::is_trivially_copyable<RtcState>::value, "must be plain bytes");
static_assert(std::has_unique_object_representations<RtcState>::value, "no padding allowed");
static_assert(offsetof(RtcState, crc) == sizeof(RtcState) - sizeof(uint32_t), "crc must be last");

// ALGORITHM: CRC-32 (the same checksum zip files and Ethernet use).
// Treats the data as one very long binary number and keeps the remainder of
// dividing it by a fixed 33-bit constant. Flip any single bit of the data and
// the remainder changes. This is the small bit-by-bit version: slower than
// the table-driven one, but we only checksum about 70 bytes per wake.
uint32_t crc32(const uint8_t* data, size_t length);

// A clean state with valid magic, version and checksum.
RtcState makeDefaultRtcState();

// Recomputes the checksum. Call after changing any field, before storing.
void sealRtcState(RtcState* state);

// True if magic, version and checksum are all correct.
bool isRtcStateValid(const RtcState& state);

}  // namespace epb
