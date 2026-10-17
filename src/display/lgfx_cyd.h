#pragma once

#include <LovyanGFX.hpp>

/* CYD ESP32-2432S028 ILI9341 + XPT2046.
 * Display HSPI: MOSI13 SCLK14 CS15 DC2 RST-1 BL21
 * Touch VSPI: CLK25 MOSI32 MISO39 CS33 IRQ36
 */

class Botcorps_LGFX_CYD : public lgfx::LGFX_Device {
    lgfx::Panel_ILI9341 _panel;
    lgfx::Bus_SPI _bus;
    lgfx::Light_PWM _light;
    lgfx::Touch_XPT2046 _touch;

public:
    Botcorps_LGFX_CYD(void) {
        {
            auto cfg = _bus.config();
            cfg.spi_host = HSPI_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 40000000;
            cfg.freq_read = 16000000;
            /* 4-wire. MISO is GPIO12 and the panel is readable.
             * spi_3wire=true would ignore MISO and fight readable=true. */
            cfg.spi_3wire = false;
            cfg.use_lock = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_sclk = 14;
            cfg.pin_mosi = 13;
            cfg.pin_miso = 12;
            cfg.pin_dc = 2;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto cfg = _panel.config();
            cfg.pin_cs = 15;
            cfg.pin_rst = -1;
            cfg.pin_busy = -1;
            cfg.memory_width = 240;
            cfg.memory_height = 320;
            cfg.panel_width = 240;
            cfg.panel_height = 320;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            /* Match setRotation(1) in botcorps_dual_begin. LovyanGFX adds
             * this to the touch transform. Confirm on a device; NVS "trot"
             * overrides it. See botcorps_cyd_load_touch_cal(). */
            cfg.offset_rotation = 1;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits = 1;
            cfg.readable = true;
            cfg.invert = false;
            cfg.rgb_order = false;
            cfg.dlen_16bit = false;
            cfg.bus_shared = false;
            _panel.config(cfg);
        }
        {
            auto cfg = _light.config();
            cfg.pin_bl = 21;
            cfg.invert = false;
            cfg.freq = 44100;
            cfg.pwm_channel = 7;
            _light.config(cfg);
            _panel.setLight(&_light);
        }
        {
            auto cfg = _touch.config();
            /* Uncalibrated placeholders. A stored NVS cal replaces these
             * before begin(). Do not treat 300..3900 as a measured panel. */
            cfg.x_min = 300;
            cfg.x_max = 3900;
            cfg.y_min = 300;
            cfg.y_max = 3900;
            cfg.pin_int = 36;
            cfg.bus_shared = false;
            cfg.offset_rotation = 0;
            cfg.spi_host = VSPI_HOST;
            cfg.freq = 1000000;
            cfg.pin_sclk = 25;
            cfg.pin_mosi = 32;
            cfg.pin_miso = 39;
            cfg.pin_cs = 33;
            _touch.config(cfg);
            _panel.setTouch(&_touch);
        }
        setPanel(&_panel);
    }

    /* Call before begin(). Values are the raw XPT2046 range. */

    /* Raw XPT2046 for calibration. Returns true if a point was read. */
    bool get_touch_raw(int *x, int *y)
    {
        lgfx::touch_point_t tp;
        if (!_touch.getTouchRaw(&tp, 1)) return false;
        *x = tp.x;
        *y = tp.y;
        return true;
    }
    /* 8-point calibrate (TL,TR,BR,BL raw x,y). Leaves offset_rotation at constructor value. */
    void apply_touch_cal8(const uint16_t cal[8])
    {
        /* LovyanGFX setTouchCalibrate expects the 8 raw values for the four corners. */
        _touch.setCalibrate(cal);
        /* Ensure the matrix is live. */
        _panel.setTouch(&_touch);
    }
    /* Legacy range path kept for load of old NVS; still rebuilds matrix, fixed rot. */
    void apply_touch_cal(uint16_t xmin, uint16_t xmax, uint16_t ymin, uint16_t ymax,
                         uint8_t /*offset_rot*/)
    {
        auto cfg = _touch.config();
        cfg.x_min = xmin;
        cfg.x_max = xmax;
        cfg.y_min = ymin;
        cfg.y_max = ymax;
        /* Do not override constructor offset_rotation — min>max already flips the axis. */
        _touch.config(cfg);
        _panel.setTouch(&_touch);
    }
};
