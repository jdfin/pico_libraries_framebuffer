#pragma once

#include "color.h"
#include "font.h"


class Framebuffer
{
public:

    Framebuffer(int width, int height) :
        _width(width),
        _height(height),
        _brightness_pct(0)
    {
    }

    virtual ~Framebuffer() {}

    virtual void brightness(int) {}
    virtual int brightness() { return _brightness_pct; }

    int width() const { return _width; }
    int height() const { return _height; }

    // set a pixel to specified color
    virtual void pixel(int h, int v, const Color c) = 0;

    // line from one absolute point to another
    virtual void line(int h1, int v1, int h2, int v2, const Color c);

    // draw rectangle outline
    // (h, v) is a corner pixel
    // (wid, hgt) is the number of pixels wide and high
    virtual void draw_rect(int h, int v, int wid, int hgt, const Color c);

    // fill rectangle
    virtual void fill_rect(int h, int v, int wid, int hgt, const Color c);

    // Bit mask controlling which quadrants get drawn in circle methods:
    //   bit 0 -> quadrant 1, (+, +), lower right
    //   bit 1 -> quadrant 2, (-, +), lower left
    //   bit 2 -> quadrant 3, (-, -), upper left
    //   bit 3 -> quadrant 4, (+, -), upper right
    enum class Quadrant {
        LowerRight = 0x1,
        LowerLeft = 0x2,
        UpperLeft = 0x4,
        UpperRight = 0x8,
        Lower = LowerRight | LowerLeft,  // lower half
        Upper = UpperRight | UpperLeft,  // upper half
        Right = UpperRight | LowerRight, // right half
        Left = UpperLeft | LowerLeft,    // left half
        All = Lower | Upper,             // full circle
    };

    // draw circle outline
    // (h, v) is the center point, r is the radius
    virtual void draw_circle(int h, int v, int r, const Color c,
                             Quadrant quadrant = Quadrant::All);

    // draw antialiased circle outline using Wu's algorithm
    // (h, v) is the center point, r is the radius
    // fg is the circle color, bg is used for antialiasing blending
    // Uses integer-only arithmetic (no floating point)
    virtual void draw_circle_aa(int h, int v, int r, const Color fg,
                                const Color bg,
                                Quadrant quadrant = Quadrant::All);

    // print character to screen
    virtual void print(int h, int v, char c, const Font &font, //
                       const Color fg, const Color bg, int align = 1) = 0;

    // print string to screen
    // align == -1 for right-aligned (exends "minus" from current position),
    // 0 for centered, and +1 for left-aligned. The default just iterates
    // through char-by-char; one might want to override this if too many
    // glyphs extend outside their "bounding box".
    virtual void print(int h, int v, const char *s, const Font &font, //
                       const Color fg, const Color bg, int align = 1);

protected:

    int _width;
    int _height;

    int _brightness_pct; // percent, 0..100

    bool q1(Quadrant q)
    {
        return static_cast<int>(q) & static_cast<int>(Quadrant::LowerRight);
    }

    bool q2(Quadrant q)
    {
        return static_cast<int>(q) & static_cast<int>(Quadrant::LowerLeft);
    }

    bool q3(Quadrant q)
    {
        return static_cast<int>(q) & static_cast<int>(Quadrant::UpperLeft);
    }

    bool q4(Quadrant q)
    {
        return static_cast<int>(q) & static_cast<int>(Quadrant::UpperRight);
    }
};
