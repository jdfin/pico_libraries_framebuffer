
#include "st7796.h"

#include <cstdio>

#include "argv.h"
#include "color.h"
#include "font.h"
#include "great_vibes_48.h"
#include "hardware/spi.h"
#include "pico/stdio.h"
#include "pico/stdio_usb.h"
#include "pico/stdlib.h"
#include "pixel_565.h"
#include "pixel_image.h"
#include "roboto_32.h"
#include "str_ops.h"
#include "sys_led.h"
#include "util.h"

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

static void rotations(St7796 &st7796);
static void corner_pixels(St7796 &st7796);
static void corner_squares(St7796 &st7796);
static void line_1(St7796 &st7796);
static void hline_1(St7796 &st7796);
static void draw_rect_1(St7796 &st7796);
static void draw_rect_2(St7796 &st7796);
static void fill_rect_1(St7796 &st7796);
static void fill_rect_2(St7796 &st7796);
static void draw_circle_1(St7796 &st7796);
static void draw_circle_2(St7796 &st7796);
static void draw_circle_aa_1(St7796 &st7796);
static void draw_circle_aa_2(St7796 &st7796);
static void print_char_1(St7796 &st7796);
static void print_string_1(St7796 &st7796);
static void print_string_2(St7796 &st7796);
static void print_string_3(St7796 &st7796);
static void print_string_4(St7796 &st7796);
static void img_chr(St7796 &st7796);
static void img_str(St7796 &st7796);
static void img_btn(St7796 &st7796);

static struct {
    const char *name;
    void (*func)(St7796 &);
} tests[] = {
    {"rotations", rotations},
    {"corner_pixels", corner_pixels},
    {"corner_squares", corner_squares},
    {"line_1", line_1},
    {"hline_1", hline_1},
    {"draw_rect_1", draw_rect_1},
    {"draw_rect_2", draw_rect_2},
    {"fill_rect_1", fill_rect_1},
    {"fill_rect_2", fill_rect_2},
    {"draw_circle_1", draw_circle_1},
    {"draw_circle_2", draw_circle_2},
    {"draw_circle_aa_1", draw_circle_aa_1},
    {"draw_circle_aa_2", draw_circle_aa_2},
    {"print_char_1", print_char_1},
    {"print_string_1", print_string_1},
    {"print_string_2", print_string_2},
    {"print_string_3", print_string_3},
    {"print_string_4", print_string_4},
    {"img_chr", img_chr},
    {"img_str", img_str},
    {"img_btn", img_btn},
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


static void reinit_screen(St7796 &lcd)
{
    // landscape, connector to the left
    lcd.rotation(St7796::Rotation::left);

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
               spi_baud_request, lcd_cd_pin, lcd_rst_pin, lcd_led_pin, work,
               work_bytes);

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


static void rotations(St7796 &st7796)
{
    const uint32_t delay_ms = 2000;
    Framebuffer &fb = st7796;

    fb.fill_rect(0, 0, fb.width(), fb.height(), Color::black());
    sleep_ms(delay_ms);

    st7796.rotation(St7796::Rotation::bottom);
    mark_origin(fb, "Rotation::bottom", Color::red());
    sleep_ms(delay_ms);

    st7796.rotation(St7796::Rotation::left);
    mark_origin(st7796, "Rotation::left", Color::green());
    sleep_ms(delay_ms);

    st7796.rotation(St7796::Rotation::top);
    mark_origin(st7796, "Rotation::top", Color::blue());
    sleep_ms(delay_ms);

    st7796.rotation(St7796::Rotation::right);
    mark_origin(st7796, "Rotation::right", Color::white());
    sleep_ms(delay_ms);
}


static void corner_pixels(St7796 &st7796)
{
    Framebuffer &fb = st7796;
    const Color c = Color::white();
    fb.pixel(0, 0, c);
    fb.pixel(0, fb.height() - 1, c);
    fb.pixel(fb.width() - 1, 0, c);
    fb.pixel(fb.width() - 1, fb.height() - 1, c);
}


static void corner_squares(St7796 &st7796)
{
    Framebuffer &fb = st7796;
    const int size = 10;
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            fb.pixel(i, j, Color::red());
            fb.pixel(i, fb.height() - 1 - j, Color::green());
            fb.pixel(fb.width() - 1 - i, j, Color::blue());
            fb.pixel(fb.width() - 1 - i, fb.height() - 1 - j, Color::white());
        }
    }
}


static void line_1(St7796 &st7796)
{
    Framebuffer &fb = st7796;
    const Color c = Color::white();
    int w1 = fb.width() - 1;
    int h1 = fb.height() - 1;
    fb.line(100, 100, 0, 0, c);
    fb.line(100, 100, 0, h1, c);
    fb.line(100, 100, w1, 0, c);
    fb.line(100, 100, w1, h1, c);
}


static void hline_1(St7796 &st7796)
{
    // should be able to see that each successive line drawn is one pixel shorter
    Framebuffer &fb = st7796;
    const Color c = Color::white();
    fb.line(0, 0, fb.width() - 1, 0, c);
    fb.line(0, 2, fb.width() - 2, 2, c);
    fb.line(0, 4, fb.width() - 3, 4, c);
    fb.line(0, 6, fb.width() - 4, 6, c);
    fb.line(0, 8, fb.width() - 5, 8, c);
}


static void draw_rect_1(St7796 &st7796)
{
    Framebuffer &fb = st7796;
    fb.draw_rect(0, 0, fb.width(), fb.height(), Color::white());
}


static void draw_rect_2(St7796 &st7796)
{
    Framebuffer &fb = st7796;
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


static void fill_rect_1(St7796 &st7796)
{
    Framebuffer &fb = st7796;
    const Color colors[] = {
        Color::red(),    Color::green(),   Color::blue(),   //
        Color::yellow(), Color::magenta(), Color::cyan(),   //
        Color::gray25(), Color::gray50(),  Color::gray75(), //
        Color::white(),
    };
    const int color_cnt = sizeof(colors) / sizeof(colors[0]);

    for (int c_num = 0; c_num < color_cnt; c_num++) {
        fb.fill_rect(0, 0, fb.width(), fb.height(), colors[c_num]);
        sleep_ms(100);
    }
}


static void fill_rect_2(St7796 &st7796)
{
    Framebuffer &fb = st7796;
    const Color colors[] = {
        Color::gray75(), Color::red(),     Color::green(), Color::blue(),
        Color::yellow(), Color::magenta(), Color::cyan(),  Color::white(),
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
            //sleep_ms(10);
        }
        if (++color >= color_cnt)
            color = 0;
    }
}


static void draw_circle_1(St7796 &st7796)
{
    Framebuffer &fb = st7796;
    fb.draw_circle(100, 100, 100, Color::white());
}


static void draw_circle_2(St7796 &st7796)
{
    Framebuffer &fb = st7796;
    int h = fb.width() / 2;
    int v = fb.height() / 2;
    int r = h < v ? h : v;
    r--;
    fb.draw_circle(h, v, r, Color::red(), Framebuffer::Quadrant::LowerRight);
    fb.line(h + 1, v + 1, h + r, v + r, Color::red());
    fb.draw_circle(h, v, r, Color::green(), Framebuffer::Quadrant::LowerLeft);
    fb.line(h - 1, v + 1, h - r, v + r, Color::green());
    fb.draw_circle(h, v, r, Color::blue(), Framebuffer::Quadrant::UpperLeft);
    fb.line(h - 1, v - 1, h - r, v - r, Color::blue());
    fb.draw_circle(h, v, r, Color::white(), Framebuffer::Quadrant::UpperRight);
    fb.line(h + 1, v - 1, h + r, v - r, Color::white());
}


static void draw_circle_aa_1(St7796 &st7796)
{
    Framebuffer &fb = st7796;
    const int h = fb.width() / 2;
    const int v = fb.height() / 2;
    const int r = (h < v ? h : v) - 20;
    fb.draw_circle(h, v, r - 10, Color::white());
    fb.draw_circle_aa(h, v, 8, Color::white(), Color::black());
    fb.draw_circle_aa(h, v, r + 10, Color::white(), Color::black());
}


static void draw_circle_aa_2(St7796 &st7796)
{
    Framebuffer &fb = st7796;
    const int h = fb.width() / 2;
    const int v = fb.height() / 2;
    const int r_max = h < v ? h : v;
    for (int r = 0; r < r_max; r += 4)
        fb.draw_circle_aa(h, v, r, Color::white(), Color::black());
}


// Should result in a gray50 background, a 1-pixel black box near the middle,
// a 1-pixel gray50 box inside that one, then a black-on-white character
static void print_char_1(St7796 &st7796)
{
    Framebuffer &fb = st7796;
    // background
    fb.fill_rect(0, 0, fb.width(), fb.height(), Color::gray50());
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


static void print_string_1(St7796 &st7796)
{
    Framebuffer &fb = st7796;
    fb.fill_rect(0, 0, fb.width(), fb.height(), Color::white());

    uint16_t hor = 20;
    uint16_t ver = 20;
    fb.print(hor, ver, "Hello,", font, Color::black(), Color::white());
    ver += font.y_adv;
    fb.print(hor, ver, "world!", font, Color::black(), Color::white());
}


static void print_string_2(St7796 &st7796)
{
    Framebuffer &fb = st7796;
    fb.fill_rect(0, 0, fb.width(), fb.height(), Color::gray50());

    int hor = fb.width() / 2;
    int ver = 20;
    fb.line(hor, 0, hor, fb.height() - 1, Color::red());

    fb.print(hor, ver, "Hello, world!", font, Color::black(), Color::white(),
             0);
}


static void print_string_3(St7796 &st7796)
{
    Framebuffer &fb = st7796;
    fb.fill_rect(0, 0, fb.width(), fb.height(), Color::gray50());

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


static void print_string_4(St7796 &st7796)
{
    Framebuffer &fb = st7796;
    const char *s1 = " >";
    const char *s2 = "< ";
    const int s1_wid = font.width(s1);
    const int s2_wid = font.width(s2);
    const uint32_t ms = 0;

    const Color fg = Color::red();
    const Color bg = Color::white();

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

        int c = stdio_getchar_timeout_us(0);
        if (0 <= c && c <= 255)
            break;
    }
}


constexpr Font chr_font = roboto_32;
constexpr char chr_char = 'Q';
constexpr int chr_wid = chr_font.info[int(chr_char)].x_adv;
constexpr int chr_hgt = chr_font.y_adv;
constexpr Color chr_fg = Color::red();
constexpr Color chr_bg = Color::black();

constexpr PixelImage<Pixel565, chr_wid, chr_hgt> chr_img =
    image_init<Pixel565, chr_char, chr_wid, chr_hgt>(chr_font, chr_fg, chr_bg);

static void img_chr(St7796 &st7796)
{
    const char *loc = mem_name(&chr_img);

    printf("img_chr: writing %dw x %dh image from %s at 0x%p (%d bytes)\n",
           chr_img.width, chr_img.height, loc, &chr_img,
           sizeof(chr_img.pixels));

    int hor = 10;
    int ver = 10;

    st7796.write(hor, ver, chr_img.width, chr_img.height, chr_img.pixels);

    ver += 40;

    printf("img_chr: printing '%c'\n", chr_char);
    Framebuffer &fb = st7796;
    fb.print(hor, ver, chr_char, chr_font, chr_fg, chr_bg);
}


constexpr Font str_font = roboto_32;
constexpr char str_msg[] = "Hello, world!";
constexpr int str_wid = str_font.width(str_msg);
constexpr int str_hgt = str_font.y_adv;
constexpr Color str_fg = Color::green();
constexpr Color str_bg = Color::black();

constexpr PixelImage<Pixel565, str_wid, str_hgt> str_img =
    image_init<Pixel565, str_msg, str_wid, str_hgt>(str_font, str_fg, str_bg);

static void img_str(St7796 &st7796)
{
    const char *loc = mem_name(&str_img);

    printf("img_str: writing %dw x %dh image from %s at 0x%p (%d bytes)\n",
           str_img.width, str_img.height, loc, &str_img,
           sizeof(str_img.pixels));

    int hor = 10;
    int ver = 10;

    st7796.write(hor, ver, str_img.width, str_img.height, str_img.pixels);

    ver += 40;

    printf("img_str: printing \"%s\"\n", str_msg);
    Framebuffer &fb = st7796;
    fb.print(hor, ver, str_msg, str_font, str_fg, str_bg);
}


// Print array of buttons, each with a box around it:
//  0  1  2  3  4  5  6  7  8  9
// 10 11 12 13 14 15 16 17 18 19
// 20 21 22 23 24 25 26 27 28 29

constexpr int img_btn_per_row = 10;
constexpr int img_btn_sz = 480 / img_btn_per_row;

constexpr Font img_btn_font = roboto_32;
constexpr Color img_btn_fg = Color::black();
constexpr Color img_btn_bg = Color::white();
constexpr int img_btn_hgt = img_btn_font.y_adv;

constexpr char img_btn_0_str[] = "0";
constexpr int img_btn_0_wid = img_btn_font.width(img_btn_0_str);
constexpr PixelImage<Pixel565, img_btn_0_wid, img_btn_hgt> img_btn_0 =
    image_init<Pixel565, img_btn_0_str, img_btn_0_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

constexpr char img_btn_1_str[] = "1";
constexpr int img_btn_1_wid = img_btn_font.width(img_btn_1_str);
constexpr PixelImage<Pixel565, img_btn_1_wid, img_btn_hgt> img_btn_1 =
    image_init<Pixel565, img_btn_1_str, img_btn_1_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

constexpr char img_btn_2_str[] = "2";
constexpr int img_btn_2_wid = img_btn_font.width(img_btn_2_str);
constexpr PixelImage<Pixel565, img_btn_2_wid, img_btn_hgt> img_btn_2 =
    image_init<Pixel565, img_btn_2_str, img_btn_2_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

constexpr char img_btn_3_str[] = "3";
constexpr int img_btn_3_wid = img_btn_font.width(img_btn_3_str);
constexpr PixelImage<Pixel565, img_btn_3_wid, img_btn_hgt> img_btn_3 =
    image_init<Pixel565, img_btn_3_str, img_btn_3_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

constexpr char img_btn_4_str[] = "4";
constexpr int img_btn_4_wid = img_btn_font.width(img_btn_4_str);
constexpr PixelImage<Pixel565, img_btn_4_wid, img_btn_hgt> img_btn_4 =
    image_init<Pixel565, img_btn_4_str, img_btn_4_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

constexpr char img_btn_5_str[] = "5";
constexpr int img_btn_5_wid = img_btn_font.width(img_btn_5_str);
constexpr PixelImage<Pixel565, img_btn_5_wid, img_btn_hgt> img_btn_5 =
    image_init<Pixel565, img_btn_5_str, img_btn_5_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

constexpr char img_btn_6_str[] = "6";
constexpr int img_btn_6_wid = img_btn_font.width(img_btn_6_str);
constexpr PixelImage<Pixel565, img_btn_6_wid, img_btn_hgt> img_btn_6 =
    image_init<Pixel565, img_btn_6_str, img_btn_6_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

constexpr char img_btn_7_str[] = "7";
constexpr int img_btn_7_wid = img_btn_font.width(img_btn_7_str);
constexpr PixelImage<Pixel565, img_btn_7_wid, img_btn_hgt> img_btn_7 =
    image_init<Pixel565, img_btn_7_str, img_btn_7_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

constexpr char img_btn_8_str[] = "8";
constexpr int img_btn_8_wid = img_btn_font.width(img_btn_8_str);
constexpr PixelImage<Pixel565, img_btn_8_wid, img_btn_hgt> img_btn_8 =
    image_init<Pixel565, img_btn_8_str, img_btn_8_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

constexpr char img_btn_9_str[] = "9";
constexpr int img_btn_9_wid = img_btn_font.width(img_btn_9_str);
constexpr PixelImage<Pixel565, img_btn_9_wid, img_btn_hgt> img_btn_9 =
    image_init<Pixel565, img_btn_9_str, img_btn_9_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

constexpr char img_btn_10_str[] = "10";
constexpr int img_btn_10_wid = img_btn_font.width(img_btn_10_str);
constexpr PixelImage<Pixel565, img_btn_10_wid, img_btn_hgt> img_btn_10 =
    image_init<Pixel565, img_btn_10_str, img_btn_10_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

constexpr char img_btn_11_str[] = "11";
constexpr int img_btn_11_wid = img_btn_font.width(img_btn_11_str);
constexpr PixelImage<Pixel565, img_btn_11_wid, img_btn_hgt> img_btn_11 =
    image_init<Pixel565, img_btn_11_str, img_btn_11_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

constexpr char img_btn_12_str[] = "12";
constexpr int img_btn_12_wid = img_btn_font.width(img_btn_12_str);
constexpr PixelImage<Pixel565, img_btn_12_wid, img_btn_hgt> img_btn_12 =
    image_init<Pixel565, img_btn_12_str, img_btn_12_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

constexpr char img_btn_13_str[] = "13";
constexpr int img_btn_13_wid = img_btn_font.width(img_btn_13_str);
constexpr PixelImage<Pixel565, img_btn_13_wid, img_btn_hgt> img_btn_13 =
    image_init<Pixel565, img_btn_13_str, img_btn_13_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

constexpr char img_btn_14_str[] = "14";
constexpr int img_btn_14_wid = img_btn_font.width(img_btn_14_str);
constexpr PixelImage<Pixel565, img_btn_14_wid, img_btn_hgt> img_btn_14 =
    image_init<Pixel565, img_btn_14_str, img_btn_14_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

constexpr char img_btn_15_str[] = "15";
constexpr int img_btn_15_wid = img_btn_font.width(img_btn_15_str);
constexpr PixelImage<Pixel565, img_btn_15_wid, img_btn_hgt> img_btn_15 =
    image_init<Pixel565, img_btn_15_str, img_btn_15_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

constexpr char img_btn_16_str[] = "16";
constexpr int img_btn_16_wid = img_btn_font.width(img_btn_16_str);
constexpr PixelImage<Pixel565, img_btn_16_wid, img_btn_hgt> img_btn_16 =
    image_init<Pixel565, img_btn_16_str, img_btn_16_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

constexpr char img_btn_17_str[] = "17";
constexpr int img_btn_17_wid = img_btn_font.width(img_btn_17_str);
constexpr PixelImage<Pixel565, img_btn_17_wid, img_btn_hgt> img_btn_17 =
    image_init<Pixel565, img_btn_17_str, img_btn_17_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

constexpr char img_btn_18_str[] = "18";
constexpr int img_btn_18_wid = img_btn_font.width(img_btn_18_str);
constexpr PixelImage<Pixel565, img_btn_18_wid, img_btn_hgt> img_btn_18 =
    image_init<Pixel565, img_btn_18_str, img_btn_18_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

constexpr char img_btn_19_str[] = "19";
constexpr int img_btn_19_wid = img_btn_font.width(img_btn_19_str);
constexpr PixelImage<Pixel565, img_btn_19_wid, img_btn_hgt> img_btn_19 =
    image_init<Pixel565, img_btn_19_str, img_btn_19_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

constexpr char img_btn_20_str[] = "20";
constexpr int img_btn_20_wid = img_btn_font.width(img_btn_20_str);
constexpr PixelImage<Pixel565, img_btn_20_wid, img_btn_hgt> img_btn_20 =
    image_init<Pixel565, img_btn_20_str, img_btn_20_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

constexpr char img_btn_21_str[] = "21";
constexpr int img_btn_21_wid = img_btn_font.width(img_btn_21_str);
constexpr PixelImage<Pixel565, img_btn_21_wid, img_btn_hgt> img_btn_21 =
    image_init<Pixel565, img_btn_21_str, img_btn_21_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

constexpr char img_btn_22_str[] = "22";
constexpr int img_btn_22_wid = img_btn_font.width(img_btn_22_str);
constexpr PixelImage<Pixel565, img_btn_22_wid, img_btn_hgt> img_btn_22 =
    image_init<Pixel565, img_btn_22_str, img_btn_22_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

constexpr char img_btn_23_str[] = "23";
constexpr int img_btn_23_wid = img_btn_font.width(img_btn_23_str);
constexpr PixelImage<Pixel565, img_btn_23_wid, img_btn_hgt> img_btn_23 =
    image_init<Pixel565, img_btn_23_str, img_btn_23_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

constexpr char img_btn_24_str[] = "24";
constexpr int img_btn_24_wid = img_btn_font.width(img_btn_24_str);
constexpr PixelImage<Pixel565, img_btn_24_wid, img_btn_hgt> img_btn_24 =
    image_init<Pixel565, img_btn_24_str, img_btn_24_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

constexpr char img_btn_25_str[] = "25";
constexpr int img_btn_25_wid = img_btn_font.width(img_btn_25_str);
constexpr PixelImage<Pixel565, img_btn_25_wid, img_btn_hgt> img_btn_25 =
    image_init<Pixel565, img_btn_25_str, img_btn_25_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

constexpr char img_btn_26_str[] = "26";
constexpr int img_btn_26_wid = img_btn_font.width(img_btn_26_str);
constexpr PixelImage<Pixel565, img_btn_26_wid, img_btn_hgt> img_btn_26 =
    image_init<Pixel565, img_btn_26_str, img_btn_26_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

constexpr char img_btn_27_str[] = "27";
constexpr int img_btn_27_wid = img_btn_font.width(img_btn_27_str);
constexpr PixelImage<Pixel565, img_btn_27_wid, img_btn_hgt> img_btn_27 =
    image_init<Pixel565, img_btn_27_str, img_btn_27_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

constexpr char img_btn_28_str[] = "28";
constexpr int img_btn_28_wid = img_btn_font.width(img_btn_28_str);
constexpr PixelImage<Pixel565, img_btn_28_wid, img_btn_hgt> img_btn_28 =
    image_init<Pixel565, img_btn_28_str, img_btn_28_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

constexpr char img_btn_29_str[] = "29";
constexpr int img_btn_29_wid = img_btn_font.width(img_btn_29_str);
constexpr PixelImage<Pixel565, img_btn_29_wid, img_btn_hgt> img_btn_29 =
    image_init<Pixel565, img_btn_29_str, img_btn_29_wid, img_btn_hgt>(
        img_btn_font, img_btn_fg, img_btn_bg);

// and one with "pressed" colors
constexpr PixelImage<Pixel565, img_btn_13_wid, img_btn_hgt> img_btn_13_inv =
    image_init<Pixel565, img_btn_13_str, img_btn_13_wid, img_btn_hgt>(
        img_btn_font, img_btn_bg, img_btn_fg);


struct {
    int wid;
    int hgt;
    const Pixel565 *pix;
} btn_img[30] = {
    {img_btn_0.width, img_btn_0.height, img_btn_0.pixels},
    {img_btn_1.width, img_btn_1.height, img_btn_1.pixels},
    {img_btn_2.width, img_btn_2.height, img_btn_2.pixels},
    {img_btn_3.width, img_btn_3.height, img_btn_3.pixels},
    {img_btn_4.width, img_btn_4.height, img_btn_4.pixels},
    {img_btn_5.width, img_btn_5.height, img_btn_5.pixels},
    {img_btn_6.width, img_btn_6.height, img_btn_6.pixels},
    {img_btn_7.width, img_btn_7.height, img_btn_7.pixels},
    {img_btn_8.width, img_btn_8.height, img_btn_8.pixels},
    {img_btn_9.width, img_btn_9.height, img_btn_9.pixels},
    {img_btn_10.width, img_btn_10.height, img_btn_10.pixels},
    {img_btn_11.width, img_btn_11.height, img_btn_11.pixels},
    {img_btn_12.width, img_btn_12.height, img_btn_12.pixels},
    {img_btn_13.width, img_btn_13.height, img_btn_13.pixels},
    {img_btn_14.width, img_btn_14.height, img_btn_14.pixels},
    {img_btn_15.width, img_btn_15.height, img_btn_15.pixels},
    {img_btn_16.width, img_btn_16.height, img_btn_16.pixels},
    {img_btn_17.width, img_btn_17.height, img_btn_17.pixels},
    {img_btn_18.width, img_btn_18.height, img_btn_18.pixels},
    {img_btn_19.width, img_btn_19.height, img_btn_19.pixels},
    {img_btn_20.width, img_btn_20.height, img_btn_20.pixels},
    {img_btn_21.width, img_btn_21.height, img_btn_21.pixels},
    {img_btn_22.width, img_btn_22.height, img_btn_22.pixels},
    {img_btn_23.width, img_btn_23.height, img_btn_23.pixels},
    {img_btn_24.width, img_btn_24.height, img_btn_24.pixels},
    {img_btn_25.width, img_btn_25.height, img_btn_25.pixels},
    {img_btn_26.width, img_btn_26.height, img_btn_26.pixels},
    {img_btn_27.width, img_btn_27.height, img_btn_27.pixels},
    {img_btn_28.width, img_btn_28.height, img_btn_28.pixels},
    {img_btn_29.width, img_btn_29.height, img_btn_29.pixels},
};

static void img_btn(St7796 &st7796)
{
    Framebuffer &fb = st7796;

    // clear screen
    fb.fill_rect(0, 0, fb.width(), fb.height(), img_btn_bg);

    int hor, ver; // top-left corner of each button

    // draw buttons
    ver = 0;
    for (int r = 0; r < 3; r++) {
        hor = 0;
        for (int c = 0; c < img_btn_per_row; c++) {
            // outline
            fb.draw_rect(hor, ver, img_btn_sz, img_btn_sz, img_btn_fg);
            // label
            int idx = r * img_btn_per_row + c;
            assert(0 <= idx && idx < 30);
            assert(is_xip(btn_img[idx].pix));
            st7796.write(hor + img_btn_sz / 2 - btn_img[idx].wid / 2, //
                         ver + img_btn_sz / 2 - btn_img[idx].hgt / 2, //
                         btn_img[idx].wid, btn_img[idx].hgt, btn_img[idx].pix);
            hor += img_btn_sz;
        }
        ver += img_btn_sz;
    }
    fb.line(0, img_btn_sz * 3, fb.width() - 1, img_btn_sz * 3, img_btn_fg);

    // flip button 13 a few times
    int btn_num = 13;
    hor = img_btn_sz * (btn_num % 10);
    ver = img_btn_sz * (btn_num / 10);
    for (int i = 0; i < 10; i++) {
        if ((i & 1) == 0) {
            // inverted
            //fb.draw_rect(hor, ver, img_btn_sz, img_btn_sz, img_btn_bg);
            fb.fill_rect(hor + 1, ver + 1,               //
                         img_btn_sz - 2, img_btn_sz - 2, //
                         img_btn_fg);
            st7796.write(hor + img_btn_sz / 2 - btn_img[btn_num].wid / 2,
                         ver + img_btn_sz / 2 - btn_img[btn_num].hgt / 2,
                         btn_img[btn_num].wid, btn_img[btn_num].hgt,
                         img_btn_13_inv.pixels);
        } else {
            // not inverted
            //fb.draw_rect(hor, ver, img_btn_sz, img_btn_sz, img_btn_fg);
            fb.fill_rect(hor + 1, ver + 1, img_btn_sz - 2, img_btn_sz - 2,
                         img_btn_bg);
            st7796.write(hor + img_btn_sz / 2 - btn_img[btn_num].wid / 2,
                         ver + img_btn_sz / 2 - btn_img[btn_num].hgt / 2,
                         btn_img[btn_num].wid, btn_img[btn_num].hgt,
                         img_btn_13.pixels);
        }
        sleep_ms(1000);
    }
}
