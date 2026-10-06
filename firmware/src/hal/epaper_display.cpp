#include "hal/epaper_display.h"

// Seeed_GFX is a fork of TFT_eSPI and kept its header name.
// include/driver.h decides which panel it drives.
#include <TFT_eSPI.h>

#include "pure/frame.h"

// PATTERN: fail loudly at compile time. If include/driver.h is missing or
// names another display, Seeed_GFX silently falls back to a default setup.
// These checks stop the build with a message that names the cause.
// (502 = the 7.5" monochrome panel, UC8179 = its controller.)
//
// Limit: the check sees what THIS file sees. The library itself finds
// driver.h only through `-I include` in platformio.ini; without that flag the
// link step fails with "undefined reference to EPaper::...".
#if !defined(USER_SETUP_ID) || USER_SETUP_ID != 502 || !defined(UC8179_DRIVER)
#error "include/driver.h must select BOARD_SCREEN_COMBO 502 (7.5 inch monochrome, UC8179)."
#endif
#if TFT_WIDTH != 800 || TFT_HEIGHT != 480
#error "Seeed_GFX panel size does not match the 800 x 480 frame format in pure/frame.h."
#endif

namespace epb {

namespace {

// PATTERN: lazy initialisation. The EPaper object allocates a 48 kB buffer.
// Most wakes end in "304" and never draw, so it is a function-local static:
// created on first use, never paid for by a wake that does not draw.
EPaper& panel() {
    static EPaper instance;
    static bool started = false;
    if (!started) {
        instance.begin();  // SPI bus up, panel reset.
        started = true;
    }
    return instance;
}

}  // namespace

void EpaperDisplay::showFrame(const uint8_t* frameBits) {
    EPaper& epaper = panel();
    // Copy our 1-bit image into the library's buffer: bit set -> first colour
    // (white), bit clear -> second (black), as defined in pure/frame.h.
    epaper.drawBitmap(0, 0, frameBits, kFrameWidth, kFrameHeight, TFT_WHITE, TFT_BLACK);
    // update(): wake the panel, send the buffer, full refresh (the flash, a
    // few seconds), panel back to its own sleep. The image stays without power.
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
