#pragma once

#include <cstdint>


class Color
{

public:

    Color(uint8_t r=0, uint8_t g=0, uint8_t b=0, uint8_t a=opaque) : _r(r), _g(g), _b(b), _a(a) {}

    uint8_t r() const { return _r; }
    uint8_t g() const { return _g; }
    uint8_t b() const { return _b; }
    uint8_t a() const { return _a; }

    void set(uint8_t r, uint8_t g, uint8_t b, uint8_t a=opaque)
    {
        _r = r;
        _g = g;
        _b = b;
        _a = a;
    }

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

    static const Color black;
    static const Color red;
    static const Color green;
    static const Color blue;
    static const Color yellow;
    static const Color magenta;
    static const Color cyan;
    static const Color white;
    static const Color gray25;
    static const Color gray50;
    static const Color gray75;

    static const Color none; // alpha = transparent

    // alpha
    static const uint8_t transparent = 0;
    static const uint8_t opaque = UINT8_MAX;

    // alpha 0 returns background, 255 returns foreground
    static Color interpolate(uint8_t alpha, const Color& bg, const Color& fg);

private:

    uint8_t _r;
    uint8_t _g;
    uint8_t _b;
    uint8_t _a;
};
