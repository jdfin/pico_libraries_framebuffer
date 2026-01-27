#pragma once

#include <cstdint>

#include "color.h"
#include "font.h"

// Compile-time image creation.

// The motivations are:
// 1. We have much more flash storage than RAM. A full 480x320x2 image is 300K,
//    which is more than the 264K RAM on a RP2040.
// 2. Rendering fonts at runtime is not particularly slow, but not as fast as
//    just pulling the pre-rendered images from flash.
// 3. If the entire image is in flash, we can DMA it directly from there to
//    the display, without needing to first copy it into RAM.

// This is the structure built at compile time that can end up in flash
// (constexpr).
//
// Making it so it could go in flash (and keeping it that way) was tricky
// (for me). If anything is changed in here, make sure the resulting images
// are still going in flash.

struct PixelImageHdr {
    int wid;
    int hgt;
};

template <typename PIXEL, int w, int h>
struct PixelImage {
    PixelImageHdr hdr{w, h};
    PIXEL pixels[w * h];
};

// Create a boxed label (string surrounded by outline box).
//
// This is intended for compile-time creation of images to be stored in flash.
//
// The resulting image is specified in the parameters, but should be at least
// the size of the string image plus the outline box.
//
// Function template parameters:
//  PIXEL       pixel type (e.g., Pixel565)
//  wid         image width in pixels
//  hgt         image height in pixels
//
// Return type template parameters:
//  PIXEL       pixel type (e.g., Pixel565) (same as function template)
//  wid         image width in pixels (same as function template)
//  hgt         image height in pixels (same as function template)
//
// Function call parameters:
//  text        string to render
//  font        font to use
//  text_clr    text color
//  bord_thk    thickness of border in pixels (0 or more)
//  bord_clr    border color
//  bgnd_clr    background color
//
template <typename PIXEL, int wid, int hgt>
static constexpr PixelImage<PIXEL, wid, hgt>                  //
label_img(const char text[], const Font font, Color text_clr, //
          Color bgnd_clr, int bord_thk = 0, Color bord_clr = Color::none())
{
    PixelImage<PIXEL, wid, hgt> img{};
    // outline and background
    for (int row = 0; row < hgt; row++) {
        for (int col = 0; col < wid; col++) {
            if (row < bord_thk || row >= (hgt - bord_thk) || //
                col < bord_thk || col >= (wid - bord_thk)) {
                img.pixels[row * wid + col] = bord_clr;
            } else {
                img.pixels[row * wid + col] = bgnd_clr;
            }
        }
    }
    int x_off = (wid - font.width(text)) / 2;
    int y_off = (hgt - font.height()) / 2;
    // for each character in the string
    const char *s = text;
    while (*s != '\0') {
        const char ch = *s;
        const int ci = int(ch);
        const uint8_t *gs = font.data + font.info[ci].off; // grayscale
        // width of character box
        const int x_adv = font.info[ci].x_adv;
        // offsets of glyph within character box
        const int ch_x_off = font.info[ci].x_off;
        const int ch_y_off = font.info[ci].y_off;
        // glyph size (usually smaller than character box)
        const int g_wid = font.info[ci].w;
        const int g_hgt = font.info[ci].h;
        // (row, col) covers character box
        for (int row = 0; row < font.y_adv; row++) {
            for (int col = 0; col < x_adv; col++) {
                // see if the glyph covers this pixel in the character box
                if (row >= ch_y_off && row < (ch_y_off + g_hgt) && //
                    col >= ch_x_off && col < (ch_x_off + g_wid)) {
                    // yes, interpolate from bgnd_clr to fg based on glyph grayscale
                    int g_row = row - ch_y_off;
                    int g_col = col - ch_x_off;
                    uint8_t gray = gs[g_row * g_wid + g_col];
                    img.pixels[(row + y_off) * wid + (x_off + col)] =
                        Color::interpolate(gray, bgnd_clr, text_clr);
                } else {
                    // nope, it's background
                    img.pixels[(row + y_off) * wid + (x_off + col)] = bgnd_clr;
                }
            }
        }
        x_off += x_adv; // next character box start
        s++;            // next character in string
    }
    return img;
}

// deprecated version with different parameter order
template <typename PIXEL, int wid, int hgt>
static constexpr PixelImage<PIXEL, wid, hgt>                  //
label_img(const char text[], const Font font, Color text_clr, //
          int bord_thk, Color bord_clr, Color bgnd_clr)
{
    PixelImage<PIXEL, wid, hgt> img{};
    // outline and background
    for (int row = 0; row < hgt; row++) {
        for (int col = 0; col < wid; col++) {
            if (row < bord_thk || row >= (hgt - bord_thk) || //
                col < bord_thk || col >= (wid - bord_thk)) {
                img.pixels[row * wid + col] = bord_clr;
            } else {
                img.pixels[row * wid + col] = bgnd_clr;
            }
        }
    }
    int x_off = (wid - font.width(text)) / 2;
    int y_off = (hgt - font.height()) / 2;
    // for each character in the string
    const char *s = text;
    while (*s != '\0') {
        const char ch = *s;
        const int ci = int(ch);
        const uint8_t *gs = font.data + font.info[ci].off; // grayscale
        // width of character box
        const int x_adv = font.info[ci].x_adv;
        // offsets of glyph within character box
        const int ch_x_off = font.info[ci].x_off;
        const int ch_y_off = font.info[ci].y_off;
        // glyph size (usually smaller than character box)
        const int g_wid = font.info[ci].w;
        const int g_hgt = font.info[ci].h;
        // (row, col) covers character box
        for (int row = 0; row < font.y_adv; row++) {
            for (int col = 0; col < x_adv; col++) {
                // see if the glyph covers this pixel in the character box
                if (row >= ch_y_off && row < (ch_y_off + g_hgt) && //
                    col >= ch_x_off && col < (ch_x_off + g_wid)) {
                    // yes, interpolate from bgnd_clr to fg based on glyph grayscale
                    int g_row = row - ch_y_off;
                    int g_col = col - ch_x_off;
                    uint8_t gray = gs[g_row * g_wid + g_col];
                    img.pixels[(row + y_off) * wid + (x_off + col)] =
                        Color::interpolate(gray, bgnd_clr, text_clr);
                } else {
                    // nope, it's background
                    img.pixels[(row + y_off) * wid + (x_off + col)] = bgnd_clr;
                }
            }
        }
        x_off += x_adv; // next character box start
        s++;            // next character in string
    }
    return img;
}


// Update a boxed label (string surrounded by outline box).
//
// This is intended for runtime modification of images stored in ram.
//
// The resulting image is specified in the parameters, but should be at least
// the size of the string image plus the outline box.
//
// Function template parameters:
//  PIXEL       pixel type (e.g., Pixel565)
//
// Function call parameters:
//  img         image to modify
//  text        string to render
//  font        font to use
//  text_clr    text color
//  bord_thk    thickness of border in pixels (0 or more)
//  bord_clr    border color
//  bgnd_clr    background color
//
template <typename PIXEL>
void label_img(PixelImageHdr *img,                                 // image
               const char text[], const Font font, Color text_clr, // label
               int bord_thk, Color bord_clr,                       // border
               Color bgnd_clr)                                     // background
{
    PixelImage<PIXEL, 0, 0> *pimg =
        reinterpret_cast<PixelImage<PIXEL, 0, 0> *>(img);
    int wid = pimg->hdr.wid;
    int hgt = pimg->hdr.hgt;
    PIXEL *pixels = pimg->pixels;

    // outline and background
    for (int row = 0; row < hgt; row++) {
        for (int col = 0; col < wid; col++) {
            if (row < bord_thk || row >= (hgt - bord_thk) || //
                col < bord_thk || col >= (wid - bord_thk)) {
                pixels[row * wid + col] = bord_clr;
            } else {
                pixels[row * wid + col] = bgnd_clr;
            }
        }
    }
    int x_off = (wid - font.width(text)) / 2;
    int y_off = (hgt - font.height()) / 2;
    // for each character in the string
    const char *s = text;
    while (*s != '\0') {
        const char ch = *s;
        const int ci = int(ch);
        const uint8_t *gs = font.data + font.info[ci].off; // grayscale
        // width of character box
        const int x_adv = font.info[ci].x_adv;
        // offsets of glyph within character box
        const int ch_x_off = font.info[ci].x_off;
        const int ch_y_off = font.info[ci].y_off;
        // glyph size (usually smaller than character box)
        const int g_wid = font.info[ci].w;
        const int g_hgt = font.info[ci].h;
        // (row, col) covers character box
        for (int row = 0; row < font.y_adv; row++) {
            for (int col = 0; col < x_adv; col++) {
                // see if the glyph covers this pixel in the character box
                if (row >= ch_y_off && row < (ch_y_off + g_hgt) && //
                    col >= ch_x_off && col < (ch_x_off + g_wid)) {
                    // yes, interpolate from bgnd_clr to fg based on glyph grayscale
                    int g_row = row - ch_y_off;
                    int g_col = col - ch_x_off;
                    uint8_t gray = gs[g_row * g_wid + g_col];
                    pixels[(row + y_off) * wid + (x_off + col)] =
                        Color::interpolate(gray, bgnd_clr, text_clr);
                } else {
                    // nope, it's background
                    pixels[(row + y_off) * wid + (x_off + col)] = bgnd_clr;
                }
            }
        }
        x_off += x_adv; // next character box start
        s++;            // next character in string
    }
}
