// One wake cycle: everything the device does between waking up and going
// back to sleep.
//
// LAYER: app. Depends only on pure/ and on the interfaces in ports.h, never
// on hardware headers. It therefore runs unchanged on the board and inside
// the host unit tests.
//
// THE BIG PICTURE (PATTERN: duty cycling / run to completion).
// A normal program has a main loop that runs forever. This firmware has
// none. The device is asleep more than 99 % of the time, drawing microamps.
// A timer or a button wakes it; it runs ONE pass of the steps below as fast
// as it can, then powers down again. Battery life is decided almost entirely
// by how short and how rare these passes are.
//
//   1. Which screen is wanted?     (wake cause -> prev / home / next)
//   2. Measure the battery.
//   3. Load settings.              (not set up? show a notice, sleep)
//   4. Join Wi-Fi.
//   5. Ask the server for the screen, mentioning the ETag of the image that
//      is already on the panel.
//   6. Radio off.
//   7. 304 Not Modified -> nothing to draw.
//      200 OK           -> check the size, draw, remember the new ETag.
//   8. Decide how long to sleep.
#pragma once

#include <cstddef>
#include <cstdint>

#include "app/ports.h"
#include "pure/buttons.h"
#include "pure/rtc_state.h"
#include "pure/sleep_plan.h"

namespace epb {

enum class WakeCause : uint8_t {
    PowerOn,  // First start, reset button, or just flashed.
    Timer,    // The regular refresh.
    Button,   // One of the three keys.
};

struct WakeInfo {
    WakeCause cause;
    uint64_t buttonMask;  // Which GPIOs woke us (bit N = GPIO N). 0 unless Button.
};

// PATTERN: dependency injection through a parameter object.
// The cycle does not create its collaborators and does not look them up in
// globals. Whoever calls it hands them in. main.cpp passes the real hardware
// adapters; the tests pass fakes. Bundling the references into one struct
// keeps the function signature readable.
struct Ports {
    ISettings& settings;
    IBatteryAdc& battery;
    IDisplay& display;
    INetwork& network;
    IScreenClient& client;
    IWallClock& clock;
};

struct CycleConfig {
    const ButtonPin* buttons;  // Table from hal/board.h, in priority order.
    size_t buttonCount;
    const char* firmwareVersion;
    SleepConfig sleep;
};

struct CycleResult {
    RtcState state;         // What to remember across the coming sleep.
    uint32_t sleepSeconds;  // How long that sleep should be.
};

// Runs one wake cycle.
//
// PATTERN: state in, state out.
// The state that survives sleep comes in as an argument and the updated
// state goes out in the result. The function does not know where the state
// is stored. That keeps storage (RTC RAM) and logic separate, and it makes a
// test read like a story: "given this state and this wake cause, expect that
// state and that sleep time".
//
// `frameBuffer` must have room for `frameCapacity` == kFrameBytes bytes.
CycleResult runWakeCycle(const WakeInfo& wake, const RtcState& previous, const CycleConfig& config, Ports& ports,
                         uint8_t* frameBuffer, size_t frameCapacity);

}  // namespace epb
