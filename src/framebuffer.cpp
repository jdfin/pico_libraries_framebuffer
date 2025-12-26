#include "framebuffer.h"

#include <cstdio>

#include "color.h"
#include "xassert.h"


void Framebuffer::line(int h1, int v1, int h2, int v2, const Color c)
{
    // Don't try clipping or anything, just insist that the whole line is on
    // the screen. (h1, v1) and (h2, v2) are both plotted.
    if (h1 < 0 || h1 >= width() || v1 < 0 || v1 >= height() || //
        h2 < 0 || h2 >= width() || v2 < 0 || v2 >= height())
        return;

    // The rest of this function was created by Claude
    // Bresenham's line drawing algorithm
    int dx = h2 - h1;
    int dy = v2 - v1;

    int dx_abs = (dx < 0) ? -dx : dx;
    int dy_abs = (dy < 0) ? -dy : dy;

    int step_h = (dx < 0) ? -1 : 1;
    int step_v = (dy < 0) ? -1 : 1;

    int h = h1;
    int v = v1;

    if (dx_abs > dy_abs) {
        // More horizontal than vertical
        int error = dx_abs / 2;
        for (int i = 0; i <= dx_abs; i++) {
            pixel(h, v, c);
            error -= dy_abs;
            if (error < 0) {
                v += step_v;
                error += dx_abs;
            }
            h += step_h;
        }
    } else {
        // More vertical than horizontal
        int error = dy_abs / 2;
        for (int i = 0; i <= dy_abs; i++) {
            pixel(h, v, c);
            error -= dx_abs;
            if (error < 0) {
                h += step_h;
                error += dy_abs;
            }
            v += step_v;
        }
    }
}


// draw rectangle outline
// (h, v) is a corner pixel
// (wid, hgt) is the number of pixels wide and high
void Framebuffer::draw_rect(int h, int v, int wid, int hgt, const Color c)
{
    int h2 = h + wid;
    int v2 = v + hgt;
    // (h, h2-1) is plotted; (h, h2) is not plotted
    if (h < 0 || h >= width() || h2 < 0 || h2 > width() ||   //
        v < 0 || v >= height() || v2 < 0 || v2 > height() || //
        h2 < h || v2 < v)
        return;

    // top and bottom
    for (int i = 0; i < wid; i++) {
        pixel(h + i, v, c);
        if (hgt > 1)
            pixel(h + i, v + hgt - 1, c);
    }

    // sides (corners already plotted)
    for (int i = 1; i < (hgt - 1); i++) {
        pixel(h, v + i, c);
        if (wid > 1)
            pixel(h + wid - 1, v + i, c);
    }
}


// fill rectangle
void Framebuffer::fill_rect(int h, int v, int wid, int hgt, const Color c)
{
    int h2 = h + wid;
    int v2 = v + hgt;
    // (h, h2-1) is plotted; (h, h2) is not plotted
    if (h < 0 || h >= width() || h2 < 0 || h2 > width() ||   //
        v < 0 || v >= height() || v2 < 0 || v2 > height() || //
        h2 < h || v2 < v)
        return;

    for (int j = 0; j < hgt; j++)
        for (int i = 0; i < wid; i++) pixel(h + i, v + j, c);
}


// draw circle outline
// Midpoint Circle Algorithm (Bresenham's circle algorithm)
// Modified from Claude's code (to add quadrant parameter)
void Framebuffer::draw_circle(int h, int v, int r, const Color c, Quadrant q)
{
    if (r < 0)
        return;

    int x = 0;
    int y = r;
    int d = 1 - r; // decision parameter

    // Helper lambda to plot a pixel with bounds checking
    auto plot = [&](int ph, int pv) {
        if (ph >= 0 && ph < width() && pv >= 0 && pv < height())
            pixel(ph, pv, c);
    };

    // Plot initial 8 symmetric points
    while (x <= y) {

        if (q1(q)) {
            plot(h + x, v + y); // Octant 1
            plot(h + y, v + x); // Octant 2
        }
        if (q2(q)) {
            plot(h - y, v + x); // Octant 3
            plot(h - x, v + y); // Octant 4
        }
        if (q3(q)) {
            plot(h - x, v - y); // Octant 5
            plot(h - y, v - x); // Octant 6
        }
        if (q4(q)) {
            plot(h + y, v - x); // Octant 7
            plot(h + x, v - y); // Octant 8
        }

        x++;

        if (d < 0) {
            // Move east
            d += 2 * x + 1;
        } else {
            // Move southeast
            y--;
            d += 2 * (x - y) + 1;
        }
    }
}


// draw antialiased circle outline
// Wu's circle algorithm using integer-only arithmetic
void Framebuffer::draw_circle_aa(int h, int v, int r, const Color fg,
                                 const Color bg, Quadrant q)
{
    if (r < 0) {
        return;
    } else if (r == 0) {
        pixel(h, v, fg);
        return;
    }

    // Helper lambda to plot a pixel with bounds checking
    auto plot = [&](int ph, int pv, const Color &c) {
        if (ph >= 0 && ph < width() && pv >= 0 && pv < height())
            pixel(ph, pv, c);
    };

    int x = 0;
    int y = r;
    int r_sq = r * r;

    // Walk the circle using Bresenham-style iteration
    while (x <= y) {
        // For position (x, y), calculate intensities for antialiasing
        // The ideal circle passes between pixels (x, y) and (x, y-1)
        // We calculate coverage based on which is closer to radius r

        // Squared distances from origin
        int dist_sq_0 = x * x + y * y;             // farther from center
        int dist_sq_1 = x * x + (y - 1) * (y - 1); // closer to center

        //int dist_sq_2 = x * x + (y - 2) * (y - 2); // still closer to center

        // Calculate alpha for outer pixel
        // range is the difference between outer and inner squared distances
        int range = dist_sq_0 - dist_sq_1;

        // Alpha values based on where r² falls between inner and outer
        // When r² = dist_sq_0, outer pixel is full intensity (255)
        // When r² = dist_sq_1, inner pixel is full intensity (255)
        int alpha_outer;
        if (range == 0) {
            // XXX when can this happen?
            alpha_outer = 128;
        } else {
            xassert(range > 0);
            // Proportion: where does ideal r² fall between inner and outer?
            alpha_outer = ((r_sq - dist_sq_1) * 255) / range;
            if (alpha_outer < 0) {
                y--;
                continue;
                //alpha_outer = 0;
            }
            if (alpha_outer > 255)
                alpha_outer = 255;
        }

        int alpha_inner = 255 - alpha_outer;

        Color c_outer = Color::interpolate(alpha_outer, bg, fg);
        Color c_inner = Color::interpolate(alpha_inner, bg, fg);

        // Plot all 8 octants with quadrant filtering
        // Each octant plots two pixels (outer and inner) for antialiasing
        //printf("(%d,%d)=%u, (%d,%d)=%u\n",  //
        //       x, y, unsigned(c_outer.r()), //
        //       x, y - 1, unsigned(c_inner.r()));
        if (q1(q)) {
            plot(h + x, v + y, c_outer);
            plot(h + x, v + y - 1, c_inner);
            plot(h + y, v + x, c_outer);
            plot(h + y - 1, v + x, c_inner);
        }
        if (q2(q)) {
            plot(h - y, v + x, c_outer);
            plot(h - (y - 1), v + x, c_inner);
            plot(h - x, v + y, c_outer);
            plot(h - x, v + y - 1, c_inner);
        }
        if (q3(q)) {
            plot(h - x, v - y, c_outer);
            plot(h - x, v - (y - 1), c_inner);
            plot(h - y, v - x, c_outer);
            plot(h - (y - 1), v - x, c_inner);
        }
        if (q4(q)) {
            plot(h + y, v - x, c_outer);
            plot(h + y - 1, v - x, c_inner);
            plot(h + x, v - y, c_outer);
            plot(h + x, v - (y - 1), c_inner);
        }

        x++;

// Decide when to step y down (similar to Bresenham's)
// Step when moving to next x would put us too far outside
#if 0
        if (x * x + y * y > r_sq + y) {
            y--;
        }
#endif
    }
}


// print string
void Framebuffer::print(int h, int v, const char *s, const Font &font, //
                        const Color fg, const Color bg, int align)
{
    if (align <= 0) {
        // right-aligned or centered, back up horizontal position
        uint16_t adjust = font.width(s); // string width in pixels
        if (align == 0)
            adjust = adjust / 2; // centered
        h -= adjust;
    }

    // The character-printing method is what notices if a character is not
    // fully on-screen and doesn't print it. Here, we just keep marching
    // through the string, so (for example) if right-align pushes it off the
    // left edge, we might still print some characters later in the string.

    while (*s != '\0') {
        char c = *s++;
        print(h, v, c, font, fg, bg); // align is already taken care of
        h += font.width(c);
    }
}
