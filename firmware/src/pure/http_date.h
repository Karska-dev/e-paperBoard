// Reading the time from an HTTP `Date` header.
//
// LAYER: pure.
//
// WHY: every HTTP response carries the server's clock, for example
//   Date: Mon, 05 Oct 2026 15:07:00 GMT
// We talk to our server on every wake anyway, so we can set the device clock
// from this header for free and skip NTP (a second protocol, a second
// request, more radio time, more battery).
#pragma once

#include <cstdint>

namespace epb {

// Parses the standard HTTP date format (RFC 9110 "IMF-fixdate"):
//   "Mon, 05 Oct 2026 15:07:00 GMT"
// On success writes the Unix time (seconds since 1970-01-01 00:00:00 UTC)
// to *epochSeconds and returns true. Returns false for anything else.
// The older date formats HTTP still allows on paper are deliberately not
// accepted: our own server only sends this one.
bool parseHttpDate(const char* text, int64_t* epochSeconds);

// ALGORITHM: days from civil date (by Howard Hinnant).
// Turns year / month / day into "days since 1970-01-01" with pure integer
// arithmetic: no loops over years, no table of month lengths, leap years
// handled by the formula itself. Exposed so the tests can check it directly.
int64_t daysFromCivil(int32_t year, uint32_t month, uint32_t day);

}  // namespace epb
