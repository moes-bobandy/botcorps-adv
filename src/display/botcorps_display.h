#pragma once

/*
 * Display for BOTcorps.
 * CYD: single 320x240 ILI9341 via LovyanGFX.
 * Adv: dual or single M5Cardputer as before.
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

inline bool botcorps_dual_begin(void) {
    g_cyd.setColorDepth(16);
    if (!g_cyd.begin()) return false;
    g_cyd.setRotation(1); // landscape 320x240
    g_cyd.setTextWrap(false, false);
    g_cyd.setTextSize(1);
    g_cyd.clearClipRect();
    g_cyd.fillScreen(0x0000);
    g_cyd.setBrightness(255);
    return (g_cyd.width() == 320 && g_cyd.height() == 240);
}
inline bool botcorps_dual_ok(void) { return true; }
inline void botcorps_lcd_quiesce(void) {}
inline void botcorps_lcd_resume_after_bus(void) {}
inline void botcorps_while_content(void (*fn)(void)) { if (fn) fn(); }
inline int botcorps_view_w(void) { return 320; }
inline int botcorps_view_h(void) { return 240; }

#else

#include <M5Cardputer.h>
#include "display/botcorps_pins.h"

#ifndef BOTCORPS_DUAL_SCREEN
#define BOTCORPS_DUAL_SCREEN 0
#endif

enum botcorps_surface_t {
    BOTCORPS_SURFACE_MENU = 0,
    BOTCORPS_SURFACE_CONTENT = 1,
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

void botcorps_while_content(void (*fn)(void));

int botcorps_view_w(void);
int botcorps_view_h(void);

#define BotcorpsDisplay botcorps_disp()
#define SCR_W botcorps_view_w()
#define SCR_H botcorps_view_h()

#else

inline lgfx::LGFX_Device &botcorps_menu(void) { return M5Cardputer.Display; }
inline lgfx::LGFX_Device &botcorps_content(void) { return M5Cardputer.Display; }
inline lgfx::LGFX_Device &botcorps_disp(void) { return M5Cardputer.Display; }
#define BotcorpsDisplay M5Cardputer.Display
#define SCR_W 240
#define SCR_H 135

inline bool botcorps_dual_begin(void) { return false; }
inline bool botcorps_dual_ok(void) { return false; }
inline void botcorps_lcd_quiesce(void) {}
inline void botcorps_lcd_resume_after_bus(void) {}
inline void botcorps_while_content(void (*fn)(void)) { if (fn) fn(); }
inline int botcorps_view_w(void) { return 240; }
inline int botcorps_view_h(void) { return 135; }

#endif

#endif /* BOTCORPS_CYD */
