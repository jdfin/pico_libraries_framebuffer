
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
#include "hy35.h"

using namespace St7796Cmd;


Hy35::Hy35(spi_inst_t *spi, int miso_pin, int mosi_pin, int clk_pin, int cs_pin,
           int baud, int cd_pin, int rst_pin, int bk_pin, int width, int height,
           void *work, int work_bytes) :
    Tft(spi, miso_pin, mosi_pin, clk_pin, cs_pin, baud, cd_pin, rst_pin, bk_pin,
        width, height, work, work_bytes)
{
}


void Hy35::init()
{
    hw_reset(2000);

    // after hw_reset:
    //   no sleep does not work
    //   sleeping 1 msec works
    //   sample code sleeps 200 msec
    sleep_ms(10);

    const uint16_t cmds[] = {
        // clang-format off
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
uint8_t Hy35::madctl() const
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
