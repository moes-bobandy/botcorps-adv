#pragma once

/*
 * botcorps_display — menu / content split for Cardputer Adv.
 *
 * Single-screen (BOTCORPS_DUAL_SCREEN=0):
 *   botcorps_menu() and botcorps_content() are both M5Cardputer.Display.
 *   BotcorpsDisplay is that same panel.
 *
 * Dual (BOTCORPS_DUAL_SCREEN=1):
 *   botcorps_menu()    → internal ST7789 240×135 (M5Cardputer.Display)
 *   botcorps_content() → external ILI9341 320×240 when begin() succeeded,
 *                        otherwise the internal ST7789
 *   BotcorpsDisplay follows the *active* surface. It does not mean
 *   "send every draw to EXT". Menu chrome (later slices) calls
 *   botcorps_menu(). Feature and overlay painters call
 *   botcorps_content() or botcorps_while_content(). Both panels stay
 *   live; this is not a Phase-1 "all UI on EXT, status stub on INT".
 *
 * SCR_W / SCR_H follow the active surface. On content with the panel
 * up they are 320 and 240, so a painter fills the glass instead of a
 * 240×135 corner. They are not constant expressions on dual builds;
 * size a buffer with BOTCORPS_VIEW_MAX_*.
 *
 * Bus: botcorps_lcd_quiesce / botcorps_lcd_resume_after_bus around SD
 * on shared SCK/MOSI. RGB565 / COLMOD is boot-only.
 *
 * Pins: see botcorps_pins.h. Cap LoRa1262 (NSS G5 / RST G3 / BUSY G6)
 * and Hydra-on-G5 cannot coexist with the external panel.
 */

#ifdef BOTCORPS_CYD

#include <LovyanGFX.hpp>
#include "display/lgfx_cyd.h"

extern Botcorps_LGFX_CYD g_cyd;

enum botcorps_surface_t {
    BOTCORPS_SURFACE_MENU = 0,
    BOTCORPS_SURFACE_CONTENT = 1,
};
inline void botcorps_set_surface(botcorps_surface_t) {}
inline botcorps_surface_t botcorps_surface(void) { return BOTCORPS_SURFACE_CONTENT; }

inline lgfx::LGFX_Device &botcorps_disp(void) { return g_cyd; }
inline lgfx::LGFX_Device &botcorps_content(void) { return g_cyd; }
inline lgfx::LGFX_Device &botcorps_menu(void) { return g_cyd; }
#define BotcorpsDisplay botcorps_disp()
#define SCR_W 320
#define SCR_H 240

inline bool botcorps_dual_ok(void) { return true; }
inline void botcorps_lcd_quiesce(void) {}
inline void botcorps_lcd_resume_after_bus(void) {}
inline void botcorps_while_content(void (*fn)(void)) { if (fn) fn(); }
inline int botcorps_view_w(void) { return 320; }
inline int botcorps_view_h(void) { return 240; }
#define BOTCORPS_VIEW_MAX_W 320
#define BOTCORPS_VIEW_MAX_H 240

/* Read NVS namespace "botcorps" before the panel begins.
 * Keys (all optional): tcal=1, c0..c7 (8-point), tx0, tx1, ty0, ty1, trot.
 * Missing keys keep the 300..3900 placeholders and offset_rotation 0. */
void botcorps_cyd_load_touch_cal(void);
bool botcorps_cyd_run_touch_cal(void);
bool botcorps_cyd_has_valid_cal(void);

/* Runtime orient: 0 landscape (rot 1, 320x240), 1 portrait (rot 0, 240x320).
 * Loaded from NVS "orient" by the UI; begin() defaults landscape until UI applies it. */
extern uint8_t botcorps_cyd_orient;
inline void botcorps_cyd_apply_rotation(void) {
    g_cyd.setRotation(botcorps_cyd_orient ? 0 : 1);
}
inline bool botcorps_dual_begin(void) {
    botcorps_cyd_load_touch_cal();
    g_cyd.setColorDepth(16);
    if (!g_cyd.begin()) return false;
    botcorps_cyd_orient = 0;
    g_cyd.setRotation(1); /* landscape 320x240 until UI loads NVS orient */
    g_cyd.setTextWrap(false, false);
    g_cyd.setTextSize(1);
    g_cyd.clearClipRect();
    g_cyd.fillScreen(0x0000);
    g_cyd.setBrightness(255);
    return (g_cyd.width() == 320 && g_cyd.height() == 240);
}

#else
#include <M5Cardputer.h>
#include "display/botcorps_pins.h"

#ifndef BOTCORPS_DUAL_SCREEN
#define BOTCORPS_DUAL_SCREEN 0
#endif

enum botcorps_surface_t {
    BOTCORPS_SURFACE_MENU = 0,     /* internal ST7789 240×135 */
    BOTCORPS_SURFACE_CONTENT = 1,  /* external ILI9341, or ST7789 fallback */
};

#if BOTCORPS_DUAL_SCREEN

lgfx::LGFX_Device &botcorps_menu(void);
lgfx::LGFX_Device &botcorps_content(void);
lgfx::LGFX_Device &botcorps_disp(void);

bool botcorps_dual_begin(void);
bool botcorps_dual_ok(void);
void botcorps_lcd_quiesce(void);
void botcorps_lcd_resume_after_bus(void);

void botcorps_set_surface(botcorps_surface_t surface);
botcorps_surface_t botcorps_surface(void);

/* Run fn with BotcorpsDisplay / SCR_* aimed at content, then restore
 * the surface that was active on entry (MENU, when called from menu).
 * Drops any clip on the external panel first so fn can address the
 * full 320×240. No-op wrapper on a missing panel: fn still runs, on
 * the ST7789 fallback. */
void botcorps_while_content(void (*fn)(void));

int botcorps_view_w(void);
int botcorps_view_h(void);

#define BotcorpsDisplay botcorps_disp()
#define SCR_W botcorps_view_w()
#define SCR_H botcorps_view_h()
#define BOTCORPS_VIEW_MAX_W 320
#define BOTCORPS_VIEW_MAX_H 240

#else /* !BOTCORPS_DUAL_SCREEN */

inline lgfx::LGFX_Device &botcorps_menu(void) { return M5Cardputer.Display; }
inline lgfx::LGFX_Device &botcorps_content(void) { return M5Cardputer.Display; }
inline lgfx::LGFX_Device &botcorps_disp(void) { return M5Cardputer.Display; }
inline bool botcorps_dual_begin(void) { return false; }
inline bool botcorps_dual_ok(void) { return false; }
inline void botcorps_lcd_quiesce(void) {}
inline void botcorps_lcd_resume_after_bus(void) {}
inline void botcorps_set_surface(botcorps_surface_t) {}
inline botcorps_surface_t botcorps_surface(void) { return BOTCORPS_SURFACE_MENU; }
inline void botcorps_while_content(void (*fn)(void)) { if (fn) fn(); }
inline int botcorps_view_w(void) { return 240; }
inline int botcorps_view_h(void) { return 135; }

#define BotcorpsDisplay botcorps_disp()
#define SCR_W 240
#define SCR_H 135
#define BOTCORPS_VIEW_MAX_W 240
#define BOTCORPS_VIEW_MAX_H 135

#endif /* BOTCORPS_DUAL_SCREEN */
#endif /* BOTCORPS_CYD */
