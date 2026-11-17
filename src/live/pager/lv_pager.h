/* fw-bigbot-1117: the five v3.1 big-bot screens are the new CYD home (the pager).
 * LV_SC_HOME draws the locked v6 board of the current theme (same stills as CONTROLbot),
 * with BACKbot replaced by the restored mounting-hole chrome, the page's identity face
 * and instrument panel (CONTROLbot keeps the locked v6 live panel), and 5 theme-ink pips.
 * v1.2.1: swipe on the coil strip (LV_PG_COIL) or tap the BLUE / RED pills flips pages, 233 ms ease-in-out cubic, no wrap
 * (edge bounce ~18 px). Default page is CONTROLbot (index 2, changeable: LV_PG_DEFAULT).
 * Center taps: BOTpheus -> EB3, GIBSONbot -> radar, CONTROLbot cycles the theme (whole
 * pager follows), HACKbot -> game, TERMINALbot -> TERM. BACKbot on every sub-screen
 * returns here (lv_go(LV_SC_HOME) already does). go ctl lands on the CONTROLbot page.
 */
#pragma once
#include <math.h>
#include "lv_pager_data.h"

#define LV_PG_N        5
#define LV_PG_DEFAULT  2   /* CONTROLbot; changeable */
#define LV_PG_MS       233
#define LV_PG_BOUNCE   18
#define LV_PG_TICK_Y1  29  /* banner + pip band: taps here do nothing on the pager */

static int lv_pg_page = LV_PG_DEFAULT;
static bool lv_pg_dirty = true;
static bool lv_pg_swiped = false;     /* swallow the tap that follows a swipe */

/* slide / bounce */
static int lv_pg_anim = 0;            /* 0 still, 1 slide, 2 bounce */
static int lv_pg_from = 2, lv_pg_to = 2, lv_pg_dir = 1;
static uint32_t lv_pg_t0 = 0;
static int lv_pg_drag = 0;            /* live finger dx while down on the ticker */
static bool lv_pg_dragging = false;
static int lv_pg_dx0 = 0, lv_pg_dy0 = 0;

/* v1.2.1 (Mobandy 2026-11-17 8:26-8:32 AM ET: BOTpheus force-closes, RADIO SCAN FAILED, every page shows the CONTROLbot
 * panel, CONTROLbot controls dead). Root cause: v1.2 inflated each whole face (24,200 B) and panel (21,584 B) into its own
 * heap block and never freed them (lv_pg_leave_cache had no caller). On the CYD the UI leaves ~53 KB free / 38.9 KB largest
 * after ui_live_init, so the face took the block the panel, lv_cb_pan, the radio stack and the sub-screens need.
 * Now: overlays are 16-row raw-deflate chunks decoded on demand. The visible page decodes into the unused tail of the
 * static lock inflate buffer (io->buf; the v6 board strip only uses its first 7,680 B), so a still page costs no heap.
 * Only the second page of a slide borrows LV_PG_SET_B of heap, for that frame only. One decode set per page also means
 * the incoming page of a slide shows its own face and panel (v1.2 shared one buffer, so it slid in with the old art). */
#define LV_PG_FCH_B  (LV_PG_CH * LV_PG_FACE_S * 2)   /* 3,520 B: one face chunk */
#define LV_PG_PCH_B  (LV_PG_CH * LV_PG_PAN_W * 2)    /* 4,864 B: one panel chunk */
#define LV_PG_GAP    32                              /* guard bytes between regions */
#define LV_PG_SET_B  (LV_PG_FCH_B + LV_PG_GAP + LV_PG_PCH_B)
#define LV_PG_TAIL   (240 * LV_CB_STRIP * 2 + 64)    /* io->buf offset past the board strip */
static_assert(LV_PG_TAIL + LV_PG_SET_B <= LK_INFL_CAP, "pager decode set must fit the io->buf tail");
struct LvPgSet { uint8_t *mem; int fkey, pkey; };
static LvPgSet lv_pg_set[2] = {{nullptr, -1, -1}, {nullptr, -1, -1}};
static uint16_t *lv_pg_corner = nullptr;   /* 722 B, freed when HOME is left */
static int lv_pg_corner_key = -1;
static bool lv_pg_heap_warned = false;

static int lv_pg_art(int page) { return (page < 2) ? page : page - 1; } /* 0,1,3,4 -> 0,1,2,3 */

static bool lv_pg_inflate(const uint8_t *z, uint32_t n, uint8_t *dst, uint32_t cap, uint32_t expect)
{
    if (!lv_cb_io || !z || !n) return false;
    const int got = lv_cb_io->inflate(z, n, dst, cap);
    return got == (int)expect;
}

static void lv_pg_free_cache(void)
{
    free(lv_pg_corner); lv_pg_corner = nullptr; lv_pg_corner_key = -1;
}

static bool lv_pg_load_corner(int slot)
{
    if (lv_pg_corner && lv_pg_corner_key == slot) return true;
    if (!lv_pg_corner) lv_pg_corner = (uint16_t *)malloc(LV_PG_CORNER_W * LV_PG_CORNER_H * 2);
    if (!lv_pg_corner) return false;
    const uint32_t a = LV_PG_CORNER_OFF[slot], b = LV_PG_CORNER_OFF[slot + 1];
    if (!lv_pg_inflate(LV_PG_CORNER_Z + a, b - a, (uint8_t *)lv_pg_corner,
                       LV_PG_CORNER_W * LV_PG_CORNER_H * 2, LV_PG_CORNER_W * LV_PG_CORNER_H * 2)) {
        free(lv_pg_corner); lv_pg_corner = nullptr; lv_pg_corner_key = -1; return false;
    }
    lv_pg_corner_key = slot; return true;
}

/* grape = slot 11 -> pan index 0..3, holo = slot 12 -> 4..7; other slots reuse grape (board already recolours) */
static int lv_pg_pan_idx(int slot, int page)
{
    const int art = lv_pg_art(page);
    return (slot == 12 ? 4 : 0) + art;
}

/* row r (0-based) of face image idx / panel image idx, decoded through set S; nullptr if the chunk would not inflate */
static const uint8_t *lv_pg_ch_row(LvPgSet &S, bool face, int idx, int r)
{
    if (!S.mem) return nullptr;
    const int w = face ? LV_PG_FACE_S : LV_PG_PAN_W, h = face ? LV_PG_FACE_S : LV_PG_PAN_H;
    const int nch = face ? LV_PG_FACE_NCH : LV_PG_PAN_NCH, c = r / LV_PG_CH, j = idx * nch + c;
    uint8_t *dst = face ? S.mem : S.mem + LV_PG_FCH_B + LV_PG_GAP;
    int &k = face ? S.fkey : S.pkey;
    if (k != j) {
        const uint32_t *O = face ? LV_PG_FACE_OFF : LV_PG_PAN_OFF;
        const uint8_t *Z = face ? LV_PG_FACE_Z : LV_PG_PAN_Z;
        const int rows = (h - c * LV_PG_CH) < LV_PG_CH ? (h - c * LV_PG_CH) : LV_PG_CH;
        if (!lv_pg_inflate(Z + O[j], O[j + 1] - O[j], dst, face ? LV_PG_FCH_B : LV_PG_PCH_B, (uint32_t)(rows * w * 2))) { k = -1; return nullptr; }
        k = j;
    }
    return dst + (r % LV_PG_CH) * w * 2;
}

static void lv_pg_ensure(int page) { (void)page; lv_pg_load_corner(lv_cb_slot()); }

/* theme ink for the pips: mean of bright pixels on the current row-band of the v6 board (ticker y=5..17) */
static void lv_pg_pip_ink_calc(uint8_t on[3], uint8_t dim[3])
{
    long sr = 0, sg = 0, sb = 0; int n = 0;
    const int slot = lv_cb_slot();
    for (int y = 5; y <= 17; ++y) {
        const uint8_t *st = lv_cb_strip(slot, y / LV_CB_STRIP);
        if (!st) continue;
        const uint8_t *r = st + (y % LV_CB_STRIP) * 240 * 2;
        for (int x = 16; x <= 223; ++x) {
            const uint16_t v = (uint16_t)(r[2 * x] | (r[2 * x + 1] << 8));
            const int R = ((v >> 11) << 3) | ((v >> 11) >> 2);
            const int G = (((v >> 5) & 63) << 2) | (((v >> 5) & 63) >> 4);
            const int B = ((v & 31) << 3) | ((v & 31) >> 2);
            const int lum = (R * 3 + G * 6 + B) / 10;
            if (lum >= 80) { sr += R; sg += G; sb += B; ++n; }
        }
    }
    if (n < 8) { on[0] = 192; on[1] = 76; on[2] = 248; }
    else { on[0] = (uint8_t)(sr / n); on[1] = (uint8_t)(sg / n); on[2] = (uint8_t)(sb / n); }
    dim[0] = (uint8_t)(on[0] * 52 / 100); dim[1] = (uint8_t)(on[1] * 52 / 100); dim[2] = (uint8_t)(on[2] * 52 / 100);
}

/* v1.2.1: same ink, computed once per slot (v1.2 re-inflated board strips 0 and 1 for every pip row of every frame) */
static void lv_pg_pip_ink(uint8_t on[3], uint8_t dim[3])
{
    static int slot_c = -1; static uint8_t on_c[3], dim_c[3];
    const int slot = lv_cb_slot();
    if (slot != slot_c) { lv_pg_pip_ink_calc(on_c, dim_c); slot_c = slot; lv_cb_cached = -1; }
    memcpy(on, on_c, 3); memcpy(dim, dim_c, 3);
}

static uint16_t lv_pg_pack(int r, int g, int b)
{
    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;
    return (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

static void lv_pg_pips_row(uint16_t *row, int y, int page)
{
    if (y < 25 || y > 29) return;
    uint8_t on[3], dim[3];
    lv_pg_pip_ink(on, dim);
    const float CX0 = 104.f, PITCH = 8.f, CY = 27.f, R_OFF = 1.35f, R_ON = 1.75f;
    auto plot = [&](int x, float cr, float cg, float cb, float a) {
        if (a <= 0.f || x < 4 || x > 235) return;
        const int R = ((row[x] >> 11) << 3) | ((row[x] >> 11) >> 2);
        const int G = (((row[x] >> 5) & 63) << 2) | (((row[x] >> 5) & 63) >> 4);
        const int B = ((row[x] & 31) << 3) | ((row[x] & 31) >> 2);
        if (R > 18 || G > 18 || B > 18) return;
        if (a > 1.f) a = 1.f;
        const int nr = (int)(cr * a + R * (1.f - a) + 0.5f);
        const int ng = (int)(cg * a + G * (1.f - a) + 0.5f);
        const int nb = (int)(cb * a + B * (1.f - a) + 0.5f);
        row[x] = lv_pg_pack(nr, ng, nb);
    };
    for (int i = 0; i < 5; ++i) {
        const bool lit = (i == page);
        const float cx = CX0 + i * PITCH, rad = lit ? R_ON : R_OFF, a0 = lit ? 1.f : 0.85f;
        const uint8_t *c = lit ? on : dim;
        const int x0 = (int)floorf(cx - rad - 1.5f), x1 = (int)ceilf(cx + rad + 1.5f);
        for (int x = x0; x <= x1; ++x) {
            const float d = hypotf(x + 0.5f - cx, y + 0.5f - CY) - rad;
            const float cov = fminf(1.f, fmaxf(0.f, 0.5f - d));
            if (cov > 0.f) plot(x, c[0], c[1], c[2], cov * a0);
        }
        if (lit) {
            const float sig = 1.1f, inv = 1.f / (2.f * sig * sig);
            for (int x = x0 - 1; x <= x1 + 1; ++x) {
                const float d = hypotf(x + 0.5f - cx, y + 0.5f - CY) - rad;
                if (d <= 0.f) continue;
                plot(x, c[0], c[1], c[2], 0.35f * expf(-d * d * inv));
            }
        }
    }
}

/* Compose one page into a 240-wide RGB565 row at y. Does not paint the live ticker text. */
static void lv_pg_compose_row(uint16_t *row, int y, int page, LvPgSet &S)
{
    const int slot = lv_cb_slot();
    const uint8_t *st = lv_cb_strip(slot, y / LV_CB_STRIP);
    if (!st) { for (int x = 0; x < 240; ++x) row[x] = 0; return; }
    const uint8_t *r = st + (y % LV_CB_STRIP) * 240 * 2;
    for (int x = 0; x < 240; ++x) row[x] = (uint16_t)(r[2 * x] | (r[2 * x + 1] << 8));

    if (page == 2 && lv_cb_pan && lv_cb_pan_slot == slot && y >= LV_CB_PY0 && y < LV_CB_PY0 + LV_CB_PH)
        memcpy(row + LV_CB_PX0, lv_cb_pan + (y - LV_CB_PY0) * LV_CB_PW, LV_CB_PW * 2);
    if (page != 2 && y >= LV_CB_PY0 && y < LV_CB_PY0 + LV_CB_PH) {
        const uint8_t *pr = lv_pg_ch_row(S, false, lv_pg_pan_idx(slot, page), y - LV_CB_PY0);
        if (pr) memcpy(row + LV_CB_PX0, pr, LV_PG_PAN_W * 2);
    }

    if (page != 2 && y >= 88 && y <= 197) {
        const uint8_t *fr = lv_pg_ch_row(S, true, lv_pg_art(page), y - 88);
        if (fr) memcpy(row + 65, fr, LV_PG_FACE_S * 2);
    }

    if (lv_pg_corner && y >= 301 && y <= 319)
        memcpy(row + 221, lv_pg_corner + (y - 301) * LV_PG_CORNER_W, LV_PG_CORNER_W * 2);

    if (lv_cb_flash_on && page == 2 && y >= LV_CB_BOT.y0 && y <= LV_CB_BOT.y1)
        for (int x = LV_CB_BOT.x0; x <= LV_CB_BOT.x1; ++x) row[x] = lv_hi(row[x]);
    if (lv_cb_blk_on && page == 2 && lv_cb_pan && y >= lv_cb_blk.y0 && y <= lv_cb_blk.y1)
        for (int x = lv_cb_blk.x0; x <= lv_cb_blk.x1; ++x) row[x] = lv_cb_invert(row[x], x, y);

    lv_pg_pips_row(row, y, page);
}

/* ---- v1.2.1 (Mobandy 8:36 AM ET): Matrix pills. BLUE on the left pin rail = previous page, RED on the right = next.
 * Fixed colours (not themed), glossy, painted over the first three pins of each rail (3V/GND/D23, EN/VP/VN) at the coil's
 * height, below the top border. Nothing else is covered. Tap = one page, no wrap (edge = the usual 18 px bounce). ---- */
static const LvRect LV_PG_PILL_L = {5, 32, 28, 67}, LV_PG_PILL_R = {211, 32, 234, 67};    /* painted capsules (24 x 36) */
static const LvRect LV_PG_PILL_LH = {3, 30, 30, 69}, LV_PG_PILL_RH = {209, 30, 236, 69};  /* hit rects: rail only, never the coil well */
static const LvRect LV_PG_COIL = {32, 30, 207, 69};   /* the only swipe zone: the coil strip above the chip */
static int lv_pg_pill_on = 0; static uint32_t lv_pg_pill_t = 0;   /* 1 blue, 2 red: pressed flash */
#define LV_PG_PILL_MS 130
static void lv_pg_pill_row(uint16_t *row, int y, const LvRect &P, int R, int G, int B, bool pressed)
{
    if (y < P.y0 || y > P.y1) return;
    const float w = (float)(P.x1 - P.x0 + 1), rad = w * 0.5f, cx = P.x0 + rad, ya = P.y0 + rad, yb = P.y1 + 1 - rad;
    const float py = y + 0.5f, cy = py < ya ? ya : py > yb ? yb : py;
    const uint16_t bg = row[P.x0 < 120 ? P.x0 - 1 : P.x1 + 1];   /* rail background: pins 1-3 under the pill box are hidden, not peeking past the caps */
    for (int x = P.x0; x <= P.x1; ++x) row[x] = bg;
    for (int x = P.x0; x <= P.x1; ++x) {
        const float px = x + 0.5f, dx = px - cx, dy = py - cy, dist = sqrtf(dx * dx + dy * dy);
        float cov = rad - dist + 0.5f; if (cov <= 0.f) continue; if (cov > 1.f) cov = 1.f;
        const float n = dx / rad;                                      /* -1 left .. +1 right */
        float sh = 0.42f + 0.58f * sqrtf(fmaxf(0.f, 1.f - n * n));    /* cylinder */
        sh *= 1.f - 0.25f * (py - P.y0) / (P.y1 - P.y0 + 1);           /* a little darker toward the bottom */
        float r = R * sh, g = G * sh, b = B * sh;
        const float t = (py - P.y0) / (P.y1 - P.y0 + 1);
        const float sx = (n + 0.38f) / 0.16f;                          /* specular streak, upper left */
        const float spec = expf(-sx * sx) * (t < 0.72f ? 1.f : fmaxf(0.f, 1.f - (t - 0.72f) / 0.12f)) * (t > 0.06f ? 0.85f : 0.4f);
        r += (255.f - r) * spec; g += (255.f - g) * spec; b += (255.f - b) * spec;
        if (dist > rad - 1.2f) { r *= 0.55f; g *= 0.55f; b *= 0.55f; }   /* dark rim */
        if (pressed) { r += (255.f - r) * 0.55f; g += (255.f - g) * 0.55f; b += (255.f - b) * 0.55f; }
        const uint16_t v = row[x];
        const int R0 = ((v >> 11) << 3) | ((v >> 11) >> 2), G0 = (((v >> 5) & 63) << 2) | (((v >> 5) & 63) >> 4), B0 = ((v & 31) << 3) | ((v & 31) >> 2);
        row[x] = lv_pg_pack((int)(r * cov + R0 * (1.f - cov) + 0.5f), (int)(g * cov + G0 * (1.f - cov) + 0.5f), (int)(b * cov + B0 * (1.f - cov) + 0.5f));
    }
}
static void lv_pg_pills_row(uint16_t *row, int y)
{
    if (y < LV_PG_PILL_L.y0 || y > LV_PG_PILL_L.y1) return;
    lv_pg_pill_row(row, y, LV_PG_PILL_L, 30, 84, 235, lv_pg_pill_on == 1);   /* blue pill */
    lv_pg_pill_row(row, y, LV_PG_PILL_R, 220, 26, 30, lv_pg_pill_on == 2);   /* red pill */
}

static float lv_pg_ease(float t)
{
    if (t < 0.f) t = 0.f; if (t > 1.f) t = 1.f;
    return t < 0.5f ? 4.f * t * t * t : 1.f - powf(-2.f * t + 2.f, 3.f) / 2.f;
}

static int lv_pg_off_now(void)
{
    if (lv_pg_dragging) return lv_pg_drag;
    if (lv_pg_anim == 0) return 0;
    const float t = (float)((int32_t)(lv_now - lv_pg_t0)) / (float)LV_PG_MS;
    const float e = lv_pg_ease(t);
    if (lv_pg_anim == 1) return (int)lroundf((lv_pg_dir > 0 ? -240.f : 240.f) * e);
    /* bounce: nudge toward the missing neighbour, then back */
    const float u = (t < 0.5f) ? lv_pg_ease(t * 2.f) : (1.f - lv_pg_ease((t - 0.5f) * 2.f));
    return (int)lroundf((lv_pg_dir > 0 ? -1.f : 1.f) * (float)LV_PG_BOUNCE * u);
}

static void lv_pg_scene_row(LvBand &B, int page_a, int page_b, int off)
{
    uint16_t ra[240], rb[240], out[240];
    for (int y = B.y0; y < B.y0 + B.h; ++y) {
        lv_pg_compose_row(ra, y, page_a, lv_pg_set[0]);
        if (off == 0) memcpy(out, ra, sizeof out);
        else {
            lv_pg_compose_row(rb, y, page_b, lv_pg_set[1]);
            /* off < 0 : A slides left, B enters from the right */
            if (off <= -240) memcpy(out, rb, sizeof out);
            else if (off >= 240) memcpy(out, rb, sizeof out);
            else if (off < 0) {
                const int k = -off;
                memcpy(out, ra + k, (240 - k) * 2);
                memcpy(out + (240 - k), rb, k * 2);
            } else {
                memcpy(out, rb + (240 - off), off * 2);
                memcpy(out + off, ra, (240 - off) * 2);
            }
        }
        lv_pg_pills_row(out, y);   /* chrome: the pills stay put while the pages slide */
        uint16_t *d = B.px + (y - B.y0) * B.w;
        const int x0 = B.x0, x1 = B.x0 + B.w;
        for (int x = x0; x < x1; ++x) d[x - x0] = out[x];
    }
}

struct LvPgCtx { int a, b, off; };
static void lv_pg_scene(LvBand &B, void *v) { const LvPgCtx *c = (const LvPgCtx *)v; lv_pg_scene_row(B, c->a, c->b, c->off); }

static void lv_pg_blit(int a, int b, int off, bool full)
{
    lv_cb_cached = -1;
    /* page a: io->buf tail (static). page b (slides only): heap for this frame. Keys reset: io->buf may have held a clip. */
    lv_pg_set[0] = {lv_cb_io ? lv_cb_io->buf + LV_PG_TAIL : nullptr, -1, -1};
    lv_pg_set[1] = {nullptr, -1, -1};
    const bool two = off != 0 && b != a;
    if (two && b != 2) {
        if (a == 2) lv_pg_set[1].mem = lv_pg_set[0].mem;   /* page a is CONTROLbot (no chunks): b takes the static set */
        else if (!(lv_pg_set[1].mem = (uint8_t *)malloc(LV_PG_SET_B))) {
            /* no heap: draw this frame still on page a rather than page b with no art */
            if (!lv_pg_heap_warned) { lv_print("PAGER: slide frame heap %u B unavailable", (unsigned)LV_PG_SET_B); lv_pg_heap_warned = true; }
            off = 0; b = a; full = true;
        }
    }
    const bool need_cb = (a == 2) || (off != 0 && b == 2);
    if (need_cb) lv_cb_panel_build(lv_cb_slot());   /* heap only while drawing, same as the v1.1 CONTROLbot screen */
    LvPgCtx c{a, b, off};
    if (full || off != 0) {
        lv_render(lv_blit_hw, LvRect{0, 0, 239, 319}, lv_pg_scene, &c);
    } else {
        /* still page: same band split as CONTROLbot so the live ticker can paint y=1..23 */
        lv_render(lv_blit_hw, LvRect{0, 0, 239, 0}, lv_pg_scene, &c);
        lv_render(lv_blit_hw, LvRect{0, 1, 0, 23}, lv_pg_scene, &c);
        lv_render(lv_blit_hw, LvRect{239, 1, 239, 23}, lv_pg_scene, &c);
        lv_render(lv_blit_hw, LvRect{0, 24, 239, 319}, lv_pg_scene, &c);
    }
    if (need_cb) lv_cb_panel_free();
    if (lv_pg_set[1].mem && lv_pg_set[1].mem != lv_pg_set[0].mem) free(lv_pg_set[1].mem);
    lv_pg_set[1] = {nullptr, -1, -1};
    lv_cb_cached = -1;   /* the tail decode does not touch the strip, but a clip may reuse io->buf next */
}

static void lv_pg_enter(void)
{
    lv_pg_dirty = true;
    lv_cb_enter();
    lv_pg_anim = 0; lv_pg_dragging = false; lv_pg_drag = 0;
}

static void lv_pg_leave_cache(void) { lv_pg_free_cache(); lv_cb_panel_free(); lv_pg_dragging = false; lv_pg_drag = 0; lv_pg_anim = 0; lv_pg_pill_on = 0; }   /* v1.2.1: called by lv_go when HOME is left */

static void lv_pg_start_slide(int dir)
{
    const int nxt = lv_pg_page + dir;
    if (nxt < 0 || nxt >= LV_PG_N) {
        lv_pg_anim = 2; lv_pg_dir = dir; lv_pg_from = lv_pg_page; lv_pg_to = lv_pg_page; lv_pg_t0 = lv_now;
        return;
    }
    lv_pg_anim = 1; lv_pg_dir = dir; lv_pg_from = lv_pg_page; lv_pg_to = nxt; lv_pg_t0 = lv_now;
    lv_pg_ensure(lv_pg_to);
}

static void lv_pg_frame(bool full)
{
    static int seen_slot = -1;
    const int slot = lv_cb_slot();
    if (slot != seen_slot) { seen_slot = slot; lv_pg_dirty = true; lv_pg_corner_key = -1; }
    lv_pg_ensure(lv_pg_page);
    if (lv_pg_anim) {
        if ((int32_t)(lv_now - lv_pg_t0) >= LV_PG_MS) {
            if (lv_pg_anim == 1) lv_pg_page = lv_pg_to;
            lv_pg_anim = 0; lv_pg_drag = 0; full = true;
        }
    }
    if (lv_pg_pill_on && (int32_t)(lv_now - lv_pg_pill_t) >= LV_PG_PILL_MS) { lv_pg_pill_on = 0; lv_pg_dirty = true; }
    const int off = lv_pg_off_now();
    const int a = (lv_pg_anim == 1) ? lv_pg_from : lv_pg_page;
    const int b = (lv_pg_anim == 1) ? lv_pg_to : lv_pg_page;
    if (full || lv_pg_dirty || lv_pg_anim || lv_pg_dragging || lv_cb_panel_dirty || lv_cb_flash_on || lv_cb_blk_on || lv_pg_pill_on) {
        lv_pg_blit(a, b, off, full || lv_pg_anim || lv_pg_dragging);
        lv_pg_dirty = false;
    }
    if (lv_cb_blk_on && (int32_t)(lv_now - lv_cb_blk_t) >= LV_CB_BLK_MS) { lv_cb_blk_on = false; lv_cb_panel_dirty = true; }
    if (lv_cb_flash_on && (int32_t)(lv_now - lv_cb_flash_t) >= 130) {
        lv_cb_flash_on = false; lv_pg_dirty = true;
    }
    lv_ticker_tick();
}

static void lv_pg_pill_go(int pill, int dir)
{
    lv_pg_pill_on = pill; lv_pg_pill_t = lv_now; lv_pg_dirty = true;
    lv_pg_start_slide(dir);   /* no wrap: past an end this is the 18 px bounce */
}

static const char *lv_pg_tap(int x, int y)
{
    if (lv_pg_swiped) { lv_pg_swiped = false; return "pager-swipe"; }
    if (lv_pg_anim) return "pager-busy";
    if (lv_in(LV_PG_PILL_LH, x, y)) { lv_pg_pill_go(1, -1); return "pager-pill-blue-prev"; }
    if (lv_in(LV_PG_PILL_RH, x, y)) { lv_pg_pill_go(2, +1); return "pager-pill-red-next"; }
    if (y <= LV_PG_TICK_Y1) return "pager-ticker";   /* banner: not a swipe zone, does not jump to SCAN */
    if (lv_in(LV_PG_COIL, x, y)) return "pager-coil";
    if (lv_pg_page == 2) return lv_controlbot_tap(x, y); /* v1.1 panel + face theme cycle; BACKbot skipped when lv_scr is HOME */
    if (lv_in(LV_CB_FACE, x, y)) {
        if (lv_pg_page == 0) { lv_go_soon(LV_SC_MO, 0); return "pager-botpheus-eb3"; }
        if (lv_pg_page == 1) { lv_go_soon(LV_SC_RADAR, 0); return "pager-gibson-radar"; }
        if (lv_pg_page == 3) { lv_go_soon(LV_SC_DL, 0); return "pager-hackbot-game"; }
        if (lv_pg_page == 4) { lv_go_soon(LV_SC_TERM, 0); return "pager-terminal"; }
    }
    return "none";
}

static void lv_pg_touch(bool down, int x, int y)
{
    if (lv_pg_anim) return;
    if (down) {
        if (!lv_pg_dragging) {
            if (!lv_in(LV_PG_COIL, x, y)) return;   /* v1.2.1: swipe starts on the coil strip only (not the banner) */
            lv_pg_dragging = true; lv_pg_dx0 = x; lv_pg_dy0 = y; lv_pg_drag = 0;
        } else {
            int dx = x - lv_pg_dx0;
            if ((lv_pg_page == 0 && dx > 0) || (lv_pg_page == LV_PG_N - 1 && dx < 0)) dx = dx * 35 / 100;
            lv_pg_drag = dx;
            lv_pg_dirty = true;
        }
        return;
    }
    if (!lv_pg_dragging) return;
    const int dx = lv_pg_drag;
    lv_pg_dragging = false; lv_pg_drag = 0;
    const int thresh = 240 * 18 / 100;   /* 18 % of the panel, same idea as the HTML mock */
    if (dx <= -thresh || dx <= -40) { lv_pg_swiped = true; lv_pg_start_slide(+1); }
    else if (dx >= thresh || dx >= 40) { lv_pg_swiped = true; lv_pg_start_slide(-1); }
    else if ((lv_pg_page == 0 && dx > 8) || (lv_pg_page == LV_PG_N - 1 && dx < -8)) {
        lv_pg_swiped = true; lv_pg_start_slide(dx < 0 ? +1 : -1); /* bounce at the end */
    } else {
        lv_pg_dirty = true; /* snap back */
    }
}

static void lv_pg_set_page(int p)
{
    if (p < 0) p = 0; if (p >= LV_PG_N) p = LV_PG_N - 1;
    lv_pg_page = p; lv_pg_anim = 0; lv_pg_dirty = true;
}
