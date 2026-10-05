// Tests for the smaller pure modules: refresh policy, HTTP date, sleep plan,
// RTC state and the request URL.
#include <cstring>

#include "mini_test.h"
#include "pure/frame.h"
#include "pure/http_date.h"
#include "pure/refresh_policy.h"
#include "pure/rtc_state.h"
#include "pure/screen_request.h"
#include "pure/sleep_plan.h"

using namespace epb;

// ------------------------------------------------------------ refresh policy

TEST(whole_screen_change_is_always_a_full_refresh) {
    const RefreshDecision d = decideRefresh(true, 2, 3);
    CHECK(d.kind == RefreshKind::Full);
    CHECK_EQ(d.partialsSinceFull, 0);
}

TEST(partials_are_allowed_up_to_the_limit_then_forced_full) {
    RefreshDecision d = decideRefresh(false, 0, 3);
    CHECK(d.kind == RefreshKind::Partial);
    CHECK_EQ(d.partialsSinceFull, 1);
    d = decideRefresh(false, 2, 3);
    CHECK(d.kind == RefreshKind::Partial);
    CHECK_EQ(d.partialsSinceFull, 3);
    d = decideRefresh(false, 3, 3);  // Three partials done: the next one must be full.
    CHECK(d.kind == RefreshKind::Full);
    CHECK_EQ(d.partialsSinceFull, 0);
}

// ----------------------------------------------------------------- HTTP date

TEST(days_from_civil_known_dates) {
    CHECK_EQ(daysFromCivil(1970, 1, 1), 0);
    CHECK_EQ(daysFromCivil(1970, 1, 2), 1);
    CHECK_EQ(daysFromCivil(1969, 12, 31), -1);
    CHECK_EQ(daysFromCivil(2000, 3, 1), 11017);  // The day after a century leap day.
}

TEST(http_date_parses_the_standard_format) {
    int64_t epoch = 0;
    // The example from the HTTP specification itself.
    CHECK(parseHttpDate("Sun, 06 Nov 1994 08:49:37 GMT", &epoch));
    CHECK_EQ(epoch, 784111777);
    CHECK(parseHttpDate("Thu, 01 Jan 1970 00:00:00 GMT", &epoch));
    CHECK_EQ(epoch, 0);
    CHECK(parseHttpDate("Mon, 05 Oct 2026 15:07:00 GMT", &epoch));
    CHECK_EQ(epoch, 1791212820);
}

TEST(http_date_handles_leap_days) {
    int64_t epoch = 0;
    CHECK(parseHttpDate("Thu, 29 Feb 2024 12:00:00 GMT", &epoch));
    CHECK_EQ(epoch, 1709208000);
    CHECK(!parseHttpDate("Sat, 29 Feb 2025 12:00:00 GMT", &epoch));  // 2025 is not a leap year.
}

TEST(http_date_rejects_malformed_input) {
    int64_t epoch = 123;
    CHECK(!parseHttpDate(nullptr, &epoch));
    CHECK(!parseHttpDate("", &epoch));
    CHECK(!parseHttpDate("Sun, 06 Nov 1994 08:49:37", &epoch));       // No zone.
    CHECK(!parseHttpDate("Sun, 06 Nov 1994 08:49:37 PST", &epoch));   // Wrong zone.
    CHECK(!parseHttpDate("Sun, 6 Nov 1994 08:49:37 GMT ", &epoch));   // One-digit day.
    CHECK(!parseHttpDate("Sun, 06 Foo 1994 08:49:37 GMT", &epoch));   // Not a month.
    CHECK(!parseHttpDate("Sun, 31 Apr 1994 08:49:37 GMT", &epoch));   // April has 30 days.
    CHECK(!parseHttpDate("Sun, 06 Nov 1994 24:00:00 GMT", &epoch));   // Hour out of range.
    CHECK(!parseHttpDate("Sun, 06 Nov 1994 08-49-37 GMT", &epoch));   // Wrong separators.
    CHECK(!parseHttpDate("Sunday, 06-Nov-94 08:49:37 GMT", &epoch));  // The obsolete format.
    CHECK_EQ(epoch, 123);                                             // A failed parse must not touch the output.
}

// ---------------------------------------------------------------- sleep plan

TEST(success_uses_the_default_without_a_hint) {
    const SleepConfig config;
    CHECK_EQ(planSleepSeconds(true, 0, 0, config), config.defaultSeconds);
}

TEST(success_uses_the_server_hint_within_limits) {
    const SleepConfig config;
    CHECK_EQ(planSleepSeconds(true, 0, 900, config), 900);
    CHECK_EQ(planSleepSeconds(true, 0, 5, config), config.minSeconds);            // Too short.
    CHECK_EQ(planSleepSeconds(true, 0, 4000000000u, config), config.maxSeconds);  // Too long.
}

TEST(failures_back_off_exponentially_up_to_the_cap) {
    const SleepConfig config;  // base 60 s, cap 3600 s
    CHECK_EQ(planSleepSeconds(false, 1, 0, config), 60);
    CHECK_EQ(planSleepSeconds(false, 2, 0, config), 120);
    CHECK_EQ(planSleepSeconds(false, 3, 0, config), 240);
    CHECK_EQ(planSleepSeconds(false, 6, 0, config), 1920);
    CHECK_EQ(planSleepSeconds(false, 7, 0, config), 3600);    // 3840 capped.
    CHECK_EQ(planSleepSeconds(false, 255, 0, config), 3600);  // No overflow after many failures.
}

TEST(failure_ignores_the_server_hint) {
    const SleepConfig config;
    CHECK_EQ(planSleepSeconds(false, 1, 900, config), 60);
}

// ----------------------------------------------------------------- RTC state

TEST(crc32_matches_the_standard_check_value) {
    // Every CRC-32 implementation must give 0xCBF43926 for the text
    // "123456789". Comparing against a published value like this is how you
    // know an algorithm is implemented correctly, not just consistently.
    CHECK(crc32(reinterpret_cast<const uint8_t*>("123456789"), 9) == 0xCBF43926u);
    CHECK(crc32(nullptr, 0) == 0u);
}

TEST(default_state_is_valid_and_empty) {
    const RtcState state = makeDefaultRtcState();
    CHECK(isRtcStateValid(state));
    CHECK_EQ(state.shownScreen, 0);
    CHECK_EQ(state.hasFrame, 0);
    CHECK_EQ(state.wifi.valid, 0);
    CHECK_STR_EQ(state.etag, "");
}

TEST(state_changes_need_a_new_seal) {
    RtcState state = makeDefaultRtcState();
    state.shownScreen = 3;
    CHECK(!isRtcStateValid(state));  // Changed but not sealed: looks corrupted.
    sealRtcState(&state);
    CHECK(isRtcStateValid(state));
}

TEST(garbage_and_old_versions_are_rejected) {
    RtcState zeros;
    std::memset(&zeros, 0, sizeof(zeros));
    CHECK(!isRtcStateValid(zeros));

    RtcState noise;
    std::memset(&noise, 0xA5, sizeof(noise));
    CHECK(!isRtcStateValid(noise));

    RtcState old = makeDefaultRtcState();
    old.version = kRtcVersion + 1;
    sealRtcState(&old);  // Correct checksum, wrong layout version.
    CHECK(!isRtcStateValid(old));
}

TEST(a_single_flipped_bit_is_detected) {
    RtcState state = makeDefaultRtcState();
    reinterpret_cast<uint8_t*>(&state)[20] ^= 0x04;
    CHECK(!isRtcStateValid(state));
}

// --------------------------------------------------------------- request URL

namespace {
ScreenRequest sampleRequest() {
    return {"home", 3940, 68, false, "v0.1.0", "timer"};
}
}  // namespace

TEST(url_contains_every_field) {
    char url[200];
    const size_t length = buildScreenUrl(url, sizeof(url), "http://192.168.1.20:8080", sampleRequest());
    CHECK_STR_EQ(url, "http://192.168.1.20:8080/screen?id=home&bat_mv=3940&bat_pct=68&low=0&fw=v0.1.0&wake=timer");
    CHECK_EQ(length, std::strlen(url));
}

TEST(url_tolerates_a_trailing_slash_on_the_base) {
    char url[200];
    buildScreenUrl(url, sizeof(url), "http://board.local/", sampleRequest());
    CHECK(std::strncmp(url, "http://board.local/screen?", 26) == 0);
}

TEST(url_reports_truncation_instead_of_cutting) {
    char url[40];  // Far too small.
    CHECK_EQ(buildScreenUrl(url, sizeof(url), "http://192.168.1.20:8080", sampleRequest()), 0);
    CHECK_STR_EQ(url, "");
}

TEST(url_rejects_unsafe_values) {
    char url[200];
    ScreenRequest request = sampleRequest();
    request.firmwareVersion = "v0.1.0+build 7";  // '+' and ' ' change meaning inside a URL.
    CHECK_EQ(buildScreenUrl(url, sizeof(url), "http://host", request), 0);
    request = sampleRequest();
    request.screenId = "home&admin=1";  // Would smuggle in an extra parameter.
    CHECK_EQ(buildScreenUrl(url, sizeof(url), "http://host", request), 0);
    CHECK_EQ(buildScreenUrl(url, sizeof(url), "", sampleRequest()), 0);
}

TEST(git_describe_style_versions_are_url_safe) {
    CHECK(isUrlSafeToken("v0.1.0-3-g1a2b3c4-dirty"));
    CHECK(isUrlSafeToken("0.0.0-dev"));
    CHECK(!isUrlSafeToken(""));
    CHECK(!isUrlSafeToken(nullptr));
}

// --------------------------------------------------------------------- frame

TEST(frame_is_48000_bytes) {
    CHECK_EQ(kFrameBytes, 48000);
}
