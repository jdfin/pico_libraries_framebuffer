
#include <cassert>
#include <cstdint>
#include <cstdlib>
// pico
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
// framebuffer
#include "st7796_cmd.h"
#include "tft.h"
//
#include "ws35.h"

using namespace St7796Cmd;


Ws35::Ws35(spi_inst_t *spi, int miso_pin, int mosi_pin, int clk_pin, int cs_pin,
           int baud, int cd_pin, int rst_pin, int bk_pin, int width, int height,
           void *work, int work_bytes) :
    Tft(spi, miso_pin, mosi_pin, clk_pin, cs_pin, baud, cd_pin, rst_pin, bk_pin,
        width, height, work, work_bytes)
{
}


void Ws35::init()
{
    hw_reset(2000);

    // after hw_reset:
    //   no sleep does not work
    //   sleeping 1 msec works
    //   sample code sleeps 200 msec
    sleep_ms(10);

    const uint16_t cmds[] = {
        // clang-format off
        // From the Waveshare python code
        wr_cmd | INVON,
        wr_cmd | PWR3, 0x33,
        wr_cmd | VCMPCTL, 0x00, 0x1e, 0x80,
        wr_cmd | FRMCTR1, 0xB0,
        wr_cmd | PGC, 0x00, 0x13, 0x18, 0x04, 0x0F, 0x06, 0x3a, 0x56,
                      0x4d, 0x03, 0x0a, 0x06, 0x30, 0x3e, 0x0f,
        wr_cmd | NGC, 0x00, 0x13, 0x18, 0x01, 0x11, 0x06, 0x38, 0x34,
                      0x4d, 0x06, 0x0d, 0x0b, 0x31, 0x37, 0x0f,
        wr_cmd | COLMOD, 0x55,
        wr_cmd | SLPOUT,
        wr_delay_ms | 120,
        wr_cmd | DISPON,
        wr_cmd | DFC, 0x00, 0x62,
        wr_cmd | MADCTL, madctl(),
        // clang-format on
    };
    const int cmds_len = sizeof(cmds) / sizeof(cmds[0]);

    write_cmds(cmds, cmds_len); // sets to 8-bit spi
}


// MADCTL: top three bits control orientation and y/row direction
//   80 MY  row address order
//   40 MX  column address order
//   20 MV  row/column exchange
//   10 ML  vertical refresh order (always 0)
//   08 RGB RGB-BGR order (always 1)
//   04 MH  horizontal refresh order (always 0)
uint8_t Ws35::madctl() const
{
    if (get_rotation() == Rotation::portrait) {
        return 0x48;
    } else if (get_rotation() == Rotation::landscape) {
        return 0xe8;
    } else if (get_rotation() == Rotation::portrait2) {
        return 0x88;
    } else {
        assert(get_rotation() == Rotation::landscape2);
        return 0x28;
    }
}
