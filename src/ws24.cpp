
#include <cassert>
#include <cstdint>
#include <cstdlib>
// pico
#include "hardware/spi.h"
#include "pico/stdlib.h"
// framebuffer
#include "ili9341_cmd.h"
#include "tft.h"
//
#include "util.h"
//
#include "ws24.h"

using namespace Ili9341Cmd;


Ws24::Ws24(spi_inst_t *spi, int miso_pin, int mosi_pin, int clk_pin, int cs_pin,
           int baud, int cd_pin, int rst_pin, int bk_pin, int width, int height,
           void *work, int work_bytes) :
    Tft(spi, miso_pin, mosi_pin, clk_pin, cs_pin, baud, cd_pin, rst_pin, bk_pin,
        width, height, work, work_bytes)
{
}


void Ws24::init()
{
    sleep_ms(150);
    hw_reset(20);
    sleep_ms(150);

    const uint16_t cmds[] = {
        // clang-format off
        wr_cmd | SLPOUT,
        wr_delay_ms | 150,
        wr_cmd | PWRCTLB,   0x00, 0xc1, 0x30,
        wr_cmd | PWRSEQCTL, 0x64, 0x03, 0x12, 0x81,
        wr_cmd | DRVTMGA,   0x85, 0x00, 0x79,
        wr_cmd | PWRCTLA,   0x39, 0x2c, 0x00, 0x34, 0x02,
        wr_cmd | PMPCTL,    0x20,
        wr_cmd | DRVTGMB,   0x00, 0x00,
        wr_cmd | PWRCTL1,   0x1d,
        wr_cmd | PWRCTL2,   0x12,
        wr_cmd | VCOMCTL1,  0x33, 0x3f,
        wr_cmd | VCOMCTL2,  0x92,
        wr_cmd | PIXSET,    0x55,
        wr_cmd | MADCTL,    madctl(),
        wr_cmd | FRMCTL,    0x00, 0x12,
        wr_cmd | DISPCTL,   0x0a, 0xa2,
        wr_cmd | SETTS,     0x02,
        wr_cmd | EN3G,      0x00,
        wr_cmd | GAMMASET,  0x01,
        wr_cmd | POSGAMMA,  0x0f, 0x22, 0x1c, 0x1b, 0x08, 0x0f, 0x48, 0xb8,
                            0x34, 0x05, 0x0c, 0x09, 0x0f, 0x07, 0x00,
        wr_cmd | NEGGAMMA,  0x00, 0x23, 0x24, 0x07, 0x10, 0x07, 0x38, 0x47,
                            0x4b, 0x0a, 0x13, 0x06, 0x30, 0x38, 0x0f,
        // clang-format on
    };
    const int cmds_len = sizeof(cmds) / sizeof(cmds[0]);

    write_cmds(cmds, cmds_len); // sets to 8-bit spi

    init_colors();

    const uint8_t cmd = DISPON;
    command();
    spi_write_blocking(_spi, &cmd, 1);
}


// MADCTL: top three bits control orientation and y/row direction
//   80 MY  row address order
//   40 MX  column address order
//   20 MV  row/column exchange
//   10 ML  vertical refresh order (always 0)
//   08 RGB RGB-BGR order (always 1)
//   04 MH  horizontal refresh order (always 0)
uint8_t Ws24::madctl() const
{
    if (get_rotation() == Rotation::portrait) {
        return 0x08;
    } else if (get_rotation() == Rotation::landscape) {
        return 0xa8;
    } else if (get_rotation() == Rotation::portrait2) {
        return 0xc8;
    } else {
        assert(get_rotation() == Rotation::landscape2);
        return 0x68;
    }
}


// For red and blue, the msb of the 5-bit index is used for the lsb so the
// full range is covered (lightest light and darkest dark). It's probably
// not visible.
// constexpr can be used directly from flash
static constexpr uint8_t color_lut[128] = {
    // clang-format off
    // red
    0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e,
    0x10, 0x12, 0x14, 0x16, 0x18, 0x1a, 0x1c, 0x1e,
    0x21, 0x23, 0x25, 0x27, 0x29, 0x2b, 0x2d, 0x2f,
    0x31, 0x33, 0x35, 0x37, 0x39, 0x3b, 0x3d, 0x3f,
    // green
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
    0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    // blue
    0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e,
    0x10, 0x12, 0x14, 0x16, 0x18, 0x1a, 0x1c, 0x1e,
    0x21, 0x23, 0x25, 0x27, 0x29, 0x2b, 0x2d, 0x2f,
    0x31, 0x33, 0x35, 0x37, 0x39, 0x3b, 0x3d, 0x3f,
    // clang-format on
};

// initialize 16-bit to 18-bit color mapping in controller
void Ws24::init_colors()
{
    // The table should be in flash. Not super critical, but that's the intent
    // so let's check. If it's not, a valid fix is to delete this assert and
    // the xip_nocache() call, then just spi from color_lut.
    assert(is_xip(color_lut));
    const uint8_t *color_lut_nocache = (const uint8_t *)xip_nocache(color_lut);

    command();
    uint8_t cmd = RGBSET;
    spi_write_blocking(_spi, &cmd, 1);

    data();
    spi_write_blocking(_spi, color_lut_nocache, 128);
}
