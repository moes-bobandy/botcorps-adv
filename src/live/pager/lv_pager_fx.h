/* v1.2.4: shared Matrix-rain face overlay + role-specific readout panels.
 * Rain is one routine, two palettes (HACKbot green, BOTpheus/GIBSONbot white).
 * v1.2.5: HACKbot <-> TERMINALbot UI swap; TERMINALbot gold; GIBSONbot white rain.
 * Panels are procedural in the locked 152x71 plate (45,220)-(196,290); CONTROLbot is untouched. */
#pragma once

#define LV_PG_FX_X0 65
#define LV_PG_FX_Y0 88
#define LV_PG_FX_Y1 197
#define LV_PG_FX_S  110
#define LV_PG_LIME  0xC8FF10u

/* Locked TERMINALbot eye boxes, face-local inclusive. */
static const int LV_PG_EYE[2][4] = {{22, 22, 43, 43}, {66, 22, 87, 43}};

/* BOTpheus sunglasses: keep these 565 values on top of the white rain. */
static int lv_pg_bot_feat(uint16_t c)
{
    return c == 0 || c == 0xFFFF || c == lv_rgb(132, 138, 148) || c == lv_rgb(82, 89, 99);
}
/* GIBSONbot shades + bevel (not the ivory fill): keep these on top of the white rain. */
static int lv_pg_gib_feat(uint16_t c)
{
    return c == 0 || c == lv_rgb(181, 166, 148) || c == lv_rgb(33, 36, 33)
        || c == lv_rgb(123, 113, 99) || c == lv_rgb(107, 117, 132) || c == lv_rgb(33, 32, 24);
}
static int lv_pg_keep_feat(uint16_t c, int page)
{
    if (page == LV_PG_BOTPHEUS) return lv_pg_bot_feat(c);
    if (page == LV_PG_GIBSON) return lv_pg_gib_feat(c);
#if LV_PG_TERMINAL_PAGE
    if (page == LV_PG_TERMINAL) return c == lv_rgb(40, 24, 8) || c == lv_rgb(41, 24, 8);   /* 7db0c6d gold-face eyes (lv_pg_gold of black) stay on top */
#endif
#if LV_CYD19_LOGBOT
    if (page == LV_PG_LOGBOT) return c == 0;   /* house chip-plate sockets (blob 2 / lv_pg_ch_row), same source eyes as TERMINALbot, stay on top */
#endif
    return 0;
}
/* Metallic gold (not flat yellow): bronze shadow, rich mid, champagne highlight. */
static uint16_t lv_pg_gold(uint16_t c)
{
    const int r = (c >> 11) << 3, g = ((c >> 5) & 63) << 2, b = (c & 31) << 3;
    const int y = (r * 3 + g * 6 + b) / 10;
    int R, G, B;
    if (y < 110) {
        const int k = y, n = 110;
        R = (40 * (n - k) + 196 * k) / n;
        G = (24 * (n - k) + 148 * k) / n;
        B = (8 * (n - k) + 42 * k) / n;
    } else {
        const int k = y - 110, n = 145;
        R = (196 * (n - k) + 255 * k) / n;
        G = (148 * (n - k) + 230 * k) / n;
        B = (42 * (n - k) + 168 * k) / n;
    }
    if (R > 255) R = 255; if (G > 255) G = 255; if (B > 255) B = 255;
    return lv_rgb((uint8_t)R, (uint8_t)G, (uint8_t)B);
}

static uint16_t lv_pg_fx_acc(void)
{
    const int t = (lv_theme >= 0 && lv_theme < LV_NTHEME_ALL) ? lv_theme : 0;
    return lv_hex(LV_ACC[t]);
}

static int lv_pg_fx_skip_page = 0;
static void lv_pg_fx_glyph(uint16_t *row, int y, int x0, int gy, char ch, uint16_t c, int xlo, int xhi, int skip)
{
    const int bit = y - gy;
    if (bit < 0 || bit > 6) return;
    unsigned uc = (unsigned char)ch;
    if (uc < 32 || uc > 126) uc = 32;
    const uint8_t *g = LV_GLCD + (uc - 32) * 5;
    for (int col = 0; col < 5; ++col) {
        if (!(g[col] & (1u << bit))) continue;
        const int x = x0 + col;
        if (x < xlo || x > xhi) continue;
        if (skip && lv_pg_keep_feat(row[x], lv_pg_fx_skip_page)) continue;
        row[x] = c;
    }
}

/* Shared Matrix rain over the face box, one screen row. pal[0]=head, pal[1]=mid, pal[2]=dim. */
static void lv_pg_rain_row(uint16_t *row, int y, const uint16_t pal[3], int skip, int page)
{
    if (y < LV_PG_FX_Y0 || y > LV_PG_FX_Y1) return;
    lv_pg_fx_skip_page = page;
    static const char GL[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ#$%*+<>/|";
    const int ngl = (int)sizeof(GL) - 1, CW = 6, CH = 8, NCOL = 18;
    const int phase = (int)(lv_now / 30u);
    const int ly = y - LV_PG_FX_Y0;
    for (int col = 0; col < NCOL; ++col) {
        const uint32_t s = 0xC0DEU ^ (uint32_t)(col * 2654435761u);
        const int speed = 1 + (int)((s >> 8) % 3);
        const int trail = 9 + (int)((s >> 3) % 7);
        const int period = LV_PG_FX_S + trail * CH + 24;
        const int off = (int)((s >> 16) % (uint32_t)period);
        const int head_y = (phase * speed + off) % period;
        const int x0 = LV_PG_FX_X0 + 1 + col * CW;
        for (int t = 0; t < trail; ++t) {
            const int gy = head_y - t * CH;
            if (gy > LV_PG_FX_S - 1 || gy + 6 < 0) continue;
            if (ly < gy || ly > gy + 6) continue;
            const unsigned k = (unsigned)(s + (unsigned)((gy + period) / CH) * 17u + (unsigned)col * 3u);
            const char ch = GL[k % (unsigned)ngl];
            const uint16_t c = t == 0 ? pal[0] : t == 1 ? pal[1] : lv_mix(pal[1], pal[2], t, trail);
            lv_pg_fx_glyph(row, y, x0, LV_PG_FX_Y0 + gy, ch, c, LV_PG_FX_X0, LV_PG_FX_X0 + LV_PG_FX_S - 1, skip);
        }
    }
}

static void lv_pg_lime_eyes_row(uint16_t *row, int y)
{
    if (y < LV_PG_FX_Y0 || y > LV_PG_FX_Y1) return;
    const uint16_t lime = lv_hex(LV_PG_LIME);
    const int ly = y - LV_PG_FX_Y0;
    for (int e = 0; e < 2; ++e) {
        if (ly < LV_PG_EYE[e][1] || ly > LV_PG_EYE[e][3]) continue;
        for (int lx = LV_PG_EYE[e][0]; lx <= LV_PG_EYE[e][2]; ++lx)
            row[LV_PG_FX_X0 + lx] = lime;
    }
}

#if LV_CYD19_LOGBOT
static void lv_lb_face_row(uint16_t *row, int y, const uint8_t *fr);   /* lv_pager_log19.h (included below, after the CYD-17 helpers) */
static void lv_lb_panel_row(uint16_t *row, int y);
static int lv_lb_tab = 0;   /* 0 = SCAN LOG, 1 = SYSTEM LOG */
#endif
static void lv_pg_face_fx_row(uint16_t *row, int y, int page, const uint8_t *fr)
{
#if LV_CYD19_LOGBOT
    if (page == LV_PG_LOGBOT) { if (y >= LV_PG_FX_Y0 && y <= LV_PG_FX_Y1) lv_lb_face_row(row, y, fr); return; }
#endif
    if (y < LV_PG_FX_Y0 || y > LV_PG_FX_Y1 || page == LV_PG_CTL) return;
    if (page == LV_PG_HACKBOT) {   /* v1.2.5: the green Matrix face + lime eyes moved here from TERMINALbot */
        for (int x = LV_PG_FX_X0; x <= LV_PG_FX_X0 + LV_PG_FX_S - 1; ++x) row[x] = 0;
        const uint16_t pal[3] = { lv_hex(0xD8FFD8), lv_hex(0x00FF41), lv_hex(0x001800) };
        lv_pg_rain_row(row, y, pal, 0, page);
        lv_pg_lime_eyes_row(row, y);
        return;
    }
    if (fr) memcpy(row + LV_PG_FX_X0, fr, LV_PG_FACE_S * 2);
#if LV_PG_TERMINAL_PAGE
    if (page == LV_PG_TERMINAL) {   /* 7db0c6d gold remap; a98a25b pitch-black rain (same pal + skip=1 as GIBSON) */
        for (int x = LV_PG_FX_X0; x <= LV_PG_FX_X0 + LV_PG_FX_S - 1; ++x) row[x] = lv_pg_gold(row[x]);
        const uint16_t pal[3] = { lv_hex(0x000000), lv_hex(0x000000), lv_hex(0x000000) };
        lv_pg_rain_row(row, y, pal, 1, page);
        return;
    }
#endif
    if (page == LV_PG_BOTPHEUS) {   /* v1.2.6: brighter white, pure #FFFFFF heads, brighter trails */
        const uint16_t pal[3] = { lv_hex(0xFFFFFF), lv_hex(0xF0F2F6), lv_hex(0xC4C8D0) };
        lv_pg_rain_row(row, y, pal, 1, page);
    } else if (page == LV_PG_GIBSON) {   /* v1.2.6: pitch-black glyphs, shades stay on top */
        const uint16_t pal[3] = { lv_hex(0x000000), lv_hex(0x000000), lv_hex(0x000000) };
        lv_pg_rain_row(row, y, pal, 1, page);
    }
}

/* ---- role panels: only (45,220) 152x71, four distinct smooth meters ---- */
static void lv_pg_pan_text(uint16_t *row, int y, int x, int gy, const char *s, uint16_t c)
{
    for (; *s; ++s, x += 6) lv_pg_fx_glyph(row, y, x, gy, *s, c, LV_CB_PX0, LV_CB_PX0 + LV_CB_PW - 1, 0);
}

static void lv_pg_pan_span(uint16_t *row, int x0, int x1, uint16_t c)
{
    if (x0 < LV_CB_PX0) x0 = LV_CB_PX0;
    if (x1 > LV_CB_PX0 + LV_CB_PW - 1) x1 = LV_CB_PX0 + LV_CB_PW - 1;
    for (int x = x0; x <= x1; ++x) row[x] = c;
}

static void lv_pg_pan_blend(uint16_t *row, int x, uint16_t c, int k, int n)
{
    if (x < LV_CB_PX0 || x > LV_CB_PX0 + LV_CB_PW - 1 || k <= 0) return;
    if (k >= n) { row[x] = c; return; }
    row[x] = lv_mix(row[x], c, k, n);
}

static void lv_pg_pan_plate(uint16_t *row, int y, uint16_t acc, uint16_t fill)
{
    const int x0 = LV_CB_PX0, x1 = LV_CB_PX0 + LV_CB_PW - 1, y0 = LV_CB_PY0, y1 = LV_CB_PY0 + LV_CB_PH - 1;
    if (y == y0 || y == y1) { lv_pg_pan_span(row, x0, x1, acc); return; }
    lv_pg_pan_span(row, x0, x1, fill);
    row[x0] = row[x1] = acc;
    if (y == y0 + 1 || y == y1 - 1) {
        lv_pg_pan_span(row, x0 + 1, x1 - 1, lv_mix(acc, fill, 3, 8));
        row[x0] = row[x1] = acc;
    } else {
        row[x0 + 1] = row[x1 - 1] = lv_mix(acc, fill, 2, 8);
    }
}

/* BOTpheus: dojo enso ring + falling ink, theme accent. */
static void lv_pg_pan_botpheus(uint16_t *row, int y, uint16_t acc)
{
    const uint16_t fill = lv_hex(0x0A0C10), ink = lv_hex(0xE8ECF0);
    lv_pg_pan_plate(row, y, acc, fill);
    const int cx = 132, cy = 258;
    const float r = 22.5f, th = 2.6f;
    const float ang = (lv_now % 4000) * (6.2831853f / 4000.f);
    for (int x = LV_CB_PX0 + 2; x <= LV_CB_PX0 + LV_CB_PW - 3; ++x) {
        const float dx = x + 0.5f - cx, dy = y + 0.5f - cy;
        const float d = sqrtf(dx * dx + dy * dy);
        const float e = fabsf(d - r);
        if (e < th + 1.2f) {
            int k = (int)((th + 1.2f - e) * 8.f / (th + 1.2f) + 0.5f);
            if (k > 8) k = 8;
            float a = atan2f(dy, dx);
            float da = a - ang; while (da > 3.1416f) da -= 6.2832f; while (da < -3.1416f) da += 6.2832f;
            const uint16_t c = (da > -0.7f && da < 0.7f) ? lv_mix(acc, ink, (int)(fabsf(da) * 8.f / 0.7f), 8) : acc;
            lv_pg_pan_blend(row, x, c, k, 8);
        }
    }
    /* left ink drips */
    for (int i = 0; i < 4; ++i) {
        const int x = 54 + i * 7;
        const int head = LV_CB_PY0 + 8 + (int)((lv_now / (80u + (unsigned)i * 17u) + (unsigned)i * 19u) % (unsigned)(LV_CB_PH - 16));
        const int d = y - (LV_CB_PY0 + 8);
        if (d >= 0 && y <= head && head - y < 18)
            lv_pg_pan_blend(row, x, (y == head) ? ink : acc, 8 - (head - y) / 3, 8);
    }
    lv_pg_pan_text(row, y, 52, 226, "DOJO", acc);
    lv_pg_pan_text(row, y, 52, 276, "CONSTRUCT", lv_mix(acc, fill, 5, 8));
}

/* HACKbot: smooth attack waveform + exploit spike, not a ring or sweep. */
static void lv_pg_pan_hackbot(uint16_t *row, int y, uint16_t acc, int gold)
{
    const uint16_t fill = gold ? lv_hex(0x140E06) : lv_hex(0x0C0808);
    const uint16_t hot = gold ? lv_hex(0xE8C050) : lv_hex(0xFF5A3C);
    const uint16_t ink = gold ? lv_hex(0xD4AF37) : acc;
    lv_pg_pan_plate(row, y, gold ? ink : acc, fill);
    lv_pg_pan_text(row, y, 52, 226, "ATK", hot);
    lv_pg_pan_text(row, y, 86, 226, "TRACE", ink);
    const int wy0 = 248, wy1 = 280;
    if (y >= wy0 && y <= wy1) {
        const float mid = (wy0 + wy1) * 0.5f;
        const float t = (lv_now % 2000) / 2000.f;
        for (int x = 50; x <= 190; ++x) {
            const float u = (x - 50) / 140.f;
            float w = 0.55f * sinf((u * 5.2f + t * 6.2832f) * 3.1416f);
            if (u > 0.62f && u < 0.78f) w += 1.15f * expf(-((u - 0.70f) * (u - 0.70f)) * 280.f); /* exploit spike */
            const float yy = mid - w * ((wy1 - wy0) * 0.42f);
            const float e = fabsf(y + 0.5f - yy);
            if (e < 2.8f) {
                const uint16_t c = (u > 0.62f && u < 0.78f) ? hot : ink;
                lv_pg_pan_blend(row, x, c, (int)((2.8f - e) * 8.f / 2.8f + 0.5f), 8);
            }
        }
    }
    /* smooth charge pill under the label, not 8-seg */
    if (y >= 232 && y <= 239) {
        const int x0 = 128, w = 60, fillw = 18 + (int)((lv_now / 40u) % 43u);
        for (int x = x0; x <= x0 + w; ++x) {
            const int edge = (x == x0 || x == x0 + w || y == 232 || y == 239);
            if (edge) lv_pg_pan_blend(row, x, ink, 6, 8);
            else if (x < x0 + fillw) lv_pg_pan_blend(row, x, ink, 5, 8);
        }
    }
}

/* GIBSONbot: radar sweep + soft RSSI arc + mini tower, not a waveform. */
static void lv_pg_pan_gibson(uint16_t *row, int y, uint16_t acc)
{
    const uint16_t fill = lv_hex(0x080610);
    lv_pg_pan_plate(row, y, acc, fill);
    lv_pg_pan_text(row, y, 52, 226, "SCAN", acc);
    lv_pg_pan_text(row, y, 90, 226, "RSSI", lv_mix(acc, fill, 5, 8));
    /* mini tower at left */
    const int tx0 = 56, tx1 = 68, typ = 252, tyb = 284;
    if (y >= typ && y <= tyb) {
        const int inset = (tyb - y) / 10;
        lv_pg_pan_span(row, tx0 + inset, tx1 - inset, lv_mix(acc, fill, 4, 8));
        if (y == typ || y == tyb) lv_pg_pan_span(row, tx0 + inset, tx1 - inset, acc);
        row[tx0 + inset] = row[tx1 - inset] = acc;
    }
    /* concentric rings + rotating beam, centre right */
    const int cx = 150, cy = 258;
    if (y < cy - 29 || y > cy + 28) return;                                   /* perf-all r2: |dy| > 28 => d > 28 for every x */
    const float beam = (lv_now % 3000) * (6.2831853f / 3000.f);
    const float rssi_r = 24.f, span = 1.1f + 0.7f * sinf((lv_now % 5000) / 5000.f * 6.2832f);   /* same value every pixel: hoisted */
    for (int x = cx - 29; x <= cx + 28; ++x) {                                 /* was 80..190; outside this |dx| > 28 => skipped */
        const float dx = x + 0.5f - cx, dy = y + 0.5f - cy;
        const float d = sqrtf(dx * dx + dy * dy);
        if (d > 28.f) continue;
        /* perf-all r2: atan2f only where its value can change a pixel. The RSSI test needs it only on the ring (|d-24| < 2) near the arc;
         * the beam test only when d > 4 and the angle is near the beam. "Near" uses a cheap atan2 (max error 0.0039 rad,
         * here given a 0.03 rad margin), so every pixel that could pass the exact test still gets the exact atan2f and the same
         * pixels. */
        bool ring = fabsf(d - rssi_r) < 2.0f, need = false;
        if (ring || d > 4.f) {
            const float ax = fabsf(dx), ay = fabsf(dy), z = ax > ay ? ay / ax : ax / ay;
            float aa = z * (0.7853982f + 0.273f * (1.f - z)); if (ay > ax) aa = 1.5707963f - aa;
            if (dx < 0.f) aa = 3.1415927f - aa; if (dy < 0.f) aa = -aa;
            if (ring) ring = aa > -2.23f && aa < -2.17f + span;                 /* arc test a in (-2.2, -2.2 + span), same margin */
            float q = aa - beam; while (q > 3.1416f) q -= 6.2832f; while (q < -3.1416f) q += 6.2832f;
            need = ring || (d > 4.f && q > -0.03f && q < 0.58f);
        }
        float a = 0.f;
        if (need) {
            a = atan2f(dy, dx);
            float da = a - beam; while (da > 3.1416f) da -= 6.2832f; while (da < -3.1416f) da += 6.2832f;
            if (da >= 0.f && da < 0.55f && d > 4.f)
                lv_pg_pan_blend(row, x, acc, (int)((0.55f - da) * 8.f / 0.55f), 8);
        }
        for (int ri = 0; ri < 3; ++ri) {
            const float rr = 9.f + ri * 8.f, e = fabsf(d - rr);
            if (e < 1.4f) lv_pg_pan_blend(row, x, acc, (int)((1.4f - e) * 5.f / 1.4f), 8);
        }
        /* soft RSSI arc, top-right quadrant */
        if (ring && a > -2.2f && a < -2.2f + span) {
            const float e = fabsf(d - rssi_r);
            if (e < 2.0f) lv_pg_pan_blend(row, x, acc, (int)((2.0f - e) * 8.f / 2.0f), 8);
        }
    }
}

/* TERMINALbot: CRT phosphor lines + prompt, green, not a ring or radar. */
static void lv_pg_pan_terminal(uint16_t *row, int y, uint16_t acc)
{
    const uint16_t fill = lv_hex(0x020604), ph = lv_hex(0x30FF70), dim = lv_hex(0x0A2814);
    lv_pg_pan_plate(row, y, acc, fill);
    static const char *lines[4] = { "ready.", "scan  --live", "ok    12 hits", "> _" };
    const int scroll = (int)((lv_now / 700u) % 4u);
    for (int i = 0; i < 4; ++i) {
        const int gy = 228 + ((i + 4 - scroll) % 4) * 10;
        const uint16_t c = (i == 3) ? ph : lv_mix(ph, dim, 3, 8);
        lv_pg_pan_text(row, y, 52, gy, lines[(i + scroll) % 4], c);
    }
    /* soft phosphor baseline, not a block meter */
    if (y >= 272 && y <= 286) {
        const float mid = 279.f, t = (lv_now % 1600) / 1600.f;
        for (int x = 50; x <= 190; ++x) {
            const float u = (x - 50) / 140.f;
            const float w = 0.7f * sinf((u * 4.0f + t * 6.2832f) * 3.1416f) * (0.4f + 0.6f * u);
            const float yy = mid - w * 6.f;
            const float e = fabsf(y + 0.5f - yy);
            if (e < 2.2f) lv_pg_pan_blend(row, x, ph, (int)((2.2f - e) * 8.f / 2.2f + 0.5f), 8);
        }
    }
}

#include "lv_pager_term17.h"   /* CYD-17 mock: terminal-prompt readout panels (off unless LV_CYD17_PANEL_TERM) */
#include "lv_pager_log19.h"   /* CYD-19 mock: LOGbot page (off unless LV_CYD19_LOGBOT) */
#include "lv_pager_logsub.h"   /* CYD-27 mock: LOGbot subscreen (off unless LV_CYD27_LOGSUB) */
static void lv_pg_panel_fx_row(uint16_t *row, int y, int page)
{
#if LV_CYD19_LOGBOT
    if (page == LV_PG_LOGBOT) { if (lv_c17_alloc()) lv_lb_panel_row(row, y); return; }
#endif
#if LV_CYD17_PANEL_TERM
    if (page != LV_PG_CTL && lv_c17_alloc()) { lv_c17_row(row, y, page); return; }   /* alloc fail -> locked panel */
#endif
    if (page == LV_PG_CTL || y < LV_CB_PY0 || y >= LV_CB_PY0 + LV_CB_PH) return;
    const uint16_t acc = lv_pg_fx_acc();
    if (page == LV_PG_BOTPHEUS) lv_pg_pan_botpheus(row, y, acc);
    else if (page == LV_PG_HACKBOT) lv_pg_pan_terminal(row, y, acc);         /* v1.2.5: CRT panel moved with the green rain face */
    else if (page == LV_PG_GIBSON) lv_pg_pan_gibson(row, y, acc);
#if LV_PG_TERMINAL_PAGE
    else if (page == LV_PG_TERMINAL) lv_pg_pan_hackbot(row, y, acc, 1);     /* 7db0c6d: old HACKbot meter, metallic gold */
#endif
}
