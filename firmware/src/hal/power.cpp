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
    // BACKGROUND: in deep sleep the main part of the chip is unpowered,
    // including the normal GPIO circuitry and its pull-up resistors. A small
    // always-on island, the "RTC domain", keeps running: a timer, a few
    // kilobytes of memory, and its own access to some pins. Only this island
    // can wake the chip, so the buttons have to be handed over to it.
    for (size_t i = 0; i < board::kButtonCount; ++i) {
        const gpio_num_t pin = static_cast<gpio_num_t>(board::kButtons[i].gpio);
        // The keys connect the pin to ground when pressed. Without a pull-up
        // an open key leaves the pin floating; it would pick up noise and
        // wake the chip at random. The RTC domain has its own pull-ups.
        rtc_gpio_pullup_en(pin);
        rtc_gpio_pulldown_dis(pin);
    }
    // Keep the RTC peripherals powered so those pull-ups stay active.
    // This costs a little sleep current. The chip can also "hold" the pull-up
    // setting with the peripherals off; that is an optimisation to try once
    // sleep current is measured on the real board. Make it work first.
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

    // Wake source 1: any of the three keys going low ("ext1" can watch
    // several pins at once; "ext0" only one).
    // The call fails if a pin in the mask cannot be used from the RTC domain.
    // We still go to sleep then (the timer will wake us), but say so loudly:
    // silently losing the buttons would be a miserable bug to track down.
    const uint64_t wakeMask = wakeMaskFor(board::kButtons, board::kButtonCount);
    if (esp_sleep_enable_ext1_wakeup_io(wakeMask, ESP_EXT1_WAKEUP_ANY_LOW) != ESP_OK) {
        logf("ERROR: button wake-up could not be enabled; only the timer will wake the board");
    }

    // Wake source 2: the RTC timer. The API takes microseconds; the
    // multiplication must happen in 64 bits (ULL) or anything above 71
    // minutes would overflow.
    esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(seconds) * 1000000ULL);

    Serial.flush();  // Let the last log line out before the lights go off.
    esp_deep_sleep_start();

    // esp_deep_sleep_start() does not return. The compiler cannot know that
    // for certain, and this function promises [[noreturn]], so:
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
            // A 64-bit mask with one bit per GPIO that caused the wake.
            return {WakeCause::Button, esp_sleep_get_ext1_wakeup_status()};
        default:
            // Power-on, reset button, fresh flash, or a crash restart.
            return {WakeCause::PowerOn, 0};
    }
}

void releaseButtonPins() {
    for (size_t i = 0; i < board::kButtonCount; ++i) {
        const uint8_t pin = board::kButtons[i].gpio;
        // While asleep the pin belonged to the RTC domain. deinit returns it
        // to the normal GPIO circuitry, where digitalRead() works.
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
    // The keys wake the chip while they are LOW. If the user is still
    // holding one, the chip would wake again the instant it falls asleep.
    // So wait for all keys to be released, but not forever.
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
