#pragma once

#include <cstdint>

#include "color.h"


// An RGB pixel is sent to the display in two bytes: 5 bits red, 6 bits green,
// 5 bits blue. The 16 bits as sent over SPI should be:
//
//     | r7 r6 r5 r4 r3 g7 g6 g5 | g4 g3 g2 b7 b6 b5 b4 b3 |
//
// If the SPI is configured as 16 bits, then the bits should be laid out in a
// uint16_t as above; red at the top, blue at the bottom, green in the middle:
//
//     | 15 14 13 12 11 10  9  8 |  7  6  5  4  3  2  1  0 |
//     | r7 r6 r5 r4 r3 g7 g6 g5 | g4 g3 g2 b7 b6 b5 b4 b3 |
//
// The SPI controller will get all 16 bits at once, and send the MSB first, so
// the display will get the bits in the correct order.
//
// However, if the SPI is configured as 8 bits, then the lower byte of each
// pixel goes first, so the bytes need to be swapped in memory:
//
//     | 15 14 13 12 11 10  9  8 |  7  6  5  4  3  2  1  0 |
//     | g4 g3 g2 b7 b6 b5 b4 b3 | r7 r6 r5 r4 r3 g7 g6 g5 |
//
// In this case, the SPI controller will first send the lower byte, then the
// higher byte, so the display will get the bits in the correct order.
//
// For efficient transfer, layout of the pixel in memory is then dependent on
// whether we send the pixels in 16-bit or 8-bit mode.
//
// API calls accept 8 bits for each color, then the top 5 or 6 bits is used,
// so (for red) we're storing and using r7..r3, and r2..r0 are dropped.
// Similar for green and blue.

class Pixel565
{

public:

    constexpr Pixel565(Color c = Color::black()) :
        _pixel(color_to_uint16(c))
    {
    }

    constexpr Pixel565 &operator=(const Color &c)
    {
        _pixel = color_to_uint16(c);
        return *this;
    }

    static constexpr int xfer_size = 16;
    static_assert(xfer_size == 8 || xfer_size == 16,
                  "Pixel565: xfer_size must be 8 or 16");

    constexpr uint16_t value() const
    {
        return _pixel;
    }

private:

    uint16_t _pixel;

    static constexpr uint16_t color_to_uint16(const Color &c)
    {
        if constexpr (xfer_size == 16) {
            // SPI in 16-bit mode
            return ((uint16_t(c.r()) << 8) & 0xf800) |
                   ((uint16_t(c.g()) << 3) & 0x07e0) |
                   ((uint16_t(c.b()) >> 3) & 0x001f);
        } else {
            // SPI in 8-bit mode
            return ((uint16_t(c.g()) << 11) & 0xe000) |
                   ((uint16_t(c.b()) << 5) & 0x1f00) |
                   ((uint16_t(c.r()) << 0) & 0x00f8) |
                   ((uint16_t(c.g()) >> 5) & 0x0007);
        }
    }
};

static_assert(sizeof(Pixel565) == sizeof(uint16_t), "Pixel565 size incorrect");
