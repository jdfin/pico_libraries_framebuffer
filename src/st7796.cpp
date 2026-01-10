
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <utility>
//
#include "hardware/dma.h"
#include "hardware/spi.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"
//
#include "dbg_gpio.h"
#include "dma_irq_mux.h"
#include "font.h"
#include "pixel_565.h"
#include "pixel_image.h"
#include "spi_extra.h"
#include "util.h"
//
#include "st7796.h"


St7796::St7796(spi_inst_t *spi, int miso_pin, int mosi_pin, int clk_pin,
               int cs_pin, int baud, int cd_pin, int rst_pin, int bk_pin,
               void *work, int work_bytes) :
    Framebuffer(phys_wid, phys_hgt),
    _spi(spi),
    _spi_freq(0),
    _miso_pin(miso_pin),
    _mosi_pin(mosi_pin),
    _clk_pin(clk_pin),
    _cs_pin(cs_pin), // optional; it's always asserted
    _baud(baud),     // requested; actual may be different
    _cd_pin(cd_pin),
    _rst_pin(rst_pin),
    _bk_pin(bk_pin),
    // _dma_ch
    // _dma_cfg
    _dma_running(false),
    _dma_pixel(0),
    _rotation(Rotation::bottom),
    _pix_buf((Pixel565 *)work),
    _pix_buf_len(work_bytes / sizeof(Pixel565)),
    // _ops[]
    _ops_stall_cnt(0),
    _op_next(0),
    _op_free(0)
{
    xassert(spi != nullptr);
    xassert(_miso_pin >= 0 && _mosi_pin >= 0 && _clk_pin >= 0);
    xassert(_cd_pin >= 0 && _rst_pin >= 0);

    DbgGpio::init(28);

    _spi_freq = spi_init(_spi, _baud);
    gpio_set_function(_miso_pin, GPIO_FUNC_SPI);
    gpio_set_function(_mosi_pin, GPIO_FUNC_SPI);
    gpio_set_function(_clk_pin, GPIO_FUNC_SPI);
    spi_set_format(_spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    if (_cs_pin >= 0) {
        gpio_init(_cs_pin);
        gpio_set_dir(_cs_pin, gpio_out);
        gpio_put(_cs_pin, cs_assert);
    }

    gpio_init(_cd_pin);
    gpio_set_dir(_cd_pin, gpio_out);
    // don't care if it's high or low at this point (but it's low)

    gpio_init(_rst_pin);
    gpio_put(_rst_pin, rst_assert);
    gpio_set_dir(_rst_pin, gpio_out);

    if (_bk_pin >= 0) {
        gpio_init(_bk_pin);
        brightness(0); // off
        gpio_set_dir(_bk_pin, gpio_out);
    } else {
        // no control, assume it's max brightness (e.g. pulled up)
        brightness(100);
    }

    // DMA is only used for pixel data
    _dma_ch = dma_claim_unused_channel(true);
    _dma_cfg = dma_channel_get_default_config(_dma_ch);
    channel_config_set_dreq(&_dma_cfg, spi_get_dreq(_spi, true));
    channel_config_set_transfer_data_size(&_dma_cfg, DMA_SIZE_16);
    channel_config_set_write_increment(&_dma_cfg, false); // write to spi
    dma_irq_mux_connect(0, _dma_ch, dma_raw_handler, this);
    dma_irq_mux_enable(0, _dma_ch, true);
}


St7796::~St7796()
{
#if 1
    // Waveshare
#else
    // Hosyond
    // After gpio_deinit, the pullup on the backlight will set the
    // signal high (it is not off)
    if (_bk_pin >= 0)
        gpio_deinit(_bk_pin);
#endif
}


void St7796::init()
{
    //printf("St7796::init(): sizeof(_ops[0]) = %zu\n", sizeof(_ops[0]));

    hw_reset();

    // after hw_reset:
    //   no sleep does not work
    //   sleeping 1 msec works
    //   sample code sleeps 200 msec
    sleep_ms(10);

    // In each uint16_t in cmds[], the hi byte controls what the lo byte means:
    // high byte == "command", low byte goes out with C/D = command
    // high byte == "data", low byte goes out with C/D = data
    // high byte == "delay", low byte is milliseconds to sleep

    // so we don't have to OR-in wr_data in every data byte below
    xassert(wr_data == 0x0000);

    // clang-format off
    const uint16_t cmds[] = {
#if 1
        // Waveshare, from the python code
        wr_cmd | St7796Cmd::INVON,
        wr_cmd | St7796Cmd::PWR3, 0x33,
        wr_cmd | St7796Cmd::VCMPCTL, 0x00, 0x1e, 0x80,
        wr_cmd | St7796Cmd::FRMCTR1, 0xB0,
        wr_cmd | St7796Cmd::PGC, 0x00, 0x13, 0x18, 0x04, 0x0F, 0x06, 0x3a, 0x56,
                      0x4d, 0x03, 0x0a, 0x06, 0x30, 0x3e, 0x0f,
        wr_cmd | St7796Cmd::NGC, 0x00, 0x13, 0x18, 0x01, 0x11, 0x06, 0x38, 0x34,
                      0x4d, 0x06, 0x0d, 0x0b, 0x31, 0x37, 0x0f,
        wr_cmd | St7796Cmd::COLMOD, 0x55,
        wr_cmd | St7796Cmd::SLPOUT,
        wr_delay_ms | 120,
        wr_cmd | St7796Cmd::DISPON,
        wr_cmd | St7796Cmd::DFC, 0x00, 0x62,
        wr_cmd | St7796Cmd::MADCTL, madctl(),
#else
        // Hosyond 3.5"
        // After power on: sleep in, normal display, idle off
        wr_cmd | SLPOUT,
        // After SLPOUT: wait 5 msec before any new commands
        wr_delay_ms | 5,
        // Now we are: sleep out, normal display, idle off
        // This is where we want to stay.
        wr_cmd | MADCTL, madctl(),
        wr_cmd | COLMOD, 0x55,  // 16 bits/pixel
        wr_cmd | CSCON, 0xc3,   // enable cmd 2 part I
        wr_cmd | CSCON, 0x96,   // enable cmd 2 part II
        wr_cmd | DIC, 0x02,     // display inversion control
        wr_cmd | EM, 0xc6,      // 64k -> 256k color mapping, panel-specific
        wr_cmd | PWR1, 0xc0, 0x00, // AVDD=6.8, AVCL=-4.4, VGH=12.541, VGL=-7.158
        wr_cmd | PWR2, 0x13,    // VAP=4.5
        wr_cmd | PWR3, 0xa7,    // src current low, gamma current high
        wr_cmd | VCMPCTL, 0x21, // VCOM=0.825
        wr_cmd | DOCA, 0x40, 0x8a, 0x1b, 0x1b, 0x23, 0x0a, 0xac, 0x33,
        wr_cmd | PGC, 0xd2, 0x05, 0x08, 0x06, 0x05, 0x02, 0x2a, // pos gamma ctrl
                      0x44, 0x46, 0x39, 0x15, 0x15, 0x2d, 0x32,
        wr_cmd | NGC, 0x96, 0x08, 0x0c, 0x09, 0x09, 0x25, 0x2e, // neg gamma ctrl
                      0x43, 0x42, 0x35, 0x11, 0x11, 0x28, 0x2e,
        wr_cmd | CSCON, 0x3c,   // disable cmd 2 part I
        wr_cmd | CSCON, 0x69,   // disable cmd 2 part II
        wr_cmd | INVON,         // display inversion on
        wr_cmd | DISPON,        // display on
#endif
    };
    // clang-format on
    const int cmds_len = sizeof(cmds) / sizeof(cmds[0]);

    write_cmds(cmds, cmds_len); // sets to 8-bit spi
}


void St7796::write_cmds(const uint16_t *b, int b_len)
{
    xassert(b != nullptr && b_len >= 1);
    spi_set_format(_spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    while (b_len > 0) {
        uint16_t next = *b++;
        if ((next & wr_mask) == wr_cmd) {
            command();
            uint8_t d = uint8_t(next);
            spi_write_blocking(_spi, &d, 1);
        } else if ((next & wr_mask) == wr_data) {
            data();
            uint8_t d = uint8_t(next);
            spi_write_blocking(_spi, &d, 1);
        } else if ((next & wr_mask) == wr_delay_ms) {
            uint8_t d = uint8_t(next);
            sleep_ms(d);
        }
        b_len--;
    }
}


// pulse hardware reset signal to controller
void St7796::hw_reset()
{
    if (_rst_pin >= 0) {
        gpio_put(_rst_pin, rst_assert);
        sleep_ms(2);
        gpio_put(_rst_pin, rst_deassert);
    }
}


// Allow for pwm backlight some day: 'brightness_pct' is 0%-100% brightness.
// For now, zero turns it off, nonzero turns it on.
void St7796::brightness(int brightness_pct)
{
    xassert(0 <= brightness_pct && brightness_pct <= 100);
    _brightness_pct = brightness_pct;
    if (_bk_pin >= 0)
        gpio_put(_bk_pin, brightness_pct > 0);
}


void St7796::rotation(enum Rotation rot)
{
    _rotation = rot;
    if (_rotation == Rotation::bottom || _rotation == Rotation::top) {
        // portrait
        _width = phys_wid;
        _height = phys_hgt;
    } else {
        // landscape
        _width = phys_hgt;
        _height = phys_wid;
    }

    wait_idle(); // wait for any queued dmas to finish

    spi_set_format(_spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    spi_write_command(St7796Cmd::MADCTL);
    spi_write_data(madctl());
}


// MADCTL: top three bits control orientation and y/row direction
//   80 MY  row address order
//   40 MX  column address order
//   20 MV  row/column exchange
//   10 ML  vertical refresh order (always 0)
//   08 RGB RGB-BGR order (always 1)
//   04 MH  horizontal refresh order (always 0)
uint8_t St7796::madctl() const
{
    if (_rotation == Rotation::bottom) {
        return 0x48;
    } else if (_rotation == Rotation::left) {
        return 0xe8;
    } else if (_rotation == Rotation::top) {
        return 0x88;
    } else {
        xassert(_rotation == Rotation::right);
        return 0x28;
    }
}


void St7796::set_window(uint16_t hor, uint16_t ver, uint16_t wid, uint16_t hgt)
{
    // Timings with DbgGpio show this takes about 8.5 usec at 12.5 MHz
    // (theoretical wire time is 6.4 usec).

    //DbgGpio d(28);

    spi_set_format(_spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    spi_write_command(St7796Cmd::CASET);

    const uint h2 = hor + wid - 1;
    spi_write_data(uint8_t(hor >> 8), uint8_t(hor), uint8_t(h2 >> 8),
                   uint8_t(h2));

    spi_write_command(St7796Cmd::RASET);

    const uint v2 = ver + hgt - 1;
    spi_write_data(uint8_t(ver >> 8), uint8_t(ver), uint8_t(v2 >> 8),
                   uint8_t(v2));
}


void St7796::pixel(int hor, int ver, const Color c)
{
    wait_idle();

    set_window(hor, ver, 1, 1); // sets to 8-bit spi
    spi_write_command(St7796Cmd::RAMWR);
    const Pixel565 p = c; // Pixel565::operator= converts from Color
    spi_write_data(p.value());
}


// line extends right from ('h', 'v') for 'len' pixels
void St7796::hline(int h, int v, int wid, const Color c)
{
    fill_rect(h, v, wid, 1, c);
}


// line extends up or down (positive direction) from ('h', 'v') for 'len' pixels
void St7796::vline(int h, int v, int hgt, const Color c)
{
    fill_rect(h, v, 1, hgt, c);
}


void St7796::line(int h1, int v1, int h2, int v2, const Color c)
{
    if (h1 == h2) {
        if (v1 > v2)
            std::swap(v1, v2);
        vline(h1, v1, v2 - v1 + 1, c);
    } else if (v1 == v2) {
        if (h1 > h2)
            std::swap(h1, h2);
        hline(h1, v1, h2 - h1 + 1, c);
    } else {
        Framebuffer::line(h1, v1, h2, v2, c);
    }
}


// ('hor', 'ver') is the top left pixel
// 'wid' and 'hgt' are the number of pixels in each direction
void St7796::draw_rect(int hor, int ver, int wid, int hgt, const Color c)
{
    // no need to paint the corner pixels twice
    hline(hor, ver, wid - 1, c);               // top
    vline(hor + wid - 1, ver, hgt - 1, c);     // right
    hline(hor + 1, ver + hgt - 1, wid - 1, c); // bottom
    vline(hor, ver + 1, hgt - 1, c);           // left
}


void St7796::dma_handler()
{
    spi_wait();

    // anything new to do?
    if (ops_empty()) {
        busy(false);
    } else {
        if (_ops[_op_next].op == AsyncOp::Fill) {
            const int hor = _ops[_op_next].hor;
            const int ver = _ops[_op_next].ver;
            const int wid = _ops[_op_next].wid;
            const int hgt = _ops[_op_next].hgt;
            _dma_pixel = _ops[_op_next].pixel;
            __dmb(); // _dma_pixel must be in memory before starting dma
            set_window(hor, ver, wid, hgt); // sets to 8-bit spi
            spi_write_command(St7796Cmd::RAMWR);
            data();
            spi_set_format(_spi, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
            channel_config_set_read_increment(&_dma_cfg, false);
            dma_channel_configure(_dma_ch, &_dma_cfg, &spi_get_hw(_spi)->dr,
                                  &_dma_pixel, wid * hgt, true); // go!
        } else if (_ops[_op_next].op == AsyncOp::Copy) {
            const int hor = _ops[_op_next].hor;
            const int ver = _ops[_op_next].ver;
            const int wid = _ops[_op_next].wid;
            const int hgt = _ops[_op_next].hgt;
            const void *pixels = _ops[_op_next].pixels;
            set_window(hor, ver, wid, hgt); // sets to 8-bit spi
            spi_write_command(St7796Cmd::RAMWR);
            data();
            spi_set_format(_spi, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
            channel_config_set_read_increment(&_dma_cfg, true);
            dma_channel_configure(_dma_ch, &_dma_cfg, &spi_get_hw(_spi)->dr,
                                  pixels, wid * hgt, true); // go!
        } else {
            xassert(false); // only Fill and Copy
        }
        op_next_inc();
    }
}


// ('hor', 'ver') is the top left pixel
// 'wid' and 'hgt' are the number of pixels in each direction
void St7796::fill_rect(int hor, int ver, int wid, int hgt, const Color c)
{
    int h2 = hor + wid - 1;
    if (h2 >= width())
        h2 = width() - 1;
    if (hor > h2)
        return;
    wid = h2 - hor + 1;

    int v2 = ver + hgt - 1;
    if (v2 >= height())
        v2 = height() - 1;
    if (ver > v2)
        return;
    hgt = v2 - ver + 1;

    if (ops_full()) {
        // wait for space
        _ops_stall_cnt++;
        while (ops_full())
            tight_loop_contents();
    }

    Pixel565 p = c; // Pixel565::operator= converts from Color

    _ops[_op_free].op = AsyncOp::Fill;
    _ops[_op_free].hor = uint16_t(hor);
    _ops[_op_free].ver = uint16_t(ver);
    _ops[_op_free].wid = uint16_t(wid);
    _ops[_op_free].hgt = uint16_t(hgt);
    _ops[_op_free].pixel = p.value();

    // _ops[] must be visible in memory (to isr) before updating _op_free
    __dmb();

    uint32_t irq_state = save_and_disable_interrupts();

    op_free_inc();

    // force interrupt to start if it's there's not something already running
    if (!busy()) {
        dma_irqn_mux_force(0, _dma_ch, true);
        busy(true);
    }

    restore_interrupts(irq_state);

} // void St7796::fill_rect


// write array of pixels to screen
// ('hor', 'ver') is the top left pixel
// 'pixels' points to a PixelImage, which contains width, height, and the
// pixel data. It cannot be reused or reallocated until the write is finished,
// which will be some time after this function returns.
void St7796::write(int hor, int ver, const void *image)
{
    // We don't know the height and width until we look inside 'image'.
    PixelImageInfo *pi = (PixelImageInfo *)image;

    // can't start off the left edge or above the top
    if (hor < 0 || ver < 0)
        return;

    // Don't try to go past right edge. Since (hor + wid) is the first pixel
    // after the one we're writing, (hor + wid) == width is okay
    if ((hor + pi->wid) > width())
        return;

    // Don't try to go past bottom edge.
    if ((ver + pi->hgt) > height())
        return;

    if (ops_full()) {
        // wait for space
        _ops_stall_cnt++;
        while (ops_full())
            tight_loop_contents();
    }

    const void *pixels = (Pixel565 *)(pi->pixels);

    // if pixels is in XIP memory, use non-cached access
    if (is_xip(pixels))
        pixels = xip_nocache(pixels);

    _ops[_op_free].op = AsyncOp::Copy;
    _ops[_op_free].hor = uint16_t(hor);
    _ops[_op_free].ver = uint16_t(ver);
    _ops[_op_free].wid = uint16_t(pi->wid);
    _ops[_op_free].hgt = uint16_t(pi->hgt);
    _ops[_op_free].pixels = pixels;

    // _ops[] must be visible in memory (to isr) before updating _op_free
    __dmb();

    uint32_t irq_state = save_and_disable_interrupts();

    op_free_inc();

    // force interrupt to start if it's there's not something already running
    if (!busy()) {
        dma_irqn_mux_force(0, _dma_ch, true);
        busy(true);
    }

    restore_interrupts(irq_state);

} // St7796::write


// Print one character to screen
//
// 'hor', 'ver' top left pixel of the character cell
// 'c'          character to print
// 'font'       font to use
// 'fg', 'bg'   the foreground (glyph) and background (margin) colors
// 'align'      +1 for left (default), 0 for center, -1 for right
//
// The combination of 'c' and 'font' determine how big the character cell is.
//
// If the character would extend off the screen, nothing is printed.
//
// It doesn't make sense to do this asynchronously (with dma) because of the
// rendering of the glyph into _pix_buf. Even if _pix_buf were big enough to
// hold the entire character, we'd have to wait for the dma to finish before
// reusing _pix_buf for the next character.
void St7796::print(int hor, int ver, char c, const Font &font, //
                   const Color fg, const Color bg, int align)
{
    if (!font.printable(c))
        return;

    int ci = int(c);

    // align: +1 for left (default), 0 for center, -1 for right
    if (align <= 0) {
        // Right-aligned or centered, back up horizontal location.
        int adjust = font.info[ci].x_adv; // char width in pixels
        if (align == 0)
            adjust = adjust / 2; // centered
        hor -= adjust;
    }

    // Can't start off the left edge or above the top.
    if (hor < 0 || ver < 0)
        return;

    // Start of glyph data - each byte is a grayscale.
    const uint8_t *gs = font.data + font.info[ci].off;

    // These are used to "raster through" the glyph data.
    const int8_t x_off = font.info[ci].x_off;
    const int8_t y_off = font.info[ci].y_off;
    const int8_t wid = font.info[ci].w;
    const int8_t hgt = font.info[ci].h;
    const int8_t x_adv = font.info[ci].x_adv;

    // Don't try to go past right edge. Since (hor + x_adv) is the first pixel
    // after the one we're printing, (hor + x_adv) == width is okay.
    if ((hor + x_adv) > width())
        return;

    // Don't try to go past bottom edge.
    if ((ver + font.y_adv) > height())
        return;

    // The character's 'box' is [hor...hor+x_adv) horizontally, and
    // [ver...ver+y_adv) vertically; the pixel at (hor, ver) will be filled,
    // and the pixel at (hor+x_adv, ver+y_adv) will not.
    //
    // But the glyph does not necessarly fit completely in the box (and often
    // doesn't); any of these can be true, and we just crop any part of the
    // glyph outside the character's box:
    //   x_off can be negative - glyph extends left of 'hor'
    //   y_off can be negative - glyph extends above 'ver'
    //   x_off + wid can extend past x_adv
    //   y_off + hgt can extend below y_adv
    //
    // Fonts that make a habit of extending outside the character box don't
    // render nicely. Many do it occasionally and you don't notice.

    // Wait for any queued dmas to finish.
    wait_idle();

    // Set spi transfer window - all pixels in this window will be filled.
    set_window(hor, ver, font.info[ci].x_adv, font.y_adv); // sets to 8-bit spi

    const uint8_t cmd = St7796Cmd::RAMWR;
    command();
    spi_write_blocking(_spi, &cmd, 1);
    data();

    // Iterate through entire character box. Margins around the glyph are
    // background. Pixels in the glyph are interpolated from the glyph data.
    // As we iterate through the character box, we gradually fill up the
    // working buffer _pix_buf, which might be smaller than the character box.
    // When _pix_buf fills up, we write it out to the display then start over.

    // Assigning to _pix_buf[] from Color uses Pixel565::operator=.

    spi_set_format(_spi, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    Pixel565 bg_pix = bg; // convert once

    int p = 0; // indexes through _pix_buf
    // row, col covers character box
    for (int row = 0; row < font.y_adv; row++) {
        for (int col = 0; col < x_adv; col++) {
            // See if the glyph covers this pixel.
            if (row >= y_off && row < (y_off + hgt) && //
                col >= x_off && col < (x_off + wid)) {
                // Yes.
                int g_row = row - y_off;
                xassert(g_row >= 0 && g_row < hgt);
                int g_col = col - x_off;
                xassert(g_col >= 0 && g_col < wid);
                uint8_t gray = gs[g_row * wid + g_col];
                _pix_buf[p++] = Color::interpolate(gray, bg, fg);
            } else {
                // No, outside glyph's margins.
                _pix_buf[p++] = bg_pix;
            }
            if (p >= _pix_buf_len) {
                // Send buffer and restart it.
                spi_write16_blocking(_spi, (const uint16_t *)(_pix_buf),
                                     _pix_buf_len);
                p = 0;
            }
        }
    }
    // Send final (partial) buffer if necessary.
    if (p > 0)
        spi_write16_blocking(_spi, (const uint16_t *)(_pix_buf), p);
}
