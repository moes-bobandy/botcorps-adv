/*
 * Cell-grid rain. Gap 6, 140 ms, drawn straight to the external panel.
 * The 3×1200 cell planes and the 56×8 column records are the Tier 0
 * buffers. No 153600-byte sprite is allocated.
 */
#include "rain/botcorps_rain.h"

#include "display/botcorps_display.h"
#include "ui/botcorps_band.h"
#include "ui/botcorps_bots.h"
#include "ui/botcorps_glyph.h"
#include "ui/botcorps_sprite.h"

#include <string.h>

static_assert(sizeof(botcorps_cell_t) == 3, "rain cell is 3 bytes");

extern uint8_t botcorps_t0_rain_cell[3][1200];
extern uint8_t botcorps_t0_rain_col[56][8];

static const uint16_t COL_FIELD_N = 0x0041; /* #070a08, native, for blends */
static const uint16_t COL_CODE_N  = 0x7E6C;
static const uint16_t COL_HEAD_N  = 0xEFF9;
static const int FADE_A = 46;
static const int BACK_A = 141;

static const uint16_t PALETTE[14] = {
    0x76C2, 0xFDC9, 0x1677, 0x591D, 0xE8EA, 0x1BDD, 0xF689,
    0xFA69, 0xE2FF, 0xCF89, 0xF79C, 0xFC43, 0x6BBF, 0x5FF8,
};
static const char DEV_GLYPHS[] = "*/#+@%&$X";
static int s_shape = 0;

static const uint16_t SOLID_INK[6] = {
    0x26E8, 0x269C, 0x331F, 0xB21F, 0xF586, 0xFFFF,
};

static const char CODE_GLYPHS[] = "01<>[]{}:/\\|+-=*#";
static const int CODE_N = 18;
/* v1016f classic code rain: 40x30 cells of 8 px, two drops per column, a white
 * lead glyph, a four-step fading trail, mostly 0/1 bits. Some leads are bots in
 * their own colour or device glyphs. State is the Tier 0 column records (56x8)
 * plus one glyph byte per cell in plane 0 of the Tier 0 cell buffer. */
static const int STEP_MS = 90;
static const int RAIN_COLS = 56;
static const int CELL = 8;
static const int CELL_COLS = 40;
static const int CELL_ROWS = 30;
static const int GAP = CELL;

enum { LEAD_CODE = 0, LEAD_BOT = 1, LEAD_DEV = 2 };

static int s_mode = 0;
static int s_mix = BOTCORPS_RAIN_MIX_FIELD;
static int s_ncols = 0;
static uint32_t s_rng = 0xA5C3E1u;
static uint32_t s_last_ms = 0;
static bool s_built = false;
static bool s_hub_on = false;
static bool s_draw = false;
static bool s_logged = false;
static bool s_radar = false;
static botcorps_cover_t s_cover[24];
static int s_ncover = 0;
static int s_slice_y0 = 0;
static int s_slice_y1 = 240;
/* Planes 1 and 2, bytes 1120..1179, hold the Tab completion scratch. Rain uses plane 0 only. */

static uint32_t rnd_u(void)
{
    s_rng = s_rng * 1664525u + 1013904223u;
    return s_rng >> 8;
}

static int rnd_n(int n)
{
    if (n <= 1) return 0;
    return (int)(rnd_u() % (uint32_t)n);
}

static uint16_t scale_native(uint16_t c, int num, int den)
{
    const int r5 = (c >> 11) & 31, g6 = (c >> 5) & 63, b5 = c & 31;
    if (den <= 0) return c;
    return (uint16_t)(((r5 * num / den) << 11) | ((g6 * num / den) << 5) | (b5 * num / den));
}

static uint8_t *col_at(int i) { return botcorps_t0_rain_col[i]; }
static uint8_t *glyph_at(int c, int r) { return &botcorps_t0_rain_cell[0][r * CELL_COLS + c]; }

/* Column record: [0] head A (int8 row), [1] len A, [2] head B, [3] len B,
 * [4] rows-per-step period, [5] tick, [6] lead A (kind | payload << 2), [7] lead B. */
static int drop_head(const uint8_t *c, int d) { return (int8_t)c[d * 2]; }

static uint8_t new_glyph(void)
{
    /* v1016l: bots + devices ride INSIDE the strands too (mock rainNext), bits stay the majority */
    const int r = rnd_n(100);
    if (r < 28) return '0';
    if (r < 56) return '1';                                                                /* bits stay the majority */
    if (r < 60) return (uint8_t)CODE_GLYPHS[2 + rnd_n(CODE_N - 2)];
    if (s_mix != BOTCORPS_RAIN_MIX_DEV && r < 82) return (uint8_t)(0x80 | rnd_n(12));      /* bot */
    if (s_mix != BOTCORPS_RAIN_MIX_BOTS) return (uint8_t)(0xA0 | rnd_n(9));               /* device */
    return '1';
}

static uint8_t new_lead(void)
{
    const int r = rnd_n(100);
    int bot = 20, dev = 12; /* v1016j: denser bots/devices in strands (match mock) */
    if (s_mix == BOTCORPS_RAIN_MIX_BOTS) { bot = 22; dev = 0; }
    else if (s_mix == BOTCORPS_RAIN_MIX_DEV) { bot = 0; dev = 18; }
    if (r < bot) return (uint8_t)(LEAD_BOT | (rnd_n(12) << 2));
    if (r < bot + dev) return (uint8_t)(LEAD_DEV | (rnd_n(9) << 2));
    return LEAD_CODE;
}

static void spawn(uint8_t *c, int d, bool scatter)
{
    const int len = 6 + rnd_n(17);
    c[d * 2 + 1] = (uint8_t)len;
    c[d * 2] = (uint8_t)(int8_t)(scatter ? rnd_n(CELL_ROWS + len) - len / 2 : -rnd_n(24));
    c[6 + d] = new_lead();
}

static void build_columns(void)
{
    s_ncols = CELL_COLS;
    memset(botcorps_t0_rain_col, 0, sizeof(botcorps_t0_rain_col));
    for (int r = 0; r < CELL_ROWS; ++r)
        for (int c = 0; c < CELL_COLS; ++c) *glyph_at(c, r) = new_glyph();
    for (int i = 0; i < s_ncols; ++i) {
        uint8_t *c = col_at(i);
        spawn(c, 0, true);
        spawn(c, 1, true);
        c[2] = (uint8_t)(int8_t)(drop_head(c, 0) - 15 - rnd_n(10));
        c[4] = (uint8_t)(1 + (rnd_n(10) < 3 ? 1 : 0) + (rnd_n(10) < 1 ? 1 : 0));
        c[5] = (uint8_t)rnd_n(3);
    }
    s_built = true;
    if (!s_logged) {
        s_logged = true;
        Serial.printf("[botcorps] rain path=classic %dx%d drops=2 step=%d\n", CELL_COLS, CELL_ROWS, STEP_MS);
    }
}

static uint16_t code_native(void)
{
    return s_mode == 0 ? COL_CODE_N : SOLID_INK[s_mode - 1];
}

/* 0 none, 1 lead, 2..5 trail steps. Brightest of the two drops wins. */
static int cell_level(int col, int row, int *lead_of)
{
    const uint8_t *c = col_at(col);
    int best = 0;
    for (int d = 0; d < 2; ++d) {
        const int h = drop_head(c, d), L = c[d * 2 + 1];
        const int k = h - row;
        if (k < 0 || k >= L) continue;
        int lv;
        if (k == 0) lv = 1;
        else if (k <= 2) lv = 2;
        else if (k * 100 < L * 50) lv = 3;
        else if (k * 100 < L * 78) lv = 4;
        else lv = 5;
        if (best == 0 || lv < best) {
            best = lv;
            if (lead_of) *lead_of = (lv == 1) ? c[6 + d] : 0;
        }
    }
    return best;
}

static bool rain_pixel_ok(int x, int y);
static bool icon_pixel_ok(int x, int y);

static bool cell_open(int x, int y)
{
    for (int dy = 0; dy < CELL; dy += 3)
        for (int dx = 0; dx < CELL; dx += 3)
            if (!rain_pixel_ok(x + dx, y + dy)) return false;
    return rain_pixel_ok(x + CELL - 1, y + CELL - 1);
}

static void draw_cell(lgfx::LovyanGFX &panel, int col, int row)
{
    const int x = col * CELL, wy = row * CELL;
    if (wy + CELL <= s_slice_y0 || wy >= s_slice_y1) return;
    if (!cell_open(x, wy)) return;
    const int ly = wy - s_slice_y0;
    int lead = 0;
    const int lv = cell_level(col, row, &lead);
    panel.setClipRect(x, ly < 0 ? 0 : ly, CELL, CELL);
    botcorps_pal_rect(panel, x, ly, CELL, CELL, PAL_FIELD);
    if (lv == 0) { panel.clearClipRect(); return; }
    if (lv == 1 && (lead & 3) == LEAD_BOT) {
        const int b = (lead >> 2) % 12;
        botcorps_sprite_draw(panel, x + 4, ly + 4, botcorps_shape_for(b), 1,
                             botcorps_pal_fit(BOTCORPS_BOT_COLOR[b]), false);
        panel.clearClipRect();
        return;
    }
    const uint8_t gl = *glyph_at(col, row);
    if (lv >= 2 && gl >= 0x80 && gl < 0x8C) {            /* bot inside the trail, fading with age */
        const int b = gl & 0x0F;
        const uint16_t c = BOTCORPS_BOT_COLOR[b];
        botcorps_sprite_draw(panel, x + 4, ly + 4, botcorps_shape_for(b), 1,
                             botcorps_pal_fit(lv <= 3 ? c : scale_native(c, lv == 4 ? 2 : 1, 3)), false);
        panel.clearClipRect();
        return;
    }
    uint8_t ink;
    char ch = (char)gl;
    if (lv >= 2 && gl >= 0xA0) {                          /* device glyph inside the trail */
        const int d = (gl & 0x0F) % 9;
        ch = DEV_GLYPHS[d];
        const uint16_t dc = BOTCORPS_BOT_COLOR[(d * 5 + 3) % 12];          /* neon device colours like the mock */
        ink = botcorps_pal_fit(lv <= 3 ? dc : scale_native(dc, lv == 4 ? 2 : 1, 3));
        const char s2[2] = { ch, 0 };
        panel.setFont(&fonts::Font0);
        panel.setTextSize(1);
        botcorps_pal_text(panel, ink);
        panel.setCursor(x + 1, ly);
        panel.print(s2);
        panel.clearClipRect();
        return;
    }
    if (lv == 1 && (lead & 3) == LEAD_DEV) {
        const int d = (lead >> 2) % 9;
        ink = botcorps_pal_fit(BOTCORPS_BOT_COLOR[(d * 5 + 3) % 12]);
        ch = DEV_GLYPHS[d];
    } else if (lv == 1) {
        ink = PAL_WHITE;                    /* bright lead glyph */
        if (gl >= 0x80) ch = (gl & 1) ? '1' : '0';   /* icon codes never print as raw glyphs */
    } else {
        static const int AGE[4] = { 0, 5, 9, 13 };
        if (!botcorps_pal_ramp_for(code_native(), AGE[lv - 2], &ink)) ink = botcorps_pal_fit(code_native());
    }
    const char s[2] = { ch, 0 };
    panel.setFont(&fonts::Font0);
    panel.setTextSize(1);
    botcorps_pal_text(panel, ink);
    panel.setCursor(x + 1, ly);
    panel.print(s);
    panel.clearClipRect();
}

/* One step. Direct mode repaints only the cells whose look changed. */
static void step_columns(lgfx::LovyanGFX *panel)
{
    for (int i = 0; i < s_ncols; ++i) {
        uint8_t *c = col_at(i);
        c[5] = (uint8_t)(c[5] + 1);
        if (c[5] < c[4]) continue;
        c[5] = 0;
        for (int d = 0; d < 2; ++d) {
            int h = drop_head(c, d) + 1;
            const int L = c[d * 2 + 1];
            if (h - L > CELL_ROWS) {
                spawn(c, d, false);
                h = drop_head(c, d);
            }
            c[d * 2] = (uint8_t)(int8_t)h;
            if (h >= 0 && h < CELL_ROWS) *glyph_at(i, h) = new_glyph();
            if (panel) {
                const int ks[6] = { 0, 1, 3, (L * 50 + 99) / 100, (L * 78 + 99) / 100, L };
                for (int q = 0; q < 6; ++q) {
                    const int r = h - ks[q];
                    if (r >= 0 && r < CELL_ROWS) draw_cell(*panel, i, r);
                }
            }
        }
        /* the odd bit flips inside a visible trail */
        if (rnd_n(4) == 0) {
            const int r = rnd_n(CELL_ROWS);
            *glyph_at(i, r) = new_glyph();
            if (panel && cell_level(i, r, nullptr) > 1) draw_cell(*panel, i, r);
        }
    }
}

static void paint_all_cells(lgfx::LovyanGFX &panel)
{
    panel.setTextWrap(false, false);
    const int r0 = s_slice_y0 / CELL, r1 = (s_slice_y1 + CELL - 1) / CELL;
    for (int col = 0; col < s_ncols; ++col)
        for (int row = r0; row < r1 && row < CELL_ROWS; ++row)
            if (cell_level(col, row, nullptr) > 0) draw_cell(panel, col, row);
}

static bool rain_pixel_ok(int x, int y)
{
    if (x < 0 || y < 0 || x >= 320 || y >= 240) return false;
    for (int i = 0; i < s_ncover; ++i) {
        const botcorps_cover_t &b = s_cover[i];
        if (x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h) return false;
    }
    if (s_radar) {
        const int dx = x - 160;
        const int dy = y - 108;
        if (dx * dx + dy * dy < 88 * 88) return false;
    }
    return true;
}

static bool icon_pixel_ok(int x, int y)
{
    const int world_y = y + s_slice_y0;
    if (y < 0) return false;
    return rain_pixel_ok(x, world_y);
}

void botcorps_rain_set_covers(const botcorps_cover_t *boxes, int n, bool radar_disc)
{
    if (n < 0) n = 0;
    if (n > (int)(sizeof(s_cover) / sizeof(s_cover[0]))) {
        n = (int)(sizeof(s_cover) / sizeof(s_cover[0]));
    }
    s_ncover = n;
    if (n > 0 && boxes) memcpy(s_cover, boxes, (size_t)n * sizeof(s_cover[0]));
    s_radar = radar_disc;
}

void botcorps_rain_set_draw(bool enable)
{
    s_draw = enable;
}

bool botcorps_rain_step(void)
{
    if (!s_built) build_columns();
    const uint32_t now = millis();
    if (s_last_ms == 0) s_last_ms = now;
    if ((uint32_t)(now - s_last_ms) < (uint32_t)STEP_MS) return false;
    s_last_ms = now;
    if (s_draw && botcorps_dual_ok() && botcorps_band_direct()) {
        s_slice_y0 = 0;
        s_slice_y1 = 240;
        botcorps_pal_hub_set(s_hub_on);
        botcorps_content().startWrite();
        step_columns(&botcorps_content());
        botcorps_content().endWrite();
        botcorps_content().clearClipRect();
        botcorps_pal_hub_set(false);
    } else {
        step_columns(nullptr);
    }
    return true;
}

void botcorps_rain_set_hub(bool on)
{
    s_hub_on = on;
}

void botcorps_rain_draw(lgfx::LovyanGFX &dst, int y0, int y1)
{
    if (!s_built) build_columns();
    if (y1 <= y0) return;
    s_slice_y0 = y0;
    s_slice_y1 = y1;
    botcorps_pal_hub_set(s_hub_on);
    dst.startWrite();
    paint_all_cells(dst);
    dst.endWrite();
    dst.clearClipRect();
    botcorps_pal_hub_set(false);
    s_slice_y0 = 0;
    s_slice_y1 = 240;
}

int botcorps_rain_cell_bytes(void)
{
    return (int)sizeof(botcorps_cell_t);
}

void botcorps_rain_paint(void)
{
    if (!s_built) build_columns();
    if (!s_draw || !botcorps_dual_ok()) return;
    botcorps_content().startWrite();
    paint_all_cells(botcorps_content());
    botcorps_content().endWrite();
}

void botcorps_rain_set_parked(bool parked)
{
    (void)parked;
}

bool botcorps_rain_sprite_held(void)
{
    return false;
}

void botcorps_rain_release_sprite(void)
{
}

void botcorps_rain_log_heap(const char *tag)
{
    Serial.printf("[botcorps] heap %s free=%u max_alloc=%u sprite=0\n",
                  tag ? tag : "?",
                  (unsigned)ESP.getFreeHeap(),
                  (unsigned)ESP.getMaxAllocHeap());
}

uint8_t botcorps_rain_palette(int index)
{
    if (index < 0 || index >= 14) index = 0;
    return botcorps_pal_fit(PALETTE[index]);
}

void botcorps_rain_set_mode(int mode)
{
    if (mode < 0 || mode >= BOTCORPS_RAIN_COLORS) mode = 0;
    if (mode == s_mode) return;
    s_mode = mode;
}

int botcorps_rain_mode(void)
{
    return s_mode;
}

void botcorps_rain_set_shape(int shape)
{
    if (shape < 0 || shape > 4) shape = 0;
    s_shape = shape;
}

void botcorps_rain_set_mix(int mix)
{
    if (mix < 0 || mix > BOTCORPS_RAIN_MIX_DEV) mix = BOTCORPS_RAIN_MIX_FIELD;
    if (mix == s_mix && s_built) return;
    s_mix = mix;
    s_built = false;
    build_columns();
}

int botcorps_rain_mix(void)
{
    return s_mix;
}

uint8_t botcorps_rain_solid(void)
{
    if (s_mode <= 0 || s_mode >= BOTCORPS_RAIN_COLORS) return PAL_FIELD;
    return botcorps_pal_fit(SOLID_INK[s_mode - 1]);
}
