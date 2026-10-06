#include "hal/power.h"

#include <Arduino.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>
#include <esp_timer.h>

#include "app/log.h"
#include "config.h"
#include "hal/board.h"
#include "pure/buttons.h"

namespace epb::power {

namespace {

uint32_t g_awakeLimitSleepSeconds = 0;

// Programs the wake-up sources and powers down.
[[noreturn]] void enterDeepSleep(uint32_t seconds) {
    // In deep sleep the main part of the chip is off, including the normal GPIO
    // circuitry and its pull-ups. Only a small always-on island, the "RTC
    // domain" (a timer, a little memory, access to some pins), can wake the
    // chip, so the keys are handed over to it.
    for (size_t i = 0; i < board::kButtonCount; ++i) {
        const gpio_num_t pin = static_cast<gpio_num_t>(board::kButtons[i].gpio);
        // Without a pull-up an open key leaves the pin floating: it would pick
        // up noise and wake the chip at random.
        rtc_gpio_pullup_en(pin);
        rtc_gpio_pulldown_dis(pin);
    }
    // Keep the RTC peripherals powered so the pull-ups stay active. That costs
    // a little sleep current; the chip can also "hold" the setting with them
    // off. Try that once sleep current is measured. Make it work first.
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

    // Wake source 1: any key going low ("ext1" watches several pins, "ext0"
    // one). If the call fails we still sleep (the timer will wake us), but
    // say so loudly: silently losing the keys would be miserable to debug.
    const uint64_t wakeMask = wakeMaskFor(board::kButtons, board::kButtonCount);
    if (esp_sleep_enable_ext1_wakeup_io(wakeMask, ESP_EXT1_WAKEUP_ANY_LOW) != ESP_OK) {
        logf("ERROR: button wake-up could not be enabled; only the timer will wake the board");
    }

    // Wake source 2: the RTC timer, in microseconds. Multiply in 64 bits
    // (ULL), or anything above 71 minutes overflows.
    esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(seconds) * 1000000ULL);

    Serial.flush();  // Let the last log line out.
    esp_deep_sleep_start();

    // esp_deep_sleep_start() does not return, but the compiler cannot know
    // that, and this function promises [[noreturn]].
    while (true) {
    }
}

// Runs on the system timer task when the awake limit expires.
void onAwakeLimit(void* /*unused*/) {
    logf("AWAKE LIMIT reached: forcing deep sleep");
    enterDeepSleep(g_awakeLimitSleepSeconds);
}

}  // namespace

WakeInfo readWakeInfo() {
    switch (esp_sleep_get_wakeup_cause()) {
        case ESP_SLEEP_WAKEUP_TIMER:
            return {WakeCause::Timer, 0};
        case ESP_SLEEP_WAKEUP_EXT1:
            // One bit per GPIO that caused the wake.
            return {WakeCause::Button, esp_sleep_get_ext1_wakeup_status()};
        default:
            // Power-on, reset button, fresh flash, or a crash restart.
            return {WakeCause::PowerOn, 0};
    }
}

void releaseButtonPins() {
    for (size_t i = 0; i < board::kButtonCount; ++i) {
        const uint8_t pin = board::kButtons[i].gpio;
        // While asleep the pin belonged to the RTC domain; deinit returns it to
        // the normal GPIO circuitry, where digitalRead() works.
        rtc_gpio_deinit(static_cast<gpio_num_t>(pin));
        pinMode(pin, INPUT_PULLUP);
    }
}

void armAwakeLimit(uint32_t maxAwakeSeconds, uint32_t thenSleepSeconds) {
    g_awakeLimitSleepSeconds = thenSleepSeconds;

    esp_timer_create_args_t args = {};
    args.callback = &onAwakeLimit;
    args.name = "awake_limit";

    esp_timer_handle_t timer = nullptr;
    if (esp_timer_create(&args, &timer) == ESP_OK) {
        esp_timer_start_once(timer, static_cast<uint64_t>(maxAwakeSeconds) * 1000000ULL);
    }
}

void deepSleepFor(uint32_t seconds) {
    // The keys wake the chip while LOW. A key still held would wake it again
    // at once, so wait for release, but not forever.
    const uint32_t start = millis();
    while (millis() - start < config::kButtonReleaseWaitMs) {
        bool anyKeyDown = false;
        for (size_t i = 0; i < board::kButtonCount; ++i) {
            anyKeyDown = anyKeyDown || digitalRead(board::kButtons[i].gpio) == LOW;
        }
        if (!anyKeyDown) {
            break;
        }
        delay(10);
    }

    logf("sleeping for %lu s", static_cast<unsigned long>(seconds));
    enterDeepSleep(seconds);
}

}  // namespace epb::power
