// Builds the URL of the one request the device makes.
// LAYER: pure. The full contract is in docs/SERVER_CONTRACT.md.
//
//   http://192.168.1.20:8080/screen?id=home&bat_mv=3940&bat_pct=68&low=0&fw=v0.1.0&wake=timer
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

// Writes the URL into `out` and returns its length, or 0 (and an empty
// string) if it does not fit or a field is not URL-safe.
//
// PATTERN: caller-provided buffer with capacity. Never write past it, and
// report truncation; do not cut the URL short silently.
// PATTERN: validate at the boundary. Check values where they leave the
// program; do not hope they are well-formed.
size_t buildScreenUrl(char* out, size_t capacity, const char* serverUrl, const ScreenRequest& request);

// True if `text` is non-empty and uses only A-Z a-z 0-9 . _ -
// Those characters need no percent-encoding anywhere in a URL.
bool isUrlSafeToken(const char* text);

}  // namespace epb
