# Architecture

How the firmware is put together and why. For the catalogue of individual patterns and algorithms see [PATTERNS.md](PATTERNS.md).

## The one idea everything follows from

**The device should be asleep.** An ESP32 with Wi-Fi on draws roughly 100 mA; in deep sleep it draws microamps. A 2000 mAh battery lasts under a day in the first state and months in the second. So the firmware is not a program that runs and occasionally rests. It is a program that sleeps and occasionally runs: one short pass, called a **wake cycle**, and then power off again.

Two consequences shape the whole design:

1. **Every wake is a reboot.** Deep sleep powers down the CPU and RAM. Waking starts the program from the top with all variables reset. Anything worth remembering must be stored somewhere that survives.
2. **Work is moved off the device.** Rendering text, laying out a calendar and talking to weather services all cost awake time. A server does all of it and hands the device a finished image.

## Layers

```
┌──────────────────────────────────────────────────────────────┐
│ main.cpp            creates the adapters, runs one cycle     │
├───────────────────────────────┬──────────────────────────────┤
│ hal/                          │ storage/                     │
│ display, Wi-Fi, HTTP,         │ settings in flash (NVS)      │
│ battery ADC, sleep, RTC RAM   │                              │
├───────────────────────────────┴──────────────────────────────┤
│ app/                the wake cycle: all decisions            │
│                     ports.h: the interfaces it needs         │
├──────────────────────────────────────────────────────────────┤
│ pure/               maths and rules, no dependencies         │
└──────────────────────────────────────────────────────────────┘
```

**The rule:** a layer may include headers from layers below it, never from above. And `pure/` and `app/` may not include any Arduino or ESP-IDF header.

| Folder | Contains | May include | Tested |
| --- | --- | --- | --- |
| `pure/` | Battery maths, button decoding, screen navigation, refresh policy, HTTP date parsing, sleep planning, state checksum, URL building | C++ standard library only | Unit tests on the host |
| `app/` | `runWakeCycle()` and the interfaces (`ports.h`) it talks through | `pure/` | Unit tests on the host, with fake adapters |
| `hal/` | One adapter per piece of hardware; `board.h` with all pin numbers | Arduino, ESP-IDF, `app/`, `pure/` | On the board |
| `storage/` | Settings in NVS | Arduino, `app/` | On the board |
| `main.cpp` | Wiring only | everything | On the board |

Why split it this way: code that touches hardware can only be tested on hardware, which is slow and manual. So the hardware code is kept as thin as possible (read a pin, send a buffer), and every decision is pushed down into code that runs anywhere. The host test build compiles only `pure/` and `app/`; if a hardware header sneaks into them, that build fails.

### Ports and adapters

`app/ports.h` declares small interfaces: `IBatteryAdc`, `IDisplay`, `INetwork`, `IScreenClient`, `ISettings`, `IWallClock`. The wake cycle only knows these. `hal/` and `storage/` provide the real implementations, `tests/host/fakes.h` provides fake ones, and `main.cpp` decides which set gets plugged in.

```
                    ┌────────────────────┐
   wake_cycle.cpp ─▶│ IDisplay (port)    │◀─ hal/epaper_display.cpp   (on the board)
                    └────────────────────┘◀─ tests/host/fakes.h       (in unit tests)
```

## The wake cycle

Implemented in [`firmware/src/app/wake_cycle.cpp`](../firmware/src/app/wake_cycle.cpp). The comments there explain each step; this is the overview.

| Step | What happens | Why it is done this way |
| --- | --- | --- |
| 1 | Decode the wake cause into prev / home / next and compute the wanted screen | Navigation is relative to the screen on the panel, and only becomes the new "shown" screen after a successful draw. State cannot drift from reality. |
| 2 | Measure the battery | Before Wi-Fi, because radio bursts pull the voltage down and would skew the reading. |
| 3 | Load settings; if none, show a notice once and sleep | The notice is drawn once, not on every wake: e-paper keeps its image for free. |
| 4 | Build the request URL, then join Wi-Fi | Cheap checks first; never pay for the radio if the request cannot be built. |
| 5 | `GET /screen?...` with `If-None-Match` | The server answers `304` when the image is unchanged: no download and, more important, no panel refresh. |
| 6 | Radio off | Before drawing. A refresh takes seconds and does not need the radio. |
| 7 | `200` with exactly 48,000 bytes: draw and store the ETag. `304`: nothing. Anything else: failure. | Exact size is the whole validity check for a raw bitmap. |
| 8 | Plan the sleep: server hint or default on success, exponential backoff on failure | A long outage must not drain the battery with retries. |

`main.cpp` wraps this in: read wake cause → load state → **run cycle** → save state → deep sleep. An **awake limit** timer is armed first thing and forces sleep after 90 s whatever happens.

## Where state lives

| Kind of data | Where | Survives deep sleep | Survives power loss | Write cost |
| --- | --- | --- | --- | --- |
| Per-wake state: shown screen, ETag, failure count, Wi-Fi hint, low-battery flag | RTC RAM (`RtcState`) | yes | no | free |
| Settings: Wi-Fi credentials, server address | Flash, NVS | yes | yes | wears the flash |
| Everything else | normal RAM | no | no | free |

`RtcState` carries a magic number, a layout version and a CRC-32. If any of the three is wrong the firmware starts from clean defaults. Details in [`pure/rtc_state.h`](../firmware/src/pure/rtc_state.h) and [`hal/rtc_store.cpp`](../firmware/src/hal/rtc_store.cpp).

## Flash layout

16 MB, defined in [`firmware/partitions.csv`](../firmware/partitions.csv): settings (NVS), two 3 MB firmware slots for safe over-the-air updates, a 9.9 MB FAT partition reserved for an offline screen cache, and a crash-dump area. OTA and the cache are not implemented in v0.1, but the layout is in place because changing it later requires a USB re-flash.

## Build and versions

Everything is pinned in [`firmware/platformio.ini`](../firmware/platformio.ini): the platform release (Arduino-ESP32 core 3.3.9) and the display library (an exact commit). The firmware version string comes from `git describe` at build time and is sent to the server with every request.

## Decisions and their reasons

| Decision | Reason | Alternative considered |
| --- | --- | --- |
| Server renders, device displays | Shortest possible awake time; fonts, layout and API keys stay off the device | Rendering on the device (as the reference projects do): weeks of battery become days |
| Raw 1-bit bitmap, 48,000 bytes | No decoder, no decode memory, validity = size check | PNG: smaller on the wire, but needs a decoder and RAM |
| ETag / `304` for change detection | Standard HTTP, works with any server or cache, zero custom protocol | A custom hash parameter |
| Clock from the HTTP `Date` header | Already in every response | NTP: an extra request and more radio time |
| Device owns the screen order (a table of ids) | Works offline-first and keeps the request simple | Server-driven navigation (`from=home&nav=next`): add screens without re-flashing. Worth revisiting when the server exists. |
| No exceptions, result structs | Standard for embedded C++: smaller binary, explicit error paths | Exceptions |
| Hand-written mini test framework | Tests build with nothing but a compiler; shows how a framework works | GoogleTest / doctest: more features, one more dependency |
| RTC pull-ups with the RTC peripherals kept on during sleep | The documented, safe way to stop the button pins floating | Pad "hold" with peripherals off: lower sleep current, to be tried once current is measured |

## Not in v0.1

HTTPS, OTA updates, Wi-Fi provisioning (Improv, setup portal), long-press gestures, partial refresh, docked mode, the offline cache. Each has a place reserved in the structure: new adapters behind new ports, new rules in `pure/`.
