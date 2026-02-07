#pragma once

#include <cstdint>
// pico
#include "hardware/spi.h"
#include "pico/stdlib.h"
// framebuffer
#include "tft.h"


// Hosyond 3.5" TFT, ST7796 controller, 320x480, SPI interface
class Hy35 : public Tft
{

public:

    Hy35(spi_inst_t *spi, int miso_pin, int mosi_pin, int clk_pin, int cs_pin,
         int baud, int cd_pin, int rst_pin, int bk_pin, int width, int height,
         void *work = nullptr, int work_bytes = 0);

    void init();

private:

    virtual uint8_t madctl() const;
};
