/*
 * Dual-screen Cardputer Adv. Built for real only when
 * BOTCORPS_DUAL_SCREEN=1. Single-screen builds use the inlines in
 * botcorps_display.h and do not construct an ILI9341.
 */
#include "display/botcorps_display.h"

#if BOTCORPS_DUAL_SCREEN

#include "display/lgfx_ili9341.h"

static Botcorps_LGFX_ILI9341 g_ext;
static bool s_ext_ok = false;
static botcorps_surface_t s_surface = BOTCORPS_SURFACE_MENU;

lgfx::LGFX_Device &botcorps_menu(void)
{
    return M5Cardputer.Display;
}

lgfx::LGFX_Device &botcorps_content(void)
{
    if (s_ext_ok) return g_ext;
    return M5Cardputer.Display;
}

lgfx::LGFX_Device &botcorps_disp(void)
{
    if (s_surface == BOTCORPS_SURFACE_CONTENT) return botcorps_content();
    return botcorps_menu();
}

bool botcorps_dual_ok(void)
{
    return s_ext_ok;
}

botcorps_surface_t botcorps_surface(void)
{
    return s_surface;
}

void botcorps_set_surface(botcorps_surface_t surface)
{
    s_surface = surface;
}

int botcorps_view_w(void)
{
    if (s_surface == BOTCORPS_SURFACE_CONTENT && s_ext_ok) return 320;
    return 240;
}

int botcorps_view_h(void)
{
    if (s_surface == BOTCORPS_SURFACE_CONTENT && s_ext_ok) return 240;
    return 135;
}

void botcorps_lcd_quiesce(void)
{
    if (!s_ext_ok) return;
    /* Hand the shared SPI3 lines to SD. CS high = LCD off the bus
     * and Cardputer SD enabled. Do not rewrite COLMOD. */
    g_ext.endWrite();
    g_ext.waitDisplay();
    digitalWrite(BOTCORPS_PIN_LCD_CS, HIGH);
}

void botcorps_lcd_resume_after_bus(void)
{
    if (!s_ext_ok) return;
    /* After SD on shared SCK/MOSI: release CS only.
     * No setColorDepth, no writeCommand(0x3A). */
    digitalWrite(BOTCORPS_PIN_LCD_CS, HIGH);
    delay(2);
}

void botcorps_while_content(void (*fn)(void))
{
    if (!fn) return;
    const botcorps_surface_t prev = s_surface;
    s_surface = BOTCORPS_SURFACE_CONTENT;
    if (s_ext_ok) {
        /* A leftover clip (for example 240×135) would park the painter
         * in a corner of the 320×240 panel. Drop it before fn runs. */
        g_ext.endWrite();
        g_ext.clearClipRect();
        g_ext.setTextWrap(false, false);
    }
    fn();
    s_surface = prev;
}

bool botcorps_dual_begin(void)
{
    if (s_ext_ok) return true;

    /* M5Cardputer.begin() has already sampled G5/G6 for Adv detect.
     * Claiming those pins earlier would lie to that probe. Cap LoRa
     * (RST on G3) is not parked low here — G3 is the panel reset. */
    delay(100);

    pinMode(BOTCORPS_PIN_LCD_CS, OUTPUT);
    digitalWrite(BOTCORPS_PIN_LCD_CS, HIGH);

    /* Boot-only color depth, before begin() writes COLMOD.
     * One retry on begin or size failure. Pins and rotation stay. */
    g_ext.setColorDepth(16);
    bool up = false;
    bool live = false;
    for (int attempt = 0; attempt < 2 && !up; ++attempt) {
        if (attempt) {
            Serial.println("[botcorps] ILI9341 begin retry");
            /* Drop the SPI host before begin() runs again. */
            g_ext.releaseBus();
            live = false;
            delay(50);
        }
        if (!g_ext.begin()) continue;
        live = true;
        delay(50);
        g_ext.setRotation(3);
        g_ext.setTextWrap(false, false);
        g_ext.setTextSize(1);
        g_ext.clearClipRect();
        g_ext.fillScreen(0x0000);
        up = (g_ext.width() == 320 && g_ext.height() == 240);
        if (!up) {
            Serial.printf("[botcorps] EXT geometry %dx%d, want 320x240\n",
                          (int)g_ext.width(), (int)g_ext.height());
        }
    }
    if (!up) {
        if (live) g_ext.fillScreen(0x0000);
        Serial.println("[botcorps] ILI9341 begin() failed — content stays on ST7789");
        s_ext_ok = false;
        pinMode(BOTCORPS_PIN_LCD_CS, OUTPUT);
        digitalWrite(BOTCORPS_PIN_LCD_CS, HIGH);
        return false;
    }

    /* Leave swap-bytes false so this panel matches the internal
     * ST7789. A later RGB565 sprite path must not be swapped twice. */
    s_ext_ok = true;
    Serial.printf("[botcorps] ILI9341 %dx%d SPI3 CS%d DC%d RST%d\n",
                  (int)g_ext.width(), (int)g_ext.height(),
                  BOTCORPS_PIN_LCD_CS, BOTCORPS_PIN_LCD_DC, BOTCORPS_PIN_LCD_RST);
    return true;
}

#endif /* BOTCORPS_DUAL_SCREEN */

#ifdef BOTCORPS_CYD
#include "display/lgfx_cyd.h"
#include <Preferences.h>

Botcorps_LGFX_CYD g_cyd;

void botcorps_cyd_load_touch_cal(void)
{
    /* Placeholders until a device pass writes a measured range.
     * Namespace matches the rain color store. Read-only here. */
    uint16_t xmin = 300;
    uint16_t xmax = 3900;
    uint16_t ymin = 300;
    uint16_t ymax = 3900;
    uint8_t rot = 1;
    Preferences prefs;
    if (prefs.begin("botcorps", true)) {
        if (prefs.getUChar("tcal", 0) == 1) {
            xmin = prefs.getUShort("tx0", xmin);
            xmax = prefs.getUShort("tx1", xmax);
            ymin = prefs.getUShort("ty0", ymin);
            ymax = prefs.getUShort("ty1", ymax);
            rot = prefs.getUChar("trot", rot);
        }
        prefs.end();
    }
    g_cyd.apply_touch_cal(xmin, xmax, ymin, ymax, rot);
    Serial.printf("[botcorps] touch cal x=%u..%u y=%u..%u rot=%u %s\n",
                  (unsigned)xmin, (unsigned)xmax, (unsigned)ymin, (unsigned)ymax,
                  (unsigned)rot,
                  (xmin == 300 && xmax == 3900) ? "placeholder" : "nvs");
}
#endif
