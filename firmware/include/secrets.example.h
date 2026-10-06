// Development-only settings.
//
//   1. Copy this file to secrets.h (same folder) and fill in your values.
//   2. Build and flash. On its next start the board stores them (NVS).
//
// secrets.h is in .gitignore, so it cannot be committed by accident. But a
// firmware .bin built with it CONTAINS the password in plain text: never
// share such a file. A stop-gap until Wi-Fi setup from the browser exists.
#pragma once

#define EPB_WIFI_SSID "your-wifi-name"
#define EPB_WIFI_PASSWORD "your-wifi-password"

// Address of the server that renders the screens. Plain http in v0.1.
#define EPB_SERVER_URL "http://192.168.1.20:8080"
