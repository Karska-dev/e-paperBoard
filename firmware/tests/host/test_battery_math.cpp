#include "mini_test.h"
#include "pure/battery_math.h"

using namespace epb;

TEST(trimmed_mean_ignores_outliers) {
    // Seven honest readings around 2000, plus two wild ones on each side.
    uint16_t samples[11] = {2000, 2001, 0, 1999, 4095, 2000, 2002, 5, 1998, 4000, 2000};
    CHECK_EQ(trimmedMean(samples, 11, 2), 2000);
}

TEST(trimmed_mean_rounds_to_nearest) {
    uint16_t samples[5] = {0, 10, 11, 11, 99};  // Middle three: 10 + 11 + 11 = 32, / 3 = 10.67
    CHECK_EQ(trimmedMean(samples, 5, 1), 11);
}

TEST(trimmed_mean_with_nothing_left_is_zero) {
    uint16_t samples[4] = {1, 2, 3, 4};
    CHECK_EQ(trimmedMean(samples, 4, 2), 0);
    CHECK_EQ(trimmedMean(nullptr, 11, 2), 0);
}

TEST(adc_to_millivolts_matches_the_board_formula) {
    // raw / 4095 * 3.6 V * 2 * 0.968
    CHECK_EQ(adcRawToMillivolts(0), 0);
    CHECK_EQ(adcRawToMillivolts(4095), 6970);  // 6969.6 rounded
    CHECK_EQ(adcRawToMillivolts(2350), 4000);  // 3999.7 rounded: a nearly full cell
    CHECK_EQ(adcRawToMillivolts(9999), 6970);  // Out-of-range input is clamped.
}

TEST(percent_is_clamped_at_both_ends) {
    CHECK_EQ(millivoltsToPercent(0), 0);
    CHECK_EQ(millivoltsToPercent(3270), 0);
    CHECK_EQ(millivoltsToPercent(4200), 100);
    CHECK_EQ(millivoltsToPercent(5000), 100);
}

TEST(percent_hits_table_points_exactly) {
    CHECK_EQ(millivoltsToPercent(3610), 5);
    CHECK_EQ(millivoltsToPercent(3840), 50);
    CHECK_EQ(millivoltsToPercent(4150), 95);
}

TEST(percent_interpolates_between_points) {
    // 3950 mV is 70 %, 3980 mV is 75 %. Halfway (3965) is 72 (72.5 rounded down).
    CHECK_EQ(millivoltsToPercent(3965), 72);
    // 3270 mV is 0 %, 3610 mV is 5 %. 3440 is exactly halfway: 2.5 -> 2.
    CHECK_EQ(millivoltsToPercent(3440), 2);
}

TEST(percent_never_decreases_as_voltage_rises) {
    // PROPERTY TEST: check a rule that must hold for EVERY input, not single
    // values. Catches table typos that spot checks miss.
    uint8_t last = 0;
    for (uint16_t mv = 3000; mv <= 4300; ++mv) {
        const uint8_t now = millivoltsToPercent(mv);
        CHECK(now >= last);
        last = now;
    }
}

TEST(low_flag_has_hysteresis) {
    CHECK(updateLowBatteryFlag(false, 8));    // Turns on at 8 %.
    CHECK(!updateLowBatteryFlag(false, 9));   // Not yet on at 9 %.
    CHECK(updateLowBatteryFlag(true, 11));    // Once on, stays on at 11 %...
    CHECK(!updateLowBatteryFlag(true, 12));   // ...and turns off at 12 %.
    CHECK(!updateLowBatteryFlag(false, 10));  // In the dead band it keeps its value.
    CHECK(updateLowBatteryFlag(true, 10));
}
