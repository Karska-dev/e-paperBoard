// A unit-test framework in about 60 lines.
//
// Real projects use GoogleTest, Catch2 or doctest. Ours builds with nothing
// but a compiler, and shows that a framework is no magic. It needs:
//   1. a way to declare a test          -> the TEST macro
//   2. a list of all declared tests     -> a linked list built at start-up
//   3. a way to report a failed check   -> the CHECK macros
#pragma once

#include <cstdio>
#include <cstring>

namespace mini_test {

struct TestCase {
    const char* name;
    void (*run)();
    TestCase* next;
};

// The registry: head of the list, and the failure counter. `inline`
// variables (C++17) may live in a header; the linker merges the copies.
inline TestCase* g_firstTest = nullptr;
inline int g_failedChecks = 0;

// PATTERN: self-registration. Each TEST creates a global Registrar, and
// globals are constructed before main() runs. So writing a TEST anywhere is
// enough for it to be found: no central list to keep up to date.
struct Registrar {
    Registrar(TestCase* testCase) {
        testCase->next = g_firstTest;
        g_firstTest = testCase;
    }
};

inline int runAll() {
    int count = 0;
    for (TestCase* t = g_firstTest; t != nullptr; t = t->next) {
        const int failedBefore = g_failedChecks;
        t->run();
        std::printf("[%s] %s\n", g_failedChecks == failedBefore ? " ok " : "FAIL", t->name);
        ++count;
    }
    std::printf("\n%d tests, %d failed checks\n", count, g_failedChecks);
    return g_failedChecks == 0 ? 0 : 1;  // Non-zero tells ctest and CI "failed".
}

}  // namespace mini_test

// `TEST(name) { ... }` expands to a function, a TestCase describing it and a
// Registrar that files it. (## glues two tokens together; # makes a string.)
#define TEST(name)                                                           \
    static void test_##name();                                               \
    static mini_test::TestCase case_##name = {#name, &test_##name, nullptr}; \
    static mini_test::Registrar registrar_##name(&case_##name);              \
    static void test_##name()

// A failed check prints file and line and keeps going, so one run shows
// every problem, not only the first.
#define CHECK(condition)                                                                \
    do {                                                                                \
        if (!(condition)) {                                                             \
            ++mini_test::g_failedChecks;                                                \
            std::printf("  %s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #condition); \
        }                                                                               \
    } while (0)

#define CHECK_EQ(actual, expected)                                                                    \
    do {                                                                                              \
        const long long a_ = static_cast<long long>(actual);                                          \
        const long long e_ = static_cast<long long>(expected);                                        \
        if (a_ != e_) {                                                                               \
            ++mini_test::g_failedChecks;                                                              \
            std::printf("  %s:%d: %s is %lld, expected %lld\n", __FILE__, __LINE__, #actual, a_, e_); \
        }                                                                                             \
    } while (0)

#define CHECK_STR_EQ(actual, expected)                                                                     \
    do {                                                                                                   \
        if (std::strcmp((actual), (expected)) != 0) {                                                      \
            ++mini_test::g_failedChecks;                                                                   \
            std::printf("  %s:%d: %s is \"%s\", expected \"%s\"\n", __FILE__, __LINE__, #actual, (actual), \
                        (expected));                                                                       \
        }                                                                                                  \
    } while (0)
