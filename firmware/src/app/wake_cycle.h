// One wake cycle: everything between waking up and going back to sleep.
// LAYER: app. Uses pure/ and the interfaces in ports.h, never hardware
// headers, so it runs unchanged on the board and in the host tests.
//
// PATTERN: duty cycling (run to completion). There is no main loop. The
// device sleeps; a timer or a key wakes it; it runs ONE pass and powers down.
// Battery life is set by how short and how rare these passes are.
//
//   wake -> 1 which screen? -> 2 battery -> 3 settings -> 4 Wi-Fi
//        -> 5 GET with ETag -> 6 radio off -> 7 draw (304: nothing to draw)
//        -> 8 plan the sleep -> sleep
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
    uint64_t buttonMask;  // Bit N = GPIO N. 0 unless Button.
};

// PATTERN: dependency injection (parameter object). The cycle neither creates
// its collaborators nor looks them up; the caller hands them in: real
// adapters from main.cpp, fakes from the tests.
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
// PATTERN: state in, state out. The sleep-surviving state comes in as an
// argument and goes out in the result. The logic never learns where it is
// stored, and a test reads "given this state and wake cause, expect that".
// `frameBuffer` must hold `frameCapacity` == kFrameBytes bytes.
CycleResult runWakeCycle(const WakeInfo& wake, const RtcState& previous, const CycleConfig& config, Ports& ports,
                         uint8_t* frameBuffer, size_t frameCapacity);

}  // namespace epb
