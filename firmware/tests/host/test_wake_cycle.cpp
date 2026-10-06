// Tests for the whole wake cycle, run against fake hardware.
// The most valuable tests here: the real wake_cycle.cpp, driven through
// situations that are awkward to produce on a desk (Wi-Fi down, confused
// server, tenth failure in a row).
#include <cstring>

#include "app/wake_cycle.h"
#include "fakes.h"
#include "hal/board.h"
#include "mini_test.h"
#include "pure/frame.h"
#include "pure/screens.h"

using namespace epb;
using namespace epb::test;

namespace {

// PATTERN: test fixture. The shared set-up (six fakes, a config, a buffer)
// lives in one struct, so each test shows only what makes it different:
// arrange, act, assert.
struct Rig {
    FakeSettings settings;
    FakeBattery battery;
    FakeDisplay display;
    FakeNetwork network;
    FakeClient client;
    FakeClock clock;
    CycleConfig config = {board::kButtons, board::kButtonCount, "v0.1.0", SleepConfig{}};
    uint8_t frame[kFrameBytes] = {};

    Rig() {
        std::strcpy(settings.stored.ssid, "HomeWifi");
        std::strcpy(settings.stored.password, "not-a-real-password");
        std::strcpy(settings.stored.serverUrl, "http://192.168.1.20:8080");
    }

    CycleResult run(const WakeInfo& wake, const RtcState& state) {
        Ports ports = {settings, battery, display, network, client, clock};
        return runWakeCycle(wake, state, config, ports, frame, sizeof(frame));
    }
};

const WakeInfo kPowerOn = {WakeCause::PowerOn, 0};
const WakeInfo kTimer = {WakeCause::Timer, 0};

// Wake info for "the key that means `action` was pressed". The pin comes
// from the real board table: tests that copy configuration break for the
// wrong reason. The layout itself is pinned down in test_navigation.cpp.
WakeInfo pressed(NavAction action) {
    for (size_t i = 0; i < board::kButtonCount; ++i) {
        if (board::kButtons[i].action == action) {
            return {WakeCause::Button, 1ULL << board::kButtons[i].gpio};
        }
    }
    return {WakeCause::Button, 0};
}

// A state as it looks after the home screen was drawn with ETag "v1".
RtcState stateShowing(uint8_t screen, const char* etag) {
    RtcState state = makeDefaultRtcState();
    state.shownScreen = screen;
    state.hasFrame = 1;
    std::strcpy(state.etag, etag);
    return state;
}

bool contains(const std::string& text, const char* part) {
    return text.find(part) != std::string::npos;
}

}  // namespace

TEST(first_boot_downloads_and_draws_home) {
    Rig rig;
    const CycleResult result = rig.run(kPowerOn, makeDefaultRtcState());

    CHECK_EQ(rig.display.framesShown, 1);
    CHECK(contains(rig.client.lastUrl, "/screen?id=home&"));
    CHECK(contains(rig.client.lastUrl, "&wake=boot"));
    CHECK(contains(rig.client.lastUrl, "&fw=v0.1.0"));
    CHECK_STR_EQ(rig.client.lastEtagSent.c_str(), "");  // Nothing on the panel yet.
    CHECK_EQ(result.state.hasFrame, 1);
    CHECK_STR_EQ(result.state.etag, "\"v1\"");
    CHECK_EQ(result.state.wakeCount, 1);
    CHECK_EQ(result.sleepSeconds, rig.config.sleep.defaultSeconds);
}

TEST(battery_reading_is_reported_to_the_server) {
    Rig rig;
    rig.battery.rawValue = 2350;  // 4000 mV, between 75 % and 80 % in the table.
    rig.run(kTimer, makeDefaultRtcState());
    CHECK(contains(rig.client.lastUrl, "&bat_mv=4000&bat_pct=77&low=0&"));
}

TEST(timer_wake_with_unchanged_image_does_not_touch_the_panel) {
    Rig rig;
    rig.client.answerStatus(FetchStatus::NotModified, 304);
    const CycleResult result = rig.run(kTimer, stateShowing(kHomeScreen, "\"v1\""));

    CHECK_STR_EQ(rig.client.lastEtagSent.c_str(), "\"v1\"");  // The conditional GET.
    CHECK_EQ(rig.display.framesShown, 0);
    CHECK_STR_EQ(result.state.etag, "\"v1\"");
    CHECK_EQ(result.state.consecutiveFailures, 0);
    CHECK_EQ(result.sleepSeconds, rig.config.sleep.defaultSeconds);
}

TEST(timer_wake_with_changed_image_draws_and_stores_the_new_etag) {
    Rig rig;
    rig.client.answerOk("\"v2\"");
    const CycleResult result = rig.run(kTimer, stateShowing(kHomeScreen, "\"v1\""));

    CHECK_EQ(rig.display.framesShown, 1);
    CHECK_STR_EQ(result.state.etag, "\"v2\"");
}

TEST(next_button_requests_the_next_screen_without_an_etag) {
    Rig rig;
    const CycleResult result = rig.run(pressed(NavAction::Next), stateShowing(kHomeScreen, "\"v1\""));

    CHECK(contains(rig.client.lastUrl, "/screen?id=time-left&"));
    CHECK(contains(rig.client.lastUrl, "&wake=next"));
    // A different screen is wanted, so the old screen's ETag must not be sent.
    CHECK_STR_EQ(rig.client.lastEtagSent.c_str(), "");
    CHECK_EQ(result.state.shownScreen, 1);
}

TEST(prev_button_wraps_from_home_to_the_last_screen) {
    Rig rig;
    const CycleResult result = rig.run(pressed(NavAction::Prev), stateShowing(kHomeScreen, "\"v1\""));
    CHECK_EQ(result.state.shownScreen, kScreenCount - 1);
    CHECK(contains(rig.client.lastUrl, "&wake=prev"));
}

TEST(home_button_on_home_is_a_conditional_refresh) {
    Rig rig;
    rig.client.answerStatus(FetchStatus::NotModified, 304);
    rig.run(pressed(NavAction::Home), stateShowing(kHomeScreen, "\"v1\""));
    CHECK_STR_EQ(rig.client.lastEtagSent.c_str(), "\"v1\"");  // Same screen: ETag is sent.
    CHECK_EQ(rig.display.framesShown, 0);
}

TEST(failed_download_after_a_button_press_keeps_the_old_screen) {
    Rig rig;
    rig.client.answerStatus(FetchStatus::TransportError, 0);
    const CycleResult result = rig.run(pressed(NavAction::Next), stateShowing(kHomeScreen, "\"v1\""));

    CHECK_EQ(rig.display.framesShown, 0);
    CHECK_EQ(result.state.shownScreen, kHomeScreen);  // State still matches the panel.
    CHECK_STR_EQ(result.state.etag, "\"v1\"");
    CHECK_EQ(result.state.consecutiveFailures, 1);
    CHECK_EQ(result.sleepSeconds, 60);
}

TEST(wifi_failure_skips_the_server_and_backs_off) {
    Rig rig;
    rig.network.connectSucceeds = false;
    RtcState state = makeDefaultRtcState();
    state.consecutiveFailures = 2;
    const CycleResult result = rig.run(kTimer, state);

    CHECK_EQ(rig.client.fetchCalls, 0);
    CHECK_EQ(rig.display.framesShown, 0);
    CHECK_EQ(result.state.consecutiveFailures, 3);
    CHECK_EQ(result.sleepSeconds, 240);  // 60 s doubled twice.
    CHECK(!rig.network.radioOn);
}

TEST(wrong_size_image_is_never_drawn) {
    Rig rig;
    rig.client.answerStatus(FetchStatus::WrongSize, 200);
    const CycleResult result = rig.run(kTimer, makeDefaultRtcState());
    CHECK_EQ(rig.display.framesShown, 0);
    CHECK_EQ(result.state.hasFrame, 0);
    CHECK_EQ(result.state.consecutiveFailures, 1);
}

TEST(server_error_counts_as_a_failure) {
    Rig rig;
    rig.client.answerStatus(FetchStatus::HttpError, 500);
    const CycleResult result = rig.run(kTimer, stateShowing(kHomeScreen, "\"v1\""));
    CHECK_EQ(rig.display.framesShown, 0);
    CHECK_EQ(result.state.consecutiveFailures, 1);
}

TEST(unexpected_304_is_a_failure_not_a_blank_panel) {
    Rig rig;
    rig.client.answerStatus(FetchStatus::NotModified, 304);
    const CycleResult result = rig.run(kPowerOn, makeDefaultRtcState());  // No ETag was sent.
    CHECK_EQ(result.state.consecutiveFailures, 1);
    CHECK_EQ(result.state.hasFrame, 0);
}

TEST(success_resets_the_failure_counter) {
    Rig rig;
    RtcState state = makeDefaultRtcState();
    state.consecutiveFailures = 5;
    const CycleResult result = rig.run(kTimer, state);
    CHECK_EQ(result.state.consecutiveFailures, 0);
}

TEST(failure_counter_saturates_instead_of_wrapping) {
    Rig rig;
    rig.network.connectSucceeds = false;
    RtcState state = makeDefaultRtcState();
    state.consecutiveFailures = 255;
    const CycleResult result = rig.run(kTimer, state);
    CHECK_EQ(result.state.consecutiveFailures, 255);
    CHECK_EQ(result.sleepSeconds, rig.config.sleep.backoffMaxSeconds);
}

TEST(radio_is_always_off_when_the_cycle_ends) {
    Rig rig;
    rig.run(kTimer, makeDefaultRtcState());
    CHECK_EQ(rig.network.connectCalls, 1);
    CHECK(rig.network.offCalls >= 1);
    CHECK(!rig.network.radioOn);
}

TEST(wifi_hint_is_stored_and_reused) {
    Rig rig;
    const CycleResult first = rig.run(kPowerOn, makeDefaultRtcState());
    CHECK(!rig.network.hintWasValidOnConnect);  // Nothing known on the first connect.
    CHECK_EQ(first.state.wifi.valid, 1);
    CHECK_EQ(first.state.wifi.channel, 6);

    rig.run(kTimer, first.state);
    CHECK(rig.network.hintWasValidOnConnect);  // The second connect gets the hint.
}

TEST(server_hint_sets_the_sleep_time) {
    Rig rig;
    rig.client.answer.nextWakeSeconds = 900;
    const CycleResult result = rig.run(kTimer, makeDefaultRtcState());
    CHECK_EQ(result.sleepSeconds, 900);
}

TEST(clock_is_set_from_the_date_header) {
    Rig rig;
    std::strcpy(rig.client.answer.date, "Sun, 06 Nov 1994 08:49:37 GMT");
    rig.run(kTimer, makeDefaultRtcState());
    CHECK_EQ(rig.clock.setCalls, 1);
    CHECK_EQ(rig.clock.lastEpoch, 784111777);
}

TEST(missing_or_broken_date_header_leaves_the_clock_alone) {
    Rig rig;
    rig.run(kTimer, makeDefaultRtcState());  // No Date header at all.
    std::strcpy(rig.client.answer.date, "yesterday-ish");
    rig.run(kTimer, makeDefaultRtcState());
    CHECK_EQ(rig.clock.setCalls, 0);
}

TEST(without_settings_a_notice_is_shown_once_and_the_radio_stays_off) {
    Rig rig;
    rig.settings.stored = {};  // Nothing stored.

    const CycleResult first = rig.run(kPowerOn, makeDefaultRtcState());
    CHECK_EQ(rig.display.noticesShown, 1);
    CHECK_EQ(rig.network.connectCalls, 0);
    CHECK_EQ(first.sleepSeconds, rig.config.sleep.maxSeconds);
    CHECK_EQ(first.state.consecutiveFailures, 0);  // Not set up is not a failure.

    rig.run(kTimer, first.state);
    CHECK_EQ(rig.display.noticesShown, 1);  // Not drawn a second time.
}

TEST(after_setup_the_notice_is_replaced_by_a_full_download) {
    Rig rig;
    rig.settings.stored = {};
    const CycleResult notSetUp = rig.run(kPowerOn, stateShowing(kHomeScreen, "\"v1\""));
    CHECK_STR_EQ(notSetUp.state.etag, "");  // The panel no longer shows that image.

    std::strcpy(rig.settings.stored.ssid, "HomeWifi");
    std::strcpy(rig.settings.stored.serverUrl, "http://192.168.1.20:8080");
    const CycleResult afterSetup = rig.run(kTimer, notSetUp.state);
    CHECK_STR_EQ(rig.client.lastEtagSent.c_str(), "");  // Unconditional: must get an image.
    CHECK_EQ(rig.display.framesShown, 1);
    CHECK_EQ(afterSetup.state.setupNoticeShown, 0);
}

TEST(unbuildable_request_fails_before_using_the_radio) {
    Rig rig;
    rig.config.firmwareVersion = "not url safe";  // The URL builder refuses spaces.
    const CycleResult result = rig.run(kTimer, makeDefaultRtcState());
    CHECK_EQ(rig.network.connectCalls, 0);
    CHECK_EQ(result.state.consecutiveFailures, 1);
}

TEST(result_state_is_not_sealed_by_the_cycle) {
    // Sealing (checksumming) is the storage adapter's job. This test pins
    // that division of labour down so nobody "fixes" it in the wrong layer.
    Rig rig;
    const CycleResult result = rig.run(kPowerOn, makeDefaultRtcState());
    CHECK(!isRtcStateValid(result.state));
}
