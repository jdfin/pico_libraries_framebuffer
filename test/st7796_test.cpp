
#include "st7796.h"

#include <cstdio>

#include "color.h"
#include "font.h"
#include "great_vibes_48.h"
#include "hardware/spi.h"
#include "pico/stdio.h"
#include "pico/stdio_usb.h"
#include "pico/stdlib.h"
#include "roboto_32.h"

// Pico:
//
// Signal Pin
//
// MISO   21  SPI0_RX
// CS     22  SPI0_CSn
//        23  GND
// SCK    24  SPI0_SCK
// MOSI   25  SPI0_TX
// CD     26  GPIO20
// RST    27  GPIO21
//        28  GND
// LED    29  GPIO22
//        30  RUN
//        31  GPIO26
//        32  GPIO27
//        33  GND
//        34  GPIO28
//        35  ADC_VREF
//        36  3V3_OUT
//        37  3V3_EN
//        38  GND
//        39  VSYS_IN_OUT
//        40  VBUS_OUT

static const int spi_miso_pin = 16;
static const int spi_mosi_pin = 19;
static const int spi_clk_pin = 18;
static const int spi_cs_pin = 17;
static const int spi_baud = 15'000'000;

static const int lcd_cd_pin = 20;
static const int lcd_rst_pin = 21;
static const int lcd_led_pin = 22;

// Most glyphs of great_vibes_48 extend past the x_adv, so it does not render
// nicely but it does show that right-side cropping works instead of crashing
//static const Font& font = great_vibes_48;

static const Font &font = roboto_32;

static void rotations(St7796 &lcd);

static void corner_pixels(Framebuffer &fb);
static void corner_squares(Framebuffer &fb, int size = 10);
static void line_1(Framebuffer &fb);
static void hline_1(Framebuffer &fb);
static void draw_rect_1(Framebuffer &fb);
static void draw_rect_2(Framebuffer &fb);
static void fill_rect_1(Framebuffer &fb);
static void draw_circle_1(Framebuffer &fb);
static void draw_circle_2(Framebuffer &fb);
static void draw_circle_aa_1(Framebuffer &fb);
static void draw_circle_aa_2(Framebuffer &fb);
static void print_char_1(Framebuffer &fb);
static void print_string_1(Framebuffer &fb);
static void print_string_2(Framebuffer &fb);
static void print_string_3(Framebuffer &fb);
static void print_string_4(Framebuffer &fb);

static const int work_bytes = 128;
static uint8_t work[work_bytes];


static void reinit_screen(St7796 &lcd)
{
    // landscape, connector to the left, (0, 0) in lower left
    lcd.rotation(St7796::Rotation::left);

    // fill with black
    lcd.fill_rect(0, 0, lcd.width(), lcd.height(), Color::black);
}


int main()
{
    stdio_init_all();

    //while (!stdio_usb_connected()) tight_loop_contents();

    St7796 lcd(spi0, spi_miso_pin, spi_mosi_pin, spi_clk_pin, spi_cs_pin,
               spi_baud, lcd_cd_pin, lcd_rst_pin, lcd_led_pin, work,
               work_bytes);

    printf("spi running at %lu Hz\n", lcd.spi_freq());

    lcd.init();

    // Turning on the backlight here shows whatever happens to be in RAM
    // (previously displayed or random junk), so we turn it on after filling
    // the screen with something.

    reinit_screen(lcd); // set rotation, fill background

    // Now turn on backlight
    lcd.brightness(100);

    do {

        rotations(lcd);

        //corner_pixels(lcd);
        //corner_squares(lcd);
        //line_1(lcd);
        //hline_1(lcd);
        //draw_rect_1(lcd);
        //draw_rect_2(lcd);
        //fill_rect_1(lcd);
        //draw_circle_1(lcd);
        //draw_circle_2(lcd);
        //draw_circle_aa_1(lcd);
        //draw_circle_aa_2(lcd);
        //print_char_1(lcd);
        //print_string_1(lcd);
        //print_string_2(lcd);
        //print_string_3(lcd);
        //print_string_4(lcd);

    } while (true);

    return 0;
}


static void mark_origin(Framebuffer &fb, const char *label, const Color c)
{
    fb.line(0, 0, fb.width() / 2 - 1, 0, c);
    fb.line(0, 1, 0, fb.height() / 2 - 1, c);
    fb.print(1, 1, label, font, c, Color::black);
    char str[32];
    sprintf(str, "%dw x %dh", fb.width(), fb.height());
    fb.print(1, 1 + font.height(), str, font, c, Color::black);
}


[[maybe_unused]]
static void rotations(St7796 &lcd)
{
    const uint32_t delay_ms = 2000;
    Framebuffer &fb = lcd;

    fb.fill_rect(0, 0, fb.width(), fb.height(), Color::black);
    sleep_ms(delay_ms);

    lcd.rotation(St7796::Rotation::bottom);
    mark_origin(fb, "Rotation::bottom", Color::red);
    sleep_ms(delay_ms);

    lcd.rotation(St7796::Rotation::left);
    mark_origin(lcd, "Rotation::left", Color::green);
    sleep_ms(delay_ms);

    lcd.rotation(St7796::Rotation::top);
    mark_origin(lcd, "Rotation::top", Color::blue);
    sleep_ms(delay_ms);

    lcd.rotation(St7796::Rotation::right);
    mark_origin(lcd, "Rotation::right", Color::white);
    sleep_ms(delay_ms);
}


[[maybe_unused]]
static void corner_pixels(Framebuffer &fb)
{
    const Color c = Color::white;
    fb.pixel(0, 0, c);
    fb.pixel(0, fb.height() - 1, c);
    fb.pixel(fb.width() - 1, 0, c);
    fb.pixel(fb.width() - 1, fb.height() - 1, c);
}


[[maybe_unused]]
static void corner_squares(Framebuffer &fb, int size)
{
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            fb.pixel(i, j, Color::red);
            fb.pixel(i, fb.height() - 1 - j, Color::green);
            fb.pixel(fb.width() - 1 - i, j, Color::blue);
            fb.pixel(fb.width() - 1 - i, fb.height() - 1 - j, Color::white);
        }
    }
}


[[maybe_unused]]
static void line_1(Framebuffer &fb)
{
    const Color c = Color::white;
    int w1 = fb.width() - 1;
    int h1 = fb.height() - 1;
    fb.line(100, 100, 0, 0, c);
    fb.line(100, 100, 0, h1, c);
    fb.line(100, 100, w1, 0, c);
    fb.line(100, 100, w1, h1, c);
}


[[maybe_unused]]
static void hline_1(Framebuffer &fb)
{
    // should be able to see that each successive line drawn is one pixel shorter
    const Color c = Color::white;
    fb.line(0, 0, fb.width() - 1, 0, c);
    fb.line(0, 2, fb.width() - 2, 2, c);
    fb.line(0, 4, fb.width() - 3, 4, c);
    fb.line(0, 6, fb.width() - 4, 6, c);
    fb.line(0, 8, fb.width() - 5, 8, c);
}


[[maybe_unused]]
static void draw_rect_1(Framebuffer &fb)
{
    fb.draw_rect(0, 0, fb.width(), fb.height(), Color::white);
}


[[maybe_unused]]
static void draw_rect_2(Framebuffer &fb)
{
    uint16_t wid = fb.width();
    uint16_t hgt = fb.height();
    uint16_t hor = 0;
    uint16_t ver = 0;
    while (true) {
        fb.draw_rect(hor, ver, wid, hgt, Color::white);
        hor += 2;
        ver += 2;
        if (wid <= 4 || hgt <= 4)
            break;
        wid -= 4;
        hgt -= 4;
    }
}


[[maybe_unused]]
static void fill_rect_1(Framebuffer &fb)
{
    const Color colors[] = {
        Color::gray75, Color::red,     Color::green, Color::blue,
        Color::yellow, Color::magenta, Color::cyan,  Color::white,
    };
    const int color_cnt = sizeof(colors) / sizeof(colors[0]);

    const int ver_sz = fb.height() / color_cnt;
    xassert((ver_sz * color_cnt) == fb.height());

    const int hor_sz = fb.width() / color_cnt;
    xassert((hor_sz * color_cnt) == fb.width());

    int color = 0;

    for (int ver_blk = 0; ver_blk < color_cnt; ver_blk++) {
        for (int hor_blk = 0; hor_blk < color_cnt; hor_blk++) {
            int hor = hor_blk * hor_sz;
            int ver = ver_blk * ver_sz;
            fb.fill_rect(hor, ver, hor_sz, ver_sz, colors[color]);
            if (++color >= color_cnt)
                color = 0;
            sleep_ms(10);
        }
        if (++color >= color_cnt)
            color = 0;
    }
}


[[maybe_unused]]
static void draw_circle_1(Framebuffer &fb)
{
    fb.draw_circle(100, 100, 100, Color::white);
}


[[maybe_unused]]
static void draw_circle_2(Framebuffer &fb)
{
    int h = fb.width() / 2;
    int v = fb.height() / 2;
    int r = h < v ? h : v;
    r--;
    fb.draw_circle(h, v, r, Color::red, Framebuffer::Quadrant::LowerRight);
    fb.line(h + 1, v + 1, h + r, v + r, Color::red);
    fb.draw_circle(h, v, r, Color::green, Framebuffer::Quadrant::LowerLeft);
    fb.line(h - 1, v + 1, h - r, v + r, Color::green);
    fb.draw_circle(h, v, r, Color::blue, Framebuffer::Quadrant::UpperLeft);
    fb.line(h - 1, v - 1, h - r, v - r, Color::blue);
    fb.draw_circle(h, v, r, Color::white, Framebuffer::Quadrant::UpperRight);
    fb.line(h + 1, v - 1, h + r, v - r, Color::white);
}


[[maybe_unused]]
static void draw_circle_aa_1(Framebuffer &fb)
{
    const int h = fb.width() / 2;
    const int v = fb.height() / 2;
    const int r = (h < v ? h : v) - 20;
    fb.draw_circle(h, v, r - 10, Color::white);
    fb.draw_circle_aa(h, v, 8, Color::white, Color::black);
    fb.draw_circle_aa(h, v, r + 10, Color::white, Color::black);
}


[[maybe_unused]]
static void draw_circle_aa_2(Framebuffer &fb)
{
    const int h = fb.width() / 2;
    const int v = fb.height() / 2;
    const int r_max = h < v ? h : v;
    for (int r = 0; r < r_max; r += 4)
        fb.draw_circle_aa(h, v, r, Color::white, Color::black);
}


// Should result in a gray50 background, a 1-pixel black box near the middle,
// a 1-pixel gray50 box inside that one, then a black-on-white character
[[maybe_unused]]
static void print_char_1(Framebuffer &fb)
{
    // background
    fb.fill_rect(0, 0, fb.width(), fb.height(), Color::gray50);
    // somewhere near the middle, not exactly
    int h = fb.width() / 2;
    int v = fb.height() / 2;
    // character and its expected size
    char c = 'A';
    int wid = font.width(c);
    int hgt = font.height();
    // draw black rectangle just outside where the character is expected
    fb.draw_rect(h - 2, v - 2, wid + 4, hgt + 4, Color::black);
    // draw character, leaving one pixel of background around it in the box
    fb.print(h, v, c, font, Color::black, Color::white);
}


[[maybe_unused]]
static void print_string_1(Framebuffer &fb)
{
    fb.fill_rect(0, 0, fb.width(), fb.height(), Color::white);

    uint16_t hor = 20;
    uint16_t ver = 20;
    fb.print(hor, ver, "Hello,", font, Color::black, Color::white);
    ver += font.y_adv;
    fb.print(hor, ver, "world!", font, Color::black, Color::white);
}


[[maybe_unused]]
static void print_string_2(Framebuffer &fb)
{
    fb.fill_rect(0, 0, fb.width(), fb.height(), Color::gray50);

    int hor = fb.width() / 2;
    int ver = 20;
    fb.line(hor, 0, hor, fb.height() - 1, Color::red);

    fb.print(hor, ver, "Hello, world!", font, Color::black, Color::white, 0);
}


[[maybe_unused]]
static void print_string_3(Framebuffer &fb)
{
    fb.fill_rect(0, 0, fb.width(), fb.height(), Color::gray50);

    // thin border around edge lets us see when we plot and edge pixel
    fb.draw_rect(0, 0, fb.width(), fb.height(), Color::black);

    const char *s = "Hello";

    int w = font.width(s);

    Color fg = Color::black;
    Color bg = Color::white;

    // print with one pixel to spare on the right
    int ver = 20;
    int hor = fb.width() - w - 1;
    fb.print(hor, ver, s, font, fg, bg);

    // print right up against the right edge
    ver += font.y_adv + 1;
    hor = fb.width() - w;
    fb.print(hor, ver, s, font, fg, bg);

    // crop the final 'o'
    ver += font.y_adv + 1;
    hor = fb.width() - w + 1;
    fb.print(hor, ver, s, font, fg, bg);

    // print with one pixel to spare on the left
    ver = 20;
    hor = w + 1;
    fb.print(hor, ver, s, font, fg, bg, -1);

    // print right up against the left edge
    ver += font.y_adv + 1;
    hor = w;
    fb.print(hor, ver, s, font, fg, bg, -1);

    // crop the first character
    ver += font.y_adv + 1;
    hor = w - 1;
    fb.print(hor, ver, s, font, fg, bg, -1);

    // print with one pixel to spare below
    ver = fb.height() - font.y_adv - 1;
    hor = fb.width() / 4;
    fb.print(hor, ver, s, font, fg, bg, 0);

    // print right up against the bottom
    ver = fb.height() - font.y_adv;
    hor = 2 * fb.width() / 4;
    fb.print(hor, ver, s, font, fg, bg, 0);

    // doesn't print since it would extend off the bottom
    ver = fb.height() - font.y_adv + 1;
    hor = 3 * fb.width() / 4;
    fb.print(hor, ver, s, font, fg, bg, 0);
}


[[maybe_unused]]
static void print_string_4(Framebuffer &fb)
{
    const char *s1 = " >";
    const char *s2 = "< ";
    const int s1_wid = font.width(s1);
    const int s2_wid = font.width(s2);
    const uint32_t ms = 0;

    const Color fg = Color::red;
    const Color bg = Color::white;

    fb.fill_rect(0, 0, fb.width(), fb.height(), bg);

    int v = 0;
    const int v_inc = font.height() / 4;
    while (true) {

        // >>>
        for (int h = -s1_wid; h < fb.width(); h++) {
            fb.print(h, v, s1, font, fg, bg);
            sleep_ms(ms);
        }

        v += v_inc;
        if (v > (fb.height() - font.height()))
            v = 0;

        // <<<
        for (int h = fb.width(); h > -s2_wid; h--) {
            fb.print(h, v, s2, font, fg, bg);
            sleep_ms(ms);
        }

        v += v_inc;
        if (v > (fb.height() - font.height()))
            v = 0;
    }
}
