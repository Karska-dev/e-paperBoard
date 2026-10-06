// Entry point of the host tests, plus their version of logf() (the link
// seam described in app/log.h).
#include "app/log.h"
#include "mini_test.h"

namespace epb {

// The tests swallow the log lines the firmware would print.
void logf(const char* /*format*/, ...) {}

}  // namespace epb

int main() {
    return mini_test::runAll();
}
