#include "storage/nvs_settings.h"

#include <Preferences.h>

#include <cstring>

#include "app/log.h"

// __has_include asks the compiler "does this file exist?" without failing
// when it does not. That makes secrets.h truly optional: present on a
// developer's machine, absent in a fresh clone and in CI.
#if __has_include("secrets.h")
#include "secrets.h"
#define EPB_HAS_DEV_SECRETS 1
#else
#define EPB_HAS_DEV_SECRETS 0
#endif

namespace epb {

namespace {

// NVS groups keys into namespaces so different parts of a program cannot
// overwrite each other's values. Names are limited to 15 characters.
constexpr const char* kNamespace = "epb";
constexpr const char* kKeySsid = "ssid";
constexpr const char* kKeyPassword = "pass";
constexpr const char* kKeyServerUrl = "url";

void readString(Preferences& prefs, const char* key, char* out, size_t capacity) {
    out[0] = '\0';
    if (prefs.isKey(key)) {
        prefs.getString(key, out, capacity);
    }
}

}  // namespace

void NvsSettings::load(Settings* out) {
    *out = {};
    Preferences prefs;
    // `true` opens the namespace read-only. It fails if nothing was ever
    // stored, which simply leaves all settings empty.
    if (prefs.begin(kNamespace, true)) {
        readString(prefs, kKeySsid, out->ssid, sizeof(out->ssid));
        readString(prefs, kKeyPassword, out->password, sizeof(out->password));
        readString(prefs, kKeyServerUrl, out->serverUrl, sizeof(out->serverUrl));
        prefs.end();
    }
}

void NvsSettings::save(const Settings& settings) {
    Preferences prefs;
    if (prefs.begin(kNamespace, false)) {
        prefs.putString(kKeySsid, settings.ssid);
        prefs.putString(kKeyPassword, settings.password);
        prefs.putString(kKeyServerUrl, settings.serverUrl);
        prefs.end();
    }
}

void NvsSettings::applyDevSecrets() {
#if EPB_HAS_DEV_SECRETS
    Settings wanted = {};
    // strncpy with capacity - 1 leaves the last byte as the zero that the
    // `= {}` above put there, so the result is always terminated.
    std::strncpy(wanted.ssid, EPB_WIFI_SSID, sizeof(wanted.ssid) - 1);
    std::strncpy(wanted.password, EPB_WIFI_PASSWORD, sizeof(wanted.password) - 1);
    std::strncpy(wanted.serverUrl, EPB_SERVER_URL, sizeof(wanted.serverUrl) - 1);

    Settings stored = {};
    load(&stored);

    // Write only when something changed: this runs on every wake, and a
    // flash write on every wake is exactly what NVS is not meant for.
    if (std::memcmp(&wanted, &stored, sizeof(Settings)) != 0) {
        save(wanted);
        logf("settings: stored development values from secrets.h");
    }
#endif
}

}  // namespace epb
