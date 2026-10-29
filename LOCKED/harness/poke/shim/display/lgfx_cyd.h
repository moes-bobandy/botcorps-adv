#pragma once
/* cyd-sim replacement for src/display/lgfx_cyd.h (found first because shim/ precedes src/ on the
 * include path). Same class name and public methods the firmware calls; the ILI9341-on-HSPI bus and
 * the XPT2046-on-VSPI touch are replaced by LovyanGFX's Panel_sdl, 240x320 native like the ILI9341
 * memory (panel offset_rotation 0, so firmware setRotation(0)=portrait 240x320, (1)=landscape 320x240).
 *
 * Touch: Panel_sdl reports the mouse in native panel pixels and LovyanGFX's identity affine converts
 * it through the current rotation. The XPT2046 8-point calibration matrix is BYPASSED:
 * apply_touch_cal8 / apply_touch_cal are no-ops. Scripted taps (sim_main.cpp) inject raw points. */
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/sdl/Panel_sdl.hpp>
#include "Arduino.h"

struct SimTouch { volatile bool on = false; volatile int rx = 0, ry = 0; volatile bool mouse = true; };
extern SimTouch g_sim_touch;

class Botcorps_SimPanel : public lgfx::Panel_sdl {
public:
    uint_fast8_t getTouchRaw(lgfx::touch_point_t *tp, uint_fast8_t count) override
    {
        if (g_sim_touch.on) {
            tp->x = g_sim_touch.rx; tp->y = g_sim_touch.ry; tp->size = 1; tp->id = 0;
            return 1;
        }
        if (!g_sim_touch.mouse) return 0;
        return lgfx::Panel_sdl::getTouchRaw(tp, count);
    }
};

class Botcorps_LGFX_CYD : public lgfx::LGFX_Device {
    Botcorps_SimPanel _panel;
public:
    Botcorps_LGFX_CYD(void)
    {
        auto cfg = _panel.config();
        cfg.memory_width = 240;
        cfg.memory_height = 320;
        cfg.panel_width = 240;
        cfg.panel_height = 320;
        cfg.offset_x = 0;
        cfg.offset_y = 0;
        cfg.offset_rotation = 0;
        cfg.readable = true;
        cfg.invert = false;
        cfg.rgb_order = false;
        _panel.config(cfg);
        setPanel(&_panel);
    }
    Botcorps_SimPanel &sim_panel(void) { return _panel; }

    bool get_touch_raw(int *x, int *y)
    {
        lgfx::touch_point_t tp;
        if (!_panel.getTouchRaw(&tp, 1)) return false;
        *x = tp.x; *y = tp.y;
        return true;
    }
    uint8_t panel_offset_rotation(void) const { return 0; }
    uint8_t touch_offset_rotation(void) const { return 0; }
    void apply_touch_cal8(const uint16_t *) {}                       /* sim: calibration bypassed */
    void apply_touch_cal(uint16_t, uint16_t, uint16_t, uint16_t, uint8_t) {}
};
