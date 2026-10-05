// Ports: the interfaces through which the application reaches the outside
// world (battery, display, Wi-Fi, server, settings, clock).
//
// PATTERN: ports and adapters (also called hexagonal architecture), built on
// the dependency inversion principle.
//
// The application logic in wake_cycle.cpp never calls analogRead() or
// WiFi.begin() directly. It calls the small abstract interfaces declared
// here (the "ports"). The real implementations live in src/hal/ and
// src/storage/ (the "adapters") and are plugged in by main.cpp.
//
// "Dependency inversion" names the direction of the arrows: normally
// high-level code depends on low-level code (app -> Wi-Fi library). Here
// both depend on an interface that the HIGH-level code owns:
//
//     wake_cycle.cpp  --->  INetwork  <---  hal/wifi_network.cpp
//
// What that buys us:
//   - The whole wake cycle runs on a laptop with fake adapters, so its logic
//     is unit tested without a board (tests/host/test_wake_cycle.cpp).
//   - Swapping the display library or the transport touches one adapter and
//     nothing else.
//
// Each interface is as small as the application needs (the "interface
// segregation" idea): one or two methods, named after what the app wants,
// not after what the hardware offers.
#pragma once

#include <cstddef>
#include <cstdint>

#include "pure/rtc_state.h"

namespace epb {

// ---------------------------------------------------------------- settings

// Sizes follow the Wi-Fi standard: an SSID is at most 32 bytes, a WPA2
// passphrase at most 63 characters. One extra byte each for the final zero.
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
    // Switches the measuring circuit on, takes `count` raw 12-bit readings
    // into `out`, and switches the circuit off again. All the maths happens
    // in pure/battery_math, so this stays a few lines of hardware access.
    // (PATTERN: humble object. Code that is hard to test is made so simple
    // that it hardly needs testing; everything interesting moves elsewhere.)
    virtual void sample(uint16_t* out, size_t count) = 0;
};

// ----------------------------------------------------------------- display

class IDisplay {
  public:
    virtual ~IDisplay() = default;
    // Shows a full-screen image (format: pure/frame.h) with a full refresh.
    // Blocks for the few seconds the panel needs.
    virtual void showFrame(const uint8_t* frameBits) = 0;
    // Shows two lines of plain text. For states where no server image can
    // exist yet, such as "not set up".
    virtual void showNotice(const char* title, const char* detail) = 0;
};

// ----------------------------------------------------------------- network

class INetwork {
  public:
    virtual ~INetwork() = default;
    // Joins Wi-Fi. `hint` is both input and output: a valid hint speeds up
    // the connect, and after a successful connect it holds fresh values for
    // next time. Returns false if the network could not be joined in time.
    virtual bool connect(const Settings& settings, WifiHint* hint) = 0;
    // Switches the radio off. Safe to call when not connected.
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

// PATTERN: result struct instead of exceptions.
// Embedded C++ is normally built with exceptions switched off: they cost
// flash space, and "what happens to the hardware if this throws halfway" is
// hard to reason about. Functions return a plain struct that says what
// happened, and the caller has to look at it.
struct FetchResult {
    FetchStatus status;
    int httpCode;              // 0 if no HTTP response arrived.
    char etag[kEtagCapacity];  // "" if the server sent none (or it was too long).
    char date[32];             // Raw `Date` header, "" if absent.
    uint32_t nextWakeSeconds;  // From `X-Next-Wake`, 0 if absent.
};

class IScreenClient {
  public:
    virtual ~IScreenClient() = default;
    // GETs `url`. If `etag` is not empty it is sent as If-None-Match.
    // On FetchStatus::Ok, exactly `frameCapacity` bytes are in `frameOut`.
    virtual FetchResult fetch(const char* url, const char* etag, uint8_t* frameOut, size_t frameCapacity) = 0;
};

// ------------------------------------------------------------------- clock

class IWallClock {
  public:
    virtual ~IWallClock() = default;
    // Sets the device's calendar clock (Unix time, UTC).
    virtual void set(int64_t epochSeconds) = 0;
};

}  // namespace epb
