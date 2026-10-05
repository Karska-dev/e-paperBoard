// Display configuration for the Seeed_GFX library.
//
// Seeed_GFX supports dozens of board and panel combinations. It finds out
// which one to build for by including a file called "driver.h" from the
// project. These two lines were confirmed on our unit on 2026-10-05 with
// Seeed's HelloWorld example.
#pragma once

// 502 = 7.5 inch monochrome ePaper, 800 x 480, UC8179 controller.
#define BOARD_SCREEN_COMBO 502

// The driver board in the TRMNL 7.5" (OG) DIY kit: XIAO ePaper Display
// Board EE04. Selects the right SPI and control pins inside the library.
#define USE_XIAO_EPAPER_DISPLAY_BOARD_EE04
