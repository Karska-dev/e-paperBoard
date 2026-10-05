// Adapter: the chip's calendar clock.
#pragma once

#include "app/ports.h"

namespace epb {

class WallClock : public IWallClock {
  public:
    void set(int64_t epochSeconds) override;
};

}  // namespace epb
