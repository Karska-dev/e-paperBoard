// Entry point of the firmware.
//
// PATTERN: composition root. The one place where the concrete parts are
// created and wired together. It makes no decisions; it only connects.
//
// THE LAYERS (an arrow means "may include"):
//
//   main.cpp ---> hal/, storage/     adapters: talk to hardware
//       |               |
//       +-------> app/               the wake cycle: decisions, no hardware
//                       |
//                    pure/           maths and rules: no dependencies
//
// Arrows only point down. pure/ and app/ never include Arduino headers,
// which is why they are unit tested on a laptop.
#include <Arduino.h>

#include "app/log.h"
#include "app/wake_cycle.h"
#include "config.h"
#include "fw_version.h"  // Generated at build time by scripts/git_version.py.
#include "hal/battery_adc.h"
#include "hal/board.h"
#include "hal/epaper_display.h"
#include "hal/http_screen_client.h"
#include "hal/power.h"
#include "hal/rtc_store.h"
#include "hal/wall_clock.h"
#include "hal/wifi_network.h"
#include "pure/frame.h"
#include "storage/nvs_settings.h"

namespace {

// Download buffer for one frame (48,000 bytes).
// PATTERN: static allocation. Reserved by the linker for the whole run: it
// cannot fail or fragment the heap, and the memory budget is known at build
// time. The embedded default for anything large with a fixed size.
uint8_t g_frame[epb::kFrameBytes];

}  // namespace

// Arduino calls setup() once per start. Waking from deep sleep IS a start,
// so setup() runs once per wake and contains the whole program.
void setup() {
    using namespace epb;

    // First thing: arm the safety net, before anything can hang.
    power::armAwakeLimit(config::kMaxAwakeSeconds, config::kAwakeLimitSleepSeconds);

    Serial.begin(config::kSerialBaud);
#ifdef EPB_BOOT_DELAY_MS
    delay(EPB_BOOT_DELAY_MS);  // Debug build: let USB serial reconnect.
#endif
    logf("e-paperBoard firmware %s", FW_VERSION);

    // Read why we woke BEFORE touching the button pins.
    const WakeInfo wake = power::readWakeInfo();
    power::releaseButtonPins();

    // The adapters: plain locals that live as long as the device is awake.
    BatteryAdc battery;
    EpaperDisplay display;
    WifiNetwork network;
    HttpScreenClient client;
    NvsSettings settings;
    WallClock clock;
    settings.applyDevSecrets();

    Ports ports = {settings, battery, display, network, client, clock};
    const CycleConfig cycleConfig = {board::kButtons, board::kButtonCount, FW_VERSION, SleepConfig{}};

    // Load state -> run one cycle -> save state -> sleep.
    const CycleResult result = runWakeCycle(wake, loadRtcState(), cycleConfig, ports, g_frame, sizeof(g_frame));
    saveRtcState(result.state);
    power::deepSleepFor(result.sleepSeconds);
}

// Never reached: setup() ends in deep sleep. Must exist for the program to
// link.
void loop() {}
