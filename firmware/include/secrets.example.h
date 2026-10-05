// Development-only settings. HOW TO USE:
//
//   1. Copy this file to secrets.h (same folder).
//   2. Fill in your values.
//   3. Build and flash. On its next start the board copies the values into
//      its settings storage (NVS).
//
// secrets.h is listed in .gitignore, so git will never commit it and your
// Wi-Fi password cannot end up on GitHub by accident.
//
// This is a stop-gap until proper provisioning exists (Wi-Fi setup from the
// browser over USB, planned after v0.1). Two things to know:
//   - A firmware binary built with secrets.h CONTAINS the password in plain
//     text. Never share such a .bin file.
//   - Builds made without secrets.h (for example by CI) contain no
//     credentials and leave the stored settings untouched.
#pragma once

#define EPB_WIFI_SSID "your-wifi-name"
#define EPB_WIFI_PASSWORD "your-wifi-password"

// Address of the server that renders the screens. Plain http only in v0.1.
// No trailing slash needed.
#define EPB_SERVER_URL "http://192.168.1.20:8080"
