#pragma once

#include <cstdint>

// ILI9341 command bytes

namespace Ili9341Cmd {

static constexpr uint8_t SLPOUT = 0x11;
static constexpr uint8_t GAMMASET = 0x26;
static constexpr uint8_t DISPON = 0x29;
static constexpr uint8_t CASET = 0x2a;
static constexpr uint8_t RASET = 0x2b;
static constexpr uint8_t RAMWR = 0x2c;
static constexpr uint8_t RGBSET = 0x2d;
static constexpr uint8_t MADCTL = 0x36;
static constexpr uint8_t PIXSET = 0x3a;
static constexpr uint8_t SETTS = 0x44;
static constexpr uint8_t FRMCTL = 0xb1;
static constexpr uint8_t DISPCTL = 0xb6;
static constexpr uint8_t PWRCTL1 = 0xc0;
static constexpr uint8_t PWRCTL2 = 0xc1;
static constexpr uint8_t VCOMCTL1 = 0xc5;
static constexpr uint8_t VCOMCTL2 = 0xc7;
static constexpr uint8_t PWRCTLA = 0xcb;
static constexpr uint8_t PWRCTLB = 0xcf;
static constexpr uint8_t POSGAMMA = 0xe0;
static constexpr uint8_t NEGGAMMA = 0xe1;
static constexpr uint8_t DRVTMGA = 0xe8;
static constexpr uint8_t DRVTGMB = 0xea;
static constexpr uint8_t PWRSEQCTL = 0xed;
static constexpr uint8_t EN3G = 0xf2;
static constexpr uint8_t PMPCTL = 0xf7;

}; // namespace Ili9341Cmd
