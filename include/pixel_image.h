// compile-time image creation

#include "color.h"
#include "font.h"


template <typename PIXEL, int wid, int hgt>
struct PixelImage {
    int width = wid;
    int height = hgt;
    PIXEL pixels[wid * hgt];
};


// create an image from a character in a font
// 'wid' is the character box width in pixels
// 'hgt' is the character box height in pixels
template <typename PIXEL, char ch, int wid, int hgt>
static constexpr PixelImage<PIXEL, wid, hgt> image_init(const Font font,
                                                        Color fg, Color bg)
{
    PixelImage<PIXEL, wid, hgt> img{};
    const int ci = int(ch);
    const uint8_t *gs = font.data + font.info[ci].off;
    // width of character box
    const int x_adv = font.info[ci].x_adv;
    // offsets of glyph within character box
    const int x_off = font.info[ci].x_off;
    const int y_off = font.info[ci].y_off;
    // glyph size (usually smaller than character box)
    const int g_wid = font.info[ci].w;
    const int g_hgt = font.info[ci].h;
    // (row, col) covers character box
    for (int row = 0; row < font.y_adv; row++) {
        for (int col = 0; col < x_adv; col++) {
            // see if the glyph covers this pixel in the character box
            if (row >= y_off && row < (y_off + g_hgt) && //
                col >= x_off && col < (x_off + g_wid)) {
                // yes, interpolate from bg to fg based on glyph grayscale
                int g_row = row - y_off;
                int g_col = col - x_off;
                uint8_t gray = gs[g_row * g_wid + g_col];
                img.pixels[row * x_adv + col] =
                    Color::interpolate(gray, bg, fg);
            } else {
                // nope, it's background
                img.pixels[row * x_adv + col] = bg;
            }
        }
    }
    return img;
}


// create an image from a string in a font
// 'wid' is the string box width in pixels
// 'hgt' is the string box height in pixels
template <typename PIXEL, const char s[], int wid, int hgt>
static constexpr PixelImage<PIXEL, wid, hgt> image_init(const Font font,
                                                        Color fg, Color bg)
{
    PixelImage<PIXEL, wid, hgt> img{};
    int x_off = 0;
    // for each character in the string
    const char *s1 = s;
    while (*s1 != '\0') {
        const char ch = *s1;
        const int ci = int(ch);
        const uint8_t *gs = font.data + font.info[ci].off;
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
                    // yes, interpolate from bg to fg based on glyph grayscale
                    int g_row = row - ch_y_off;
                    int g_col = col - ch_x_off;
                    uint8_t gray = gs[g_row * g_wid + g_col];
                    img.pixels[row * wid + (x_off + col)] =
                        Color::interpolate(gray, bg, fg);
                } else {
                    // nope, it's background
                    img.pixels[row * wid + (x_off + col)] = bg;
                }
            }
        }
        x_off += x_adv; // next character box start
        s1++;           // next character in string
    }
    return img;
}
