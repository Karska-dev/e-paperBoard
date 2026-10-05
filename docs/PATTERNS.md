# Patterns and algorithms

Everything reusable that this firmware applies, in one list. Each entry says what the idea is, why it is used here, and where to read it in the code, where a longer comment explains it in context.

A **pattern** is a named, proven way to organise code. An **algorithm** is a step-by-step method for computing something. Neither is specific to this project: you will meet all of them again.

## Architecture patterns

| Pattern | The idea in one sentence | Why here | Where |
| --- | --- | --- | --- |
| Duty cycling / run to completion | Sleep by default; wake, do one pass, sleep again. | Battery life is set by how little time is spent awake. | `app/wake_cycle.h`, `main.cpp` |
| Layered architecture | Code is stacked in layers and dependencies only point downwards. | Keeps hardware code from leaking into logic. | `main.cpp` (diagram), [ARCHITECTURE.md](ARCHITECTURE.md) |
| Functional core, imperative shell | Decisions live in side-effect-free functions; a thin outer shell does the I/O. | The core (`pure/`) is trivially testable; the shell (`hal/`) has almost nothing to test. | `pure/` versus `hal/` |
| Ports and adapters (hexagonal) | The application defines interfaces for what it needs; implementations plug in from outside. | The same wake cycle runs on the board and in tests. | `app/ports.h` |
| Dependency inversion | High-level code and low-level code both depend on an interface owned by the high-level side. | The app does not depend on the Wi-Fi library; the Wi-Fi adapter depends on the app's interface. | `app/ports.h` |
| Dependency injection | A function receives its collaborators as arguments and never creates or looks them up. | Tests hand in fakes, `main.cpp` hands in hardware. | `Ports` in `app/wake_cycle.h` |
| Composition root | One place creates all concrete objects and wires them together. | Everything hardware-specific is visible in one file. | `main.cpp` |
| Adapter (wrapper) | A small class translates the interface you want into the one a library offers. | Swapping the display library touches one file. | `hal/epaper_display.h` |
| Humble object | Make hard-to-test code so simple it barely needs testing. | The battery adapter only switches a pin and reads samples; the maths is elsewhere. | `IBatteryAdc` in `app/ports.h` |
| Interface segregation | Many small interfaces beat one big one. | Each port has one or two methods; fakes stay tiny. | `app/ports.h` |
| Link seam | Swap an implementation at link time: one declaration, two definitions, the build picks. | Logging is called everywhere; passing a logger around would be noise. | `app/log.h` |
| State in, state out | A function takes the old state as input and returns the new state. | The cycle does not know where state is stored; tests read like "given / expect". | `runWakeCycle()` |
| Single source of truth | Each fact is written down in exactly one place. | Pins in `hal/board.h`, version from git, frame size in `pure/frame.h`. | those files |

## Reliability patterns

| Pattern | The idea in one sentence | Why here | Where |
| --- | --- | --- | --- |
| Timeout on every wait | Never wait on the outside world without a limit. | A stuck wait with the radio on empties the battery. | `hal/wifi_network.cpp`, `hal/http_screen_client.cpp` |
| Stall timeout | Give up after a period with no progress, not after a total time. | A slow download is fine; a silent one is not. | `readBody()` in `hal/http_screen_client.cpp` |
| Dead man's switch (watchdog) | A timer that forces a safe state unless everything finishes in time. | Catches the hang nobody anticipated. | `armAwakeLimit()` in `hal/power.h` |
| Exponential backoff | Double the wait after each consecutive failure, up to a cap. | Quick recovery from a hiccup, low cost during an outage. | `pure/sleep_plan.h` |
| Saturating counter | A counter that stops at its maximum and never wraps to zero. | Failure 256 must not look like failure 0. | `finishFailed()` in `app/wake_cycle.cpp` |
| Fast path with fallback | Try the quick way that usually works; fall back to the slow way that always works. | Wi-Fi connect with a remembered channel, then a full scan. | `hal/wifi_network.cpp` |
| Validated persistent record | Guard stored data with a magic number, a version and a checksum. | Memory that survives a reboot can be stale or corrupt. | `pure/rtc_state.h` |
| Validate at the boundary | Check data where it enters or leaves the program. | Image size on the way in, URL characters on the way out. | `hal/http_screen_client.cpp`, `pure/screen_request.h` |
| Validate before you pay | Do the cheap checks before the expensive work. | Check `Content-Length` before downloading; build the URL before joining Wi-Fi. | `hal/http_screen_client.cpp`, `app/wake_cycle.cpp` |
| Result struct, no exceptions | Functions return a value that says what happened. | Exceptions are normally off in embedded C++. | `FetchResult` in `app/ports.h` |
| A/B firmware slots | Two firmware regions; update the idle one, then switch. | A failed update leaves the working firmware intact. | `firmware/partitions.csv` |
| Reproducible build | Pin every dependency to an exact version. | The same source gives the same binary next year. | `firmware/platformio.ini` |

## Embedded techniques

| Technique | The idea in one sentence | Where |
| --- | --- | --- |
| Static allocation | Reserve big fixed-size buffers at build time, not from the heap at run time. | `g_frame` in `main.cpp` |
| Lazy initialisation | Create an expensive object on first use, not at start-up. | `panel()` in `hal/epaper_display.cpp` |
| Caller-provided buffer with capacity | Functions write into memory the caller owns and are told its size. | `buildScreenUrl()` in `pure/screen_request.h` |
| Integer maths instead of floating point | Multiply first, divide last, express fractions as ratios. | `adcRawToMillivolts()` in `pure/battery_math.cpp` |
| Wrap-safe elapsed time | `now - start < timeout` stays correct when the millisecond counter overflows. | `waitUntilConnected()` in `hal/wifi_network.cpp` |
| Compile-time checks | `constexpr`, `static_assert` and `#error` let the compiler verify facts for free and stop a misconfigured build. | `pure/frame.h`, `pure/rtc_state.h`, `hal/epaper_display.cpp` |
| Named constants | No bare numbers in logic; each has a name and a reason. | `config.h` |
| RTC memory versus flash | Frequently changing state in RTC RAM, rare settings in flash. | `pure/rtc_state.h`, `storage/nvs_settings.h` |
| Switched voltage divider | Halve the battery voltage for the ADC, and disconnect the divider when not measuring. | `hal/battery_adc.cpp` |
| RTC-domain pull-ups | Buttons that wake the chip need pull-ups that work during deep sleep. | `enterDeepSleep()` in `hal/power.cpp` |
| Conditional write | Compare before writing to flash; skip the write if nothing changed. | `applyDevSecrets()` in `storage/nvs_settings.cpp`, `scripts/git_version.py` |

## Algorithms

| Algorithm | What it computes | Where |
| --- | --- | --- |
| Trimmed mean | An average that ignores the highest and lowest samples. | `trimmedMean()` in `pure/battery_math.cpp` |
| Insertion sort | Sorting a handful of values with the simplest method there is. | inside `trimmedMean()` |
| Lookup table with linear interpolation | A curved relationship (battery voltage to charge) from a few measured points. | `millivoltsToPercent()` in `pure/battery_math.cpp` |
| Hysteresis | A flag with separate on and off thresholds, so it does not flicker. | `updateLowBatteryFlag()` in `pure/battery_math.cpp` |
| Bit masks | A set of pins stored as one number, one bit per pin. | `pure/buttons.cpp` |
| Table-driven mapping | Behaviour described as rows of data and not as branches of code. | `kButtons` in `hal/board.h` |
| Modular arithmetic (ring) | Wrap-around navigation with the remainder operator. | `navigate()` in `pure/screens.cpp` |
| Days from civil date | Calendar date to day count with pure integer arithmetic. | `daysFromCivil()` in `pure/http_date.cpp` |
| Strict fixed-format parsing | Parse by position and reject anything unexpected. | `parseHttpDate()` in `pure/http_date.cpp` |
| CRC-32 | A checksum that changes if any bit of the data changes. | `crc32()` in `pure/rtc_state.cpp` |
| Clamping | Force a value into an allowed range. | `planSleepSeconds()` in `pure/sleep_plan.cpp` |

## Protocol ideas

| Idea | What it does | Where |
| --- | --- | --- |
| Conditional GET (ETag, `If-None-Match`, `304`) | "Send it only if it changed." | step 5 in `app/wake_cycle.cpp`, [SERVER_CONTRACT.md](SERVER_CONTRACT.md) |
| Clock from the `Date` header | Time sync without NTP. | `pure/http_date.h` |
| Raw fixed-size payload | A format with no parser: validity is a size check. | `pure/frame.h` |

## Testing techniques

| Technique | The idea in one sentence | Where |
| --- | --- | --- |
| Host unit tests | Run the device's logic on a computer, in milliseconds, without a board. | `firmware/tests/host/` |
| Test doubles (fakes) | Small stand-in classes that script the outside world and record what was done to them. | `tests/host/fakes.h` |
| Test fixture | Shared set-up in one struct so each test shows only what makes it different. | `Rig` in `tests/host/test_wake_cycle.cpp` |
| Self-registering tests | Each test adds itself to a list before `main()` runs. | `tests/host/mini_test.h` |
| Known-answer test | Check an algorithm against a published reference value. | CRC-32 and HTTP-date tests in `tests/host/test_pure_misc.cpp` |
| Property test | Check a rule that must hold for every input, not single examples. | `percent_never_decreases_as_voltage_rises` in `tests/host/test_battery_math.cpp` |
| Sanitizers | Compiler-inserted run-time checks for memory errors and undefined behaviour. | `tests/host/CMakeLists.txt` |
| Warnings as errors | The compiler's strictest warnings, and none allowed. | `tests/host/CMakeLists.txt` |
| Static analysis | A tool that reads the code looking for bugs without running it. | `.github/workflows/ci.yml` |

## Adding to this list

When a change introduces a pattern or algorithm that is not here yet, add a row, and explain it in a comment at the place it is used. That is part of the definition of done for this project (see [CONTRIBUTING.md](../CONTRIBUTING.md)).
