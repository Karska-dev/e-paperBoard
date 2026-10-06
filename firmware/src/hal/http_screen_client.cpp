#include "hal/http_screen_client.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include <cstdlib>
#include <cstring>

#include "app/log.h"
#include "config.h"

namespace epb {

namespace {

// Copies a response header into a fixed buffer. A value that does not fit is
// dropped, never cut short: half an ETag is worse than none.
void copyHeader(HTTPClient& http, const char* name, char* out, size_t capacity) {
    out[0] = '\0';
    const String value = http.header(name);
    if (value.length() > 0 && value.length() < capacity) {
        std::memcpy(out, value.c_str(), value.length() + 1);  // +1 copies the final zero.
    }
}

// Reads exactly `capacity` bytes of body into `out`.
// PATTERN: stall timeout. A slow download is fine (weak signal); silence is
// not. The timer restarts whenever bytes arrive, and we give up only after
// kHttpStallTimeoutMs without progress.
bool readBody(HTTPClient& http, uint8_t* out, size_t capacity) {
    Stream* stream = http.getStreamPtr();
    size_t received = 0;
    uint32_t lastProgress = millis();

    while (received < capacity) {
        const int available = stream->available();
        if (available > 0) {
            const size_t want = min(static_cast<size_t>(available), capacity - received);
            received += stream->readBytes(out + received, want);
            lastProgress = millis();
        } else if (!http.connected()) {
            break;  // The server closed the connection early.
        } else if (millis() - lastProgress > config::kHttpStallTimeoutMs) {
            break;
        } else {
            delay(1);  // Nothing yet: let the Wi-Fi task run.
        }
    }
    return received == capacity;
}

}  // namespace

FetchResult HttpScreenClient::fetch(const char* url, const char* etag, uint8_t* frameOut, size_t frameCapacity) {
    FetchResult result = {};
    result.status = FetchStatus::TransportError;  // Until proven otherwise.

    // v0.1 speaks plain http only. https needs a TLS client and certificates.
    WiFiClient transport;
    HTTPClient http;
    http.setConnectTimeout(static_cast<int32_t>(config::kHttpConnectTimeoutMs));
    http.setTimeout(static_cast<uint16_t>(config::kHttpStallTimeoutMs));
    if (!http.begin(transport, url)) {
        logf("http: cannot use url '%s'", url);
        return result;
    }

    // The Arduino HTTP client drops response headers unless it is told
    // beforehand which ones to keep.
    static const char* kWantedHeaders[] = {"ETag", "Date", "X-Next-Wake"};
    http.collectHeaders(kWantedHeaders, sizeof(kWantedHeaders) / sizeof(kWantedHeaders[0]));

    if (etag != nullptr && etag[0] != '\0') {
        http.addHeader("If-None-Match", etag);  // The conditional GET, see wake_cycle.cpp.
    }

    const int code = http.GET();  // Negative values are transport errors.
    if (code <= 0) {
        logf("http: request failed (%s)", HTTPClient::errorToString(code).c_str());
        http.end();
        return result;
    }

    result.httpCode = code;
    copyHeader(http, "ETag", result.etag, sizeof(result.etag));
    copyHeader(http, "Date", result.date, sizeof(result.date));
    // strtoul gives 0 for a missing or non-numeric header: "no hint".
    result.nextWakeSeconds = static_cast<uint32_t>(std::strtoul(http.header("X-Next-Wake").c_str(), nullptr, 10));

    if (code == HTTP_CODE_NOT_MODIFIED) {
        result.status = FetchStatus::NotModified;
    } else if (code == HTTP_CODE_OK) {
        // PATTERN: validate before you pay. The server must announce the body
        // size (Content-Length). If it is not exactly one frame we reject the
        // response without downloading a byte. getSize() is -1 when the header
        // is missing (chunked encoding, which this reader does not decode).
        const int declared = http.getSize();
        if (declared != static_cast<int>(frameCapacity)) {
            logf("http: body is %d bytes, expected %u", declared, static_cast<unsigned>(frameCapacity));
            result.status = FetchStatus::WrongSize;
        } else if (readBody(http, frameOut, frameCapacity)) {
            result.status = FetchStatus::Ok;
        } else {
            logf("http: download stalled or was cut off");
            result.status = FetchStatus::TransportError;
        }
    } else {
        logf("http: server answered %d", code);
        result.status = FetchStatus::HttpError;
    }

    http.end();
    return result;
}

}  // namespace epb
