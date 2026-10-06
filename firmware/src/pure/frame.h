// The image format: what the server sends and the panel shows.
// LAYER: pure.
//
// Raw 1-bit bitmap, 800 x 480, no header: always exactly 48,000 bytes.
//
//    byte 0            byte 1             bit 1 = white, bit 0 = black
//   [7 6 5 4 3 2 1 0] [7 6 5 4 3 2 1 0] ...
//    ^ pixel x = 0     ^ pixel x = 8      rows top to bottom, 100 bytes each
//
// Why raw and not PNG: no decoder, no decode memory, and "is it valid?" is
// one size comparison. The server does the rendering and dithering.
#pragma once

#include <cstddef>
#include <cstdint>

namespace epb {

inline constexpr uint16_t kFrameWidth = 800;
inline constexpr uint16_t kFrameHeight = 480;
inline constexpr size_t kFrameBytes = static_cast<size_t>(kFrameWidth) * kFrameHeight / 8;

// TECHNIQUE: compile-time check. The build fails if these numbers ever stop
// agreeing, and the check costs nothing at run time.
static_assert(kFrameBytes == 48000, "frame size must match the server contract");
static_assert(kFrameWidth % 8 == 0, "rows must be whole bytes");

}  // namespace epb
