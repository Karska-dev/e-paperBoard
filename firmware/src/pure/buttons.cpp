#include "pure/buttons.h"

namespace epb {

uint64_t wakeMaskFor(const ButtonPin* table, size_t count) {
    uint64_t mask = 0;
    for (size_t i = 0; i < count; ++i) {
        // 1ULL, not 1: a plain `1` is a 32-bit int, and shifting it by 32 or more
        // is undefined behaviour.
        mask |= (1ULL << table[i].gpio);
    }
    return mask;
}

NavAction decodeWakeMask(uint64_t wakeMask, const ButtonPin* table, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        if ((wakeMask & (1ULL << table[i].gpio)) != 0) {
            return table[i].action;
        }
    }
    return NavAction::None;
}

const char* navActionName(NavAction action) {
    switch (action) {
        case NavAction::Prev:
            return "prev";
        case NavAction::Home:
            return "home";
        case NavAction::Next:
            return "next";
        case NavAction::None:
            break;
    }
    return "none";
}

}  // namespace epb
