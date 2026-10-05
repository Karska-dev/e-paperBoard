#include "hal/wifi_network.h"

#include <Arduino.h>
#include <WiFi.h>

#include <cstring>

#include "app/log.h"
#include "config.h"

namespace epb {

namespace {

// PATTERN: polling with a deadline.
// WiFi.begin() only STARTS the connection; it finishes in the background.
// So we ask "connected yet?" in a loop and give up after a time limit. Every
// wait on the outside world in this firmware has such a limit: a device that
// waits forever with the radio on has a flat battery by morning.
bool waitUntilConnected(uint32_t timeoutMs) {
    const uint32_t start = millis();
    // `millis() - start` stays correct even when millis() overflows and
    // wraps back to 0 (after 49 days): unsigned subtraction wraps the same
    // way. Comparing `millis() < start + timeout` would break at the wrap.
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
    // WiFi.begin(). We keep them in our own settings store, so switch that
    // off: no hidden flash writes on every wake.
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);  // Station = a client of an access point.

    // An open network has no password; the API wants a null pointer then.
    const char* password = settings.password[0] != '\0' ? settings.password : nullptr;

    // PATTERN: fast path with fallback.
    // A normal connect scans all 13 channels for the network name first,
    // which takes most of the connect time. If we remember which channel and
    // which access point (BSSID) we used last time, we can go straight
    // there. If that fails (router restarted on another channel, device
    // moved) we forget the hint and take the slow, always-correct path.
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
