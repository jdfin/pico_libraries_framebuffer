#pragma once

#include <cassert>

#include "color.h"
#include "font.h"
#include "pixel_image.h"


class Framebuffer
{
public:

    Framebuffer(int width, int height) :
        _phys_wid(width),
        _phys_hgt(height),
        _width(width),
        _height(height),
        _brightness_pct(0),
        _rotation(Rotation::landscape)
    {
        // Initialization of width, height, and rotation assume we
        // start out in landscape mode and _phys_wid >= _phys_hgt.
        assert(_rotation == Rotation::landscape ||
               _rotation == Rotation::landscape2);
        assert(_phys_wid >= _phys_hgt);
    }

    virtual ~Framebuffer() = default;

    virtual void brightness(int)
    {
    }
    virtual int brightness()
    {
        return _brightness_pct;
    }

    int width() const
    {
        return _width;
    }

    int height() const
    {
        return _height;
    }

    // orientation
    enum class Rotation {
        portrait,   // portrait
        landscape,  // landscape, 90 degrees clockwise
        portrait2,  // portrait, 180 degrees from portrait
        landscape2, // landscape, 180 degrees from landscape
    };

    // Horizontal alignment. Note that right alignment does not use the
    // specified point, whereas left alignment does. The intent is that
    // left- and right-aligned widgets are drawn right next to each other
    // when their specified points are the same.
    enum class HAlign {
        //                      V
        Right = -1, // draw_here
        Center = 0, //      draw_here
        Left = +1,  //          draw_here
    };

    // Vertical alignment.
    enum class VAlign {
        Bottom = -1, // draw above reference point
        Center = 0,  // center on reference point
        Top = +1,    // draw reference point and below
    };

    virtual void set_rotation(Rotation r)
    {
        // subclass should do most of the work
        _rotation = r;
        if (_rotation == Rotation::landscape ||
            _rotation == Rotation::landscape2) {
            _width = _phys_wid;
            _height = _phys_hgt;
            assert(_width >= _height);
        } else {
            assert(_rotation == Rotation::portrait ||
                   _rotation == Rotation::portrait2);
            _width = _phys_hgt;
            _height = _phys_wid;
            assert(_width <= _height);
        }
    }

    Rotation get_rotation() const
    {
        return _rotation;
    }

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

    // write array of pixels to screen
    virtual void write(int hor, int ver, const PixelImageHdr *image,
                       HAlign align = HAlign::Left) = 0;

    // write number to screen using pre-rendered digit images
    virtual void write(int hor, int ver, int num, const PixelImageHdr *dig[10],
                       HAlign align = HAlign::Left, //
                       int *wid = nullptr, int *hgt = nullptr) = 0;

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
    virtual void draw_circle(int hor, int ver, int rad, const Color c,
                             Quadrant quadrant = Quadrant::All);

    // draw antialiased circle outline using Wu's algorithm
    // (h, v) is the center point, r is the radius
    // fg is the circle color, bg is used for antialiasing blending
    // Uses integer-only arithmetic (no floating point)
    virtual void draw_circle_aa(int hor, int ver, int rad, const Color fg,
                                const Color bg,
                                Quadrant quadrant = Quadrant::All);

    // print character to screen
    virtual void print(int hor, int ver, char ch, const Font &font, //
                       const Color fg, const Color bg,
                       HAlign align = HAlign::Left) = 0;

    // print string to screen
    // The default just iterates through char-by-char; one might want to
    // override this if too many glyphs extend outside their bounding box.
    virtual void print(int hor, int ver, const char *str, const Font &font, //
                       const Color fg, const Color bg,
                       HAlign align = HAlign::Left);

protected:

    const int _phys_wid;
    const int _phys_hgt;

    // these two depend on rotation
    int _width;
    int _height;

    int _brightness_pct; // percent, 0..100

    Rotation _rotation;

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
