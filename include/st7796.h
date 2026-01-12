#pragma once

#include <cstdint>
//
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "xassert.h"
//
#include "color.h"
#include "font.h"
#include "framebuffer.h"
#include "pixel_565.h"
#include "spi_extra.h"
#include "st7796_cmd.h"


// It's not difficult to handle either 8-bit or 16-bit pixel transfers, but
// the code is simpler if we just always require 16-bit pixel transfers.
static_assert(Pixel565::xfer_size == 16,
              "St7796: Pixel565::xfer_size must be 16");

class St7796 : public Framebuffer
{

public:

    // baud normally 15'000'000
    St7796(spi_inst_t *spi, int miso_pin, int mosi_pin, int clk_pin,  //
           int cs_pin, int baud, int cd_pin, int rst_pin, int bk_pin, //
           int width, int height, void *work = nullptr, int work_bytes = 0);

    virtual ~St7796();

    void init();

    void hw_reset();

    // brightness_pct: 0..100
    // (For now, zero turns it off, nonzero turns it on)
    virtual void brightness(int brightness) override;

    uint32_t spi_freq() const
    {
        return _spi_freq;
    }

    virtual void set_rotation(Rotation r) override;

    virtual void pixel(int h, int v, const Color c) override;

    void hline(int h, int v, int wid, const Color c);

    void vline(int h, int v, int hgt, const Color c);

    // line from one absolute point to another
    virtual void line(int h1, int v1, int h2, int v2, const Color c) override;

    virtual void draw_rect(int h, int v, int wid, int hgt,
                           const Color c) override;

    virtual void fill_rect(int h, int v, int wid, int hgt,
                           const Color c) override;

    // Write array of pixels to screen.
    // 'pixels' is a pointer to a PixelImage<Pixel565, wid, hgt>, and we use a
    // PixelImageInfo to get wid and hgt.
    void write(int hor, int ver, const void *pixels) override;

    // print character to screen
    virtual void print(int h, int v, char c, const Font &font, //
                       const Color fg, const Color bg, int align) override;

private:

    spi_inst_t *_spi;

    uint32_t _spi_freq;

    // spi pins
    int _miso_pin, _mosi_pin, _clk_pin, _cs_pin;
    int _baud;

    // control/data
    int _cd_pin;

    // hardware reset
    int _rst_pin;

    static constexpr bool rst_assert = false;
    static constexpr bool rst_deassert = true;

    static constexpr bool gpio_in = false;
    static constexpr bool gpio_out = true;

    // backlight
    int _bk_pin;

    uint _dma_ch;
    dma_channel_config _dma_cfg;

    volatile bool _dma_running; // main/isr shared

    bool busy() const
    {
        return _dma_running;
    }

    void busy(bool bz)
    {
        _dma_running = bz;
    }

    // Wait for all pending dma operations to complete
    void wait_idle()
    {
        while (busy())
            tight_loop_contents();
    }

    volatile uint16_t _dma_pixel; // isr/dma shared

    // DMA interrupts: The mux in dma_irq_mux.c handles dma interrupts.
    // Calling dma_irqn_mux_connect() connnects our handler to interrupts for
    // our channel. When we connect to the mux, we provide a void* argument
    // that is passed back to the handler on each interrupt. We this 'this'
    // so the static handler (dma_raw_handler) can call the instance method
    // (dma_handler).

    // static handler called by dma_irq_mux.c
    static void dma_raw_handler(void *arg)
    {
        ((St7796 *)arg)->dma_handler();
    }

    // instance method called by static handler
    void dma_handler();

    // Calculate MADCTL value for current rotation.
    uint8_t madctl() const;

    // Working buffer used to render character. Any size is okay, but bigger
    // means fewer transfers. Supplied to constructor.
    static_assert(sizeof(Pixel565) == sizeof(uint16_t));
    Pixel565 *_pix_buf;
    int _pix_buf_len; // number of pixels

    static constexpr bool cs_assert = false;
    static constexpr bool cs_deassert = true;

    static constexpr bool cd_gpio_command = false;
    static constexpr bool cd_gpio_data = true;

    void data()
    {
        gpio_put(_cd_pin, cd_gpio_data);
    }
    void command()
    {
        gpio_put(_cd_pin, cd_gpio_command);
    }

    // write_cmds() takes an array of uint16_t. Each entry uses the upper
    // byte to indicate command, data, or delay, and the lower byte is an
    // associated value. The wr_* constants are or'ed into the upper byte,
    // and a byte of command, data, or delay value is in the lower byte.

    static constexpr uint16_t wr_data = 0x0000;     // lsb is data
    static constexpr uint16_t wr_cmd = 0x0100;      // lsb is command
    static constexpr uint16_t wr_delay_ms = 0x0200; // lsb is ms
    static constexpr uint16_t wr_mask = 0xff00;

    void write_cmds(const uint16_t *b, int b_len);

    void set_window(uint16_t hor, uint16_t ver, uint16_t wid, uint16_t hgt);

    inline void spi_wait()
    {
        while (spi_is_busy(_spi))
            tight_loop_contents();
    }

    inline void spi_write_command(uint8_t b0)
    {
        xassert(spi_get_bits(_spi) == 8);
        command();
        spi_get_hw(_spi)->dr = b0;
        spi_wait();
    }

    inline void spi_write_data(uint8_t b0)
    {
        xassert(spi_get_bits(_spi) == 8);
        data();
        spi_get_hw(_spi)->dr = b0;
        spi_wait();
    }

    inline void spi_write_data(uint16_t p0)
    {
        xassert(spi_get_bits(_spi) == 8);
        data();
        spi_get_hw(_spi)->dr = (uint32_t)(p0 >> 8);
        spi_get_hw(_spi)->dr = (uint32_t)p0;
        spi_wait();
    }

    inline void spi_write_data(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3)
    {
        xassert(spi_get_bits(_spi) == 8);
        data();
        spi_get_hw(_spi)->dr = (uint32_t)b0;
        spi_get_hw(_spi)->dr = (uint32_t)b1;
        spi_get_hw(_spi)->dr = (uint32_t)b2;
        spi_get_hw(_spi)->dr = (uint32_t)b3;
        spi_wait();
    }

    // async support

    static const int op_max = 4;

    int _ops_stall_cnt; // times we had to wait for space in _ops[]

    enum class AsyncOp : uint8_t { None, Fill, Copy, Max };

    volatile struct {
        AsyncOp op;
        uint16_t hor, ver; // top left corner
        uint16_t wid, hgt; // rectangle to fill or copy
        union {
            uint16_t pixel;     // pixel to fill with
            const void *pixels; // pixels to copy from
        };
    } _ops[op_max]; // main/isr shared

    volatile int _op_next; // index of next command to execute (main/isr shared)
    volatile int _op_free; // index of next free slot (main/isr shared)
    // op_next == op_free means empty
    bool ops_empty()
    {
        return _op_next == _op_free;
    }
    // op_free + 1 == op_next (mod op_max) means full
    bool ops_full()
    {
        return ((_op_free + 1) % op_max) == _op_next;
    }
    void op_next_inc()
    {
        _op_next = ((_op_next + 1) % op_max);
    }
    void op_free_inc()
    {
        _op_free = ((_op_free + 1) % op_max);
    }
};
