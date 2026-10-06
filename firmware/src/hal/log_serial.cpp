// The firmware's logf(): prints to the USB serial port.
// (The host tests link another definition, see app/log.h.)
#include <Arduino.h>

#include <cstdarg>
#include <cstdio>

#include "app/log.h"

namespace epb {

void logf(const char* format, ...) {
    // Fixed buffer on the stack: no heap, and vsnprintf cannot write past the
    // end (an overlong line is cut short).
    char line[192];

    // va_list is how C reads a variable number of arguments ("...").
    va_list args;
    va_start(args, format);
    vsnprintf(line, sizeof(line), format, args);
    va_end(args);

    // millis() = time since this wake began, so the log doubles as a timing
    // profile of the cycle.
    Serial.printf("[%6lu ms] %s\n", static_cast<unsigned long>(millis()), line);
}

}  // namespace epb
