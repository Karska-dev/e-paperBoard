// Entry point of the host test program, plus the test build's version of
// logf() (see the "link seam" note in src/app/log.h).
#include "app/log.h"
#include "mini_test.h"

namespace epb {

// The firmware prints log lines to the serial port. In tests they would only
// clutter the output, so this version swallows them.
void logf(const char* /*format*/, ...) {}

}  // namespace epb

int main() {
    return mini_test::runAll();
}
