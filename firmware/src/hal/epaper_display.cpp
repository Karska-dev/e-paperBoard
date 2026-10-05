#include "hal/epaper_display.h"

// Seeed_GFX is a fork of the TFT_eSPI library and kept its header name.
// Which panel it drives is decided by include/driver.h.
#include <TFT_eSPI.h>

#include "pure/frame.h"

// PATTERN: fail loudly at compile time.
// Seeed_GFX takes its configuration from include/driver.h. If that file is
// missing or names another display, the library does not complain: it
// quietly falls back to a default setup, and the mistake only shows up later
// as a confusing error somewhere else. These checks state the requirement
// right here and stop the build with a message that names the cause.
// Setup 502 is the 7.5" monochrome panel; UC8179 is its controller.
//
// Note the limit of the check: it sees what THIS file sees. The library is
// compiled separately and finds driver.h only through the `-I include` flag
// in platformio.ini. Without that flag the build stops at the link step
// with "undefined reference to EPaper::..." (see OPERATIONS.md).
#if !defined(USER_SETUP_ID) || USER_SETUP_ID != 502 || !defined(UC8179_DRIVER)
#error "include/driver.h must select BOARD_SCREEN_COMBO 502 (7.5 inch monochrome, UC8179)."
#endif
#if TFT_WIDTH != 800 || TFT_HEIGHT != 480
#error "Seeed_GFX panel size does not match the 800 x 480 frame format in pure/frame.h."
#endif

namespace epb {

namespace {

// PATTERN: lazy initialisation.
// The EPaper object allocates a 48 kB drawing buffer when it is created.
// Most wakes end with "304 Not Modified" and never draw anything, so the
// object is not a global (which would be created on every wake) but a
// function-local static: it comes into existence the first time this
// function is called. Wakes that do not draw never pay for it.
EPaper& panel() {
    static EPaper instance;
    static bool started = false;
    if (!started) {
        instance.begin();  // Sets up the SPI bus and resets the panel.
        started = true;
    }
    return instance;
}

}  // namespace

void EpaperDisplay::showFrame(const uint8_t* frameBits) {
    EPaper& epaper = panel();
    // Copies our 1-bit image into the library's drawing buffer. For every
    // pixel: bit set -> first colour (white), bit clear -> second (black).
    // That matches the wire format described in pure/frame.h.
    epaper.drawBitmap(0, 0, frameBits, kFrameWidth, kFrameHeight, TFT_WHITE, TFT_BLACK);
    // update() wakes the panel, sends the buffer, runs a full refresh (the
    // visible flash, a few seconds) and puts the panel controller back into
    // its own deep sleep. The image stays visible with the power off.
    epaper.update();
}

void EpaperDisplay::showNotice(const char* title, const char* detail) {
    EPaper& epaper = panel();
    epaper.fillScreen(TFT_WHITE);
    epaper.setTextColor(TFT_BLACK);
    epaper.setTextSize(4);
    epaper.drawString(title, 30, 30);
    epaper.setTextSize(2);
    epaper.drawString(detail, 30, 110);
    epaper.update();
}

}  // namespace epb
