/* fw-bigbot-1117: the five v3.1 big-bot screens are the new CYD home (the pager).
 * LV_SC_HOME draws the locked v6 board of the current theme (same stills as CONTROLbot),
 * with BACKbot replaced by the restored mounting-hole chrome, the page's identity face
 * and instrument panel (CONTROLbot keeps the locked v6 live panel), and LV_PG_N theme-ink pips.
 * v1.2.2: arrow-chip cycle buttons (theme-ink accent); page order HACKbot, BOTpheus,
 * GIBSONbot, CONTROLbot (starts on GIBSONbot; v1.2.6 dropped the TERMINALbot page).
 * LV_PG_TERMINAL_PAGE (mock/v1219, default 1): restore the v1.2.5 gold TERMINALbot page as
 * slot 3 in the 5-page order BOTpheus, HACKbot, GIBSONbot, TERMINALbot, CONTROLbot. Swipe on the coil strip
 * (LV_PG_COIL) or tap a chip flips pages, 233 ms ease-in-out cubic, no wrap (edge bounce
 * ~18 px). Center taps: BOTpheus -> EB3, HACKbot -> game, GIBSONbot -> radar,
 * TERMINALbot -> TERM, CONTROLbot cycles the theme (whole pager follows). BACKbot on
 * every sub-screen returns here. go ctl lands on the CONTROLbot page.
 */
#pragma once
#include <math.h>
#include "lv_pager_data.h"
#if !defined(ARDUINO)
#include <time.h>
#define LV_PG_PROF 1
#else
#define LV_PG_PROF 0
#endif

#ifndef LV_PG_TERMINAL_PAGE
#define LV_PG_TERMINAL_PAGE 1   /* mock/v1219: restore v1.2.5 TERMINALbot pager page; set 0 to drop this layer */
#endif
#ifndef LV_CYD19_LOGBOT
#define LV_CYD19_LOGBOT 0   /* CYD-19 mock (sim-only, NOT in any bin): LOGbot pager page between TERMINALbot and CONTROLbot */
#endif
#if LV_PG_TERMINAL_PAGE && LV_CYD19_LOGBOT
#define LV_PG_N        6
#define LV_PG_BOTPHEUS 0
#define LV_PG_HACKBOT  1
#define LV_PG_GIBSON   2
#define LV_PG_TERMINAL 3
#define LV_PG_LOGBOT   4
#define LV_PG_CTL      5
#elif LV_PG_TERMINAL_PAGE
#define LV_PG_N        5
#define LV_PG_BOTPHEUS 0
#define LV_PG_HACKBOT  1
#define LV_PG_GIBSON   2
#define LV_PG_TERMINAL 3
#define LV_PG_CTL      4
#else
#define LV_PG_N        4
#define LV_PG_HACKBOT  0
#define LV_PG_BOTPHEUS 1
#define LV_PG_GIBSON   2
#define LV_PG_CTL      3
#endif
#define LV_PG_DEFAULT  LV_PG_GIBSON
#ifndef LV_PG_MS
#define LV_PG_MS       100   /* v1.2.20 (Mobandy "then build", hit-list item 2): 233 -> 100 ms, ~3 slide frames instead of ~8: each 240x296 slide frame is ~30 ms of SPI (no TE sync), so fewer frames = fewer torn frames */
#endif
#define LV_PG_BOUNCE   18
#define LV_PG_TICK_Y1  29  /* banner + pip band: taps here do nothing on the pager */
#define LV_PG_SLIDE_CAP 45056u  /* 44 KB: board RLE (~33 KB) + second 8 KB decode set, under the ~50 KB HOME heap */

static int lv_pg_page = LV_PG_DEFAULT;
static bool lv_pg_dirty = true;
static bool lv_pg_swiped = false;     /* swallow the tap that follows a swipe */

/* slide / bounce */
static int lv_pg_anim = 0;            /* 0 still, 1 slide, 2 bounce */
static int lv_pg_from = LV_PG_DEFAULT, lv_pg_to = LV_PG_DEFAULT, lv_pg_dir = 1;
static uint32_t lv_pg_t0 = 0;
static int lv_pg_drag = 0;            /* live finger dx while down on the coil */
static bool lv_pg_dragging = false;
static int lv_pg_dx0 = 0, lv_pg_dy0 = 0;

/* v1.2.1 heap/chunk notes still apply. v1.2.2 smoothness: each 233 ms flip was re-inflating all
 * 20 board strips plus every face/panel chunk, and malloc/free of the second decode set, on every
 * frame (keys and lv_cb_cached were reset in lv_pg_blit). The slide cache inflates the board once
 * into a budgeted RLE, pre-decodes as many overlays as the same budget allows, keeps the second
 * set and the live CONTROLbot panel for the flip, and does not touch BOTpheus Dojo internals. */
#define LV_PG_FCH_B  (LV_PG_CH * LV_PG_FACE_S * 2)   /* 3,520 B: one face chunk */
#define LV_PG_PCH_B  (LV_PG_CH * LV_PG_PAN_W * 2)    /* 4,864 B: one panel chunk */
#define LV_PG_GAP    32                              /* guard bytes between regions */
#define LV_PG_SET_B  (LV_PG_FCH_B + LV_PG_GAP + LV_PG_PCH_B)
#define LV_PG_TAIL   (240 * LV_CB_STRIP * 2 + 64)    /* io->buf offset past the board strip */
static_assert(LV_PG_TAIL + LV_PG_SET_B <= LK_INFL_CAP, "pager decode set must fit the io->buf tail");
struct LvPgSet { uint8_t *mem; int fkey, pkey; };
static LvPgSet lv_pg_set[2] = {{nullptr, -1, -1}, {nullptr, -1, -1}};
static uint8_t *lv_pg_set1_heap = nullptr;
static uint16_t *lv_pg_corner = nullptr;   /* 722 B, freed when HOME is left */
static int lv_pg_corner_key = -1;
static bool lv_pg_heap_warned = false;
static uint8_t *lv_pg_rle = nullptr;
static uint16_t lv_pg_rle_off[320];
static uint32_t lv_pg_rle_n = 0, lv_pg_rle_cap = 0;
static int lv_pg_rle_slot = -1;
static uint32_t lv_pg_slide_used = 0;
static bool lv_pg_cb_hold = false;
static bool lv_pg_cache_on = false;

#if LV_PG_PROF
static bool lv_pg_prof = false;
static uint32_t lv_pg_flip_frames, lv_pg_flip_us_sum, lv_pg_flip_us_max, lv_pg_flip_infl, lv_pg_ninfl;
static uint32_t lv_pg_us_now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000000ul + (uint32_t)(ts.tv_nsec / 1000));
}
#endif

static int lv_pg_art(int page)
{
#if LV_PG_TERMINAL_PAGE && LV_CYD19_LOGBOT
    static const int8_t A[LV_PG_N] = {0, 2, 1, 3, 2, -1};   /* CYD-19: LOGbot reuses blob 2 (the chip-plate shading) like TERMINALbot */
#elif LV_PG_TERMINAL_PAGE
    /* 7db0c6d: blobs 0 BOTpheus, 1 GIBSONbot, 2 HACKbot, 3 TERMINALbot; compose uses blob 2 + gold for TERMINAL */
    static const int8_t A[LV_PG_N] = {0, 2, 1, 3, -1};
#else
    /* blobs stay 0 BOTpheus, 1 GIBSONbot, 2 HACKbot; HACKbot face is procedural so art[0] is unused */
    static const int8_t A[LV_PG_N] = {2, 0, 1, -1};
#endif
    return (page >= 0 && page < LV_PG_N) ? A[page] : -1;
}

static bool lv_pg_inflate(const uint8_t *z, uint32_t n, uint8_t *dst, uint32_t cap, uint32_t expect)
{
    if (!lv_cb_io || !z || !n) return false;
#if LV_PG_PROF
    if (lv_pg_prof) ++lv_pg_ninfl;
#endif
    const int got = lv_cb_io->inflate(z, n, dst, cap);
    return got == (int)expect;
}

#if LV_PG_PROF
static const uint8_t *lv_pg_strip(int slot, int s)
{
    const int key = slot * LV_CB_NSTRIP + s;
    if (lv_pg_prof && key != lv_cb_cached) ++lv_pg_ninfl;
    return lv_cb_strip(slot, s);
}
#else
#define lv_pg_strip(slot, s) lv_cb_strip((slot), (s))
#endif

static void *lv_pg_slide_alloc(uint32_t bytes)
{
    if (!bytes || lv_pg_slide_used + bytes > LV_PG_SLIDE_CAP) return nullptr;
    void *p = malloc(bytes);
    if (!p) return nullptr;
    lv_pg_slide_used += bytes;
    return p;
}

static void lv_pg_slide_reclaim(void *p, uint32_t bytes)
{
    if (!p) return;
    free(p);
    if (lv_pg_slide_used >= bytes) lv_pg_slide_used -= bytes; else lv_pg_slide_used = 0;
}

static void lv_pg_rle_free(void)
{
    if (lv_pg_rle) { lv_pg_slide_reclaim(lv_pg_rle, lv_pg_rle_cap); lv_pg_rle = nullptr; }
    lv_pg_rle_n = 0; lv_pg_rle_cap = 0; lv_pg_rle_slot = -1;
}

static void lv_pg_set1_free(void)
{
    if (lv_pg_set1_heap) { lv_pg_slide_reclaim(lv_pg_set1_heap, LV_PG_SET_B); lv_pg_set1_heap = nullptr; }
    lv_pg_set[1] = {nullptr, -1, -1};
}

static void lv_pg_cb_release(void)
{
    if (lv_pg_cb_hold) { lv_cb_panel_free(); lv_pg_cb_hold = false; }
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

/* pack the current theme's board into a 16-bit-pixel RLE (count, lo, hi). one inflate of each strip. */
static bool lv_pg_rle_build(void)
{
    const int slot = lv_cb_slot();
    if (lv_pg_rle && lv_pg_rle_slot == slot) return true;
    /* scratch is not charged to the budget; the kept buffer is realloc'd to the exact run length */
    const uint32_t cap = LV_PG_SLIDE_CAP - lv_pg_slide_used;
    if (cap < 1024) return false;
    uint8_t *z = (uint8_t *)malloc(cap);
    if (!z) return false;
    uint32_t n = 0;
    for (int y = 0; y < 320; ++y) {
        if (n > 0xFFF0) { free(z); return false; }
        lv_pg_rle_off[y] = (uint16_t)n;
        const uint8_t *st = lv_pg_strip(slot, y / LV_CB_STRIP);
        if (!st) { free(z); return false; }
        const uint8_t *r = st + (y % LV_CB_STRIP) * 240 * 2;
        int x = 0;
        while (x < 240) {
            const uint16_t v = (uint16_t)(r[2 * x] | (r[2 * x + 1] << 8));
            int k = 1;
            while (x + k < 240 && k < 255) {
                const uint16_t u = (uint16_t)(r[2 * (x + k)] | (r[2 * (x + k) + 1] << 8));
                if (u != v) break;
                ++k;
            }
            if (n + 3 > cap) { free(z); return false; }
            z[n++] = (uint8_t)k; z[n++] = (uint8_t)v; z[n++] = (uint8_t)(v >> 8);
            x += k;
        }
    }
    uint8_t *keep = (uint8_t *)realloc(z, n);
    if (!keep) { free(z); return false; }
    if (lv_pg_slide_used + n > LV_PG_SLIDE_CAP) { free(keep); return false; }
    lv_pg_slide_used += n;
    lv_pg_rle = keep; lv_pg_rle_n = n; lv_pg_rle_cap = n; lv_pg_rle_slot = slot;
    return true;
}

static void lv_pg_rle_row(uint16_t *row, int y)
{
    if (!lv_pg_rle || y < 0 || y > 319) { for (int x = 0; x < 240; ++x) row[x] = 0; return; }
    const uint8_t *p = lv_pg_rle + lv_pg_rle_off[y];
    const uint8_t *end = lv_pg_rle + (y < 319 ? lv_pg_rle_off[y + 1] : lv_pg_rle_n);
    int x = 0;
    while (p + 2 < end && x < 240) {
        const int k = p[0];
        const uint16_t v = (uint16_t)(p[1] | (p[2] << 8));
        int n = k; if (x + n > 240) n = 240 - x;
        for (int i = 0; i < n; ++i) row[x++] = v;
        p += 3;
    }
    while (x < 240) row[x++] = 0;
}

static void lv_pg_cache_begin(int a, int b)
{
    if (!lv_pg_cache_on) lv_pg_slide_used = 0;
    /* reserve the incoming 16-row set first so a large board RLE cannot disable the slide */
    if (!lv_pg_set1_heap && b != a && b != LV_PG_CTL && a != LV_PG_CTL)
        lv_pg_set1_heap = (uint8_t *)lv_pg_slide_alloc(LV_PG_SET_B);
    if (!lv_pg_rle || lv_pg_rle_slot != lv_cb_slot()) lv_pg_rle_build();
    lv_pg_cache_on = true;
}

static void lv_pg_cache_end(void)
{
    lv_pg_rle_free();
    lv_pg_set1_free();
    if (lv_pg_page != LV_PG_CTL) lv_pg_cb_release();
    lv_pg_cache_on = false;
    lv_pg_slide_used = 0;
}

/* row r (0-based) of face image idx / panel image idx, decoded through set S; nullptr if the chunk would not inflate */
static const uint8_t *lv_pg_ch_row(LvPgSet &S, bool face, int idx, int r)
{
    if (!S.mem || !face) return nullptr;   /* v1.2.4: panels are procedural */
    const int w = LV_PG_FACE_S, h = LV_PG_FACE_S;
    const int nch = LV_PG_FACE_NCH, c = r / LV_PG_CH, j = idx * nch + c;
    uint8_t *dst = S.mem;
    int &k = S.fkey;
    if (k != j) {
        const uint32_t *O = LV_PG_FACE_OFF;
        const uint8_t *Z = LV_PG_FACE_Z;
        const int rows = (h - c * LV_PG_CH) < LV_PG_CH ? (h - c * LV_PG_CH) : LV_PG_CH;
        if (!lv_pg_inflate(Z + O[j], O[j + 1] - O[j], dst, LV_PG_FCH_B, (uint32_t)(rows * w * 2))) { k = -1; return nullptr; }
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
    /* v1.2.22 (Mobandy 2026-11-25): the board ticker band is blank in LV_CB_Z, so this always fell back to GRAPE #C04CF8.
     * The pips now take the live theme accent (LV_ACC) when the band has no ink. */
    const int th = (lv_theme >= 0 && lv_theme < LV_NTHEME_ALL) ? lv_theme : 0;
    if (n < 8) { on[0] = (uint8_t)(LV_ACC[th] >> 16); on[1] = (uint8_t)(LV_ACC[th] >> 8); on[2] = (uint8_t)LV_ACC[th]; }
    else { on[0] = (uint8_t)(sr / n); on[1] = (uint8_t)(sg / n); on[2] = (uint8_t)(sb / n); }
    dim[0] = (uint8_t)(on[0] * 52 / 100); dim[1] = (uint8_t)(on[1] * 52 / 100); dim[2] = (uint8_t)(on[2] * 52 / 100);
}

/* v1.2.1: same ink, computed once per slot (v1.2 re-inflated board strips 0 and 1 for every pip row of every frame) */
static void lv_pg_pip_ink(uint8_t on[3], uint8_t dim[3])
{
    static int slot_c = -1; static uint8_t on_c[3], dim_c[3];
    const int slot = lv_theme;   /* v1.2.22: cache per theme (HOT PINK / BLUE share a board slot but not an accent) */
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
#if LV_PG_TERMINAL_PAGE && LV_CYD19_LOGBOT
    const float CX0 = 100.f, PITCH = 8.f, CY = 27.f, R_OFF = 1.35f, R_ON = 1.75f;   /* CYD-19: 6 pips, still centred on x=120 */
#elif LV_PG_TERMINAL_PAGE
    const float CX0 = 104.f, PITCH = 8.f, CY = 27.f, R_OFF = 1.35f, R_ON = 1.75f;   /* 7db0c6d: 5 pips, still centred on x=120 */
#else
    const float CX0 = 108.f, PITCH = 8.f, CY = 27.f, R_OFF = 1.35f, R_ON = 1.75f;   /* v1.2.6: 4 pips, still centred on x=120 */
#endif
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
    for (int i = 0; i < LV_PG_N; ++i) {
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
#include "lv_pager_fx.h"

static void lv_pg_compose_row(uint16_t *row, int y, int page, LvPgSet &S)
{
    const int slot = lv_cb_slot();
    if (lv_pg_rle && lv_pg_rle_slot == slot) lv_pg_rle_row(row, y);
    else {
        const uint8_t *st = lv_pg_strip(slot, y / LV_CB_STRIP);
        if (!st) { for (int x = 0; x < 240; ++x) row[x] = 0; }
        else {
            const uint8_t *r = st + (y % LV_CB_STRIP) * 240 * 2;
            for (int x = 0; x < 240; ++x) row[x] = (uint16_t)(r[2 * x] | (r[2 * x + 1] << 8));
        }
    }

    if (page == LV_PG_CTL && lv_cb_pan && lv_cb_pan_slot == slot && y >= LV_CB_PY0 && y < LV_CB_PY0 + LV_CB_PH)
        memcpy(row + LV_CB_PX0, lv_cb_pan + (y - LV_CB_PY0) * LV_CB_PW, LV_CB_PW * 2);
    if (page != LV_PG_CTL && y >= LV_CB_PY0 && y < LV_CB_PY0 + LV_CB_PH)
        lv_pg_panel_fx_row(row, y, page);
#if LV_CYD17_PANEL_TERM && LV_C17_CTL
    if (page == LV_PG_CTL && y >= LV_CB_PY0 && y < LV_CB_PY0 + LV_CB_PH) lv_c17_row(row, y, page);
#endif

    if (page != LV_PG_CTL && y >= 88 && y <= 197) {
#if LV_PG_TERMINAL_PAGE
        const uint8_t *fr = (page == LV_PG_HACKBOT) ? nullptr : lv_pg_ch_row(S, true, page == LV_PG_TERMINAL ? 2 : lv_pg_art(page), y - 88);   /* LOGbot: lv_pg_art = 2 too */
#else
        const uint8_t *fr = (page == LV_PG_HACKBOT) ? nullptr : lv_pg_ch_row(S, true, lv_pg_art(page), y - 88);
#endif
        lv_pg_face_fx_row(row, y, page, fr);
    }

    if (lv_pg_corner && y >= 301 && y <= 319)
        memcpy(row + 221, lv_pg_corner + (y - 301) * LV_PG_CORNER_W, LV_PG_CORNER_W * 2);

    if (lv_cb_flash_on && page == LV_PG_CTL && y >= LV_CB_BOT.y0 && y <= LV_CB_BOT.y1)
        for (int x = LV_CB_BOT.x0; x <= LV_CB_BOT.x1; ++x) row[x] = lv_hi(row[x]);
    if (lv_cb_blk_on && page == LV_PG_CTL && lv_cb_pan && y >= lv_cb_blk.y0 && y <= lv_cb_blk.y1)
        for (int x = lv_cb_blk.x0; x <= lv_cb_blk.x1; ++x) row[x] = lv_cb_invert(row[x], x, y);

    lv_pg_pips_row(row, y, page);
}

/* ---- v1.2.2 (Mobandy 12:04 PM ET): arrow chips replace the Matrix pills. Same 24 x 36 boxes,
 * same hit rects, same 130 ms press flash, over pins 1-3 only (3V/GND/D23 and EN/VP/VN), never the
 * coil (32,30)-(207,69) or the face (65,88)-(174,197). Dark IC body, metallic side pins, theme-ink
 * arrow (left = previous page, right = next). Drawn with integer fillRoundRect / fillTriangle / pin
 * rects, so the glossy float pill shader can go and the app still fits ota_0. ---- */
static const LvRect LV_PG_PILL_L = {5, 32, 28, 67}, LV_PG_PILL_R = {211, 32, 234, 67};    /* painted chips (24 x 36) */
static const LvRect LV_PG_PILL_LH = {3, 30, 30, 69}, LV_PG_PILL_RH = {209, 30, 236, 69};  /* hit rects: rail only, never the coil well */
static const LvRect LV_PG_COIL = {32, 30, 207, 69};   /* the only swipe zone: the coil strip above the chip */
static int lv_pg_pill_on = 0; static uint32_t lv_pg_pill_t = 0;   /* 1 left, 2 right: pressed flash */
#define LV_PG_PILL_MS 130
static uint32_t lv_pg_pill_guard = 0;   /* v1.2.5: ignore further chip taps until the flip has settled */
#define LV_PG_PILL_DEBOUNCE 280
static int lv_pg_chip_at(int x, int y)   /* 1 left, 2 right, 0 none; chip wins over the coil when the point is near both */
{
    const int pad = 6;
    const LvRect *R[2] = { &LV_PG_PILL_LH, &LV_PG_PILL_RH };
    int best = 0; long bd = 1L << 30;
    for (int i = 0; i < 2; ++i) {
        const LvRect &C = *R[i];
        const bool in = (x >= C.x0 - pad && x <= C.x1 + pad && y >= C.y0 - pad && y <= C.y1 + pad);
        if (!in) continue;
        const int cx = (C.x0 + C.x1) / 2, cy = (C.y0 + C.y1) / 2;
        const long d = (long)(x - cx) * (x - cx) + (long)(y - cy) * (y - cy);
        if (d < bd) { bd = d; best = i + 1; }
    }
    if (best == 1 && lv_pg_page == 0) return 0;   /* no page to the left of the first pager page */
    if (best == 2 && lv_pg_page == LV_PG_N - 1) return 0;   /* no page to the right of the last pager page */
    return best;
}

/* compact IC: rounded body + 6 metallic side pins + a 10x15 bit-mask arrow in theme ink */
static const uint16_t LV_PG_ARW[15] = { /* bit 0 = base, bit 7 = tip */
    0x0080, 0x00C0, 0x00E0, 0x00F0, 0x00F8,
    0x00FC, 0x00FE, 0x00FF, 0x00FE, 0x00FC,
    0x00F8, 0x00F0, 0x00E0, 0x00C0, 0x0080
};

static void lv_pg_chip_row(uint16_t *row, int y, const LvRect &P, int dir, bool pressed)
{
    if (y < P.y0 || y > P.y1) return;
    const int ly = y - P.y0;
    const uint16_t bg = row[P.x0 < 120 ? P.x0 - 1 : P.x1 + 1];
    const int th = (lv_theme >= 0 && lv_theme < LV_NTHEME_ALL) ? lv_theme : 0;
    const uint16_t acc = lv_hex(LV_ACC[th]), accd = lv_hex(LV_ACCD[th]);
    const uint16_t body = lv_pg_pack(28, 30, 34), bhi = lv_pg_pack(58, 60, 66), blo = lv_pg_pack(12, 12, 14);
    const uint16_t pin = lv_pg_pack(196, 188, 160), plo = lv_pg_pack(110, 102, 80);
    for (int lx = 0; lx < 24; ++lx) {
        uint16_t c = bg;
        const int rel = ly - 5;
        if (rel >= 0 && rel < 30 && (rel % 5) <= 1 && (lx <= 2 || lx >= 21))
            c = (lx == 0 || lx == 23 || (rel % 5) == 1) ? plo : pin;
        const int bx = lx - 3, by = ly - 2;
        if (bx >= 0 && bx < 18 && by >= 0 && by < 32) {
            int cx = bx < 2 ? 2 : (bx > 15 ? 15 : bx);
            int cy = by < 2 ? 2 : (by > 29 ? 29 : by);
            const int dx = bx - cx, dy = by - cy;
            if (dx * dx + dy * dy <= 4) {
                c = body;
                if (by <= 6 || bx == 0 || by == 0) c = bhi;
                if (bx == 17 || by == 31) c = blo;
                if (by <= 3 && bx >= 7 && bx <= 10) c = blo;
            }
        }
        const int ay = ly - 11, ax = (dir < 0) ? (lx - 7) : (16 - lx);
        if (ay >= 0 && ay < 15 && ax >= 0 && ax < 13 && (LV_PG_ARW[ay] & (1 << ax)))
            c = (ax == 0 || ay == 0 || ay == 14) ? accd : acc;
        if (pressed && c != bg) c = lv_hi(c);
        row[P.x0 + lx] = c;
    }
}

static void lv_pg_pills_row(uint16_t *row, int y)
{
    if (y < LV_PG_PILL_L.y0 || y > LV_PG_PILL_L.y1) return;
    if (lv_pg_page != 0)   /* no left arrow chip on the first pager page */
        lv_pg_chip_row(row, y, LV_PG_PILL_L, -1, lv_pg_pill_on == 1);
    if (lv_pg_page != LV_PG_N - 1)   /* no right arrow chip on the last pager page */
        lv_pg_chip_row(row, y, LV_PG_PILL_R, +1, lv_pg_pill_on == 2);
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
        lv_pg_pills_row(out, y);   /* chrome: the chips stay put while the pages slide */
        uint16_t *d = B.px + (y - B.y0) * B.w;
        const int x0 = B.x0, x1 = B.x0 + B.w;
        for (int x = x0; x < x1; ++x) d[x - x0] = out[x];
    }
}

struct LvPgCtx { int a, b, off; };
static void lv_pg_scene(LvBand &B, void *v) { const LvPgCtx *c = (const LvPgCtx *)v; lv_pg_scene_row(B, c->a, c->b, c->off); }

#if LV_PG_PROF
static void lv_pg_flip_log(const char *why)
{
    if (!lv_pg_flip_frames) return;
    const uint32_t avg = lv_pg_flip_us_sum / lv_pg_flip_frames;
    printf("PAGER: %s frames=%u avg=%u us max=%u us infl=%u cache=%u rle=%u\n",
             why, (unsigned)lv_pg_flip_frames, (unsigned)avg, (unsigned)lv_pg_flip_us_max,
             (unsigned)lv_pg_flip_infl, (unsigned)lv_pg_slide_used, (unsigned)lv_pg_rle_n);
}
#endif

#ifndef LV_PG_SLIDE_SPLIT
#define LV_PG_SLIDE_SPLIT 1   /* mock/v1220 item 2 (banner flicker on page change); 0 = v1.2.19 */
#endif
static void lv_pg_blit(int a, int b, int off, bool full)
{
#if LV_PG_PROF
    const uint32_t t0 = lv_pg_prof ? lv_pg_us_now() : 0;
    const uint32_t infl0 = lv_pg_ninfl;
#endif
    /* keep decode keys across flip frames; only bind the buffers */
    if (!lv_pg_set[0].mem) lv_pg_set[0] = {lv_cb_io ? lv_cb_io->buf + LV_PG_TAIL : nullptr, -1, -1};
    else lv_pg_set[0].mem = lv_cb_io ? lv_cb_io->buf + LV_PG_TAIL : nullptr;
    const bool two = off != 0 && b != a;
    if (two && b != LV_PG_CTL) {
        if (a == LV_PG_CTL) {
            lv_pg_set[1].mem = lv_pg_set[0].mem;
            if (lv_pg_set[1].fkey < 0 && lv_pg_set[1].pkey < 0) { lv_pg_set[1].fkey = -1; lv_pg_set[1].pkey = -1; }
        } else {
            if (!lv_pg_set1_heap) lv_pg_set1_heap = (uint8_t *)lv_pg_slide_alloc(LV_PG_SET_B);
            lv_pg_set[1].mem = lv_pg_set1_heap;
            if (!lv_pg_set[1].mem) {
                if (!lv_pg_heap_warned) { lv_print("PAGER: heap"); lv_pg_heap_warned = true; }
                off = 0; b = a; full = true;
            }
        }
    }
    const bool need_cb = (a == LV_PG_CTL) || (off != 0 && b == LV_PG_CTL);
    /* v1.2.25: rebuild when the theme slot changes (CONTROLbot face tap). The locked v6
     * stills still carry SCAN/VOL/CHIME/TIME/TONE; compose_row only overlays the live
     * RADAR/SLEEP/BRIGHT/RECAL/ROTATE/EXIT panel when lv_cb_pan_slot == slot. Same
     * condition as lv_cb_frame (ee9b449, never shipped). */
    if (need_cb && (!lv_cb_pan || lv_cb_panel_dirty || lv_cb_pan_slot != lv_cb_slot())) {
        if (lv_cb_panel_build(lv_cb_slot())) lv_pg_cb_hold = true;
    }
    LvPgCtx c{a, b, off};
    /* v1.2.25: never whole-screen-blit the still (its baked banner is the old look).
     * Entry/return used that path (lv_pg_entry_full), so the stale still banner sat on
     * the glass until the (slow) full push finished and lv_ticker_tick ran after it.
     * Band-split + early ticker (lv_pg_frame) paints the current banner first. */
    if (full || off != 0) {   /* band split: banner interior x1..238 y1..23 is not sent; ticker fills it */
        lv_render(lv_blit_hw, LvRect{0, 0, 239, 0}, lv_pg_scene, &c);
        lv_render(lv_blit_hw, LvRect{0, 1, 0, 23}, lv_pg_scene, &c);
        lv_render(lv_blit_hw, LvRect{239, 1, 239, 23}, lv_pg_scene, &c);
        lv_render(lv_blit_hw, LvRect{0, 24, 239, 319}, lv_pg_scene, &c);
    } else {
        /* still page: same band split as CONTROLbot so the live ticker can paint y=1..23 */
        lv_render(lv_blit_hw, LvRect{0, 0, 239, 0}, lv_pg_scene, &c);
        lv_render(lv_blit_hw, LvRect{0, 1, 0, 23}, lv_pg_scene, &c);
        lv_render(lv_blit_hw, LvRect{239, 1, 239, 23}, lv_pg_scene, &c);
        lv_render(lv_blit_dirty_hw, LvRect{0, 24, 239, 319}, lv_pg_scene, &c);   /* perf-all r2: still page sends changed rows only */
    }
    if (!lv_pg_cache_on && a != LV_PG_CTL) lv_pg_cb_release();
#if LV_PG_PROF
    if (lv_pg_prof) {
        const uint32_t dt = lv_pg_us_now() - t0;
        lv_pg_flip_us_sum += dt;
        if (dt > lv_pg_flip_us_max) lv_pg_flip_us_max = dt;
        lv_pg_flip_infl += (lv_pg_ninfl - infl0);
        ++lv_pg_flip_frames;
    }
#endif
}

static void lv_pg_enter(void)
{
    lv_pg_dirty = true;
    lv_cb_enter();
    lv_pg_anim = 0; lv_pg_dragging = false; lv_pg_drag = 0;
}

static void lv_pg_leave_cache(void)
{
    lv_pg_cache_end();
    lv_pg_free_cache();
    lv_cb_panel_free();
    lv_pg_dragging = false; lv_pg_drag = 0; lv_pg_anim = 0; lv_pg_pill_on = 0;
    lv_pg_set[0] = {nullptr, -1, -1};
}

#if LV_PG_PROF
static void lv_pg_prof_start(int kind)
{
    (void)kind;
    lv_pg_prof = true; lv_pg_ninfl = 0;
    lv_pg_flip_frames = 0; lv_pg_flip_us_sum = 0; lv_pg_flip_us_max = 0; lv_pg_flip_infl = 0;
}
#else
#define lv_pg_prof_start(kind) ((void)0)
#endif

static void lv_pg_start_slide(int dir)
{
    const int nxt = lv_pg_page + dir;
    if (nxt < 0 || nxt >= LV_PG_N) {
        lv_pg_anim = 2; lv_pg_dir = dir; lv_pg_from = lv_pg_page; lv_pg_to = lv_pg_page; lv_pg_t0 = lv_now;
        lv_pg_cache_begin(lv_pg_page, lv_pg_page);
        lv_pg_prof_start(2);
        return;
    }
    lv_pg_anim = 1; lv_pg_dir = dir; lv_pg_from = lv_pg_page; lv_pg_to = nxt; lv_pg_t0 = lv_now;
    lv_pg_ensure(lv_pg_to);
    lv_pg_cache_begin(lv_pg_from, lv_pg_to);
    lv_pg_prof_start(1);
}

static void lv_pg_frame(bool full)
{
    static int seen_slot = -1;
    const int slot = lv_cb_slot();
    if (slot != seen_slot) { seen_slot = slot; lv_pg_dirty = true; lv_pg_corner_key = -1; lv_pg_rle_slot = -1; }
    lv_pg_ensure(lv_pg_page);
    if (lv_pg_anim) {
        if ((int32_t)(lv_now - lv_pg_t0) >= LV_PG_MS) {
            if (lv_pg_anim == 1) lv_pg_page = lv_pg_to;
            lv_pg_anim = 0; lv_pg_drag = 0; full = true;
#if LV_PG_PROF
            if (lv_pg_prof) { lv_pg_flip_log("slide"); lv_pg_prof = false; }
#endif
            lv_pg_cache_end();
        }
    }
    if (lv_pg_pill_on && (int32_t)(lv_now - lv_pg_pill_t) >= LV_PG_PILL_MS) { lv_pg_pill_on = 0; lv_pg_dirty = true; }
    const int off = lv_pg_off_now();
    const int a = (lv_pg_anim == 1) ? lv_pg_from : lv_pg_page;
    const int b = (lv_pg_anim == 1) ? lv_pg_to : lv_pg_page;
    /* v1.2.5: rain faces + role panels are procedural and must paint every tick. After a CONTROLbot
     * theme change, seen_slot dirties once, the still blit runs, dirty is cleared, and the rain froze. */
    if (lv_pg_page != LV_PG_CTL && !lv_pg_anim && !lv_pg_dragging) lv_pg_dirty = true;
    /* v1.2.25: on a full entry/return frame, paint the live ticker first so the
     * current banner is on the glass before the slow body push. Do not tick twice
     * in that frame (that would double the scroll). */
    const bool body = full || lv_pg_dirty || lv_pg_anim || lv_pg_dragging || lv_cb_panel_dirty || lv_cb_flash_on || lv_cb_blk_on || lv_pg_pill_on;
    const bool banner_first = body && full && !lv_pg_anim && !lv_pg_dragging;
    if (banner_first) lv_ticker_tick();
    if (body) {
        lv_pg_blit(a, b, off, full || lv_pg_anim || lv_pg_dragging);
        lv_pg_dirty = false;
    }
    if (lv_cb_blk_on && (int32_t)(lv_now - lv_cb_blk_t) >= LV_CB_BLK_MS) { lv_cb_blk_on = false; lv_cb_panel_dirty = true; }
    if (lv_cb_flash_on && (int32_t)(lv_now - lv_cb_flash_t) >= 130) {
        lv_cb_flash_on = false; lv_pg_dirty = true;
    }
    if (!banner_first) lv_ticker_tick();
}

static void lv_pg_pill_go(int pill, int dir)
{
    lv_pg_pill_on = pill; lv_pg_pill_t = lv_now; lv_pg_pill_guard = lv_now; lv_pg_dirty = true;
    lv_pg_start_slide(dir);   /* no wrap: past an end this is the 18 px bounce */
}

static const char *lv_pg_tap(int x, int y)
{
    if (lv_pg_swiped) { lv_pg_swiped = false; return "pager-swipe"; }
    if (lv_pg_anim) return "pager-busy";
    if (lv_pg_pill_guard && (int32_t)(lv_now - lv_pg_pill_guard) < LV_PG_PILL_DEBOUNCE) return "pager-chip-debounce";
    { const int ch = lv_pg_chip_at(x, y);
      if (ch == 1) { lv_pg_pill_go(1, -1); return "pager-chip-left-prev"; }
      if (ch == 2) { lv_pg_pill_go(2, +1); return "pager-chip-right-next"; } }
    if (y <= LV_PG_TICK_Y1) return "pager-ticker";   /* banner: not a swipe zone, does not jump to SCAN */
    if (lv_in(LV_PG_COIL, x, y)) return "pager-coil";
    if (lv_pg_page == LV_PG_CTL) return lv_controlbot_tap(x, y);
    if (lv_in(LV_CB_FACE, x, y)) {
        if (lv_pg_page == LV_PG_BOTPHEUS) { lv_go_soon(LV_SC_MO, 0); return "pager-botpheus-eb3"; }
        if (lv_pg_page == LV_PG_HACKBOT)  { lv_go_soon(LV_SC_DL, 0); return "pager-hackbot-game"; }
        if (lv_pg_page == LV_PG_GIBSON)   { lv_go_soon(LV_SC_RADAR, 0); return "pager-gibson-radar"; }
#if LV_PG_TERMINAL_PAGE
#if LV_TPS_G2_ONLY
        if (lv_pg_page == LV_PG_TERMINAL) { lv_go_soon(LV_SC_TPSG2, 0); return "pager-terminal-g2"; }   /* v1.2.23: G2 is THE TERMINALbot screen */
#else
        if (lv_pg_page == LV_PG_TERMINAL) { lv_go_soon(LV_SC_TERM, 0); return "pager-terminal"; }   /* 7db0c6d */
#endif
#endif
    }
#if LV_PG_TERMINAL_PAGE && LV_TPS_G2_ONLY
    /* v1.2.25: the CYD-17 typist panel also opens G2 (it looks like the terminal). */
    if (lv_pg_page == LV_PG_TERMINAL && x >= LV_CB_PX0 && x < LV_CB_PX0 + LV_CB_PW && y >= LV_CB_PY0 && y < LV_CB_PY0 + LV_CB_PH) {
        lv_go_soon(LV_SC_TPSG2, 0); return "pager-terminal-g2-panel";
    }
#endif
#if LV_CYD19_LOGBOT
    if (lv_pg_page == LV_PG_LOGBOT && (lv_in(LV_CB_FACE, x, y) || (x >= LV_CB_PX0 && x < LV_CB_PX0 + LV_CB_PW && y >= LV_CB_PY0 && y < LV_CB_PY0 + LV_CB_PH))) {
        lv_lb_tab ^= 1; lv_pg_dirty = true; return "pager-logbot-tab";   /* CYD-19 mock: tap the face or the log panel toggles SCAN LOG <-> SYSTEM LOG */
    }
#endif
    return "none";
}

static void lv_pg_touch(bool down, int x, int y)
{
    if (lv_pg_anim) return;
    if (down) {
        if (!lv_pg_dragging) {
            if (lv_pg_chip_at(x, y)) return;   /* v1.2.5: a chip press is not a coil swipe; wait for the release tap */
            if (!lv_in(LV_PG_COIL, x, y)) return;   /* v1.2.1: swipe starts on the coil strip only (not the banner) */
            lv_pg_dragging = true; lv_pg_dx0 = x; lv_pg_dy0 = y; lv_pg_drag = 0;
            const int left = lv_pg_page - 1, right = lv_pg_page + 1;
            lv_pg_cache_begin(lv_pg_page, (right < LV_PG_N) ? right : (left >= 0 ? left : lv_pg_page));
            lv_pg_prof_start(3);
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
#if LV_PG_PROF
        if (lv_pg_prof) { lv_pg_flip_log("drag-cancel"); lv_pg_prof = false; }
#endif
        lv_pg_cache_end();
        lv_pg_dirty = true; /* snap back */
    }
}

static void lv_pg_set_page(int p)
{
    if (p < 0) p = 0; if (p >= LV_PG_N) p = LV_PG_N - 1;
    lv_pg_page = p; lv_pg_anim = 0; lv_pg_dirty = true;
}
