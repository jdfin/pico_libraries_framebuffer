#pragma once

#include <cstdint>
//
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "xassert.h"
//
#include "color.h"
#include "font.h"
#include "framebuffer.h"
#include "pixel_565.h"

#define USE_DMA 1


class St7796 : public Framebuffer
{

public:

    // baud normally 15'000'000
    St7796(spi_inst_t *spi, int miso_pin, int mosi_pin, int clk_pin, int cs_pin, int baud,
           int cd_pin, int rst_pin, int bk_pin, void *work=nullptr, int work_bytes=0);

    virtual ~St7796();

    void init();

    void hw_reset();

    // brightness_pct: 0..100
    // (For now, zero turns it off, nonzero turns it on)
    virtual void brightness(int brightness) override;

    uint32_t spi_freq() const { return _spi_freq; }

    // orientation (where the connector is)
    enum class Rotation {
        bottom, // portrait, connector at bottom
        left,   // landscape, connector to left
        top,    // portrait, connecter at top
        right,  // landscape, connector to right
    };

    void rotation(enum Rotation rot);
    enum Rotation rotation() const { return _rotation; }

    virtual void pixel(int h, int v, const Color c) override;

    void hline(int h, int v, int wid, const Color c);

    void vline(int h, int v, int hgt, const Color c);

    // line from one absolute point to another
    virtual void line(int h1, int v1, int h2, int v2, const Color c) override;

    virtual void draw_rect(int h, int v, int wid, int hgt,
                           const Color c) override;

    virtual void fill_rect(int h, int v, int wid, int hgt,
                           const Color c) override;

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

#if USE_DMA
    uint _dma_ch;
    dma_channel_config _dma_cfg;
#endif

    enum Rotation _rotation;

    uint8_t madctl() const;

    // this implies the "physical screen" is portrait
    static constexpr int phys_hgt = 480;
    static constexpr int phys_wid = 320;

    // Working buffer used in a few places:
    // * filling rectangles on screen (any size is okay, but bigger means
    //   fewer transfers)
    // * rendering character
    // Supplied to constructor.
    static_assert(sizeof(Pixel565) == sizeof(uint16_t));
    Pixel565 *_pix_buf;
    int _pix_buf_len; // number of pixels

    enum Cmd : uint8_t {
        // Command Table 1
        NOP = 0x00,       // No Op
        SWRESET = 0x01,   // Software Reset
        RDDID = 0x04,     // Read Display ID
        RDNUMED = 0x05,   // Read Number of the Errors on DSI
        RDDST = 0x09,     // Read Display Status
        RDDPM = 0x0a,     // Read Display Power Mode
        RDDMADCTL = 0x0b, // Read Display MADCTL
        RDDCOLMOD = 0x0c, // Read Display Pixel Format
        RDDIM = 0x0d,     // Read Display Image Mode
        RDDSM = 0x0e,     // Read Display Signal Mode
        RDDSDR = 0x0f,    // Read Display Self-Diagnostic Result
        SLPIN = 0x10,     // Sleep in
        SLPOUT = 0x11,    // Sleep Out
        PTLON = 0x12,     // Partial Display Mode On
        NORON = 0x13,     // Normal Display Mode On
        INVOFF = 0x20,    // Display Inversion Off
        INVON = 0x21,     // Display Inversion On
        DISPOFF = 0x28,   // Display Off
        DISPON = 0x29,    // Display On
        CASET = 0x2a,     // Column Address Set
        RASET = 0x2b,     // Row Address Set
        RAMWR = 0x2c,     // Memory Write
        RAMRD = 0x2e,     // Memory Read
        PTLAR = 0x30,     // Partial Area
        VSCRDEF = 0x33,   // Vertical Scrolling Definition
        TEOFF = 0x34,     // Tearing Effect Line OFF
        TEON = 0x35,      // Tearing Effect Line On
        MADCTL = 0x36,    // Memory Data Access Control
        VSCSAD = 0x37,    // Vertical Scroll Start Address of RAM
        IDMOFF = 0x38,    // Idle Mode Off
        IDMON = 0x39,     // Idle mode on
        COLMOD = 0x3a,    // Interface Pixel Format
        WRMEMC = 0x3c,    // Write Memory Continue
        RDMEMC = 0x3e,    // Read Memory Continue
        STE = 0x44,       // Set Tear Scanline
        GSCAN = 0x45,     // Get Scanline
        WRDISBV = 0x51,   // Write Display Brightness
        RDDISBV = 0x52,   // Read Display Brightness Value
        WRCTRLD = 0x53,   // Write CTRL Display
        RDCTRLD = 0x54,   // Read CTRL value Display
        WRCABC = 0x55,    // Write Adaptive Brightness Control
        RDCABC = 0x56,    // Read Content Adaptive Brightness Control
        WRCABCMB = 0x5e,  // Write CABC Minimum Brightness
        RDCABCMB = 0x5f,  // Read CABC Minimum Brightness
        RDFCS = 0xaa,     // Read First Checksum
        RDCFCS = 0xaf,    // Read Continue Checksum
        RDID1 = 0xda,     // Read ID1
        RDID2 = 0xdb,     // Read ID2
        RDID3 = 0xdc,     // Read ID3
        // Command Table 2
        IFMODE = 0xb0,   // Interface Mode Control
        FRMCTR1 = 0xb1,  // Frame Rate Control (In Normal Mode/Full Colors)
        FRMCTR2 = 0xb2,  // Frame Rate Control 2 (In Idle Mode/8 colors)
        FRMCTR3 = 0xb3,  // Frame Rate Control3 (In Partial Mode/Full Colors)
        DIC = 0xb4,      // Display Inversion Control
        BPC = 0xb5,      // Blanking Porch Control
        DFC = 0xb6,      // Display Function Control
        EM = 0xb7,       //Entry Mode Set
        PWR1 = 0xc0,     // Power Control 1
        PWR2 = 0xc1,     // Power Control 2
        PWR3 = 0xc2,     // Power Control 3
        VCMPCTL = 0xc5,  // VCOM Control
        VCMOFF = 0xc6,   // Vcom Offset Register
        NVMADW = 0xd0,   // NVM Address/Data Write
        NVMBPROG = 0xd1, // NVM Byte Program
        NVMSTRD = 0xd2,  // NVM Status Read
        RDID4 = 0xd3,    // Read ID4
        PGC = 0xe0,      // Positive Gamma Control
        NGC = 0xe1,      // Negative Gamma Control
        DGC1 = 0xe2,     // Digital Gamma Control 1
        DGC2 = 0xe3,     // Digital Gamma Control 2
        DOCA = 0xe8,     // Display Output Ctrl Adjust
        CSCON = 0xf0,    // Command Set Control
        SPIRC = 0xfb,    // SPI Read Control
    };

    static constexpr bool cs_assert = false;
    static constexpr bool cs_deassert = true;

    void select() { gpio_put(_cs_pin, cs_assert); }
    void deselect() { gpio_put(_cs_pin, cs_deassert); }

    static constexpr bool cd_gpio_command = false;
    static constexpr bool cd_gpio_data = true;

    void data() { gpio_put(_cd_pin, cd_gpio_data); }
    void command() { gpio_put(_cd_pin, cd_gpio_command); }

    static constexpr uint16_t wr_data = 0x0000;     // lsb is data
    static constexpr uint16_t wr_cmd = 0x0100;      // lsb is command
    static constexpr uint16_t wr_delay_ms = 0x0200; // lsb is ms
    static constexpr uint16_t wr_mask = 0xff00;

    void write_cmds(const uint16_t *b, int b_len);

    void set_window(uint16_t hor, uint16_t ver, uint16_t wid, uint16_t hgt);
};
