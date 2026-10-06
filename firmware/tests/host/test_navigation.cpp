// Tests for buttons, screens and the board's button table.
#include <cstring>

#include "hal/board.h"  // Plain data, no hardware headers.
#include "mini_test.h"
#include "pure/buttons.h"
#include "pure/screens.h"

using namespace epb;

namespace {
const ButtonPin kTable[] = {
    {3, NavAction::Home},
    {2, NavAction::Prev},
    {5, NavAction::Next},
};
}  // namespace

TEST(wake_mask_has_one_bit_per_button) {
    CHECK_EQ(wakeMaskFor(kTable, 3), (1 << 2) | (1 << 3) | (1 << 5));
}

TEST(wake_mask_works_above_bit_31) {
    const ButtonPin high[] = {{40, NavAction::Home}};
    CHECK(wakeMaskFor(high, 1) == (1ULL << 40));
    CHECK(decodeWakeMask(1ULL << 40, high, 1) == NavAction::Home);
}

TEST(decode_maps_each_pin_to_its_action) {
    CHECK(decodeWakeMask(1 << 2, kTable, 3) == NavAction::Prev);
    CHECK(decodeWakeMask(1 << 3, kTable, 3) == NavAction::Home);
    CHECK(decodeWakeMask(1 << 5, kTable, 3) == NavAction::Next);
}

TEST(decode_ignores_unknown_pins) {
    CHECK(decodeWakeMask(0, kTable, 3) == NavAction::None);
    CHECK(decodeWakeMask(1 << 7, kTable, 3) == NavAction::None);
}

TEST(decode_prefers_the_first_table_row_when_two_are_pressed) {
    CHECK(decodeWakeMask((1 << 5) | (1 << 3), kTable, 3) == NavAction::Home);
    CHECK(decodeWakeMask((1 << 5) | (1 << 2), kTable, 3) == NavAction::Prev);
}

TEST(board_table_covers_all_three_actions_on_distinct_pins) {
    // Guards the real table in hal/board.h against copy-paste mistakes.
    CHECK_EQ(board::kButtonCount, 3);
    bool prev = false, home = false, next = false;
    for (size_t i = 0; i < board::kButtonCount; ++i) {
        prev = prev || board::kButtons[i].action == NavAction::Prev;
        home = home || board::kButtons[i].action == NavAction::Home;
        next = next || board::kButtons[i].action == NavAction::Next;
    }
    CHECK(prev && home && next);
    // Three distinct pins give a mask with exactly three bits set.
    CHECK_EQ(__builtin_popcountll(wakeMaskFor(board::kButtons, board::kButtonCount)), 3);
}

TEST(board_keys_are_home_next_prev_from_left_to_right) {
    // The one place where the tests state the physical layout.
    const uint64_t key1 = 1ULL << board::kPinKey1;
    const uint64_t key2 = 1ULL << board::kPinKey2;
    const uint64_t key3 = 1ULL << board::kPinKey3;
    CHECK(decodeWakeMask(key1, board::kButtons, board::kButtonCount) == NavAction::Home);
    CHECK(decodeWakeMask(key2, board::kButtons, board::kButtonCount) == NavAction::Next);
    CHECK(decodeWakeMask(key3, board::kButtons, board::kButtonCount) == NavAction::Prev);
    // Home has priority when several keys are down.
    CHECK(decodeWakeMask(key1 | key2 | key3, board::kButtons, board::kButtonCount) == NavAction::Home);
}

TEST(next_and_prev_wrap_around) {
    CHECK_EQ(navigate(0, NavAction::Next, 7), 1);
    CHECK_EQ(navigate(6, NavAction::Next, 7), 0);
    CHECK_EQ(navigate(0, NavAction::Prev, 7), 6);
    CHECK_EQ(navigate(3, NavAction::Prev, 7), 2);
}

TEST(home_always_goes_to_screen_zero) {
    CHECK_EQ(navigate(4, NavAction::Home, 7), 0);
    CHECK_EQ(navigate(0, NavAction::Home, 7), 0);
}

TEST(no_action_stays_put) {
    CHECK_EQ(navigate(4, NavAction::None, 7), 4);
}

TEST(garbage_index_is_treated_as_home) {
    CHECK_EQ(navigate(200, NavAction::Next, 7), 1);
    CHECK_STR_EQ(screenId(200), "home");
}

TEST(screen_ids_are_url_safe_and_start_with_home) {
    CHECK_STR_EQ(screenId(0), "home");
    for (uint8_t i = 0; i < kScreenCount; ++i) {
        for (const char* p = screenId(i); *p != '\0'; ++p) {
            CHECK((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') || *p == '-');
        }
    }
}
