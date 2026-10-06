#include "pure/screen_request.h"

#include <cstdio>
#include <cstring>

namespace epb {

bool isUrlSafeToken(const char* text) {
    if (text == nullptr || text[0] == '\0') {
        return false;
    }
    for (const char* p = text; *p != '\0'; ++p) {
        const char c = *p;
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' ||
                        c == '_' || c == '-';
        if (!ok) {
            return false;
        }
    }
    return true;
}

size_t buildScreenUrl(char* out, size_t capacity, const char* serverUrl, const ScreenRequest& request) {
    if (out == nullptr || capacity == 0) {
        return 0;
    }
    out[0] = '\0';

    if (serverUrl == nullptr || serverUrl[0] == '\0' || !isUrlSafeToken(request.screenId) ||
        !isUrlSafeToken(request.firmwareVersion) || !isUrlSafeToken(request.wakeReason)) {
        return 0;
    }

    // Accept the base URL with or without a trailing slash.
    size_t baseLength = std::strlen(serverUrl);
    while (baseLength > 0 && serverUrl[baseLength - 1] == '/') {
        --baseLength;
    }

    // snprintf writes at most `capacity` bytes and returns the length the full
    // text WOULD have had; comparing the two detects truncation.
    // "%.*s" prints at most `baseLength` characters.
    const int written =
        std::snprintf(out, capacity, "%.*s/screen?id=%s&bat_mv=%u&bat_pct=%u&low=%u&fw=%s&wake=%s",
                      static_cast<int>(baseLength), serverUrl, request.screenId,
                      static_cast<unsigned>(request.batteryMillivolts), static_cast<unsigned>(request.batteryPercent),
                      request.lowBattery ? 1u : 0u, request.firmwareVersion, request.wakeReason);
    if (written < 0 || static_cast<size_t>(written) >= capacity) {
        out[0] = '\0';
        return 0;
    }
    return static_cast<size_t>(written);
}

}  // namespace epb
