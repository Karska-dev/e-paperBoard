// The firmware's definition of logf(): print to the USB serial port.
// (The host tests link a different definition, see src/app/log.h.)
#include <Arduino.h>

#include <cstdarg>
#include <cstdio>

#include "app/log.h"

namespace epb {

void logf(const char* format, ...) {
    // Format into a fixed buffer on the stack. No heap allocation, and
    // vsnprintf cannot write past the end: an overlong line is cut short.
    char line[192];

    // va_list is how C reads a variable number of arguments ("...").
    va_list args;
    va_start(args, format);
    vsnprintf(line, sizeof(line), format, args);
    va_end(args);

    // millis() = milliseconds since this wake started. Prefixing it turns
    // the log into a timing profile of the wake cycle for free.
    Serial.printf("[%6lu ms] %s\n", static_cast<unsigned long>(millis()), line);
}

}  // namespace epb
