#include "app/wake_cycle.h"

#include <cstring>

#include "app/log.h"
#include "pure/battery_math.h"
#include "pure/frame.h"
#include "pure/http_date.h"
#include "pure/refresh_policy.h"
#include "pure/screen_request.h"
#include "pure/screens.h"

namespace epb {

namespace {

const char* wakeReasonFor(const WakeInfo& wake, NavAction action) {
    if (action != NavAction::None) {
        return navActionName(action);
    }
    return wake.cause == WakeCause::PowerOn ? "boot" : "timer";
}

// PATTERN: single exit helper. Every failure path ends here, so the counter
// and the backoff are handled in exactly one place.
CycleResult finishFailed(RtcState state, const CycleConfig& config, const char* reason) {
    if (state.consecutiveFailures < UINT8_MAX) {  // Saturate: never wrap to 0.
        ++state.consecutiveFailures;
    }
    const uint32_t sleepSeconds = planSleepSeconds(false, state.consecutiveFailures, 0, config.sleep);
    logf("cycle failed: %s (failure #%u in a row, retry in %u s)", reason,
         static_cast<unsigned>(state.consecutiveFailures), static_cast<unsigned>(sleepSeconds));
    return {state, sleepSeconds};
}

}  // namespace

CycleResult runWakeCycle(const WakeInfo& wake, const RtcState& previous, const CycleConfig& config, Ports& ports,
                         uint8_t* frameBuffer, size_t frameCapacity) {
    RtcState state = previous;  // Work on a copy; the caller stores the result.
    ++state.wakeCount;

    // --- 1. Which screen is wanted? ---
    // Navigation is relative to what is ON THE PANEL. shownScreen changes only
    // after a successful draw, so state and panel cannot drift apart.
    const NavAction action = wake.cause == WakeCause::Button
                                 ? decodeWakeMask(wake.buttonMask, config.buttons, config.buttonCount)
                                 : NavAction::None;
    const uint8_t targetScreen = navigate(state.shownScreen, action, kScreenCount);

    // --- 2. Battery ---
    // Before Wi-Fi: radio bursts pull the voltage down and would skew it.
    uint16_t samples[kBatterySampleCount];
    ports.battery.sample(samples, kBatterySampleCount);
    const uint16_t millivolts = adcRawToMillivolts(trimmedMean(samples, kBatterySampleCount, kBatteryTrimEachSide));
    const uint8_t percent = millivoltsToPercent(millivolts);
    state.lowBattery = updateLowBatteryFlag(state.lowBattery != 0, percent) ? 1 : 0;
    logf("wake #%u: reason=%s screen=%s battery=%u mV (%u%%)%s", static_cast<unsigned>(state.wakeCount),
         wakeReasonFor(wake, action), screenId(targetScreen), static_cast<unsigned>(millivolts),
         static_cast<unsigned>(percent), state.lowBattery != 0 ? " LOW" : "");

    // --- 3. Settings ---
    Settings settings = {};  // Zero-filled: all strings empty.
    ports.settings.load(&settings);
    if (!settings.isComplete()) {
        // Only once: e-paper keeps its image without power, so redrawing the
        // same text on every wake would cost energy and a flash for nothing.
        if (state.setupNoticeShown == 0) {
            ports.display.showNotice("Setup needed", "Wi-Fi and server address are not stored yet. See OPERATIONS.md.");
            state.setupNoticeShown = 1;
            state.hasFrame = 0;
            state.etag[0] = '\0';
        }
        logf("no settings stored: sleeping until a button is pressed");
        return {state, config.sleep.maxSeconds};
    }
    state.setupNoticeShown = 0;

    // --- 4. Build the request, then join Wi-Fi ---
    // Cheap check first: no radio time if the URL cannot even be built.
    const ScreenRequest request = {
        screenId(targetScreen),      millivolts, percent, state.lowBattery != 0, config.firmwareVersion,
        wakeReasonFor(wake, action),
    };
    char url[256];
    if (buildScreenUrl(url, sizeof(url), settings.serverUrl, request) == 0) {
        return finishFailed(state, config, "could not build the request URL");
    }

    if (!ports.network.connect(settings, &state.wifi)) {
        ports.network.off();
        return finishFailed(state, config, "Wi-Fi connect failed");
    }

    // --- 5. Ask the server ---
    // PATTERN: conditional GET. An ETag is a fingerprint the server attaches to
    // a response. Sending it back as If-None-Match means "send the body only
    // if yours differs". Unchanged -> 304 with no body: no download and, more
    // important, no panel refresh.
    // The ETag belongs to the image on the panel, so it is sent only when we
    // ask for that same screen. After a key press a 304 would be wrong.
    const bool sameScreenAsShown = state.hasFrame != 0 && targetScreen == state.shownScreen;
    const char* etagToSend = sameScreenAsShown ? state.etag : "";
    const FetchResult fetched = ports.client.fetch(url, etagToSend, frameBuffer, frameCapacity);

    // --- 6. Radio off ---
    // Before drawing: a refresh takes seconds and does not need the radio.
    ports.network.off();

    // The server's clock is in every response, even in a 304 or an error.
    int64_t epochSeconds = 0;
    if (parseHttpDate(fetched.date, &epochSeconds)) {
        ports.clock.set(epochSeconds);
    }

    // --- 7. Act on the answer ---
    switch (fetched.status) {
        case FetchStatus::Ok: {
            // Ok means exactly one frame arrived: the size check is the whole
            // "is this a valid image" test.
            ports.display.showFrame(frameBuffer);
            const RefreshDecision refresh = decideRefresh(true, state.partialsSinceFull, kMaxPartialsBeforeFull);
            state.partialsSinceFull = refresh.partialsSinceFull;
            state.shownScreen = targetScreen;
            state.hasFrame = 1;
            // Bounded copy, then force the terminating zero: never trust that a
            // string from outside is terminated.
            std::memcpy(state.etag, fetched.etag, sizeof(state.etag));
            state.etag[sizeof(state.etag) - 1] = '\0';
            logf("drew screen '%s'", screenId(targetScreen));
            break;
        }
        case FetchStatus::NotModified:
            if (etagToSend[0] == '\0') {
                // We sent no ETag, so 304 makes no sense. A confused server is a
                // failure, not a reason to believe the panel is up to date.
                return finishFailed(state, config, "server answered 304 to an unconditional request");
            }
            logf("screen unchanged (304), panel left alone");
            break;
        case FetchStatus::WrongSize:
            return finishFailed(state, config, "image is not exactly one frame");
        case FetchStatus::HttpError:
            return finishFailed(state, config, "server returned an error status");
        case FetchStatus::TransportError:
            return finishFailed(state, config, "no response from the server");
    }

    // --- 8. Plan the sleep ---
    state.consecutiveFailures = 0;
    const uint32_t sleepSeconds = planSleepSeconds(true, 0, fetched.nextWakeSeconds, config.sleep);
    return {state, sleepSeconds};
}

}  // namespace epb
