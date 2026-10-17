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

void botcorps_cyd_save_touch_cal(uint16_t xmin, uint16_t xmax, uint16_t ymin, uint16_t ymax, uint8_t rot)
{
    Preferences prefs;
    if (!prefs.begin("botcorps", false)) return;
    prefs.putUChar("tcal", 1);
    prefs.putUShort("tx0", xmin);
    prefs.putUShort("tx1", xmax);
    prefs.putUShort("ty0", ymin);
    prefs.putUShort("ty1", ymax);
    prefs.putUChar("trot", rot);
    prefs.end();
    g_cyd.apply_touch_cal(xmin, xmax, ymin, ymax, rot);
    Serial.printf("[botcorps] touch cal saved x=%u..%u y=%u..%u rot=%u\n",
                  (unsigned)xmin, (unsigned)xmax, (unsigned)ymin, (unsigned)ymax, (unsigned)rot);
}

void botcorps_cyd_load_touch_cal(void)
{
    Preferences prefs;
    if (!prefs.begin("botcorps", true)) return;
    if (prefs.getUChar("tcal", 0) != 1) { prefs.end(); return; }
    /* Prefer 8-point if present */
    if (prefs.isKey("c0")) {
        uint16_t cal[8];
        bool ok = true;
        for (int i = 0; i < 8; ++i) {
            char key[4];
            snprintf(key, sizeof(key), "c%d", i);
            if (!prefs.isKey(key)) { ok = false; break; }
            cal[i] = prefs.getUShort(key, 0);
        }
        prefs.end();
        if (ok) {
            g_cyd.apply_touch_cal8(cal);
            Serial.println("[botcorps] touch cal 8-point from NVS");
            return;
        }
    } else {
        /* Legacy 4-value path */
        uint16_t xmin = prefs.getUShort("tx0", 300);
        uint16_t xmax = prefs.getUShort("tx1", 3900);
        uint16_t ymin = prefs.getUShort("ty0", 300);
        uint16_t ymax = prefs.getUShort("ty1", 3900);
        uint8_t rot = prefs.getUChar("trot", 1);
        prefs.end();
        g_cyd.apply_touch_cal(xmin, xmax, ymin, ymax, rot);
        Serial.printf("[botcorps] touch cal legacy x=%u..%u y=%u..%u\n",
                      (unsigned)xmin, (unsigned)xmax, (unsigned)ymin, (unsigned)ymax);
    }
}
#endif

#ifdef BOTCORPS_CYD
/* Custom 15 s-per-target cal (native TL/BL/TR/BR order, 16-sample avg,
 * 20-raw jitter, one deadline across retries, restore rotation, no save on timeout). */
bool botcorps_cyd_run_touch_cal(void)
{
    /* Custom 15 s-per-target cal. Rotation computed LovyanGFX-style
     * ((touch^panel)&4)|(-(touch+panel)&3). True corners via
     * px=(w-1)*((i>>1)&1), py=(h-1)*(i&1). One deadline across jitter
     * retries; wait for lift after reject. Restores rotation on every exit.
     * No save on timeout. */
    const uint8_t saved_rot = g_cyd.getRotation();
    auto restore = [&]() { g_cyd.setRotation(saved_rot); g_cyd.fillScreen(0x0000); };
    /* LovyanGFX cal rotation: touch offset 0, panel offset 0 → 0 */
    const uint8_t touch_off = 0;
    const uint8_t panel_off = 0;
    const uint8_t cal_rot = ((touch_off ^ panel_off) & 4) | ((-(touch_off + panel_off)) & 3);
    g_cyd.setRotation(cal_rot);
    const int w = g_cyd.width();
    const int h = g_cyd.height();
    g_cyd.fillScreen(0x0000);
    /* Finger-lift pre-check */
    {
        int x, y;
        if (g_cyd.get_touch_raw(&x, &y)) {
            uint32_t t = millis();
            while (g_cyd.get_touch_raw(&x, &y) && millis() - t < 3000) delay(20);
            if (g_cyd.get_touch_raw(&x, &y)) {
                Serial.println("[botcorps] cal abort: finger never lifted");
                restore();
                return false;
            }
        }
    }
    uint16_t cal[8] = {0};
    for (int i = 0; i < 4; ++i) {
        /* True corners: i=0 TL, 1 BL, 2 TR, 3 BR in current rotation */
        int cx = (w - 1) * ((i >> 1) & 1);
        int cy = (h - 1) * (i & 1);
        g_cyd.drawLine(cx - 10, cy, cx + 10, cy, 0xFFFF);
        g_cyd.drawLine(cx, cy - 10, cx, cy + 10, 0xFFFF);
        g_cyd.drawCircle(cx, cy, 6, 0xFFFF);
        uint32_t deadline = millis() + 15000;
        bool got = false;
        while (!got) {
            if ((int32_t)(millis() - deadline) >= 0) {
                Serial.println("[botcorps] cal timeout");
                restore();
                return false;
            }
            int samples_x[16] = {0}, samples_y[16] = {0};
            int n = 0;
            while (n < 16) {
                if ((int32_t)(millis() - deadline) >= 0) {
                    Serial.println("[botcorps] cal timeout");
                    restore();
                    return false;
                }
                int x = 0, y = 0;
                if (g_cyd.get_touch_raw(&x, &y)) {
                    samples_x[n] = x;
                    samples_y[n] = y;
                    ++n;
                    delay(15);
                } else {
                    n = 0;
                    delay(10);
                }
            }
            int minx = samples_x[0], maxx = samples_x[0];
            int miny = samples_y[0], maxy = samples_y[0];
            for (int k = 1; k < 16; ++k) {
                if (samples_x[k] < minx) minx = samples_x[k];
                if (samples_x[k] > maxx) maxx = samples_x[k];
                if (samples_y[k] < miny) miny = samples_y[k];
                if (samples_y[k] > maxy) maxy = samples_y[k];
            }
            if (maxx - minx > 20 || maxy - miny > 20) {
                Serial.println("[botcorps] cal jitter reject");
                g_cyd.fillRect(cx - 12, cy - 12, 24, 24, 0x0000);
                /* wait for lift */
                uint32_t rel = millis();
                int rx, ry;
                while (g_cyd.get_touch_raw(&rx, &ry) && millis() - rel < 2000) delay(10);
                continue;
            }
            int sx = 0, sy = 0;
            for (int k = 0; k < 16; ++k) { sx += samples_x[k]; sy += samples_y[k]; }
            cal[i * 2]     = (uint16_t)(sx / 16);
            cal[i * 2 + 1] = (uint16_t)(sy / 16);
            got = true;
            g_cyd.fillCircle(cx, cy, 8, 0x07E0);
            delay(150);
            uint32_t rel = millis();
            int rx, ry;
            while (g_cyd.get_touch_raw(&rx, &ry) && millis() - rel < 2000) delay(10);
        }
    }
    g_cyd.apply_touch_cal8(cal);
    Preferences prefs;
    if (!prefs.begin("botcorps", false)) {
        Serial.println("[botcorps] cal NVS begin failed");
        restore();
        return false;
    }
    prefs.putUChar("tcal", 1);
    bool ok = true;
    for (int i = 0; i < 8; ++i) {
        char key[4];
        snprintf(key, sizeof(key), "c%d", i);
        if (!prefs.putUShort(key, cal[i])) ok = false;
    }
    prefs.end();
    if (!ok) {
        Serial.println("[botcorps] cal NVS put failed");
        restore();
        return false;
    }
    restore();
    Serial.println("[botcorps] cal done (true corners, computed rot)");
    return true;
}
#endif
