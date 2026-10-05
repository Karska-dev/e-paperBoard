// Refresh policy: when may the panel do a quick partial update, and when
// must it do a full refresh?
//
// LAYER: pure.
//
// BACKGROUND: an e-paper pixel is a capsule of black and white particles
// that are moved by an electric field. A FULL refresh drives every pixel
// through black and white before settling (the visible flash). It is slow
// but leaves a clean image. A PARTIAL refresh only nudges a small region.
// It is fast and does not flash, but every partial update leaves a little
// residue ("ghosting"), and on this panel's controller (UC8179) pixels near
// the region start to fade after about 3 partial updates.
//
// POLICY (from the project brief):
//   - a whole-screen change always gets a full refresh
//   - a small region may be updated partially
//   - after at most kMaxPartialsBeforeFull partials, force a full refresh
//
// v0.1 only ever draws whole screens, so today the answer is always Full.
// The counter is already carried in the sleep-surviving state so partial
// regions (for example an "updated HH:MM" stamp) can be added later without
// changing the state layout.
#pragma once

#include <cstdint>

namespace epb {

// Starting value. To be measured on our own panel (open question in the brief).
inline constexpr uint8_t kMaxPartialsBeforeFull = 3;

enum class RefreshKind : uint8_t {
    Full,
    Partial,
};

struct RefreshDecision {
    RefreshKind kind;
    uint8_t partialsSinceFull;  // The counter value to store for next time.
};

// PATTERN: pure decision function.
// The function gets everything it needs as arguments and returns its answer
// plus the new counter value. It reads no globals and touches no hardware,
// so the same inputs always give the same output. That makes every case
// testable in one line.
RefreshDecision decideRefresh(bool wholeScreenChanged, uint8_t partialsSinceFull, uint8_t maxPartials);

}  // namespace epb
