// The image format the server sends and the panel shows.
//
// LAYER: pure.
//
// FORMAT: raw 1-bit bitmap, no header, no compression.
//   - 800 x 480 pixels, one bit per pixel
//   - rows top to bottom, pixels left to right
//   - 8 pixels per byte, the leftmost pixel is the HIGHEST bit (MSB first)
//   - bit 1 = white, bit 0 = black
//   - total size: 800 * 480 / 8 = 48,000 bytes, always
//
// Why raw instead of PNG: the device needs no decoder and no decode memory,
// and "is this a valid image" becomes one comparison (is it exactly 48,000
// bytes?). The server does the hard work of rendering and dithering.
#pragma once

#include <cstddef>
#include <cstdint>

namespace epb {

inline constexpr uint16_t kFrameWidth = 800;
inline constexpr uint16_t kFrameHeight = 480;
inline constexpr size_t kFrameBytes = static_cast<size_t>(kFrameWidth) * kFrameHeight / 8;

// `constexpr` + `static_assert`: the compiler does the arithmetic and refuses
// to build if someone changes a dimension so that the numbers stop agreeing.
// A check at compile time costs nothing at run time.
static_assert(kFrameBytes == 48000, "frame size must match the server contract");
static_assert(kFrameWidth % 8 == 0, "rows must be whole bytes");

}  // namespace epb
