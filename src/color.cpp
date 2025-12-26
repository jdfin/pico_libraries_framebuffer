
#include "color.h"

const Color Color::black(0x00, 0x00, 0x00);
const Color Color::red(0xff, 0x00, 0x00);
const Color Color::green(0x00, 0xff, 0x00);
const Color Color::blue(0x00, 0x00, 0xff);
const Color Color::yellow(0xff, 0xff, 0x00);
const Color Color::magenta(0xff, 0x00, 0xff);
const Color Color::cyan(0x00, 0xff, 0xff);
const Color Color::white(0xff, 0xff, 0xff);
const Color Color::gray25(0x40, 0x40, 0x40); // lighter
const Color Color::gray50(0x80, 0x80, 0x80);
const Color Color::gray75(0xc0, 0xc0, 0xc0); // darker
const Color Color::none(0, 0, 0, Color::transparent);

Color Color::interpolate(uint8_t alpha, const Color& bg, const Color& fg)
{
    // alpha == 0 means background
    // alpha == 255 means foreground
    if (alpha == 0)
        return bg;
    if (alpha == 255)
        return fg;
    // arithmetic needs more than 8 bits
    int d = int(fg._r) - int(bg._r); // could be negative
    uint8_t r = bg._r + (d * alpha) / 255;
    d = int(fg._g) - int(bg._g);
    uint8_t g = bg._g + (d * alpha) / 255;
    d = int(fg._b) - int(bg._b);
    uint8_t b = bg._b + (d * alpha) / 255;
    return Color(r, g, b);
}