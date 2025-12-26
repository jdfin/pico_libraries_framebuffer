#pragma once

#include <cstdint>

#include "color.h"


// An RGB pixel is sent to the display in two bytes: 5 bits red, 6 bits green,
// 5 bits blue, packed as follows. The packing assumes a uint16_t is stored
// little-endian, so SPI sends the lower byte first. The LCD expects to
// receive 5 bits red, 6 bits green, then 5 bits blue, each MSB first.
//
// API calls accept 8 bits for each color, then the top 5 or 6 bits is used.
//
// Pixel is stored in memory so it can be SPIed in-place. SPI goes MSB first,
// big-endian.
//     | 15 14 13 12 11 10  9  8 |  7  6  5  4  3  2  1  0 |
//     | g4 g3 g2 b7 b6 b5 b4 b3 | r7 r6 r5 r4 r3 g7 g6 g5 |
//                              lower byte will be sent first
//
// Pixel as SPIed to display module (CPU is assumed little-endian).
//       little end goes first,    then big end
//     | r7 r6 r5 r4 r3 g7 g6 g5 | g4 g3 g2 b7 b6 b5 b4 b3 |
//
// Remember the top bits of an 8-bit color are used, so we're storing and
// using r7..r3, and r2..r0 are dropped. Similar for g and b.

class Pixel565
{

public:

    Pixel565(Color c = Color::black) :
        _pixel(color_to_uint16(c))
    {
    }

    const uint8_t *p_raw() const { return (const uint8_t *)(&_pixel); }

    Pixel565 &operator=(const Color &c)
    {
        _pixel = color_to_uint16(c);
        return *this;
    }

private:

    uint16_t _pixel;

    uint16_t color_to_uint16(const Color &c)
    {
        return ((uint16_t(c.g()) << 11) & 0xe000) |
               ((uint16_t(c.b()) << 5) & 0x1f00) |
               ((uint16_t(c.r()) << 0) & 0x00f8) |
               ((uint16_t(c.g()) >> 5) & 0x0007);
    }
};
