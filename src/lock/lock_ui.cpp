#if defined(BOTCORPS_CYD) && BOTCORPS_LOCK_UI
/*
 * CYD glue for lock_flow: ROM tinfl inflate, palette -> swapped RGB565
 * line, pushImage straight to the ILI9341 (no framebuffer). Touch is
 * LovyanGFX getTouch in rotation 0 with the NVS 8-point cal; one tap
 * fires on release (same drop-out guard as the hub14 UI). Theme is kept
 * in NVS "botcorps"/"lktheme".
 */
#include <Arduino.h>
#include <Preferences.h>
#include "display/botcorps_display.h"
#include "lock/lock_ui.h"
#include "lock/lock_flow.h"
extern "C" {
#include "miniz.h" /* esp_rom/include: ROM tinfl */
}

extern "C" const uint8_t lk_blob[];
extern "C" const uint8_t lk_blob_end[];

static tinfl_decompressor s_infl;           /* ~11 KB: static, the loop stack is 8 KB */
static uint8_t s_buf[LK_INFL_CAP];
static uint16_t s_line[240];
static uint16_t s_pal[256];                 /* byte-swapped RGB565 of the current clip */
static int s_pal_clip = -1;
static LkFlow s_flow;

static int lk_infl(const uint8_t *src, uint32_t len, uint8_t *dst, uint32_t cap)
{
    tinfl_init(&s_infl);
    size_t in = len, out = cap;
    tinfl_status st = tinfl_decompress(&s_infl, src, &in, dst, dst, &out, TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
    return st == TINFL_STATUS_DONE ? (int)out : -1;
}

static void lk_draw(int y, int x, int n, const uint8_t *idx)
{
    for (int i = 0; i < n; ++i) s_line[i] = s_pal[idx[i]];
    g_cyd.pushImage(x, y, n, 1, (const lgfx::swap565_t *)s_line);
}

static void lk_use_pal(void)
{
    const int c = lk_clip_of_seg(s_flow.p.seg);
    if (c == s_pal_clip) return;
    s_pal_clip = c;
    memset(s_pal, 0, sizeof s_pal);
    for (int i = 0; i < LK_CLIP[c].npal; ++i) {
        const uint16_t v = LK_CLIP[c].pal[i];
        s_pal[i] = (uint16_t)((v >> 8) | (v << 8));
    }
}

static const LkIo s_io{lk_blob, lk_infl, lk_draw, s_buf};

static bool lk_read_tap(int *ox, int *oy)
{
    static bool down_prev = false;
    static int n = 0, up = 0, px[2], py[2];
    static uint32_t upt = 0;
    int tx, ty;
    const bool down = g_cyd.getTouch(&tx, &ty);
    if (down) {
        if (!down_prev) { down_prev = true; n = 0; }
        px[0] = px[1]; py[0] = py[1]; px[1] = tx; py[1] = ty; ++n; up = 0;
        return false;
    }
    if (!down_prev) return false;
    if (++up == 1) upt = millis();
    if (up < 2 || (int32_t)(millis() - upt) < 45) return false;
    down_prev = false; up = 0;
    const int use = n >= 3 ? 0 : 1;
    *ox = px[use]; *oy = py[use]; n = 0;
    return true;
}

void lock_ui_begin(void)
{
    botcorps_cyd_orient = 1;
    botcorps_cyd_apply_rotation(); /* rot 0: portrait 240x320 */
    g_cyd.fillScreen(0x0000);
    Preferences prefs;
    int th = 0;
    if (prefs.begin("botcorps", true)) { th = prefs.getUChar("lktheme", 0); prefs.end(); }
    Serial.printf("[lock] blob=%u (expect %u) theme=%d %dx%d\n", (unsigned)(lk_blob_end - lk_blob),
                  (unsigned)LK_BLOB_SIZE, th, (int)g_cyd.width(), (int)g_cyd.height());
    s_flow.begin(th);
}

void lock_ui_loop(void)
{
    static uint32_t next = 0;
    int x, y;
    if (lk_read_tap(&x, &y)) {
        const int r = s_flow.tap(x, y);
        Serial.printf("[lock] tap %d,%d scr=%d game=%s theme=%s act=%d\n", x, y, s_flow.scr,
                      s_flow.scr == LK_SC_GAME ? LK_GAME[s_flow.game].name : "-", LK_THEME_NAME[s_flow.theme], r);
        if (s_flow.theme_dirty) {
            s_flow.theme_dirty = false;
            Preferences prefs;
            if (prefs.begin("botcorps", false)) { prefs.putUChar("lktheme", (uint8_t)s_flow.theme); prefs.end(); }
        }
        next = 0;
    }
    const uint32_t now = millis();
    if ((int32_t)(now - next) < 0) { delay(2); return; }
    next = now + s_flow.period_ms();
    lk_use_pal();
    g_cyd.startWrite();
    s_flow.tick(s_io);
    g_cyd.endWrite();
    /* A screen change inside tick() switches clip; its keyframe is drawn on the next tick
     * with the new palette (lk_use_pal runs before every tick). */
}
#endif
