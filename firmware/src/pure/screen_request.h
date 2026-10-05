// Building the URL of the one request the device makes.
//
// LAYER: pure.
//
// Example result:
//   http://192.168.1.20:8080/screen?id=home&bat_mv=3940&bat_pct=68&low=0&fw=v0.1.0&wake=timer
//
// The device reports its battery and firmware version on every request, so
// the server can draw the battery icon and knows what is running out there.
// See docs/SERVER_CONTRACT.md for the full contract.
#pragma once

#include <cstddef>
#include <cstdint>

namespace epb {

struct ScreenRequest {
    const char* screenId;  // One of kScreenIds.
    uint16_t batteryMillivolts;
    uint8_t batteryPercent;
    bool lowBattery;
    const char* firmwareVersion;  // From `git describe`, e.g. "v0.1.0".
    const char* wakeReason;       // "boot", "timer", "prev", "home" or "next".
};

// Writes the full URL into `out` (a buffer of `capacity` bytes) and returns
// its length. Returns 0, and leaves an empty string in `out`, if:
//   - the result would not fit (PATTERN: caller-provided buffer with an
//     explicit capacity; the function never writes past it and reports
//     truncation instead of silently cutting the URL short), or
//   - any text field contains a character that is not safe in a URL
//     (PATTERN: validate at the boundary; we check values at the point where
//     they leave the program instead of hoping they are well-formed).
size_t buildScreenUrl(char* out, size_t capacity, const char* serverUrl, const ScreenRequest& request);

// True if `text` is non-empty and uses only A-Z a-z 0-9 . _ -
// Those characters mean the same thing everywhere in a URL, so values made
// of them need no percent-encoding.
bool isUrlSafeToken(const char* text);

}  // namespace epb
