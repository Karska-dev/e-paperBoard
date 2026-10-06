// Ports: the interfaces through which the app reaches the outside world.
//
// PATTERN: ports and adapters, built on dependency inversion. The wake cycle
// never calls analogRead() or WiFi.begin(). It calls the small interfaces
// below ("ports"); hal/ and storage/ implement them ("adapters"); main.cpp
// plugs them in. Both sides depend on an interface that the app owns:
//
//   wake_cycle.cpp --uses--> INetwork <--implements-- hal/wifi_network.cpp
//                                     <--implements-- tests/host/fakes.h
//
// So the whole cycle runs on a laptop with fakes, and swapping a library
// touches one adapter.
//
// PATTERN: interface segregation. Each port has one or two methods, named
// after what the app wants, not after what the hardware offers.
#pragma once

#include <cstddef>
#include <cstdint>

#include "pure/rtc_state.h"

namespace epb {

// ---------------------------------------------------------------- settings

// Wi-Fi limits: SSID 32 bytes, WPA2 passphrase 63 characters, plus one byte
// each for the final zero.
struct Settings {
    char ssid[33];
    char password[65];
    char serverUrl[129];  // e.g. "http://192.168.1.20:8080"

    bool isComplete() const { return ssid[0] != '\0' && serverUrl[0] != '\0'; }
};

class ISettings {
  public:
    virtual ~ISettings() = default;
    // Fills `out`. Missing values come back as empty strings.
    virtual void load(Settings* out) = 0;
};

// ----------------------------------------------------------------- battery

class IBatteryAdc {
  public:
    virtual ~IBatteryAdc() = default;
    // Circuit on, `count` raw 12-bit readings into `out`, circuit off.
    // PATTERN: humble object. Hard-to-test code is kept so simple that it
    // hardly needs testing; the maths lives in pure/battery_math.
    virtual void sample(uint16_t* out, size_t count) = 0;
};

// ----------------------------------------------------------------- display

class IDisplay {
  public:
    virtual ~IDisplay() = default;
    // Shows a full-screen image (pure/frame.h) with a full refresh. Blocks for
    // the few seconds the panel needs.
    virtual void showFrame(const uint8_t* frameBits) = 0;
    // Shows two lines of text, for states with no server image ("not set up").
    virtual void showNotice(const char* title, const char* detail) = 0;
};

// ----------------------------------------------------------------- network

class INetwork {
  public:
    virtual ~INetwork() = default;
    // Joins Wi-Fi. `hint` is input and output: a valid hint speeds up the
    // connect, and a successful connect refreshes it. False on timeout.
    virtual bool connect(const Settings& settings, WifiHint* hint) = 0;
    // Radio off. Safe to call when not connected.
    virtual void off() = 0;
};

// ------------------------------------------------------------------ server

enum class FetchStatus : uint8_t {
    Ok,              // 200 and a frame of the right size is in the buffer.
    NotModified,     // 304: the image on the panel is still current.
    WrongSize,       // 200 but the body is not exactly one frame.
    HttpError,       // Any other HTTP status.
    TransportError,  // No answer at all (connect failed, timeout, ...).
};

// PATTERN: result struct, no exceptions. Embedded C++ is built with
// exceptions off (flash cost, unclear hardware state after a throw), so a
// function returns what happened and the caller must look at it.
struct FetchResult {
    FetchStatus status;
    int httpCode;              // 0 if no HTTP response arrived.
    char etag[kEtagCapacity];  // "" if none was sent, or it was too long.
    char date[32];             // Raw `Date` header, "" if absent.
    uint32_t nextWakeSeconds;  // From `X-Next-Wake`, 0 if absent.
};

class IScreenClient {
  public:
    virtual ~IScreenClient() = default;
    // GETs `url`, sending `etag` as If-None-Match unless it is empty.
    // On Ok, exactly `frameCapacity` bytes are in `frameOut`.
    virtual FetchResult fetch(const char* url, const char* etag, uint8_t* frameOut, size_t frameCapacity) = 0;
};

// ------------------------------------------------------------------- clock

class IWallClock {
  public:
    virtual ~IWallClock() = default;
    // Sets the calendar clock (Unix time, UTC).
    virtual void set(int64_t epochSeconds) = 0;
};

}  // namespace epb
