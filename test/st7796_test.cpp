
#include "st7796.h"

#include <cassert>
#include <cstdio>
// pico
#include "hardware/spi.h"
#include "pico/rand.h"
#include "pico/stdio.h"
#include "pico/stdio_usb.h"
#include "pico/stdlib.h"
// misc
#include "argv.h"
#include "str_ops.h"
#include "sys_led.h"
#include "util.h"
// framebuffer
#include "color.h"
#include "font.h"
#include "pixel_565.h"
#include "pixel_image.h"
#include "roboto.h"

// Pico:
//
// Signal Pin
//
// MISO   21  SPI0_RX (16)
// CS     22  SPI0_CSn (17)
//        23  GND
// SCK    24  SPI0_SCK (18)
// MOSI   25  SPI0_TX (19)
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
static const uint32_t spi_baud_request = 15'000'000;
static uint32_t spi_baud_actual = 0;
static uint32_t spi_rate_max = 0;

static const int lcd_cd_pin = 20;
static const int lcd_rst_pin = 21;
static const int lcd_led_pin = 22;

// Most glyphs of great_vibes_48 extend past the x_adv, so it does not render
// nicely but it does show that right-side cropping works instead of crashing
//static const Font& font = great_vibes_48;

static const Font &font = roboto_32;

static const int work_bytes = 128;
static uint8_t work[work_bytes];

// clang-format off
static void rotations(Framebuffer &fb);
static void corner_pixels(Framebuffer &fb);
static void corner_squares(Framebuffer &fb);
static void line_1(Framebuffer &fb);
static void hline_1(Framebuffer &fb);
static void colors_1(Framebuffer &fb);
static void colors_2(Framebuffer &fb);
static void colors_3(Framebuffer &fb);
static void draw_rect_1(Framebuffer &fb);
static void draw_rect_2(Framebuffer &fb);
static void fill_rect_1(Framebuffer &fb);
static void fill_rect_2(Framebuffer &fb);
static void fill_rect_3(Framebuffer &fb);
static void draw_circle_1(Framebuffer &fb);
static void draw_circle_2(Framebuffer &fb);
static void draw_circle_aa_1(Framebuffer &fb);
static void draw_circle_aa_2(Framebuffer &fb);
static void print_char_1(Framebuffer &fb);
static void print_string_1(Framebuffer &fb);
static void print_string_2(Framebuffer &fb);
static void print_string_3(Framebuffer &fb);
static void print_string_4(Framebuffer &fb);
namespace ImgChar { static void run(Framebuffer &fb); }
namespace ImgString { static void run(Framebuffer &fb); }
namespace ImgButton { static void run(Framebuffer &fb); }
namespace Label1 { static void run(Framebuffer &fb); }
namespace Font1 { static void run(Framebuffer &fb); };
namespace Screen { static void run(Framebuffer &fb); };
namespace ImgUpdate { static void run(Framebuffer &fb); }
namespace ImgDigits { static void run(Framebuffer &fb); }
// clang-format on

static struct {
    const char *name;
    void (*func)(Framebuffer &);
} tests[] = {
    {"rotations", rotations},
    {"corner_pixels", corner_pixels},
    {"corner_squares", corner_squares},
    {"line_1", line_1},
    {"hline_1", hline_1},
    {"colors_1", colors_1},
    {"colors_2", colors_2},
    {"colors_3", colors_3},
    {"draw_rect_1", draw_rect_1},
    {"draw_rect_2", draw_rect_2},
    {"fill_rect_1", fill_rect_1},
    {"fill_rect_2", fill_rect_2},
    {"fill_rect_3", fill_rect_3},
    {"draw_circle_1", draw_circle_1},
    {"draw_circle_2", draw_circle_2},
    {"draw_circle_aa_1", draw_circle_aa_1},
    {"draw_circle_aa_2", draw_circle_aa_2},
    {"print_char_1", print_char_1},
    {"print_string_1", print_string_1},
    {"print_string_2", print_string_2},
    {"print_string_3", print_string_3},
    {"print_string_4", print_string_4},
    {"ImgChar", ImgChar::run},
    {"ImgString", ImgString::run},
    {"ImgButton", ImgButton::run},
    {"Label1", Label1::run},
    {"Font1", Font1::run},
    {"Screen", Screen::run},
    {"ImgUpdate", ImgUpdate::run},
    {"ImgDigits", ImgDigits::run},
};
static const int num_tests = sizeof(tests) / sizeof(tests[0]);


static void help()
{
    printf("\n");
    printf("Usage: enter test number (0..%d)\n", num_tests - 1);
    for (int i = 0; i < num_tests; i++)
        printf("%2d: %s\n", i, tests[i].name);
    printf("\n");
}


static void reinit_screen(Framebuffer &lcd)
{
    // landscape, connector to the left
    lcd.set_rotation(Framebuffer::Rotation::landscape);

    // fill with black
    lcd.fill_rect(0, 0, lcd.width(), lcd.height(), Color::black());
}


int main()
{
    stdio_init_all();

    SysLed::init();
    SysLed::pattern(50, 950);

    while (!stdio_usb_connected()) {
        tight_loop_contents();
        SysLed::loop();
    }

    sleep_ms(10);

    SysLed::off();

    printf("\n");
    printf("st7796_test\n");
    printf("\n");

    Argv argv(1); // verbosity == 1 means echo

    St7796 lcd(spi0, spi_miso_pin, spi_mosi_pin, spi_clk_pin, spi_cs_pin,
               spi_baud_request, lcd_cd_pin, lcd_rst_pin, lcd_led_pin, 480, 320,
               work, work_bytes);

    spi_baud_actual = lcd.spi_freq();
    spi_rate_max = spi_baud_actual / 8;
    printf("spi: requested %lu Hz, got %lu Hz (max %lu bytes/sec)\n", //
           spi_baud_request, spi_baud_actual, spi_rate_max);

    lcd.init();

    // Turning on the backlight here shows whatever happens to be in RAM
    // (previously displayed or random junk), so we turn it on after filling
    // the screen with something.

    reinit_screen(lcd); // set rotation, fill background

    // Now turn on backlight
    lcd.brightness(100);

    help();
    printf("> ");

    while (true) {
        int c = stdio_getchar_timeout_us(0);
        if (0 <= c && c <= 255) {
            if (argv.add_char(char(c))) {
                int test_num = -1;
                if (argv.argc() != 1) {
                    printf("\n");
                    printf("One integer only (got %d)\n", argv.argc());
                    help();
                } else if (!str_to_int(argv[0], &test_num)) {
                    printf("\n");
                    printf("Invalid test number: \"%s\"\n", argv[0]);
                    help();
                } else if (test_num < 0 || test_num >= num_tests) {
                    printf("\n");
                    printf("Test number out of range: %d\n", test_num);
                    help();
                } else {
                    printf("\n");
                    printf("Running \"%s\"\n", tests[test_num].name);
                    printf("\n");
                    reinit_screen(lcd);
                    tests[test_num].func(lcd);
                    printf("> ");
                }
                argv.reset();
            }
        }
    }

    sleep_ms(100);
    return 0;
}


static void mark_origin(Framebuffer &fb, const char *label, const Color c)
{
    fb.line(0, 0, fb.width() / 2 - 1, 0, c);
    fb.line(0, 1, 0, fb.height() / 2 - 1, c);
    fb.print(1, 1, label, font, c, Color::black());
    char str[32];
    sprintf(str, "%dw x %dh", fb.width(), fb.height());
    fb.print(1, 1 + font.height(), str, font, c, Color::black());
}


static void rotations(Framebuffer &fb)
{
    const uint32_t delay_ms = 2000;

    fb.fill_rect(0, 0, fb.width(), fb.height(), Color::black());
    sleep_ms(100);

    fb.set_rotation(Framebuffer::Rotation::portrait);
    mark_origin(fb, "Rotation::portrait", Color::red());
    sleep_ms(delay_ms);

    fb.set_rotation(Framebuffer::Rotation::landscape);
    mark_origin(fb, "Rotation::landscape", Color::lime());
    sleep_ms(delay_ms);

    fb.set_rotation(Framebuffer::Rotation::portrait2);
    mark_origin(fb, "Rotation::portrait2", Color::light_blue());
    sleep_ms(delay_ms);

    fb.set_rotation(Framebuffer::Rotation::landscape2);
    mark_origin(fb, "Rotation::landscape2", Color::white());
    sleep_ms(delay_ms);
}


static void corner_pixels(Framebuffer &fb)
{
    const Color c = Color::white();
    fb.pixel(0, 0, c);
    fb.pixel(0, fb.height() - 1, c);
    fb.pixel(fb.width() - 1, 0, c);
    fb.pixel(fb.width() - 1, fb.height() - 1, c);
}


static void corner_squares(Framebuffer &fb)
{
    const int size = 10;
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            fb.pixel(i, j, Color::red());
            fb.pixel(i, fb.height() - 1 - j, Color::lime());
            fb.pixel(fb.width() - 1 - i, j, Color::blue());
            fb.pixel(fb.width() - 1 - i, fb.height() - 1 - j, Color::white());
        }
    }
}


static void line_1(Framebuffer &fb)
{
    const Color c = Color::white();
    int w1 = fb.width() - 1;
    int h1 = fb.height() - 1;
    fb.line(100, 100, 0, 0, c);
    fb.line(100, 100, 0, h1, c);
    fb.line(100, 100, w1, 0, c);
    fb.line(100, 100, w1, h1, c);
}


static void hline_1(Framebuffer &fb)
{
    // should be able to see that each successive line drawn is one pixel shorter
    const Color c = Color::white();
    fb.line(0, 0, fb.width() - 1, 0, c);
    fb.line(0, 2, fb.width() - 2, 2, c);
    fb.line(0, 4, fb.width() - 3, 4, c);
    fb.line(0, 6, fb.width() - 4, 6, c);
    fb.line(0, 8, fb.width() - 5, 8, c);
}


// This shows brightness gradations for primary and secondary colors.
//
// Visually, it does not look very "linear". Most of the change seems
// to happen in the bottom half, i.e. brighness 0..127 seems to vary
// less than brightness 128..255.
//
static void colors_1(Framebuffer &fb)
{

    // landscape, 64 brightness levels, 5 pixels per level
    const int levels = 64;
    const int hgt_band = fb.height() / levels; // 320/64=5, 480/64=7.5

    // red, yellow, green, cyan, blue, magenta, red
    const int wid_band = fb.width() / 7;

    // center it all by starting right of zero a few pixels
    const int hor_red = (fb.width() - wid_band * 7) / 2;
    const int hor_yel = hor_red + wid_band;
    const int hor_grn = hor_yel + wid_band;
    const int hor_cyn = hor_grn + wid_band;
    const int hor_blu = hor_cyn + wid_band;
    const int hor_mgt = hor_blu + wid_band;
    const int hor_rd2 = hor_mgt + wid_band;

    for (int level = 0; level < levels; level++) {
        int ver = level * hgt_band;
        int brt = level * 4;
        int brt_pct = brt * 100 / 255;
        // check:
        //Color c = Color::red(brt);
        //printf("level %d: r=%d g=%d b=%d\n", level, int(c.r()), int(c.g()), int(c.b()));
        fb.fill_rect(hor_red, ver, wid_band, hgt_band, Color::red(brt_pct));
        fb.fill_rect(hor_yel, ver, wid_band, hgt_band, Color::yellow(brt_pct));
        fb.fill_rect(hor_grn, ver, wid_band, hgt_band, Color::green(brt_pct));
        fb.fill_rect(hor_cyn, ver, wid_band, hgt_band, Color::cyan(brt_pct));
        fb.fill_rect(hor_blu, ver, wid_band, hgt_band, Color::blue(brt_pct));
        fb.fill_rect(hor_mgt, ver, wid_band, hgt_band, Color::magenta(brt_pct));
        fb.fill_rect(hor_rd2, ver, wid_band, hgt_band, Color::red(brt_pct));
    }
}


// HSB (Hue, Saturation, Brightness) color test.
//
// This demonstrates the Color::hsb() function by creating a color chart:
// - Horizontal axis: Hue sweep from 0° to 360° (full color wheel)
// - Vertical axis: Top half shows saturation levels (100% brightness)
//                  Bottom half shows brightness levels (100% saturation)
//
static void colors_3(Framebuffer &fb)
{
    // Landscape mode: 480 wide x 320 high
    assert(fb.width() == 480 && fb.height() == 320);

    // hue will go 0...359 across the width; center it
    int col_0 = (fb.width() - 360) / 2;

    // 101 rows for saturation, 101 rows for brightness
    // Top half: vary saturation (brightness = 100)
    // Bottom half: vary brightness (saturation = 100)

    const int row_0 = (fb.height() - 202) / 2;

    // Hue sweep: 0-360 degrees across the width
    for (int hue = 0; hue < 360; hue++) {

        int col = col_0 + hue;

        // Top half: saturation sweep from 0 to 100
        for (int sat = 0; sat <= 100; sat++) {
            int row = row_0 + sat;
            fb.pixel(col, row, Color::hsb(hue, sat, 100));
        }

        // Bottom half: brightness sweep from 100 to 0
        for (int brt = 100; brt >= 0; brt--) {
            // subtracting from 320 instead of 319 leaves a one-pixel (black) gap
            int row = 320 - row_0 - brt;
            fb.pixel(col, row, Color::hsb(hue, 100, brt));
        }
    }
}


// Similar to colors_1, but instead of distinct vertical bands for each color,
// we blend smoothly from one to the next horizontally.
//
// We still have the pure primary and secondary colors, but blend from one
// to the next. For example, red (255,0,0) blends to yellow (255,255,0) by
// increasing green as we move across. Then yellow blends to green (0,255,0)
// by decreasing red, and so on.
//
// There are six blends:
//   red (255,0,0)
//   red to yellow (increasing green)
//   yellow (255,255,0)
//   yellow to green (decreasing red)
//   green (0,255,0)
//   green to cyan (increasing blue)
//   cyan (0,255,255)
//   cyan to blue (decreasing green)
//   blue (0,0,255)
//   blue to magenta (increasing red)
//   magenta (255,0,255)
//   magenta to red (decreasing blue)
//   red (255,0,0)
static void colors_2(Framebuffer &fb)
{
    const int ver_levels = 64;
    const int hgt_box = fb.height() / ver_levels;
    // horizontal levels: 6 blends * 64 levels each = 384, so 1 pixel per level
    //   red                          yellow                          green
    // 252,0,0 252,4,0 ... 252,248,0 252,252,0 248,252,0 ... 4,252,0 0,252,0
    const int hor_levels = 64;
    const int wid_box = 1;
    // start offset to center it all
    const int hor_0 = (fb.width() - 6 * hor_levels) / 2;

    for (int row = 0; row < ver_levels; row++) {
        int brt = row * 4; // 0..252
        int ver = row * hgt_box;
        int hor = hor_0;
        // red to yellow (increasing green)
        for (int lvl = 0; lvl <= 252; lvl += 4, hor++)
            fb.fill_rect(hor, ver, wid_box, hgt_box, Color(252, lvl, brt));
        // yellow to green (decreasing red)
        for (int lvl = 252; lvl >= 0; lvl -= 4, hor++)
            fb.fill_rect(hor, ver, wid_box, hgt_box, Color(lvl, 252, brt));
        // green to cyan (increasing blue)
        for (int lvl = 0; lvl <= 252; lvl += 4, hor++)
            fb.fill_rect(hor, ver, wid_box, hgt_box, Color(brt, 252, lvl));
        // cyan to blue (decreasing green)
        for (int lvl = 252; lvl >= 0; lvl -= 4, hor++)
            fb.fill_rect(hor, ver, wid_box, hgt_box, Color(brt, lvl, 252));
        // blue to magenta (increasing red)
        for (int lvl = 0; lvl <= 252; lvl += 4, hor++)
            fb.fill_rect(hor, ver, wid_box, hgt_box, Color(lvl, brt, 252));
        // magenta to red (decreasing blue)
        for (int lvl = 252; lvl >= 0; lvl -= 4, hor++)
            fb.fill_rect(hor, ver, wid_box, hgt_box, Color(252, brt, lvl));
    }
}


static void draw_rect_1(Framebuffer &fb)
{
    fb.draw_rect(0, 0, fb.width(), fb.height(), Color::white());
}


static void draw_rect_2(Framebuffer &fb)
{
    uint16_t wid = fb.width();
    uint16_t hgt = fb.height();
    uint16_t hor = 0;
    uint16_t ver = 0;
    while (true) {
        fb.draw_rect(hor, ver, wid, hgt, Color::white());
        hor += 2;
        ver += 2;
        if (wid <= 4 || hgt <= 4)
            break;
        wid -= 4;
        hgt -= 4;
    }
}


static void fill_rect_1(Framebuffer &fb)
{
    const Color colors[] = {
        Color::red(),    Color::green(),   Color::blue(),   //
        Color::yellow(), Color::magenta(), Color::cyan(),   //
        Color::gray(75), Color::gray(50),  Color::gray(25), //
        Color::white(),
    };
    const int color_cnt = sizeof(colors) / sizeof(colors[0]);

    for (int c_num = 0; c_num < color_cnt; c_num++) {
        fb.fill_rect(0, 0, fb.width(), fb.height(), colors[c_num]);
        sleep_ms(100);
    }
}


static void fill_rect_2(Framebuffer &fb)
{
    // Using the "argument" version of the colors means we're getting 0x00
    // or 0xff for each color component. E.g. green(0) is (0x00,0xff,0x00)
    // (from color.h) but green() is (0x00,0x80,0x00) (from color_html.h).
    const Color colors[] = {
        Color::gray(75),  Color::red(0),     Color::green(0), Color::blue(0),
        Color::yellow(0), Color::magenta(0), Color::cyan(0),  Color::white(),
    };
    const int color_cnt = sizeof(colors) / sizeof(colors[0]);

    printf("gray(75) red(0) green(0) blue(0) ");
    printf("yellow(0) magenta(0) cyan(0) white()\n");

    const int ver_sz = fb.height() / color_cnt;
    assert((ver_sz * color_cnt) == fb.height());

    const int hor_sz = fb.width() / color_cnt;
    assert((hor_sz * color_cnt) == fb.width());

    int color = 0;

    for (int ver_blk = 0; ver_blk < color_cnt; ver_blk++) {
        for (int hor_blk = 0; hor_blk < color_cnt; hor_blk++) {
            int hor = hor_blk * hor_sz;
            int ver = ver_blk * ver_sz;
            fb.fill_rect(hor, ver, hor_sz, ver_sz, colors[color]);
            if (++color >= color_cnt)
                color = 0;
        }
        if (++color >= color_cnt)
            color = 0;
    }

    printf("\n");
}


static void fill_rect_3(Framebuffer &fb)
{
    // Recall that the display's color resolution is 5 bits red, 6 bits green,
    // 5 bits blue. In landscape mode we have 320 pixels vertically, which can
    // divide into bands 5 pixels high for 64 levels (6 bits per color) of
    // gray. Blue will change every band, while red and green will change
    // every other band.

    // In the left third, we render bands using only 5 bits of green (5/5/5).
    //      -> this one is the "bandiest" gradient
    // In the middle third, we render bands using 0-100% gray.
    //      -> this one is almost as good as the right third
    // In the right third, we render bands using all 6 bits of green (5/6/5).
    //      -> this one is the smoothest gradient

    // landscape divides nicely
    int band_hgt = fb.height() / 64; // 320 / 64 = 5, 480 / 64 = 7.5
    int band_wid = fb.width() / 3;   // 480 / 3 = 160, 320 / 3 = 106.66

    for (int i = 0; i < 64; i++) {
        printf("band %d:", i);
        // 5 bits each color
        int j = (i * 4) & 0xf8;
        printf(" %d", j);
        fb.fill_rect(0 * band_wid, i * band_hgt, band_wid, band_hgt,
                     Color(j, j, j));
        // percent gray
        int pct = i * 100 / 63;
        printf(" %d", 0xff - (pct * 255 / 100));
        fb.fill_rect(1 * band_wid, i * band_hgt, band_wid, band_hgt,
                     Color::gray(pct));
        // max bits each color
        j = i * 4;
        printf(" %d", j);
        fb.fill_rect(2 * band_wid, i * band_hgt, band_wid, band_hgt,
                     Color(j, j, j));
        printf("\n");
    }
}


static void draw_circle_1(Framebuffer &fb)
{
    fb.draw_circle(100, 100, 100, Color::white());
}


static void draw_circle_2(Framebuffer &fb)
{
    int h = fb.width() / 2;
    int v = fb.height() / 2;
    int r = h < v ? h : v;
    r--;
    fb.draw_circle(h, v, r, Color::red(), Framebuffer::Quadrant::LowerRight);
    fb.line(h + 1, v + 1, h + r, v + r, Color::red());
    fb.draw_circle(h, v, r, Color::lime(), Framebuffer::Quadrant::LowerLeft);
    fb.line(h - 1, v + 1, h - r, v + r, Color::lime());
    fb.draw_circle(h, v, r, Color::blue(), Framebuffer::Quadrant::UpperLeft);
    fb.line(h - 1, v - 1, h - r, v - r, Color::blue());
    fb.draw_circle(h, v, r, Color::white(), Framebuffer::Quadrant::UpperRight);
    fb.line(h + 1, v - 1, h + r, v - r, Color::white());
}


static void draw_circle_aa_1(Framebuffer &fb)
{
    const int h = fb.width() / 2;
    const int v = fb.height() / 2;
    const int r = (h < v ? h : v) - 20;
    fb.draw_circle(h, v, r - 10, Color::white());
    fb.draw_circle_aa(h, v, 8, Color::white(), Color::black());
    fb.draw_circle_aa(h, v, r + 10, Color::white(), Color::black());
}


static void draw_circle_aa_2(Framebuffer &fb)
{
    const int h = fb.width() / 2;
    const int v = fb.height() / 2;
    const int r_max = h < v ? h : v;
    for (int r = 0; r < r_max; r += 4)
        fb.draw_circle_aa(h, v, r, Color::white(), Color::black());
}


// Should result in a gray50 background, a 1-pixel black box near the middle,
// a 1-pixel gray50 box inside that one, then a black-on-white character
static void print_char_1(Framebuffer &fb)
{
    // background
    fb.fill_rect(0, 0, fb.width(), fb.height(), Color::gray(50));
    // somewhere near the middle, not exactly
    int h = fb.width() / 2;
    int v = fb.height() / 2;
    // character and its expected size
    char c = 'A';
    int wid = font.width(c);
    int hgt = font.height();
    // draw black rectangle just outside where the character is expected
    fb.draw_rect(h - 2, v - 2, wid + 4, hgt + 4, Color::black());
    // draw character, leaving one pixel of background around it in the box
    fb.print(h, v, c, font, Color::black(), Color::white());
}


static void print_string_1(Framebuffer &fb)
{
    fb.fill_rect(0, 0, fb.width(), fb.height(), Color::white());

    uint16_t hor = 20;
    uint16_t ver = 20;
    fb.print(hor, ver, "Hello,", font, Color::black(), Color::white());
    ver += font.y_adv;
    fb.print(hor, ver, "world!", font, Color::black(), Color::white());
}


static void print_string_2(Framebuffer &fb)
{
    fb.fill_rect(0, 0, fb.width(), fb.height(), Color::gray(50));

    int hor = fb.width() / 2;
    int ver = 20;
    fb.line(hor, 0, hor, fb.height() - 1, Color::red());

    fb.print(hor, ver, "Hello, world!", font, Color::black(), Color::white(),
             Framebuffer::HAlign::Center);
}


static void print_string_3(Framebuffer &fb)
{
    fb.fill_rect(0, 0, fb.width(), fb.height(), Color::gray(50));

    // thin border around edge lets us see when we plot and edge pixel
    fb.draw_rect(0, 0, fb.width(), fb.height(), Color::black());

    const char *s = "Hello";

    int w = font.width(s);

    Color fg = Color::black();
    Color bg = Color::white();

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
    fb.print(hor, ver, s, font, fg, bg, Framebuffer::HAlign::Right);

    // print right up against the left edge
    ver += font.y_adv + 1;
    hor = w;
    fb.print(hor, ver, s, font, fg, bg, Framebuffer::HAlign::Right);

    // crop the first character
    ver += font.y_adv + 1;
    hor = w - 1;
    fb.print(hor, ver, s, font, fg, bg, Framebuffer::HAlign::Right);

    // print with one pixel to spare below
    ver = fb.height() - font.y_adv - 1;
    hor = fb.width() / 4;
    fb.print(hor, ver, s, font, fg, bg, Framebuffer::HAlign::Center);

    // print right up against the bottom
    ver = fb.height() - font.y_adv;
    hor = 2 * fb.width() / 4;
    fb.print(hor, ver, s, font, fg, bg, Framebuffer::HAlign::Center);

    // doesn't print since it would extend off the bottom
    ver = fb.height() - font.y_adv + 1;
    hor = 3 * fb.width() / 4;
    fb.print(hor, ver, s, font, fg, bg, Framebuffer::HAlign::Center);
}


static void print_string_4(Framebuffer &fb)
{
    const char *s1 = " >";
    const char *s2 = "< ";
    const int s1_wid = font.width(s1);
    const int s2_wid = font.width(s2);
    const uint32_t ms = 0;

    const Color fg = Color::red();
    const Color bg = Color::white();

    fb.fill_rect(0, 0, fb.width(), fb.height(), bg);

    printf("(press any key to stop)\n");

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

        int c = stdio_getchar_timeout_us(0);
        if (0 <= c && c <= 255)
            break;
    }

    printf("\n");
}


namespace ImgChar {

static constexpr Font font = roboto_32;
static constexpr char ch[] = "Q";
static constexpr int wid = font.width(ch);
static constexpr int hgt = font.y_adv;
static constexpr Color fg = Color::red();
static constexpr Color bg = Color::gray(80);

static constexpr PixelImage<Pixel565, wid, hgt> img =
    label_img<Pixel565, wid, hgt>(ch, font, fg, 0, fg, bg);

static void run(Framebuffer &fb)
{
    const char *loc = mem_name(&(img.pixels[0]));

    printf("ImgChar: writing %dw x %dh image from %s at 0x%p (%d bytes)\n",
           img.width, img.height, loc, &img, sizeof(img.pixels));

    int hor = 10;
    int ver = 10;

    fb.write(hor, ver, &img);

    ver += 40;

    printf("ImgChar: printing '%s'\n", ch);

    fb.print(hor, ver, ch, font, fg, bg);
}

} // namespace ImgChar


namespace ImgString {

static constexpr Font font = roboto_32;
static constexpr char msg[] = "Hello, world!";
static constexpr int wid = font.width(msg);
static constexpr int hgt = font.y_adv;
static constexpr Color fg = Color::lime();
static constexpr Color bg = Color::gray(80);

static constexpr PixelImage<Pixel565, wid, hgt> img =
    label_img<Pixel565, wid, hgt>(msg, font, fg, 0, fg, bg);

static void run(Framebuffer &fb)
{
    const char *loc = mem_name(&img);

    printf("ImgString: writing %dw x %dh image from %s at 0x%p (%d bytes)\n",
           img.width, img.height, loc, &img, sizeof(img.pixels));

    int hor = 10;
    int ver = 10;

    fb.write(hor, ver, &img);

    ver += 40;

    printf("ImgString: printing \"%s\"\n", msg);

    fb.print(hor, ver, msg, font, fg, bg);
}

} // namespace ImgString


namespace ImgButton {

// Print array of buttons, each with a box around it:
//  0  1  2  3  4  5  6  7  8  9
// 10 11 12 13 14 15 16 17 18 19
// 20 21 22 23 24 25 26 27 28 29

static constexpr int per_row = 10;
static constexpr int btn_sz = 480 / per_row;

static constexpr Font font = roboto_32;
static constexpr Color fg = Color::black();
static constexpr Color bg = Color::white();
static constexpr int btn_hgt = font.y_adv;

// Using the preprocessor to create the buttons reduces ~150 LOC like this:
//
// static constexpr PixelImage<Pixel565, btn_sz, btn_sz> btn_0 =
//     label_img<Pixel565, btn_sz, btn_sz>("0", font, fg, bg, 1, fg);
// static constexpr PixelImage<Pixel565, btn_sz, btn_sz> btn_0_inv =
//     label_img<Pixel565, btn_sz, btn_sz>("0", font, bg, fg, 1, fg);
//
// static constexpr PixelImage<Pixel565, btn_sz, btn_sz> btn_1 =
//     label_img<Pixel565, btn_sz, btn_sz>("1", font, fg, bg, 1, fg);
// static constexpr PixelImage<Pixel565, btn_sz, btn_sz> btn_1_inv =
//     label_img<Pixel565, btn_sz, btn_sz>("1", font, bg, fg, 1, fg);
//
// ... etc.
//
// to ~30 LOC like this:

#define IMG_MAKE(NUM)                                                       \
    static constexpr PixelImage<Pixel565, btn_sz, btn_sz> btn_##NUM##_nor = \
        label_img<Pixel565, btn_sz, btn_sz>(#NUM, font, fg, bg, 1, fg);     \
    static constexpr PixelImage<Pixel565, btn_sz, btn_sz> btn_##NUM##_inv = \
        label_img<Pixel565, btn_sz, btn_sz>(#NUM, font, bg, fg, 1, fg);

// clang-format off
#define IMG_LIST IMG_MAKE(0)  IMG_MAKE(1)  IMG_MAKE(2)  IMG_MAKE(3)  IMG_MAKE(4)  \
                 IMG_MAKE(5)  IMG_MAKE(6)  IMG_MAKE(7)  IMG_MAKE(8)  IMG_MAKE(9)  \
                 IMG_MAKE(10) IMG_MAKE(11) IMG_MAKE(12) IMG_MAKE(13) IMG_MAKE(14) \
                 IMG_MAKE(15) IMG_MAKE(16) IMG_MAKE(17) IMG_MAKE(18) IMG_MAKE(19) \
                 IMG_MAKE(20) IMG_MAKE(21) IMG_MAKE(22) IMG_MAKE(23) IMG_MAKE(24) \
                 IMG_MAKE(25) IMG_MAKE(26) IMG_MAKE(27) IMG_MAKE(28) IMG_MAKE(29)
// clang-format on

// create the images themselves, two per button
IMG_LIST

#undef IMG_MAKE

// create the array of button pointers
// even indices are normal, odd indices are inverted
#define IMG_MAKE(NUM) \
    (PixelImageInfo *)&btn_##NUM##_nor, (PixelImageInfo *)&btn_##NUM##_inv,

static const PixelImageInfo *btn_img[60] = {IMG_LIST};

#undef IMG_MAKE
#undef IMG_LIST

static void run(Framebuffer &fb)
{
    // clear screen
    fb.fill_rect(0, 0, fb.width(), fb.height(), bg);

    int hor, ver; // top-left corner of each button

    // draw buttons
    ver = 0;
    for (int r = 0; r < 3; r++) {
        hor = 0;
        for (int c = 0; c < per_row; c++) {
            int btn_num = r * per_row + c;
            assert(0 <= btn_num && btn_num < 30);
            assert(is_xip(btn_img[btn_num * 2]->pixels));
            fb.write(hor, ver, btn_img[btn_num * 2]);
            hor += btn_sz;
        }
        ver += btn_sz;
    }
    fb.line(0, btn_sz * 3, fb.width() - 1, btn_sz * 3, fg);

    printf("(press any key to stop)\n");

    // flip random button until a key is pressed
    for (int i = 0;; i++) {
        int btn_num = get_rand_32() % 30; // 0..29
        assert(0 <= btn_num && btn_num < 30);
        hor = btn_sz * (btn_num % 10);
        ver = btn_sz * (btn_num / 10);
        if ((i & 1) == 0)
            fb.write(hor, ver, btn_img[btn_num * 2 + 1]); // inverted
        else
            fb.write(hor, ver, btn_img[btn_num * 2]); // normal
        sleep_ms(100);

        int c = stdio_getchar_timeout_us(0);
        if (0 <= c && c <= 255)
            break;
    }

    printf("\n");
}

} // namespace ImgButton


namespace Label1 {

static constexpr Color bg = Color::white();
static constexpr Font font = roboto_30;

// minimal size, no border
static constexpr char l0_txt[] = "Label0";
static constexpr int l0_thk = 0;
static constexpr int l0_wid = font.width(l0_txt);
static constexpr int l0_hgt = font.y_adv;
static constexpr Color l0_box = Color::black();
static constexpr Color l0_fg = Color::red();
static constexpr Color l0_bg = Color::gray(75);
static constexpr PixelImage<Pixel565, l0_wid, l0_hgt> l0 =
    label_img<Pixel565, l0_wid, l0_hgt>(l0_txt, font, l0_fg, //
                                        l0_thk, l0_box, l0_bg);

// minimal label but with border
static constexpr char l1_txt[] = "Label1";
static constexpr int l1_thk = 1;
static constexpr int l1_wid = font.width(l1_txt) + l1_thk * 2;
static constexpr int l1_hgt = font.y_adv + l1_thk * 2;
static constexpr Color l1_box = Color::black();
static constexpr Color l1_fg = Color::red();
static constexpr Color l1_bg = Color::gray(75);
static constexpr PixelImage<Pixel565, l1_wid, l1_hgt> l1 =
    label_img<Pixel565, l1_wid, l1_hgt>(l1_txt, font, l1_fg, //
                                        l1_thk, l1_box, l1_bg);

// roomier
static constexpr char l2_txt[] = "Label2";
static constexpr int l2_thk = 2;
static constexpr int l2_wid = font.width(l2_txt) + l2_thk * 2 + 10;
static constexpr int l2_hgt = font.y_adv + l2_thk * 2 + 4;
static constexpr Color l2_box = Color::black();
static constexpr Color l2_fg = Color::red();
static constexpr Color l2_bg = Color::gray(75);
static constexpr PixelImage<Pixel565, l2_wid, l2_hgt> l2 =
    label_img<Pixel565, l2_wid, l2_hgt>(l2_txt, font, l2_fg, //
                                        l2_thk, l2_box, l2_bg);
// 3d effect
static constexpr int l3_wid = 100;
static constexpr int l3_hgt = 40;

// up
static constexpr char l3u_txt[] = "Push";
static constexpr int l3u_thk = 2;
static constexpr Color l3u_box = Color::black();
static constexpr Color l3u_fg = Color::black();
static constexpr Color l3u_bg = Color::gray(75);
static constexpr PixelImage<Pixel565, l3_wid, l3_hgt> l3u =
    label_img<Pixel565, l3_wid, l3_hgt>(l3u_txt, font, l3u_fg, //
                                        l3u_thk, l3u_box, l3u_bg);

// down
static constexpr char l3d_txt[] = "Push";
static constexpr int l3d_thk = 4;
static constexpr Color l3d_box = Color::black();
static constexpr Color l3d_fg = Color::white();
static constexpr Color l3d_bg = Color::gray(50);
static constexpr PixelImage<Pixel565, l3_wid, l3_hgt> l3d =
    label_img<Pixel565, l3_wid, l3_hgt>(l3d_txt, font, l3d_fg, //
                                        l3d_thk, l3d_box, l3d_bg);

static void run(Framebuffer &fb)
{
    // clear screen
    fb.fill_rect(0, 0, fb.width(), fb.height(), bg);

    int hor;
    int ver = 1;

    // draw labels
    hor = 1;
    fb.write(hor, ver, &l0);
    printf("Label0: width=%d height=%d\n", l0.width, l0.height);

    hor = fb.width() / 2 - l1.width / 2;
    fb.write(hor, ver, &l1);
    printf("Label1: width=%d height=%d\n", l1.width, l1.height);

    hor = fb.width() - l2.width - 1;
    fb.write(hor, ver, &l2);
    printf("Label2: width=%d height=%d\n", l2.width, l2.height);

    hor = fb.width() / 2 - l3_wid / 2;
    ver = fb.height() / 2 - l3_hgt / 2;

    fb.write(hor, ver, &l3u);
    for (int i = 0; i < 5; i++) {
        sleep_ms(1000);
        fb.write(hor, ver, &l3d);
        sleep_ms(500);
        fb.write(hor, ver, &l3u);
    }
}

} // namespace Label1


namespace Font1 {

// Create labels for several font sizes and display them.
//
// Change the font used in the first LBL_MAKE() macro.
// Change the sizes used in the LBL_LIST macro.

static constexpr Color fg = Color::black();
static constexpr Color bg = Color::white();

#define LBL_MAKE(Z)                                                   \
    static constexpr char msg_##Z[] = " Roboto " #Z " ";              \
    static constexpr int wid_##Z = roboto_##Z.width(msg_##Z);         \
    static constexpr int hgt_##Z = roboto_##Z.y_adv;                  \
    static constexpr PixelImage<Pixel565, wid_##Z, hgt_##Z> img_##Z = \
        label_img<Pixel565, wid_##Z, hgt_##Z>(msg_##Z, roboto_##Z, fg, bg);

// clang-format off
#define LBL_LIST \
    LBL_MAKE(16) LBL_MAKE(18) LBL_MAKE(20) LBL_MAKE(22) LBL_MAKE(24) \
    LBL_MAKE(26) LBL_MAKE(28) LBL_MAKE(30) LBL_MAKE(32) LBL_MAKE(34) \
    LBL_MAKE(36) LBL_MAKE(38) LBL_MAKE(40) LBL_MAKE(44) LBL_MAKE(48)
// clang-format on

// create the labels
LBL_LIST

#undef LBL_MAKE

#define LBL_MAKE(Z) (PixelImageInfo *)&img_##Z,

static const PixelImageInfo *lbl_img[] = {LBL_LIST};
static const int lbl_max = sizeof(lbl_img) / sizeof(lbl_img[0]);

#undef LBL_MAKE
#undef LBL_LIST

static void run(Framebuffer &fb)
{
    const int marg = 10;
    int hor = marg;
    int ver = marg;
    const int sep = 5; // pixels between labels

    Framebuffer::HAlign align = Framebuffer::HAlign::Left;
    for (int i = 0; i < lbl_max; i++) {
        const PixelImageInfo *img = lbl_img[i];
        assert(is_xip(img));
        if (ver + img->hgt > fb.height()) {
            if (align != Framebuffer::HAlign::Left)
                break; // no more room
            hor = fb.width() - marg;
            ver = marg;
            align = Framebuffer::HAlign::Right;
        }
        fb.write(hor, ver, img, align);
        ver = ver + img->hgt + sep;
    }
}

} // namespace Font1


namespace Screen {

static constexpr Color bg = Color::white();
static constexpr Color fg = Color::black();
static constexpr int wid = 480;


namespace Nav {

static constexpr Font font = roboto_28;
static constexpr int hgt = font.y_adv + 2;
static constexpr int wid = Screen::wid / 5;

static constexpr char home_txt[] = "HOME";
static constexpr PixelImage<Pixel565, wid, hgt> home_active =
    label_img<Pixel565, wid, hgt>(home_txt, font, fg, 0, fg, bg);
static constexpr PixelImage<Pixel565, wid, hgt> home_inactive =
    label_img<Pixel565, wid, hgt>(home_txt, font, fg, 1, fg, bg);

static constexpr char loco_txt[] = "LOCO";
static constexpr PixelImage<Pixel565, wid, hgt> loco_active =
    label_img<Pixel565, wid, hgt>(loco_txt, font, fg, 0, fg, bg);
static constexpr PixelImage<Pixel565, wid, hgt> loco_inactive =
    label_img<Pixel565, wid, hgt>(loco_txt, font, fg, 1, fg, bg);

static constexpr char func_txt[] = "FUNC";
static constexpr PixelImage<Pixel565, wid, hgt> func_active =
    label_img<Pixel565, wid, hgt>(func_txt, font, fg, 0, fg, bg);
static constexpr PixelImage<Pixel565, wid, hgt> func_inactive =
    label_img<Pixel565, wid, hgt>(func_txt, font, fg, 1, fg, bg);

static constexpr char prog_txt[] = "PROG";
static constexpr PixelImage<Pixel565, wid, hgt> prog_active =
    label_img<Pixel565, wid, hgt>(prog_txt, font, fg, 0, fg, bg);
static constexpr PixelImage<Pixel565, wid, hgt> prog_inactive =
    label_img<Pixel565, wid, hgt>(prog_txt, font, fg, 1, fg, bg);

static constexpr char more_txt[] = "MORE";
static constexpr PixelImage<Pixel565, wid, hgt> more_active =
    label_img<Pixel565, wid, hgt>(more_txt, font, fg, 0, fg, bg);
static constexpr PixelImage<Pixel565, wid, hgt> more_inactive =
    label_img<Pixel565, wid, hgt>(more_txt, font, fg, 1, fg, bg);

static void draw(Framebuffer &fb, int active)
{
    if (active == 0)
        fb.write(0 * wid, 0, &home_active);
    else
        fb.write(0 * wid, 0, &home_inactive);

    if (active == 1)
        fb.write(1 * wid, 0, &loco_active);
    else
        fb.write(1 * wid, 0, &loco_inactive);

    if (active == 2)
        fb.write(2 * wid, 0, &func_active);
    else
        fb.write(2 * wid, 0, &func_inactive);

    if (active == 3)
        fb.write(3 * wid, 0, &prog_active);
    else
        fb.write(3 * wid, 0, &prog_inactive);

    if (active == 4)
        fb.write(4 * wid, 0, &more_active);
    else
        fb.write(4 * wid, 0, &more_inactive);
}

} // namespace Nav


namespace Id {

static constexpr Font font = roboto_48;
static constexpr int ver = 50;
static constexpr int hgt = font.y_adv;

static void draw(Framebuffer &fb, int num)
{
    char num_str[16];
    snprintf(num_str, sizeof(num_str), "%d", num);
    fb.print(fb.width() / 2, ver, num_str, font, fg, bg,
             Framebuffer::HAlign::Center);
}

} // namespace Id


namespace Toots {

static constexpr Font font = roboto_34;
static constexpr int hgtx = 5; // extra height above and below text
static constexpr int hgt = font.y_adv + 2 * hgtx;
static constexpr int marg = 1; // distance from edges of screen
static constexpr int wid = Screen::wid / 4;

static constexpr char lights_str[] = "Lights";
static constexpr PixelImage<Pixel565, wid, hgt> lights_img =
    label_img<Pixel565, wid, hgt>(lights_str, font, fg, 1, fg, bg);

static constexpr char engine_str[] = "Engine";
static constexpr PixelImage<Pixel565, wid, hgt> engine_img =
    label_img<Pixel565, wid, hgt>(engine_str, font, fg, 1, fg, bg);

static constexpr char horn_str[] = "Horn";
static constexpr PixelImage<Pixel565, wid, hgt> horn_img =
    label_img<Pixel565, wid, hgt>(horn_str, font, fg, 1, fg, bg);

static constexpr char bell_str[] = "Bell";
static constexpr PixelImage<Pixel565, wid, hgt> bell_img =
    label_img<Pixel565, wid, hgt>(bell_str, font, fg, 1, fg, bg);

static constexpr char rev_str[] = "REV";
static constexpr PixelImage<Pixel565, wid, hgt> rev_img =
    label_img<Pixel565, wid, hgt>(rev_str, font, fg, 1, fg, bg);

static constexpr char stop_str[] = "STOP";
static constexpr PixelImage<Pixel565, wid, hgt> stop_img =
    label_img<Pixel565, wid, hgt>(stop_str, font, fg, 1, fg, bg);

static constexpr char fwd_str[] = "FWD";
static constexpr PixelImage<Pixel565, wid, hgt> fwd_img =
    label_img<Pixel565, wid, hgt>(fwd_str, font, fg, 1, fg, bg);

static void draw(Framebuffer &fb)
{
    int ver = 50;
    int hor = marg;

    fb.write(hor, ver, &lights_img);
    ver += hgt + marg;
    fb.write(hor, ver, &engine_img);
    ver = 50;
    hor = fb.width() - marg - wid;
    fb.write(hor, ver, &horn_img);
    ver += hgt + marg;
    fb.write(hor, ver, &bell_img);
    ver = fb.height() - marg - hgt;
    hor = marg;
    fb.write(hor, ver, &rev_img);
    hor = fb.width() / 2 - wid / 2;
    fb.write(hor, ver, &stop_img);
    hor = fb.width() - marg - wid;
    fb.write(hor, ver, &fwd_img);
}

} // namespace Toots


namespace Slider {

static constexpr Font font = roboto_34;
static constexpr int hgtx = 5; // extra height above and below text
static constexpr int hgt = font.y_adv + 2 * hgtx;
static constexpr int marg = 1; // distance from edges of screen
static constexpr int wid = Screen::wid - 2 * marg; // overall, |-|slider|+|
static constexpr int wid_1 = hgt;                  // width of -/+ boxes
static constexpr int wid_2 = wid - 2 * wid_1 + 2;  // width of slider area

static constexpr char arrows_str[] = "<<<<<<<< Speed >>>>>>>>";
static constexpr PixelImage<Pixel565, wid_2, hgt> arrows_img =
    label_img<Pixel565, wid_2, hgt>(arrows_str, font, fg, 1, fg, bg);

static constexpr char minus_str[] = "-";
static constexpr PixelImage<Pixel565, wid_1, hgt> minus_img =
    label_img<Pixel565, wid_1, hgt>(minus_str, font, fg, 1, fg, bg);

static constexpr char plus_str[] = "+";
static constexpr PixelImage<Pixel565, wid_1, hgt> plus_img =
    label_img<Pixel565, wid_1, hgt>(plus_str, font, fg, 1, fg, bg);

static void draw(Framebuffer &fb)
{
    int ver = 180;
    int hor = marg;
    fb.write(hor, ver, &minus_img);
    hor += wid_1 - 1;
    fb.write(hor, ver, &arrows_img);
    hor += wid_2 - 1;
    fb.write(hor, ver, &plus_img);
}

} // namespace Slider

static void run(Framebuffer &fb)
{
    fb.fill_rect(0, 0, fb.width(), fb.height(), bg);

    Nav::draw(fb, 0);
    Toots::draw(fb);
    Id::draw(fb, 7956);
    Slider::draw(fb);

    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            Nav::draw(fb, j);
            sleep_ms(1000);
        }
    }
    Nav::draw(fb, 0);
}

} // namespace Screen


namespace ImgUpdate {

// This shows:
//
// Filling the background takes ~2.3 msec for 100w x 50h image
// --> Fill at ~2.1 Mpix/sec
//
// Rendering a zero in 32pt roboto (13w x 19h) takes ~460 usec
// --> Render at ~0.54 Mpix/sec
//
// Rendering a four-digit number in a 100w x 50h image takes ~4.2 msec
// --> Rendering to ram, and writing over spi, take roughly the same time
//
// If we didn't fill the entire background, but just the area around the
// text, four zeros should take ~3.7 msec.
//
// We might want to pre-render each digit and draw numbers by dma-ing each
// digit from flash.

static constexpr Font font = roboto_32;
static constexpr int wid = 100;
static constexpr int hgt = 50;
static constexpr Color fg = Color::lime();
static constexpr Color bg = Color::gray(80);

static PixelImage<Pixel565, wid, hgt> img;

static void run(Framebuffer &fb)
{
    PixelImageInfo *hdr = (PixelImageInfo *)&img;

    const char *msgs[] = {"", "0", "00", "000", "0000", "00000"};

    for (int i = 0; i < 6; i++) {

        uint32_t t0 = time_us_32();

        label_img<Pixel565>(hdr, msgs[i], font, fg, 0, fg, bg);

        uint32_t t1 = time_us_32();

        printf("ImgUpdate: created %dw x %dh image for \"%s\" in %lu usec\n",
               img.width, img.height, msgs[i], t1 - t0);

        printf(
            "ImgUpdate: writing %dw x %dh image for \"%s\" at 0x%p (%d "
            "bytes)\n",
            img.width, img.height, msgs[i], &img, sizeof(img.pixels));

        int hor = 100;
        int ver = 100;

        fb.write(hor, ver, &img);

        sleep_ms(1000);
    }
}

} // namespace ImgUpdate


namespace ImgDigits {

// Pre-render digits 0-9 and use that to render integers.
//
// In roboto_30, overhead is about 35 usec, and each digit adds a few usec.
// "1234" takes 40 usec, and "123456789" takes 60 usec. This is just to
// start the dma, then it continues to run after this returns.

static constexpr Font font = roboto_30;
static constexpr int hgt = font.y_adv;
static constexpr Color fg = Color::red();
static constexpr Color bg = Color::white();

// Use preprocessor to create an image for each digit 0-9. Something using
// templates or constexpr functions would be better, but this works and is
// straightforward enough.

#define IMG_MAKE(N, F, FG, BG)                                            \
    static constexpr PixelImage<Pixel565, F.width(#N), F.y_adv> img_##N = \
        label_img<Pixel565, F.width(#N), F.y_adv>(#N, F, FG, BG);

// clang-format off
#define IMG_LIST(F, FG, BG) IMG_MAKE(0, F, FG, BG) IMG_MAKE(1, F, FG, BG) \
                            IMG_MAKE(2, F, FG, BG) IMG_MAKE(3, F, FG, BG) \
                            IMG_MAKE(4, F, FG, BG) IMG_MAKE(5, F, FG, BG) \
                            IMG_MAKE(6, F, FG, BG) IMG_MAKE(7, F, FG, BG) \
                            IMG_MAKE(8, F, FG, BG) IMG_MAKE(9, F, FG, BG)
// clang-format on

// Expand to create all ten images
IMG_LIST(font, fg, bg)

// Clean up macros
#undef IMG_MAKE
#undef IMG_LIST

static const PixelImageInfo *digit_img[10] = {
    (PixelImageInfo *)&img_0, (PixelImageInfo *)&img_1,
    (PixelImageInfo *)&img_2, (PixelImageInfo *)&img_3,
    (PixelImageInfo *)&img_4, (PixelImageInfo *)&img_5,
    (PixelImageInfo *)&img_6, (PixelImageInfo *)&img_7,
    (PixelImageInfo *)&img_8, (PixelImageInfo *)&img_9,
};

// cast to pass to Framebuffer::write()
static const void **dig_img = reinterpret_cast<const void **>(digit_img);


static void run(Framebuffer &fb)
{
    const int nums[] = {0,     1,      12,      123,      1234,
                        12345, 123456, 1234567, 12345678, 123456789};
    const int nums_max = sizeof(nums) / sizeof(nums[0]);
    const int hor = 100;
    const int ver = 100;
    const int hgt = digit_img[0]->hgt;

    // all digits should be in flash
    for (int i = 0; i < 10; i++)
        assert(is_xip(digit_img[i]));

    for (int i = 0; i < nums_max; i++) {
        uint32_t t0 = time_us_32();
        fb.write(hor, ver, nums[i], dig_img, Framebuffer::HAlign::Center);
        uint32_t t1 = time_us_32();
        // 'line' includes the start and end points
        fb.line(hor, ver - hgt, hor, ver + hgt + hgt - 1, Color::lime());
        printf("ImgDigits: wrote image for %d in %lu usec\n", nums[i], t1 - t0);
        sleep_ms(1000);
    }
}

} // namespace ImgDigits
