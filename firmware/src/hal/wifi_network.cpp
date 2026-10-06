#include "hal/wifi_network.h"

#include <Arduino.h>
#include <WiFi.h>

#include <cstring>

#include "app/log.h"
#include "config.h"

namespace epb {

namespace {

// PATTERN: polling with a deadline. WiFi.begin() only STARTS the connect, so
// we ask "connected yet?" in a loop and give up after a limit. Every wait on
// the outside world has one: waiting forever with the radio on means a flat
// battery by morning.
bool waitUntilConnected(uint32_t timeoutMs) {
    const uint32_t start = millis();
    // TECHNIQUE: wrap-safe elapsed time. `millis() - start` stays correct when
    // millis() overflows to 0 (after 49 days), because unsigned subtraction
    // wraps the same way. `millis() < start + timeout` would break.
    while (millis() - start < timeoutMs) {
        if (WiFi.status() == WL_CONNECTED) {
            return true;
        }
        delay(25);
    }
    return false;
}

}  // namespace

bool WifiNetwork::connect(const Settings& settings, WifiHint* hint) {
    // By default the Arduino core saves the credentials to flash on every
    // WiFi.begin(). We keep our own, so switch that off: no hidden flash writes.
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);  // Station = client of an access point.

    // An open network has no password; the API wants a null pointer then.
    const char* password = settings.password[0] != '\0' ? settings.password : nullptr;

    // PATTERN: fast path with fallback. A normal connect scans all channels,
    // which takes most of the time. With the remembered channel and access
    // point (BSSID) we go straight there. If that fails (the router changed
    // channel) we drop the hint and take the slow path that always works.
    if (hint->valid != 0) {
        WiFi.begin(settings.ssid, password, hint->channel, hint->bssid, true);
        if (waitUntilConnected(config::kWifiFastConnectMs)) {
            logf("wifi: fast connect on channel %u", static_cast<unsigned>(hint->channel));
            return true;
        }
        logf("wifi: fast connect failed, falling back to a full scan");
        hint->valid = 0;
        WiFi.disconnect();
    }

    WiFi.begin(settings.ssid, password);
    if (!waitUntilConnected(config::kWifiFullConnectMs)) {
        logf("wifi: could not join '%s'", settings.ssid);
        return false;
    }

    // Remember where we ended up, for the fast path next time.
    const uint8_t* bssid = WiFi.BSSID();
    if (bssid != nullptr) {
        hint->channel = static_cast<uint8_t>(WiFi.channel());
        std::memcpy(hint->bssid, bssid, sizeof(hint->bssid));
        hint->valid = 1;
    }
    logf("wifi: connected, ip=%s rssi=%d dBm", WiFi.localIP().toString().c_str(), static_cast<int>(WiFi.RSSI()));
    return true;
}

void WifiNetwork::off() {
    WiFi.disconnect(true);  // true = also power the radio down.
    WiFi.mode(WIFI_OFF);
}

}  // namespace epb
