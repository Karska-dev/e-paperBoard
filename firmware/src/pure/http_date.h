// Reading the time from an HTTP `Date` header.
// LAYER: pure.
//
// Every HTTP response carries the server's clock:
//   Date: Mon, 05 Oct 2026 15:07:00 GMT
// We talk to the server on every wake anyway, so the time comes for free and
// NTP (a second request, more radio time) is not needed.
#pragma once

#include <cstdint>

namespace epb {

// Parses the standard HTTP date ("IMF-fixdate", RFC 9110) into Unix time:
// seconds since 1970-01-01 00:00:00 UTC. Returns false and leaves
// *epochSeconds alone for anything else, including HTTP's obsolete formats.
bool parseHttpDate(const char* text, int64_t* epochSeconds);

// ALGORITHM: days from civil date (Howard Hinnant). Year / month / day ->
// days since 1970-01-01 with integer arithmetic only: no loop over years, no
// table of month lengths. Exposed so the tests can check it directly.
int64_t daysFromCivil(int32_t year, uint32_t month, uint32_t day);

}  // namespace epb
