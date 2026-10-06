#include "pure/battery_math.h"

namespace epb {

namespace {

// One point of the Li-ion discharge curve.
struct CurvePoint {
    uint16_t millivolts;
    uint8_t percent;
};

// Sorted by voltage, lowest first, in steps of 5 %.
constexpr CurvePoint kLiIonCurve[] = {
    {3270, 0},  {3610, 5},  {3690, 10}, {3710, 15}, {3730, 20}, {3750, 25}, {3770, 30},
    {3790, 35}, {3800, 40}, {3820, 45}, {3840, 50}, {3850, 55}, {3870, 60}, {3910, 65},
    {3950, 70}, {3980, 75}, {4020, 80}, {4080, 85}, {4110, 90}, {4150, 95}, {4200, 100},
};
constexpr size_t kCurvePoints = sizeof(kLiIonCurve) / sizeof(kLiIonCurve[0]);

}  // namespace

uint16_t trimmedMean(uint16_t* samples, size_t count, size_t trimEachSide) {
    if (samples == nullptr || count <= 2 * trimEachSide) {
        return 0;
    }

    // ALGORITHM: insertion sort. Slide each value left until it sits behind a
    // smaller one. O(n^2), yet the right choice for 11 values: no recursion,
    // no extra memory. Pick the simplest tool that fits the data size.
    for (size_t i = 1; i < count; ++i) {
        const uint16_t value = samples[i];
        size_t j = i;
        while (j > 0 && samples[j - 1] > value) {
            samples[j] = samples[j - 1];
            --j;
        }
        samples[j] = value;
    }

    // Sum in 32 bits: eleven 16-bit values can overflow a 16-bit sum.
    uint32_t sum = 0;
    const size_t kept = count - 2 * trimEachSide;
    for (size_t i = trimEachSide; i < count - trimEachSide; ++i) {
        sum += samples[i];
    }

    // Adding half the divisor first rounds to nearest instead of down.
    return static_cast<uint16_t>((sum + kept / 2) / kept);
}

uint16_t adcRawToMillivolts(uint16_t raw) {
    // TECHNIQUE: integer maths instead of floating point. Multiply first,
    // divide last, write 0.968 as 968 / 1000. The product needs 64 bits
    // (4095 * 3600 * 2 * 968 is about 28 billion).
    constexpr uint64_t kAdcFullScaleMillivolts = 3600;
    constexpr uint64_t kDividerRatio = 2;
    constexpr uint64_t kCalibrationPerMille = 968;
    constexpr uint64_t kAdcMaxReading = 4095;

    if (raw > kAdcMaxReading) {
        raw = static_cast<uint16_t>(kAdcMaxReading);
    }
    const uint64_t numerator =
        static_cast<uint64_t>(raw) * kAdcFullScaleMillivolts * kDividerRatio * kCalibrationPerMille;
    const uint64_t denominator = kAdcMaxReading * 1000;
    return static_cast<uint16_t>((numerator + denominator / 2) / denominator);
}

uint8_t millivoltsToPercent(uint16_t millivolts) {
    // Outside the table: clamp to the ends instead of extrapolating.
    if (millivolts <= kLiIonCurve[0].millivolts) {
        return 0;
    }
    if (millivolts >= kLiIonCurve[kCurvePoints - 1].millivolts) {
        return 100;
    }

    // Find the first point at or above our voltage: it and the one before it
    // bracket the answer. With 21 entries a linear search is fine.
    for (size_t i = 1; i < kCurvePoints; ++i) {
        const CurvePoint& hi = kLiIonCurve[i];
        if (millivolts <= hi.millivolts) {
            const CurvePoint& lo = kLiIonCurve[i - 1];
            // How far we are between lo and hi, scaled to the percent step.
            const uint32_t span = hi.millivolts - lo.millivolts;
            const uint32_t offset = millivolts - lo.millivolts;
            const uint32_t step = hi.percent - lo.percent;
            return static_cast<uint8_t>(lo.percent + (offset * step) / span);
        }
    }
    return 100;  // Not reachable: the clamp above covers it.
}

bool updateLowBatteryFlag(bool wasLow, uint8_t percent) {
    if (percent <= kLowBatteryOnPercent) {
        return true;
    }
    if (percent >= kLowBatteryOffPercent) {
        return false;
    }
    return wasLow;  // Between the thresholds: no change.
}

}  // namespace epb
