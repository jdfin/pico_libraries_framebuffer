#pragma once

#include "hardware/spi.h"

// Tiny2040
//
//                     +-----| USB |-----+
//                VBUS | 1            40 | D0  MISO   (fb)
//                 GND | 2            39 | D1  CS     (fb)
//                 3V3 | 3            38 | D2  SCK    (fb)
//                 D29 | 4            37 | D3  MOSI   (fb)
//                 D28 | 5            36 | D4  CD     (fb)
//                 D27 | 6            35 | D5  BK     (fb)
//                 D26 | 7            34 | D6  RST    (fb)
//                 GND | 8            33 | D7
//                     +-----------------+

constexpr int fb_spi_miso_gpio = 0;
constexpr int fb_spi_mosi_gpio = 3;
constexpr int fb_spi_clk_gpio = 2;
constexpr int fb_spi_cs_gpio = 1;
spi_inst_t *const fb_spi_inst = spi0;

constexpr int fb_cd_gpio = 4;
constexpr int fb_rst_gpio = 6;
constexpr int fb_led_gpio = 5;
