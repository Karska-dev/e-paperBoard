#include "pure/refresh_policy.h"

namespace epb {

RefreshDecision decideRefresh(bool wholeScreenChanged, uint8_t partialsSinceFull, uint8_t maxPartials) {
    if (wholeScreenChanged || partialsSinceFull >= maxPartials) {
        return {RefreshKind::Full, 0};  // A full refresh resets the counter.
    }
    return {RefreshKind::Partial, static_cast<uint8_t>(partialsSinceFull + 1)};
}

}  // namespace epb
