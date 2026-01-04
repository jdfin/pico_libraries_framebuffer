#pragma once

#include <cstdint>

namespace St7796Cmd {

// Command Table 1
constexpr uint8_t NOP = 0x00;       // No Op
constexpr uint8_t SWRESET = 0x01;   // Software Reset
constexpr uint8_t RDDID = 0x04;     // Read Display ID
constexpr uint8_t RDNUMED = 0x05;   // Read Number of the Errors on DSI
constexpr uint8_t RDDST = 0x09;     // Read Display Status
constexpr uint8_t RDDPM = 0x0a;     // Read Display Power Mode
constexpr uint8_t RDDMADCTL = 0x0b; // Read Display MADCTL
constexpr uint8_t RDDCOLMOD = 0x0c; // Read Display Pixel Format
constexpr uint8_t RDDIM = 0x0d;     // Read Display Image Mode
constexpr uint8_t RDDSM = 0x0e;     // Read Display Signal Mode
constexpr uint8_t RDDSDR = 0x0f;    // Read Display Self-Diagnostic Result
constexpr uint8_t SLPIN = 0x10;     // Sleep in
constexpr uint8_t SLPOUT = 0x11;    // Sleep Out
constexpr uint8_t PTLON = 0x12;     // Partial Display Mode On
constexpr uint8_t NORON = 0x13;     // Normal Display Mode On
constexpr uint8_t INVOFF = 0x20;    // Display Inversion Off
constexpr uint8_t INVON = 0x21;     // Display Inversion On
constexpr uint8_t DISPOFF = 0x28;   // Display Off
constexpr uint8_t DISPON = 0x29;    // Display On
constexpr uint8_t CASET = 0x2a;     // Column Address Set
constexpr uint8_t RASET = 0x2b;     // Row Address Set
constexpr uint8_t RAMWR = 0x2c;     // Memory Write
constexpr uint8_t RAMRD = 0x2e;     // Memory Read
constexpr uint8_t PTLAR = 0x30;     // Partial Area
constexpr uint8_t VSCRDEF = 0x33;   // Vertical Scrolling Definition
constexpr uint8_t TEOFF = 0x34;     // Tearing Effect Line OFF
constexpr uint8_t TEON = 0x35;      // Tearing Effect Line On
constexpr uint8_t MADCTL = 0x36;    // Memory Data Access Control
constexpr uint8_t VSCSAD = 0x37;    // Vertical Scroll Start Address of RAM
constexpr uint8_t IDMOFF = 0x38;    // Idle Mode Off
constexpr uint8_t IDMON = 0x39;     // Idle mode on
constexpr uint8_t COLMOD = 0x3a;    // Interface Pixel Format
constexpr uint8_t WRMEMC = 0x3c;    // Write Memory Continue
constexpr uint8_t RDMEMC = 0x3e;    // Read Memory Continue
constexpr uint8_t STE = 0x44;       // Set Tear Scanline
constexpr uint8_t GSCAN = 0x45;     // Get Scanline
constexpr uint8_t WRDISBV = 0x51;   // Write Display Brightness
constexpr uint8_t RDDISBV = 0x52;   // Read Display Brightness Value
constexpr uint8_t WRCTRLD = 0x53;   // Write CTRL Display
constexpr uint8_t RDCTRLD = 0x54;   // Read CTRL value Display
constexpr uint8_t WRCABC = 0x55;    // Write Adaptive Brightness Control
constexpr uint8_t RDCABC = 0x56;    // Read Content Adaptive Brightness Control
constexpr uint8_t WRCABCMB = 0x5e;  // Write CABC Minimum Brightness
constexpr uint8_t RDCABCMB = 0x5f;  // Read CABC Minimum Brightness
constexpr uint8_t RDFCS = 0xaa;     // Read First Checksum
constexpr uint8_t RDCFCS = 0xaf;    // Read Continue Checksum
constexpr uint8_t RDID1 = 0xda;     // Read ID1
constexpr uint8_t RDID2 = 0xdb;     // Read ID2
constexpr uint8_t RDID3 = 0xdc;     // Read ID3

// Command Table 2
constexpr uint8_t IFMODE = 0xb0;   // Interface Mode Control
constexpr uint8_t FRMCTR1 = 0xb1;  // Frame Rate Control
constexpr uint8_t FRMCTR2 = 0xb2;  // Frame Rate Control 2
constexpr uint8_t FRMCTR3 = 0xb3;  // Frame Rate Control 3
constexpr uint8_t DIC = 0xb4;      // Display Inversion Control
constexpr uint8_t BPC = 0xb5;      // Blanking Porch Control
constexpr uint8_t DFC = 0xb6;      // Display Function Control
constexpr uint8_t EM = 0xb7;       //Entry Mode Set
constexpr uint8_t PWR1 = 0xc0;     // Power Control 1
constexpr uint8_t PWR2 = 0xc1;     // Power Control 2
constexpr uint8_t PWR3 = 0xc2;     // Power Control 3
constexpr uint8_t VCMPCTL = 0xc5;  // VCOM Control
constexpr uint8_t VCMOFF = 0xc6;   // Vcom Offset Register
constexpr uint8_t NVMADW = 0xd0;   // NVM Address/Data Write
constexpr uint8_t NVMBPROG = 0xd1; // NVM Byte Program
constexpr uint8_t NVMSTRD = 0xd2;  // NVM Status Read
constexpr uint8_t RDID4 = 0xd3;    // Read ID4
constexpr uint8_t PGC = 0xe0;      // Positive Gamma Control
constexpr uint8_t NGC = 0xe1;      // Negative Gamma Control
constexpr uint8_t DGC1 = 0xe2;     // Digital Gamma Control 1
constexpr uint8_t DGC2 = 0xe3;     // Digital Gamma Control 2
constexpr uint8_t DOCA = 0xe8;     // Display Output Ctrl Adjust
constexpr uint8_t CSCON = 0xf0;    // Command Set Control
constexpr uint8_t SPIRC = 0xfb;    // SPI Read Control

}; // namespace St7796Cmd
