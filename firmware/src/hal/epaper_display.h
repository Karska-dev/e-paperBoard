// Adapter: the 7.5" e-paper panel, driven through the Seeed_GFX library.
//
// PATTERN: adapter (wrapper). The app wants "show this frame" and "show this
// notice"; the library offers hundreds of functions. This class translates,
// and it is the ONLY file that includes the library, so moving to another
// display library means rewriting this one file.
#pragma once

#include "app/ports.h"

namespace epb {

class EpaperDisplay : public IDisplay {
  public:
    void showFrame(const uint8_t* frameBits) override;
    void showNotice(const char* title, const char* detail) override;
};

}  // namespace epb
