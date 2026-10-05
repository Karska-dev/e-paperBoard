# e-paperBoard

Firmware for a battery-powered 7.5" e-paper wall display that shows weather, a to-do list and a family calendar.

The device does very little on purpose. A server renders each screen as an 800 × 480 black-and-white image. The board wakes up, downloads the image, shows it and goes back to sleep. Three buttons flip between screens.

This is also an **educational project**. Every pattern and algorithm used in the code is explained where it is used, and collected in [docs/PATTERNS.md](docs/PATTERNS.md). If you are learning embedded C++, the source is meant to be read.

> **Status: skeleton, before v0.1.** The firmware compiles, its logic is unit tested on a computer, and the whole wake cycle has run on the hardware, on USB power and on battery. See [What is verified](#what-is-verified) for exactly what has and has not been tested.

## Hardware

| Part | Detail |
| --- | --- |
| Kit | Seeed Studio TRMNL 7.5" (OG) DIY Kit |
| Controller | XIAO ESP32-S3 Plus (16 MB flash, OPI PSRAM) on the XIAO ePaper Display Board EE04 |
| Display | 7.5" monochrome e-paper, 800 × 480, UC8179 controller |
| Input | 3 buttons: home, next, prev (left to right) |
| Power | 2000 mAh Li-ion battery, charged over USB-C |

Pin assignments and handling notes: [docs/HARDWARE.md](docs/HARDWARE.md).

## How it works

The board is asleep almost all the time. A timer or a button wakes it, it runs one short cycle, and it sleeps again.

```mermaid
flowchart TD
    sleep([Deep sleep]) -->|timer or button| wake[Wake: which screen is wanted?]
    wake --> battery[Measure battery]
    battery --> settings{Settings stored?}
    settings -->|no| notice[Show 'Setup needed' once] --> sleep
    settings -->|yes| wifi{Join Wi-Fi}
    wifi -->|failed| backoff[Count failure, wait longer next time] --> sleep
    wifi -->|ok| fetch[GET /screen with the ETag of the shown image]
    fetch --> off[Radio off]
    off --> answer{Server answer}
    answer -->|304 not modified| plan[Plan next wake]
    answer -->|200 and exactly 48,000 bytes| draw[Draw image, remember ETag] --> plan
    answer -->|anything else| backoff
    plan --> sleep
```

The design in more depth: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md). What the server has to provide: [docs/SERVER_CONTRACT.md](docs/SERVER_CONTRACT.md).

## Repository layout

```
e-paperBoard/
├── README.md             you are here
├── CONTRIBUTING.md       how changes are made, reviewed and merged
├── OPERATIONS.md         how to build, flash, test, release and troubleshoot
├── CHANGELOG.md          what changed in each version
├── LICENSE               MIT license
├── docs/
│   ├── ARCHITECTURE.md   layers, the wake cycle, where state lives
│   ├── PATTERNS.md       every pattern and algorithm, with pointers into the code
│   ├── SERVER_CONTRACT.md  the HTTP interface between device and server
│   └── HARDWARE.md       pins, confirmed settings, handling warnings
├── firmware/
│   ├── platformio.ini    build configuration (all versions pinned)
│   ├── partitions.csv    flash layout
│   ├── boards/           board definition for the XIAO ESP32-S3 Plus
│   ├── include/          driver.h (display config), secrets.example.h
│   ├── scripts/          build helper: firmware version from git
│   ├── src/
│   │   ├── main.cpp      wires everything together
│   │   ├── config.h      timeouts and other tunable numbers
│   │   ├── pure/         maths and rules, no hardware (unit tested)
│   │   ├── app/          the wake cycle, no hardware (unit tested)
│   │   ├── hal/          hardware adapters: display, Wi-Fi, HTTP, battery, sleep
│   │   └── storage/      settings in flash (NVS)
│   └── tests/host/       unit tests that run on your computer
├── tools/
│   └── test_server.py    stand-in server with test images, for trying the firmware
└── .github/              CI workflow, issue and pull request templates
```

The firmware lives in its own `firmware/` folder so that the server and the case design can join this repository later without a reshuffle.

## Quick start

Full instructions, including installing the tools: [OPERATIONS.md](OPERATIONS.md).

Run the unit tests (no board needed):

```sh
cmake -S firmware/tests/host -B build/host-tests
cmake --build build/host-tests
ctest --test-dir build/host-tests --output-on-failure
```

Build and flash the firmware:

```sh
cd firmware
cp include/secrets.example.h include/secrets.h   # then edit: Wi-Fi name, password, server address
pio run -t upload
pio device monitor
```

## What is verified

| Check | State |
| --- | --- |
| Unit tests of `pure/` and `app/` (65 tests, with address and undefined-behaviour sanitizers) | pass |
| Firmware build for the board (both environments, no warnings in project code) | pass, 1.05 MB of the 3 MB slot |
| Static analysis (cppcheck) of `pure/` and `app/` | clean |
| Display config (`driver.h`), button pins, battery pins and formula | confirmed on the unit with separate test sketches |
| First run on the board (2026-10-05): boot, battery reading, settings from `secrets.h` | works |
| Wi-Fi connect, and the faster reconnect on later wakes | works |
| Unreachable server: failure counted, retry after 60 s, then 120 s | works |
| Deep sleep and wake by timer, state kept across sleep | works |
| Download and draw: each screen's test image appears on the panel | works |
| `304 Not Modified`: panel left untouched on timer wakes | works |
| Wake by button: home, next and prev, including wrap-around from the first screen to the last | works |
| Orientation and colours of the image: white background, frame on all sides, mark in the top-left corner | correct |
| Running on battery with USB unplugged (battery switch on) | works |
| "Setup needed" notice on a board without settings | not tested yet |
| Sleep current | not measured |
| CI workflow on GitHub | not run yet (the repository is not on GitHub yet) |

## Roadmap

**v0.1 (this skeleton, proven on hardware)**
- [x] Flash and run on the board
- [x] Deep sleep and wake by timer
- [x] A minimal test server that serves 48,000-byte images
- [x] Download and draw an image
- [x] Wake by button; each key does what the layout says (home, next, prev)
- [x] Run on battery with USB unplugged
- [ ] Tag `v0.1.0`

**After v0.1** (each as an issue, a branch and a pull request)
- Wi-Fi setup from the browser over USB (Improv Serial) and a setup portal
- Over-the-air firmware updates (the two firmware slots already exist)
- HTTPS
- Long-press gestures; debug screen
- Partial refresh for small regions
- Docked mode: stay awake while on the charger
- Offline cache of recent screens on the FAT partition

## Acknowledgements

No code was copied from these projects. They were studied for ideas and for hardware facts.

- [Pala One firmware](https://github.com/PaulLagier/pala-one-firmware) by Paul Lagier: deep-sleep and battery-reading approach, project structure.
- [E-Ink Desk Display](https://www.huyvector.org/smart-devices/e-ink-desk-display) by Huy Vector, based on FlintOS by Ameya Angadi: pin map, build settings and refresh limits for this exact kit.
- [eink-desk-display](https://github.com/davidz-yt/eink-desk-display) by davidz-yt: rendering and layout ideas for the server side.
- [Seeed_GFX](https://github.com/Seeed-Studio/Seeed_GFX) by Seeed Studio: the display driver library this firmware links against.

## License

[MIT](LICENSE). You may use, change and share this code, including commercially, as long as the copyright notice and the license text stay with it. It comes without warranty.

The libraries the firmware is built with keep their own licenses: Seeed_GFX (MIT and BSD), the Arduino core for ESP32 (LGPL-2.1) and ESP-IDF (Apache-2.0). They are downloaded at build time and are not part of this repository.
