// Refresh policy: quick partial update or full refresh?
// LAYER: pure.
//
// FULL refresh: every pixel flashes through black and white. Slow, clean.
// PARTIAL refresh: one region is nudged. Fast, but leaves "ghosting", and on
// this panel nearby pixels fade after about 3 partials.
//
// Policy: whole-screen change -> full. Small region -> partial, with a forced
// full refresh after kMaxPartialsBeforeFull partials. v0.1 only draws whole
// screens, so today the answer is always Full; the counter is ready for later.
#pragma once

#include <cstdint>

namespace epb {

// Starting value, to be measured on our own panel.
inline constexpr uint8_t kMaxPartialsBeforeFull = 3;

enum class RefreshKind : uint8_t {
    Full,
    Partial,
};

struct RefreshDecision {
    RefreshKind kind;
    uint8_t partialsSinceFull;  // The counter to store for next time.
};

// PATTERN: pure function. Everything it needs comes in as arguments; the
// answer and the new counter go out. No globals, no hardware: the same
// inputs always give the same output, so each case is a one-line test.
RefreshDecision decideRefresh(bool wholeScreenChanged, uint8_t partialsSinceFull, uint8_t maxPartials);

}  // namespace epb
