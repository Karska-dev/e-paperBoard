// Fake adapters for testing the wake cycle on a laptop.
//
// PATTERN: test doubles (fakes). Each class implements a port from
// app/ports.h, as the real adapters do. A fake lets the test SCRIPT the
// outside world through fields set before the run ("Wi-Fi fails"), and it
// RECORDS what was done to it in fields checked afterwards ("drawn once").
// No mocking library: a fake is an ordinary small class.
#pragma once

#include <cstring>
#include <string>

#include "app/ports.h"
#include "pure/frame.h"

namespace epb::test {

class FakeSettings : public ISettings {
  public:
    Settings stored = {};
    void load(Settings* out) override { *out = stored; }
};

class FakeBattery : public IBatteryAdc {
  public:
    uint16_t rawValue = 2350;  // About 4.0 V, see test_battery_math.cpp.
    void sample(uint16_t* out, size_t count) override {
        for (size_t i = 0; i < count; ++i) {
            out[i] = rawValue;
        }
    }
};

class FakeDisplay : public IDisplay {
  public:
    int framesShown = 0;
    int noticesShown = 0;
    uint8_t firstByteOfLastFrame = 0;
    void showFrame(const uint8_t* frameBits) override {
        ++framesShown;
        firstByteOfLastFrame = frameBits[0];
    }
    void showNotice(const char* /*title*/, const char* /*detail*/) override { ++noticesShown; }
};

class FakeNetwork : public INetwork {
  public:
    bool connectSucceeds = true;
    int connectCalls = 0;
    int offCalls = 0;
    bool radioOn = false;
    bool hintWasValidOnConnect = false;
    bool connect(const Settings& /*settings*/, WifiHint* hint) override {
        ++connectCalls;
        hintWasValidOnConnect = hint->valid != 0;
        if (!connectSucceeds) {
            return false;
        }
        radioOn = true;
        hint->valid = 1;  // Like the real adapter: report what it learned.
        hint->channel = 6;
        return true;
    }
    void off() override {
        ++offCalls;
        radioOn = false;
    }
};

class FakeClient : public IScreenClient {
  public:
    // What the "server" will answer.
    FetchResult answer = {};
    uint8_t fillByte = 0xFF;
    // What the device asked for.
    int fetchCalls = 0;
    std::string lastUrl;
    std::string lastEtagSent;

    FakeClient() { answerOk("\"v1\""); }

    void answerOk(const char* etag) {
        answer = {};
        answer.status = FetchStatus::Ok;
        answer.httpCode = 200;
        std::strncpy(answer.etag, etag, sizeof(answer.etag) - 1);
    }
    void answerStatus(FetchStatus status, int httpCode) {
        answer = {};
        answer.status = status;
        answer.httpCode = httpCode;
    }

    FetchResult fetch(const char* url, const char* etag, uint8_t* frameOut, size_t frameCapacity) override {
        ++fetchCalls;
        lastUrl = url;
        lastEtagSent = etag;
        if (answer.status == FetchStatus::Ok) {
            std::memset(frameOut, fillByte, frameCapacity);
        }
        return answer;
    }
};

class FakeClock : public IWallClock {
  public:
    int setCalls = 0;
    int64_t lastEpoch = 0;
    void set(int64_t epochSeconds) override {
        ++setCalls;
        lastEpoch = epochSeconds;
    }
};

}  // namespace epb::test
