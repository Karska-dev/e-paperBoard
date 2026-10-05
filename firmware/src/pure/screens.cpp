#include "pure/screens.h"

namespace epb {

uint8_t navigate(uint8_t current, NavAction action, uint8_t count) {
    if (count == 0) {
        return 0;
    }
    if (current >= count) {
        current = kHomeScreen;  // Defensive: treat garbage as "at Home".
    }
    switch (action) {
        case NavAction::Next:
            return static_cast<uint8_t>((current + 1) % count);
        case NavAction::Prev:
            return static_cast<uint8_t>((current + count - 1) % count);
        case NavAction::Home:
            return kHomeScreen;
        case NavAction::None:
            break;
    }
    return current;
}

const char* screenId(uint8_t index) {
    return kScreenIds[index < kScreenCount ? index : kHomeScreen];
}

}  // namespace epb
