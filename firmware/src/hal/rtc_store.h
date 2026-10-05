// Storage for the state that survives deep sleep (RTC memory).
#pragma once

#include "pure/rtc_state.h"

namespace epb {

// Returns the stored state, or clean defaults if nothing valid is stored.
RtcState loadRtcState();

// Seals (checksums) and stores the state for the next wake.
void saveRtcState(const RtcState& state);

}  // namespace epb
