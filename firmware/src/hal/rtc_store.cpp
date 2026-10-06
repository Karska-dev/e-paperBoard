#include "hal/rtc_store.h"

#include <esp_attr.h>

#include "app/log.h"

namespace epb {

namespace {

// RTC_DATA_ATTR places this variable in RTC memory, the small RAM that stays
// powered in deep sleep: a normal global that keeps its value across a wake.
//
// On every OTHER start (power-on, reset, fresh flash, crash) the startup code
// resets it to zeros. Zeros fail the magic check, so the first load returns
// clean defaults: that is the validation's main job today. It also catches
// corruption during sleep (brown-out), and it becomes essential with
// RTC_NOINIT_ATTR, which keeps data across resets and firmware updates.
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
