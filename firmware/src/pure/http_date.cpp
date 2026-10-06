#include "pure/http_date.h"

#include <cstring>

namespace epb {

namespace {

// Reads exactly `digits` digits at `p`. Hand-written, not sscanf(), so the
// parser stays strict: "5 Oct" or "+5" is rejected, not half-accepted.
bool readNumber(const char* p, int digits, uint32_t* out) {
    uint32_t value = 0;
    for (int i = 0; i < digits; ++i) {
        const char c = p[i];
        if (c < '0' || c > '9') {
            return false;
        }
        value = value * 10 + static_cast<uint32_t>(c - '0');
    }
    *out = value;
    return true;
}

// Month name to 1..12, or 0 if it is not a month.
uint32_t monthFromName(const char* p) {
    static const char kNames[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    for (uint32_t m = 0; m < 12; ++m) {
        if (std::strncmp(p, kNames + m * 3, 3) == 0) {
            return m + 1;
        }
    }
    return 0;
}

uint32_t daysInMonth(int32_t year, uint32_t month) {
    static const uint8_t kDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    return (month == 2 && leap) ? 29 : kDays[month - 1];
}

}  // namespace

int64_t daysFromCivil(int32_t year, uint32_t month, uint32_t day) {
    // Trick: let the year start on 1 March. The leap day is then the LAST day
    // of the year and never shifts the days after it.
    year -= (month <= 2) ? 1 : 0;

    // The calendar repeats exactly every 400 years (an "era" of 146,097 days).
    const int32_t era = (year >= 0 ? year : year - 399) / 400;
    const uint32_t yearOfEra = static_cast<uint32_t>(year - era * 400);  // 0..399

    // (153 * m + 2) / 5 = days before month m, for the 31,30,31,30,31 pattern.
    const uint32_t marchMonth = month > 2 ? month - 3 : month + 9;  // Mar = 0 .. Feb = 11
    const uint32_t dayOfYear = (153 * marchMonth + 2) / 5 + day - 1;

    // Leap-year rule: every 4th year, except every 100th, except every 400th.
    const uint32_t dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;

    // 719,468 is the number of days from 0000-03-01 to 1970-01-01.
    return static_cast<int64_t>(era) * 146097 + static_cast<int64_t>(dayOfEra) - 719468;
}

bool parseHttpDate(const char* text, int64_t* epochSeconds) {
    // Layout, with character positions:
    //   Mon, 05 Oct 2026 15:07:00 GMT
    //   0    5  8   12   17 20 23 26
    constexpr size_t kLength = 29;
    if (text == nullptr || epochSeconds == nullptr || std::strlen(text) != kLength) {
        return false;
    }
    // Fixed punctuation first. The weekday name is redundant and ignored.
    if (text[3] != ',' || text[4] != ' ' || text[7] != ' ' || text[11] != ' ' || text[16] != ' ' || text[19] != ':' ||
        text[22] != ':' || std::strcmp(text + 25, " GMT") != 0) {
        return false;
    }

    uint32_t day = 0, year = 0, hour = 0, minute = 0, second = 0;
    const uint32_t month = monthFromName(text + 8);
    if (!readNumber(text + 5, 2, &day) || month == 0 || !readNumber(text + 12, 4, &year) ||
        !readNumber(text + 17, 2, &hour) || !readNumber(text + 20, 2, &minute) || !readNumber(text + 23, 2, &second)) {
        return false;
    }

    // Range checks. Seconds may be 60 during a leap second; treat it as 59.
    if (year < 1970 || day < 1 || day > daysInMonth(static_cast<int32_t>(year), month) || hour > 23 || minute > 59 ||
        second > 60) {
        return false;
    }
    if (second == 60) {
        second = 59;
    }

    const int64_t days = daysFromCivil(static_cast<int32_t>(year), month, day);
    *epochSeconds = days * 86400 + static_cast<int64_t>(hour) * 3600 + minute * 60 + second;
    return true;
}

}  // namespace epb
