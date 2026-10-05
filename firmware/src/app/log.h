// Logging for the application layer.
//
// PATTERN: link seam.
// This header only DECLARES logf(). There are two different DEFINITIONS, and
// the build decides which one gets linked in:
//   - firmware build:  src/hal/log_serial.cpp   (prints to the USB serial port)
//   - host test build: tests/host/test_main.cpp (prints nothing)
// The code that calls logf() is identical in both builds and never knows the
// difference. A "seam" is a place where behaviour can be swapped without
// editing the calling code; here the swap happens at link time. It is the
// lightest way to replace a dependency that is used everywhere. For
// dependencies used in one place we use interfaces instead (see ports.h).
#pragma once

namespace epb {

// printf-style logging. The attribute makes the compiler check that the
// arguments match the format string, exactly as it does for printf itself.
void logf(const char* format, ...) __attribute__((format(printf, 1, 2)));

}  // namespace epb
