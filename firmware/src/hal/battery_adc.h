// Adapter: battery voltage through the ESP32's analog-to-digital converter.
#pragma once

#include "app/ports.h"

namespace epb {

class BatteryAdc : public IBatteryAdc {
  public:
    void sample(uint16_t* out, size_t count) override;
};

}  // namespace epb
