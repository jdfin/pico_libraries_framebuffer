#pragma once

#include <cstdint>


class Color
{

public:

    constexpr Color(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0,
                    uint8_t a = opaque) :
        _r(r),
        _g(g),
        _b(b),
        _a(a)
    {
    }

    constexpr uint8_t r() const { return _r; }
    constexpr uint8_t g() const { return _g; }
    constexpr uint8_t b() const { return _b; }
    constexpr uint8_t a() const { return _a; }

#if 0
    void set(uint8_t r, uint8_t g, uint8_t b, uint8_t a=opaque)
    {
        _r = r;
        _g = g;
        _b = b;
        _a = a;
    }
#endif

    void get(uint8_t &r, uint8_t &g, uint8_t &b) const
    {
        r = _r;
        g = _g;
        b = _b;
    }

    void get(uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a) const
    {
        get(r, g, b);
        a = _a;
    }

    static constexpr Color black() { return Color(0x00, 0x00, 0x00, opaque); }
    static constexpr Color red() { return Color(0xff, 0x00, 0x00, opaque); }
    static constexpr Color green() { return Color(0x00, 0xff, 0x00, opaque); }
    static constexpr Color blue() { return Color(0x00, 0x00, 0xff, opaque); }
    static constexpr Color yellow() { return Color(0xff, 0xff, 0x00, opaque); }
    static constexpr Color magenta() { return Color(0xff, 0x00, 0xff, opaque); }
    static constexpr Color cyan() { return Color(0x00, 0xff, 0xff, opaque); }
    static constexpr Color white() { return Color(0xff, 0xff, 0xff, opaque); }
    static constexpr Color gray25() { return Color(0x40, 0x40, 0x40, opaque); } // lighter
    static constexpr Color gray50() { return Color(0x80, 0x80, 0x80, opaque); } // medium
    static constexpr Color gray75() { return Color(0xc0, 0xc0, 0xc0, opaque); } // darker
    static constexpr Color none() { return Color(0, 0, 0, transparent); } // alpha = transparent

    // alpha
    static const uint8_t transparent = 0;
    static const uint8_t opaque = UINT8_MAX;

    static constexpr Color interpolate(uint8_t alpha, const Color bg,
                                       const Color fg)
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

private:

    const uint8_t _r;
    const uint8_t _g;
    const uint8_t _b;
    const uint8_t _a;
};
