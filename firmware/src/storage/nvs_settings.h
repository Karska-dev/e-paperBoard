// Adapter: settings kept in NVS, the ESP32's key-value store in flash.
//
// LAYER: storage. NVS ("non-volatile storage") survives power loss and
// firmware updates. It spreads writes across its flash region (wear
// levelling), but flash still wears out, so it is for values that change
// rarely: Wi-Fi credentials and the server address. Everything that changes
// on each wake lives in RTC memory instead (see pure/rtc_state.h).
#pragma once

#include "app/ports.h"

namespace epb {

class NvsSettings : public ISettings {
  public:
    void load(Settings* out) override;

    // Development helper: if the firmware was built with include/secrets.h,
    // copy those values into NVS when they differ from what is stored.
    // Does nothing in builds without secrets.h. See secrets.example.h.
    void applyDevSecrets();

  private:
    void save(const Settings& settings);
};

}  // namespace epb
