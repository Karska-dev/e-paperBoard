# Host unit tests: the hardware-free parts of the firmware, built for and run
# on this computer. No board needed.
#
#   cmake -S firmware/tests/host -B build/host-tests
#   cmake --build build/host-tests
#   ctest --test-dir build/host-tests --output-on-failure
#
# CMake compiles nothing itself: it generates build files for whatever tool
# is installed (make, ninja, ...).
cmake_minimum_required(VERSION 3.16)
project(epaperboard_host_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Path to the firmware sources, relative to this file.
set(FW_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../../src)

# Only src/pure and src/app, the layers without hardware headers. If
# `#include <Arduino.h>` sneaks into one of them, this build breaks: the
# alarm we want.
add_executable(host_tests
    ${FW_SRC}/pure/battery_math.cpp
    ${FW_SRC}/pure/buttons.cpp
    ${FW_SRC}/pure/screens.cpp
    ${FW_SRC}/pure/refresh_policy.cpp
    ${FW_SRC}/pure/http_date.cpp
    ${FW_SRC}/pure/sleep_plan.cpp
    ${FW_SRC}/pure/rtc_state.cpp
    ${FW_SRC}/pure/screen_request.cpp
    ${FW_SRC}/app/wake_cycle.cpp
    test_main.cpp
    test_battery_math.cpp
    test_navigation.cpp
    test_pure_misc.cpp
    test_wake_cycle.cpp
)
target_include_directories(host_tests PRIVATE ${FW_SRC} ${CMAKE_CURRENT_SOURCE_DIR})

# Strict warnings, as errors: the laptop compiler doubles as a free static
# analyser. -DEPB_WERROR=OFF lets the build go on when a newer compiler
# knows a warning this code has not met yet.
option(EPB_WERROR "Treat compiler warnings as errors" ON)
target_compile_options(host_tests PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
if(EPB_WERROR)
    target_compile_options(host_tests PRIVATE -Werror)
endif()

# Sanitizers: run-time checks for memory errors (reading past a buffer) and
# undefined behaviour (an overflowing shift). On a microcontroller these bugs
# corrupt memory silently; here the test fails and names the line.
option(EPB_SANITIZE "Build the tests with address and undefined-behaviour sanitizers" ON)
if(EPB_SANITIZE)
    target_compile_options(host_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(host_tests PRIVATE -fsanitize=address,undefined)
endif()

# Registers the program with ctest, CMake's test runner.
enable_testing()
add_test(NAME host_tests COMMAND host_tests)
