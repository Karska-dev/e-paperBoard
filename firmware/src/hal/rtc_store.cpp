#include "hal/rtc_store.h"

#include <esp_attr.h>

#include "app/log.h"

namespace epb {

namespace {

// RTC_DATA_ATTR tells the linker to place this variable in RTC slow memory,
// the small RAM that stays powered during deep sleep. It behaves like a
// normal global, except that it still holds its value after a wake.
//
// What happens on the OTHER kinds of start (power-on, reset button, fresh
// flash, crash)? The startup code re-initialises RTC_DATA_ATTR variables
// then, here to all zeros. Zeros fail the magic-number check, so the first
// load after such a start returns clean defaults. That is the main job of
// the validation today. Its second job is protection against corruption
// during sleep (a brown-out on a nearly empty battery). And it becomes
// essential if this is ever changed to RTC_NOINIT_ATTR, which keeps data
// across resets and firmware updates too: then stale data from an older
// layout really can be sitting here, and the version field catches it.
RTC_DATA_ATTR RtcState g_stored;

}  // namespace

RtcState loadRtcState() {
    if (isRtcStateValid(g_stored)) {
        return g_stored;
    }
    logf("rtc: no valid state (first start after power-on or reset), using defaults");
    return makeDefaultRtcState();
}

void saveRtcState(const RtcState& state) {
    RtcState sealed = state;
    sealRtcState(&sealed);
    g_stored = sealed;
}

}  // namespace epb
