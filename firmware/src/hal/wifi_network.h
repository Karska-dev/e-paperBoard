// Adapter: Wi-Fi station (joins an existing network).
#pragma once

#include "app/ports.h"

namespace epb {

class WifiNetwork : public INetwork {
  public:
    bool connect(const Settings& settings, WifiHint* hint) override;
    void off() override;
};

}  // namespace epb
