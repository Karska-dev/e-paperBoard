// Logging for the application layer.
//
// PATTERN: link seam. This header only DECLARES logf(). Two files DEFINE it,
// and the build picks one:
//   firmware     hal/log_serial.cpp         prints to the USB serial port
//   host tests   tests/host/test_main.cpp   prints nothing
// The callers are identical in both builds. Good for a dependency that is
// used everywhere; for one used in one place, see the interfaces in ports.h.
#pragma once

namespace epb {

// printf-style. The attribute makes the compiler check the arguments against
// the format string, as it does for printf itself.
void logf(const char* format, ...) __attribute__((format(printf, 1, 2)));

}  // namespace epb
