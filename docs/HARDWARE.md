# Hardware notes

Facts about the board that the firmware relies on. In code, all of this lives in [`firmware/src/hal/board.h`](../firmware/src/hal/board.h) and [`firmware/include/driver.h`](../firmware/include/driver.h).

## The kit

Seeed Studio TRMNL 7.5" (OG) DIY Kit:

- XIAO ePaper Display Board (EE04) with a XIAO ESP32-S3 Plus on board (16 MB flash, OPI PSRAM)
- 7.5" monochrome e-paper panel, 800 × 480, UC8179 controller
- 2000 mAh Li-ion battery
- 3 user buttons and a reset button

## Pin map

| Function | GPIO | XIAO pin | Notes |
| --- | --- | --- | --- |
| Key 1 (left) | 2 | D1 | Active low. Home. |
| Key 2 (middle) | 3 | D2 | Active low. Next. |
| Key 3 (right) | 5 | D4 | Active low. Prev. |
| Battery voltage (ADC) | 1 | D0 | Reads half the battery voltage through a divider. |
| Battery divider enable | 6 | D5 | HIGH connects the divider. Keep LOW otherwise. |
| Display SPI and control | | | Handled inside Seeed_GFX, selected by `driver.h`. |

Button and battery pins were confirmed on our unit on 2026-10-05 with a test sketch.

### Button layout

Looking at the screen, the keys are, from left to right:

| Position | Key | GPIO | Action |
| --- | --- | --- | --- |
| left | 1 | 2 | home |
| middle | 2 | 3 | next |
| right | 3 | 5 | prev |

The assignment is one table, `kButtons` in `board.h`. To change what a key does, edit that table and nothing else.

## Battery measurement

```
voltage = raw / 4095 × 3.6 V × 2 × 0.968
```

- `raw`: 12-bit ADC reading on GPIO1
- `× 2`: the divider halves the battery voltage
- `0.968`: calibration factor taken from another project for the same kit. **Not yet checked against a multimeter on our unit.**

One reading takes 11 samples, drops the 2 highest and 2 lowest and averages the rest. Voltage is converted to percent with a Li-ion discharge table (`pure/battery_math.cpp`).

## Display

- `driver.h` must define `BOARD_SCREEN_COMBO 502` and `USE_XIAO_EPAPER_DISPLAY_BOARD_EE04`. Confirmed on our unit with Seeed's HelloWorld example.
- The panel is 1-bit: black or white. Gray is produced by dithering on the server.
- A full refresh flashes and takes a few seconds. Partial refresh is possible but leaves ghosting; other firmware for this panel found fading after about 3 partial updates. v0.1 only does full refreshes.
- A separately installed TFT_eSPI library conflicts with Seeed_GFX in the Arduino IDE (the screen stays blank). The PlatformIO build downloads its own copy of Seeed_GFX into the project folder, at the version pinned in `platformio.ini`.

## Handling

- **Never connect or disconnect the display ribbon cable while the board is powered.**
- The ribbon cable is fragile. The metal contacts face up.
- Connect the battery with USB unplugged. Red wire to the `+` marking next to the battery connector.
- **The on-off switch connects the battery.** With the switch off the board still runs while USB is plugged in, and stops the moment the cable is pulled. Switch it on before testing on battery.

## Not yet known

- Sleep current of the board in deep sleep.
- Whether the firmware can detect USB power (needed for a docked mode).
- How many partial refreshes our own panel tolerates.
