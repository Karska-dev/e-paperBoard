// Adapter: settings in NVS, the ESP32's key-value store in flash.
// LAYER: storage. NVS survives power loss and firmware updates, but flash
// wears out, so it holds only what changes rarely: Wi-Fi credentials and the
// server address. Per-wake state lives in RTC memory (pure/rtc_state.h).
#pragma once

#include "app/ports.h"

namespace epb {

class NvsSettings : public ISettings {
  public:
    void load(Settings* out) override;

    // Development helper: copies the values from include/secrets.h into NVS
    // when they differ. Does nothing in builds without secrets.h.
    void applyDevSecrets();

  private:
    void save(const Settings& settings);
};

}  // namespace epb
