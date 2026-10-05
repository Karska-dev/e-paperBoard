# Changelog

All notable changes to this project are recorded here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and versions follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Firmware skeleton for the Seeed TRMNL 7.5" (OG) DIY Kit (XIAO ESP32-S3 Plus, 800 × 480 UC8179 panel).
- Wake cycle: wake by timer or button, measure battery, join Wi-Fi, conditional download of a raw 48,000-byte frame, full refresh, deep sleep.
- Layered structure (`pure/`, `app/`, `hal/`, `storage/`) with the application logic behind interfaces.
- Battery reading with trimmed mean, Li-ion lookup table and low-battery hysteresis.
- Button table (home / next / prev, left to right) and wake-mask decoding.
- State across deep sleep in RTC memory, guarded by magic number, version and CRC-32.
- Faster Wi-Fi reconnect using the remembered channel and access point.
- Clock set from the HTTP `Date` header.
- Exponential backoff after failed cycles; awake-limit safety timer.
- Development settings through an untracked `secrets.h`, stored in NVS.
- Flash layout with two firmware slots and a FAT partition, ready for OTA and an offline cache.
- Firmware version taken from `git describe`.
- Host unit tests (CMake, own minimal test framework, sanitizers).
- Documentation: README, CONTRIBUTING, OPERATIONS, architecture, patterns, server contract, hardware notes.
- CI workflow (unit tests, formatting, static analysis, firmware build), issue and pull request templates.
- MIT license.
- Test server with per-screen test images (`tools/test_server.py`).

### Known limitations

- Sleep current and battery life are not measured yet.
- Plain HTTP only.
- No Wi-Fi provisioning, no OTA, no partial refresh, no long-press gestures.
