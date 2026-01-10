#pragma once

#include <cstdint>
#include "xassert.h"


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

    // clang-format off
    constexpr uint8_t r() const { return _r; }
    constexpr uint8_t g() const { return _g; }
    constexpr uint8_t b() const { return _b; }
    constexpr uint8_t a() const { return _a; }
    // clang-format on

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

    // I would expect the xassert() calls to prevent these from being
    // evaluated at compile time, but it seems to be okay.
    // pixel_image::label_img() calls that use them still get put in flash.

    // gray(0) is black, gray(100) is white
    static constexpr Color gray(int brt_pct)
    {
        xassert(0 <= brt_pct && brt_pct <= 100);
        int brt = brt_pct * 255 / 100;
        return Color(brt, brt, brt, opaque);
    }

    // brightness is 0..255, 0 is only the specified color, greater than zero
    // lightens it up until 255 is white.
    // E.g., red(0) is pure red, red(128) is light red, and red(255) is white.

    // Note that most "named" colors are in color_html.h, i.e. if you ask for
    // Color::red() you are getting it from there.

    static constexpr Color red(int brt_pct)
    {
        xassert(0 <= brt_pct && brt_pct <= 100);
        int brt = brt_pct * 255 / 100;
        return Color(0xff, brt, brt, opaque);
    }

    static constexpr Color green(int brt_pct)
    {
        xassert(0 <= brt_pct && brt_pct <= 100);
        int brt = brt_pct * 255 / 100;
        return Color(brt, 0xff, brt, opaque);
    }

    static constexpr Color blue(int brt_pct)
    {
        xassert(0 <= brt_pct && brt_pct <= 100);
        int brt = brt_pct * 255 / 100;
        return Color(brt, brt, 0xff, opaque);
    }

    // yellow is red + green
    static constexpr Color yellow(int brt_pct)
    {
        xassert(0 <= brt_pct && brt_pct <= 100);
        int brt = brt_pct * 255 / 100;
        return Color(0xff, 0xff, brt, opaque);
    }

    // magenta is red + blue
    static constexpr Color magenta(int brt_pct)
    {
        xassert(0 <= brt_pct && brt_pct <= 100);
        int brt = brt_pct * 255 / 100;
        return Color(0xff, brt, 0xff, opaque);
    }

    // cyan is green + blue
    static constexpr Color cyan(int brt_pct)
    {
        xassert(0 <= brt_pct && brt_pct <= 100);
        int brt = brt_pct * 255 / 100;
        return Color(brt, 0xff, 0xff, opaque);
    }

    // lots of html colors by name
#include "color_html.h"

    // constructor above uses rgb values 0-255
    // this one uses percentages 0-100
    static constexpr Color rgb(int r_pct, int g_pct, int b_pct)
    {
        xassert(0 <= r_pct && r_pct <= 100);
        xassert(0 <= g_pct && g_pct <= 100);
        xassert(0 <= b_pct && b_pct <= 100);
        int r = r_pct * 255 / 100;
        int g = g_pct * 255 / 100;
        int b = b_pct * 255 / 100;
        return Color(r, g, b, opaque);
    }

    // Convert HSB (Hue, Saturation, Brightness) to RGB
    // hue: 0-360 (hue in degrees)
    // sat_pct: 0-100 (saturation percentage)
    // brt_pct: 0-100 (brightness percentage)
    // Returns: Color with RGB values 0-255
    static constexpr Color hsb(int hue_deg, int sat_pct, int brt_pct)
    {
        // Clamp inputs to valid ranges
        if (hue_deg < 0) hue_deg = 0;
        if (hue_deg > 360) hue_deg = 360;
        if (sat_pct < 0) sat_pct = 0;
        if (sat_pct > 100) sat_pct = 100;
        if (brt_pct < 0) brt_pct = 0;
        if (brt_pct > 100) brt_pct = 100;

        // Special case: zero saturation means gray
        if (sat_pct == 0) {
            int brt_raw = (brt_pct * 255) / 100;
            return Color(brt_raw, brt_raw, brt_raw, opaque);
        }

        // Normalize hue to 0-359
        if (hue_deg == 360)
            hue_deg = 0;

        // Determine which sector (0-5) of the hue wheel
        int hue_sec = hue_deg / 60;
        int hue_off = hue_deg % 60;  // position within sector (0-59)

        xassert(0 <= hue_sec && hue_sec < 6);
        xassert(0 <= hue_off && hue_off < 60);

        // Calculate brightness value (0-255)
        int val = (brt_pct * 255) / 100;

        // Calculate chroma
        int chr = (val * sat_pct) / 100;

        // Calculate second largest component
        // x = chr * (1 - |((h / 60) mod 2) - 1|)
        // For even sectors, x increases; for odd sectors, x decreases
        int x = 0;
        if (hue_sec & 1) {
            // Odd sector: x decreases as hue_off increases
            x = (chr * (60 - hue_off)) / 60;
        } else {
            // Even sector: x increases as hue_off increases
            x = (chr * hue_off) / 60;
        }

        // Calculate minimum component
        int m = val - chr;
        int red_raw = 0;
        int grn_raw = 0;
        int blu_raw = 0;

        switch (hue_sec) {
            case 0: // Red to Yellow (0-60 degrees)
                red_raw = val;
                grn_raw = m + x;
                blu_raw = m;
                break;
            case 1: // Yellow to Green (60-120 degrees)
                red_raw = m + x;
                grn_raw = val;
                blu_raw = m;
                break;
            case 2: // Green to Cyan (120-180 degrees)
                red_raw = m;
                grn_raw = val;
                blu_raw = m + x;
                break;
            case 3: // Cyan to Blue (180-240 degrees)
                red_raw = m;
                grn_raw = m + x;
                blu_raw = val;
                break;
            case 4: // Blue to Magenta (240-300 degrees)
                red_raw = m + x;
                grn_raw = m;
                blu_raw = val;
                break;
            default: // Magenta to Red (300-360 degrees)
                red_raw = val;
                grn_raw = m;
                blu_raw = m + x;
                break;
        }

        xassert(0 <= red_raw && red_raw < 256);
        xassert(0 <= grn_raw && grn_raw < 256);
        xassert(0 <= blu_raw && blu_raw < 256);

        return Color(red_raw, grn_raw, blu_raw, opaque);
    }

    static constexpr Color none()
    {
        return Color(0, 0, 0, transparent);
    }

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
