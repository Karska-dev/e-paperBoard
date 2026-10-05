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

// Every failure path ends here, so the bookkeeping exists exactly once.
// (PATTERN: single exit helper. Three places can fail; without this helper
// each would have to remember to bump the counter and plan the backoff.)
CycleResult finishFailed(RtcState state, const CycleConfig& config, const char* reason) {
    if (state.consecutiveFailures < UINT8_MAX) {  // Saturate: never wrap back to 0.
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

    // --- 1. Which screen is wanted? --------------------------------------
    // Navigation is relative to what is ON THE PANEL (shownScreen). It only
    // becomes the new shownScreen once the image has really been drawn. If
    // the download fails, the panel still shows the old screen and the state
    // still says so: state and reality cannot drift apart.
    const NavAction action = wake.cause == WakeCause::Button
                                 ? decodeWakeMask(wake.buttonMask, config.buttons, config.buttonCount)
                                 : NavAction::None;
    const uint8_t targetScreen = navigate(state.shownScreen, action, kScreenCount);

    // --- 2. Battery --------------------------------------------------------
    // Measured BEFORE Wi-Fi starts: the radio draws short bursts of a few
    // hundred milliamps, which pull the battery voltage down and would make
    // the reading look worse than it is.
    uint16_t samples[kBatterySampleCount];
    ports.battery.sample(samples, kBatterySampleCount);
    const uint16_t millivolts = adcRawToMillivolts(trimmedMean(samples, kBatterySampleCount, kBatteryTrimEachSide));
    const uint8_t percent = millivoltsToPercent(millivolts);
    state.lowBattery = updateLowBatteryFlag(state.lowBattery != 0, percent) ? 1 : 0;
    logf("wake #%u: reason=%s screen=%s battery=%u mV (%u%%)%s", static_cast<unsigned>(state.wakeCount),
         wakeReasonFor(wake, action), screenId(targetScreen), static_cast<unsigned>(millivolts),
         static_cast<unsigned>(percent), state.lowBattery != 0 ? " LOW" : "");

    // --- 3. Settings -------------------------------------------------------
    Settings settings = {};  // Zero-filled: every string starts out empty.
    ports.settings.load(&settings);
    if (!settings.isComplete()) {
        // Draw the notice only once. E-paper keeps its image without power,
        // so redrawing the same text on every wake would cost energy and a
        // screen flash for nothing.
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

    // --- 4. Build the request, then join Wi-Fi -----------------------------
    // Cheap checks first: if the URL cannot be built there is no point in
    // spending a second or more of radio time on joining the network.
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

    // --- 5. Ask the server -------------------------------------------------
    // PATTERN: conditional GET with an ETag (standard HTTP caching).
    // An ETag is a short fingerprint the server attaches to a response. Next
    // time we send it back in an `If-None-Match` header, which means: "I
    // already have the version with this fingerprint; only send the body if
    // yours is different." If nothing changed, the server answers 304 Not
    // Modified with an empty body: 48 kB less to download and, far more
    // important, no panel refresh.
    //
    // The ETag describes the image on the panel, so it is only sent when we
    // are asking for that same screen again. After a button press we want a
    // DIFFERENT screen, and a 304 would wrongly leave the old one up.
    const bool sameScreenAsShown = state.hasFrame != 0 && targetScreen == state.shownScreen;
    const char* etagToSend = sameScreenAsShown ? state.etag : "";
    const FetchResult fetched = ports.client.fetch(url, etagToSend, frameBuffer, frameCapacity);

    // --- 6. Radio off ------------------------------------------------------
    // As early as possible, and BEFORE drawing: a panel refresh takes
    // seconds, and there is no reason to keep the radio powered through it.
    ports.network.off();

    // The server's clock is in every response, even in a 304 or an error.
    int64_t epochSeconds = 0;
    if (parseHttpDate(fetched.date, &epochSeconds)) {
        ports.clock.set(epochSeconds);
    }

    // --- 7. Act on the answer ----------------------------------------------
    switch (fetched.status) {
        case FetchStatus::Ok: {
            // The client only reports Ok for a body of exactly one frame;
            // that size check is our whole "is this a valid image" test.
            ports.display.showFrame(frameBuffer);
            const RefreshDecision refresh = decideRefresh(true, state.partialsSinceFull, kMaxPartialsBeforeFull);
            state.partialsSinceFull = refresh.partialsSinceFull;
            state.shownScreen = targetScreen;
            state.hasFrame = 1;
            // Bounded copy, then force a terminating zero. Never trust that
            // a string from outside is terminated.
            std::memcpy(state.etag, fetched.etag, sizeof(state.etag));
            state.etag[sizeof(state.etag) - 1] = '\0';
            logf("drew screen '%s'", screenId(targetScreen));
            break;
        }
        case FetchStatus::NotModified:
            if (etagToSend[0] == '\0') {
                // We sent no ETag, so "not modified" makes no sense. Treat a
                // confused server as a failure instead of pretending the
                // panel is up to date.
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

    // --- 8. Plan the sleep -------------------------------------------------
    state.consecutiveFailures = 0;
    const uint32_t sleepSeconds = planSleepSeconds(true, 0, fetched.nextWakeSeconds, config.sleep);
    return {state, sleepSeconds};
}

}  // namespace epb
