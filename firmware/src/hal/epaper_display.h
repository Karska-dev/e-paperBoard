// Adapter: the 7.5" e-paper panel, driven through the Seeed_GFX library.
//
// PATTERN: adapter (wrapper).
// The application wants two things from a display: "show this frame" and
// "show this notice". Seeed_GFX offers hundreds of functions. This class
// translates the first into the second and is the ONLY file that includes
// the library. If the project moves to Seeed_GFX2 or GxEPD2, this one file
// is rewritten and nothing else changes.
#pragma once

#include "app/ports.h"

namespace epb {

class EpaperDisplay : public IDisplay {
  public:
    void showFrame(const uint8_t* frameBits) override;
    void showNotice(const char* title, const char* detail) override;
};

}  // namespace epb
