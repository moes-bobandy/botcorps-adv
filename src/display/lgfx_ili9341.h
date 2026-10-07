#pragma once

/*
 * External ILI9341 (320x240 landscape) on Cardputer Adv EXT, SPI3.
 * Compiled only into BOTCORPS_DUAL_SCREEN builds.
 *
 * COLMOD 0x55 (RGB565) is in the boot init list. Call setColorDepth(16)
 * once, before begin(). Never again, and never write 0x3A mid-run —
 * that races startWrite on the bus shared with SD.
 *
 * Panel memory is 240x320. offset_rotation 4 plus setRotation(3)
 * addresses it as landscape 320x240. Content painters use that full
 * view. Do not clip them to the internal 240x135.
 *
 * Cap LoRa1262 (NSS G5 / RST G3 / BUSY G6) and Hydra-on-G5 cannot
 * coexist with these pins. See botcorps_pins.h.
 */

#include "display/botcorps_pins.h"

#include <M5GFX.h>
#include <lgfx/v1/panel/Panel_LCD.hpp>
#include <driver/spi_common.h>

struct Botcorps_Panel_ILI9341 : public lgfx::v1::Panel_LCD {
    Botcorps_Panel_ILI9341(void)
    {
        _cfg.memory_width  = _cfg.panel_width  = 240;
        _cfg.memory_height = _cfg.panel_height = 320;
    }

protected:
    static constexpr uint8_t CMD_PWCTR1  = 0xC0;
    static constexpr uint8_t CMD_PWCTR2  = 0xC1;
    static constexpr uint8_t CMD_VMCTR1  = 0xC5;
    static constexpr uint8_t CMD_VMCTR2  = 0xC7;
    static constexpr uint8_t CMD_FRMCTR1 = 0xB1;
    static constexpr uint8_t CMD_DFUNCTR = 0xB6;
    static constexpr uint8_t CMD_GMCTRP1 = 0xE0;
    static constexpr uint8_t CMD_GMCTRN1 = 0xE1;
    static constexpr uint8_t CMD_PIXFMT  = 0x3A;

    const uint8_t *getInitCommands(uint8_t listno) const override
    {
        /* Boot-only. 0x55 is RGB565. Gamma matches the ILI9341 sequence
         * used on this Adv EXT harness. */
        static constexpr uint8_t list0[] = {
            CMD_PWCTR1,  1, 0x23,
            CMD_PWCTR2,  1, 0x10,
            CMD_VMCTR1,  2, 0x3E, 0x28,
            CMD_VMCTR2,  1, 0x86,
            CMD_PIXFMT,  1, 0x55,
            CMD_FRMCTR1, 2, 0x00, 0x18,
            CMD_DFUNCTR, 3, 0x08, 0x82, 0x27,
            CMD_GMCTRP1, 15,
                0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
                0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00,
            CMD_GMCTRN1, 15,
                0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
                0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F,
            CMD_SLPOUT, 0 + CMD_INIT_DELAY, 120,
            CMD_IDMOFF, 0,
            CMD_DISPON, 0 + CMD_INIT_DELAY, 100,
            0xFF, 0xFF,
        };
        switch (listno) {
        case 0: return list0;
        default: return nullptr;
        }
    }
};

class Botcorps_LGFX_ILI9341 : public lgfx::v1::LGFX_Device {
    Botcorps_Panel_ILI9341 _panel;
    lgfx::v1::Bus_SPI _bus;

public:
    Botcorps_LGFX_ILI9341(void)
    {
        auto bus = _bus.config();
        /* IDF SPI3_HOST (value 2). Not Arduino HSPI (value 1). On ESP32-S3
         * those numbers differ and both names mean the SPI3 peripheral.
         * The internal ST7789 stays on SPI2. */
        bus.spi_host = SPI3_HOST;
        bus.spi_mode = 0;
        bus.freq_write = 40000000;
        bus.freq_read = 16000000;
        bus.spi_3wire = true;
        bus.use_lock = true;
        /* Non-zero: M5GFX promotes this to SPI_DMA_CH_AUTO on S3. */
        bus.dma_channel = 1;
        bus.pin_sclk = BOTCORPS_PIN_SCK;
        bus.pin_mosi = BOTCORPS_PIN_MOSI;
        bus.pin_miso = -1;
        bus.pin_dc = BOTCORPS_PIN_LCD_DC;
        _bus.config(bus);
        _panel.setBus(&_bus);

        auto cfg = _panel.config();
        cfg.pin_cs = BOTCORPS_PIN_LCD_CS;
        cfg.pin_rst = BOTCORPS_PIN_LCD_RST;
        cfg.bus_shared = true;
        cfg.readable = false;
        cfg.invert = false;
        cfg.rgb_order = false;
        cfg.dlen_16bit = false;
        cfg.memory_width = 240;
        cfg.memory_height = 320;
        cfg.panel_width = 240;
        cfg.panel_height = 320;
        cfg.offset_x = 0;
        cfg.offset_y = 0;
        cfg.offset_rotation = 4;
        cfg.dummy_read_pixel = 8;
        cfg.dummy_read_bits = 1;
        _panel.config(cfg);

        setPanel(&_panel);
    }
};
