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
    0x9F0B, 0xFDC9, 0x36B8, 0xAC7F, 0xFB72, 0x5D3F, 0xF689,
    0xFA69, 0xE2FF, 0xCF89, 0xF79C, 0xFC43, 0x6BBF, 0x5FF8,
};

static const uint16_t SOLID_INK[6] = {
    0x26E8, 0x269C, 0x331F, 0xB21F, 0xF586, 0xFFFF,
};

static const char CODE_GLYPHS[] = "01<>[]{}:/\\|+-=*#";
static const int CODE_N = 18;
static const int GAP = 6;
static const int STEP_MS = 140;
static const int RAIN_COLS = 56;
static const int CELL = 8;
static const int CELL_COLS = 40;
static const int CELL_ROWS = 30;
static const int CELL_N = CELL_COLS * CELL_ROWS;

enum { KIND_OFF = 0, KIND_CODE = 1, KIND_BOT = 2, KIND_DEVICE = 3 };

static int s_mode = 0;
static int s_mix = BOTCORPS_RAIN_MIX_FIELD;
static int s_ncols = 0;
static int s_h = 240;
static uint32_t s_rng = 0xA5C3E1u;
static uint32_t s_last_ms = 0;
static bool s_built = false;
static bool s_draw = false;
static bool s_logged = false;
static bool s_radar = false;
static botcorps_cover_t s_cover[24];
static int s_ncover = 0;
static uint8_t s_dirty[(CELL_N + 7) / 8];

static uint32_t rnd_u(void)
{
    s_rng = s_rng * 1664525u + 1013904223u;
    return s_rng;
}

static int rnd_n(int n)
{
    if (n <= 1) return 0;
    return (int)(rnd_u() % (uint32_t)n);
}

static int js_round_pos(float x)
{
    return (int)(x + 0.5f);
}

static void unpack565(uint16_t p, int *r, int *g, int *b)
{
    const int r5 = (p >> 11) & 31;
    const int g6 = (p >> 5) & 63;
    const int b5 = p & 31;
    *r = (r5 << 3) | (r5 >> 2);
    *g = (g6 << 2) | (g6 >> 4);
    *b = (b5 << 3) | (b5 >> 2);
}

static uint16_t pack565(int r, int g, int b)
{
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static uint16_t blend565(uint16_t dst, int sr, int sg, int sb, int a)
{
    int r, g, b;
    const int inv = 256 - a;
    unpack565(dst, &r, &g, &b);
    r = (sr * a + r * inv) >> 8;
    g = (sg * a + g * inv) >> 8;
    b = (sb * a + b * inv) >> 8;
    return pack565(r, g, b);
}

static void trail_rgb(int *r, int *g, int *b)
{
    if (s_mode == 0) {
        *r = 3;
        *g = 12;
        *b = 6;
        return;
    }
    *r = 0;
    *g = 8;
    *b = 8;
}

static uint16_t scale_native(uint16_t c, int num, int den)
{
    int r, g, b;
    if (den <= 0) return c;
    unpack565(c, &r, &g, &b);
    return pack565(r * num / den, g * num / den, b * num / den);
}

static uint16_t bot_ink(uint8_t idx)
{
    if (s_mode == 0) {
        if (idx > 13) idx = 0;
        return PALETTE[idx];
    }
    return SOLID_INK[s_mode - 1];
}

static uint16_t code_ink(bool head)
{
    if (s_mode == 0) return head ? COL_HEAD_N : COL_CODE_N;
    const uint16_t c = SOLID_INK[s_mode - 1];
    if (head) return c;
    return scale_native(c, 168, 256);
}

static uint8_t *col_at(int i)
{
    return botcorps_t0_rain_col[i];
}

static int16_t col_y(int i)
{
    const uint8_t *p = col_at(i);
    return (int16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static void col_set_y(int i, int16_t y)
{
    uint8_t *p = col_at(i);
    p[0] = (uint8_t)(uint16_t)y;
    p[1] = (uint8_t)((uint16_t)y >> 8);
}

static int pick_bot_color(int self)
{
    bool used[14] = {};
    int free_n = 0;
    for (int i = 0; i < s_ncols; ++i) {
        if (i == self || col_at(i)[2] != KIND_BOT) continue;
        if (col_at(i)[5] < 14) used[col_at(i)[5]] = true;
    }
    for (int i = 0; i < 14; ++i) {
        if (!used[i]) ++free_n;
    }
    if (free_n == 0) return rnd_n(14);
    int pick = rnd_n(free_n);
    for (int i = 0; i < 14; ++i) {
        if (used[i]) continue;
        if (pick == 0) return i;
        --pick;
    }
    return 0;
}

static uint8_t kind_for(int i)
{
    /* Mock column split. FIELD: i%3==0 bot, odd columns devices, the rest code. */
    if (s_mix == BOTCORPS_RAIN_MIX_DEV) {
        return (i % 3 == 0) ? KIND_DEVICE : ((i % 2 == 1) ? KIND_CODE : KIND_OFF);
    }
    if (s_mix == BOTCORPS_RAIN_MIX_BOTS) return (i % 2 == 0) ? KIND_BOT : KIND_OFF;
    if ((i % 3) == 0) return KIND_BOT;
    if ((i % 2) == 1) return KIND_DEVICE;
    return KIND_CODE;
}

static void build_columns(void)
{
    s_ncols = (320 + GAP - 1) / GAP + 1;
    if (s_ncols > RAIN_COLS) s_ncols = RAIN_COLS;
    memset(botcorps_t0_rain_cell, 0, sizeof(botcorps_t0_rain_cell));
    memset(botcorps_t0_rain_col, 0, sizeof(botcorps_t0_rain_col));
    memset(s_dirty, 0, sizeof(s_dirty));
    for (int i = 0; i < s_ncols; ++i) {
        uint8_t *c = col_at(i);
        const uint8_t kind = kind_for(i);
        col_set_y(i, (int16_t)rnd_n(s_h));
        c[2] = kind;
        c[3] = (uint8_t)(i % 2 == 0);
        c[4] = (uint8_t)(rnd_n(100) < 15);
        c[5] = (kind == KIND_DEVICE) ? (uint8_t)((i * 5) % 10) : (uint8_t)rnd_n(14);
        c[6] = (uint8_t)rnd_n(36);
    }
    if (s_mix != BOTCORPS_RAIN_MIX_DEV) {
        uint8_t deal[14];
        bool taken[RAIN_COLS] = {};
        for (int i = 0; i < 14; ++i) deal[i] = (uint8_t)i;
        for (int i = 13; i > 0; --i) {
            const int j = rnd_n(i + 1);
            const uint8_t tmp = deal[i];
            deal[i] = deal[j];
            deal[j] = tmp;
        }
        const int anchors = 7;
        for (int k = 0; k < anchors; ++k) {
            int idx = js_round_pos((k + 0.5f) * (float)(s_ncols - 1) / (float)anchors);
            if (idx < 0) idx = 0;
            if (idx >= s_ncols) idx = s_ncols - 1;
            int guard = 0;
            while (taken[idx] && guard < s_ncols) {
                idx = (idx + 1) % s_ncols;
                ++guard;
            }
            taken[idx] = true;
            uint8_t *c = col_at(idx);
            c[2] = KIND_BOT;
            col_set_y(idx, (int16_t)(((k + 0.2f) / (float)anchors) * (float)s_h));
            c[3] = (uint8_t)(k % 2 == 0);
            c[5] = deal[k];
            c[7] = (uint8_t)(k + 1);
        }
    }
    s_built = true;
    if (!s_logged) {
        s_logged = true;
        Serial.printf("[botcorps] rain path=cells %dx%d cell=%u pitch=%d step=%d\n",
                      CELL_COLS, CELL_ROWS, (unsigned)sizeof(botcorps_cell_t), GAP, STEP_MS);
    }
}

static int cell_index(int cx, int cy)
{
    if (cx < 0 || cy < 0 || cx >= CELL_COLS || cy >= CELL_ROWS) return -1;
    return cy * CELL_COLS + cx;
}

static void mark_dirty(int i)
{
    if (i < 0 || i >= CELL_N) return;
    s_dirty[i >> 3] = (uint8_t)(s_dirty[i >> 3] | (uint8_t)(1u << (i & 7)));
}

static bool take_dirty(int i)
{
    const uint8_t bit = (uint8_t)(1u << (i & 7));
    if ((s_dirty[i >> 3] & bit) == 0) return false;
    s_dirty[i >> 3] = (uint8_t)(s_dirty[i >> 3] & (uint8_t)~bit);
    return true;
}

static bool cell_hidden(int cx, int cy)
{
    const int x = cx * CELL;
    const int y = cy * CELL;
    for (int i = 0; i < s_ncover; ++i) {
        const botcorps_cover_t &b = s_cover[i];
        if (x < b.x + b.w && x + CELL > b.x && y < b.y + b.h && y + CELL > b.y) {
            return true;
        }
    }
    if (s_radar) {
        const int dx = x + CELL / 2 - 160;
        const int dy = y + CELL / 2 - 108;
        if (dx * dx + dy * dy < 88 * 88) return true;
    }
    return false;
}

static int s_slice_y0 = 0;
static int s_slice_y1 = 240;

static void cell_paint(lgfx::LovyanGFX &panel, int cx, int cy)
{
    const int i = cell_index(cx, cy);
    if (i < 0 || cell_hidden(cx, cy)) return;
    const int x = cx * CELL;
    const int world_y = cy * CELL;
    if (world_y + CELL <= s_slice_y0 || world_y >= s_slice_y1) return;
    const int y = world_y - s_slice_y0;
    const uint8_t fade = botcorps_t0_rain_cell[0][i];
    const uint8_t id = botcorps_t0_rain_cell[1][i];
    uint8_t color = PAL_FIELD;
    if (fade > 0 && id != 0) {
        int fr, fg, fb;
        trail_rgb(&fr, &fg, &fb);
        uint16_t base = code_ink(false);
        if (id >= 32 && id < 44) base = BOTCORPS_BOT_COLOR[id - 32];
        else if (id == 16) base = code_ink(true);
        else if (id == 17) base = blend565(COL_FIELD_N, fr, fg, fb, BACK_A);
        else if (id <= 14) base = bot_ink((uint8_t)(id - 1));
        int a = 256 - (fade * 16);
        if (a > 255) a = 255;
        color = botcorps_pal_fit(blend565(base, fr, fg, fb, a));
    }
    botcorps_pal_rect(panel, x, y, CELL, CELL, color);
    if (fade > 10 && id >= 1 && id <= 14) {
        botcorps_pal_rect(panel, x + 1, y + 2, 2, 2, PAL_EYE);
        botcorps_pal_rect(panel, x + 4, y + 2, 2, 2, PAL_EYE);
    } else if (fade > 8 && (id == 15 || id == 16)) {
        const char ch = CODE_GLYPHS[botcorps_t0_rain_cell[2][i] % CODE_N];
        panel.setTextSize(1);
        botcorps_pal_text(panel, color);
        panel.setCursor(x, y);
        panel.print(ch);
    }
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

static void mark_icon_cells(int x, int y)
{
    const int x0 = x < 0 ? 0 : x;
    const int y0 = y < 0 ? 0 : y;
    const int x1 = x + 12;
    const int y1 = y + 12;
    if (x1 <= 0 || y1 <= 0) return;
    for (int cy = y0 / CELL; cy <= (y1 - 1) / CELL; ++cy) {
        for (int cx = x0 / CELL; cx <= (x1 - 1) / CELL; ++cx) {
            const int i = cell_index(cx, cy);
            if (i >= 0) mark_dirty(i);
        }
    }
}

static bool icon_hits_chrome(int x, int y)
{
    for (int dy = 0; dy < 12; ++dy) {
        for (int dx = 0; dx < 12; ++dx) {
            const int px = x + dx;
            const int py = y + dy;
            if (px < 0 || py < 0 || px >= 320 || py >= 240) continue;
            if (!rain_pixel_ok(px, py)) return true;
        }
    }
    return false;
}

static bool icon_pixel_ok(int x, int y)
{
    const int world_y = y + s_slice_y0;
    if (y < 0) return false;
    if (!rain_pixel_ok(x, world_y)) return false;
    if (world_y < s_slice_y0 || world_y >= s_slice_y1) return false;
    return true;
}

static void blit_devices(lgfx::LovyanGFX &panel)
{
    const uint16_t body = (s_mode == 0) ? (uint16_t)0x5D3F : SOLID_INK[s_mode - 1];
    const uint8_t c1 = botcorps_pal_fit(body);
    const uint8_t c2 = botcorps_pal_fit(scale_native(body, 128, 256));
    const uint8_t c3 = botcorps_pal_fit(scale_native(0xFFFF, 200, 256));
    for (int i = 0; i < s_ncols; ++i) {
        uint8_t *c = col_at(i);
        if (c[2] != KIND_DEVICE) continue;
        const int x = i * GAP;
        const int y = col_y(i);
        if (y + 12 <= s_slice_y0 || y >= s_slice_y1) continue;
        if (icon_hits_chrome(x, y)) continue;
        botcorps_glyph_blit(panel, x, y - s_slice_y0, c[5] % 10,
                            c1, c2, c3, icon_pixel_ok);
    }
}

static void cell_stamp(int x, int y, int w, int h, uint8_t id, uint8_t glyph)
{
    if (w <= 0 || h <= 0) return;
    const int x0 = x < 0 ? 0 : x;
    const int y0 = y < 0 ? 0 : y;
    const int x1 = x + w;
    const int y1 = y + h;
    for (int cy = y0 / CELL; cy <= (y1 - 1) / CELL; ++cy) {
        for (int cx = x0 / CELL; cx <= (x1 - 1) / CELL; ++cx) {
            const int i = cell_index(cx, cy);
            if (i < 0) continue;
            botcorps_t0_rain_cell[0][i] = 15;
            botcorps_t0_rain_cell[1][i] = id;
            botcorps_t0_rain_cell[2][i] = glyph;
            mark_dirty(i);
        }
    }
}

static bool code_is_head(int index, int y)
{
    int q;
    if (y >= 0) q = y / GAP;
    else q = -(((-y) + GAP - 1) / GAP);
    int slot = index + q;
    int m = slot % 6;
    if (m < 0) m += 6;
    return m == 0;
}

static void fade_cells(void)
{
    for (int i = 0; i < CELL_N; ++i) {
        uint8_t fade = botcorps_t0_rain_cell[0][i];
        if (fade == 0) continue;
        const uint8_t next = (uint8_t)((fade * 210) >> 8);
        if (next == fade) continue;
        fade = next;
        if (fade < 2) {
            fade = 0;
            botcorps_t0_rain_cell[1][i] = 0;
        }
        botcorps_t0_rain_cell[0][i] = fade;
        mark_dirty(i);
    }
}

static void step_columns(void)
{
    for (int i = 0; i < s_ncols; ++i) {
        uint8_t *c = col_at(i);
        if (c[2] == KIND_OFF) continue;
        if (rnd_n(100) < c[6]) continue;
        const int x = i * GAP;
        const int y = col_y(i);
        int step = GAP;
        if (c[2] == KIND_BOT) {
            if (!c[4]) cell_stamp(x - 1, y - 18, 12, 28, 17, 0);
            uint8_t id = (uint8_t)(c[5] + 1);
            if (c[7] > 0 && c[7] <= 12) id = (uint8_t)(32 + (c[7] - 1));
            cell_stamp(x, y, 7, c[3] ? 7 : 6, id, 0);
        } else if (c[2] == KIND_DEVICE) {
            mark_icon_cells(x, y);
            c[5] = (uint8_t)((c[5] + 1 + rnd_n(4)) % 10);
            step = 12;
        } else {
            const bool head = code_is_head(i, y);
            const uint8_t glyph = (uint8_t)rnd_n(CODE_N);
            cell_stamp(x, y, 6, 8, head ? 16 : 15, glyph);
        }
        int ny = y + step;
        if (ny > s_h + step) {
            ny = -step - rnd_n(4) * GAP;
            if (c[2] == KIND_BOT) c[5] = (uint8_t)pick_bot_color(i);
            if (c[2] == KIND_DEVICE) c[5] = (uint8_t)((c[5] + 1 + rnd_n(3)) % 10);
        }
        col_set_y(i, (int16_t)ny);
    }
}

static void paint_dirty(lgfx::LovyanGFX &panel)
{
    panel.startWrite();
    panel.setTextWrap(false, false);
    for (int i = 0; i < CELL_N; ++i) {
        if (!take_dirty(i)) continue;
        cell_paint(panel, i % CELL_COLS, i / CELL_COLS);
    }
    blit_devices(panel);
    panel.endWrite();
    panel.clearClipRect();
}

static void paint_all(lgfx::LovyanGFX &panel)
{
    panel.startWrite();
    panel.setTextWrap(false, false);
    for (int i = 0; i < CELL_N; ++i) {
        s_dirty[i >> 3] = 0;
        cell_paint(panel, i % CELL_COLS, i / CELL_COLS);
    }
    blit_devices(panel);
    panel.endWrite();
    panel.clearClipRect();
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
    fade_cells();
    step_columns();
    if (s_draw && botcorps_dual_ok() && botcorps_band_direct()) {
        s_slice_y0 = 0;
        s_slice_y1 = 240;
        paint_dirty(botcorps_content());
    }
    return true;
}

void botcorps_rain_draw(lgfx::LovyanGFX &dst, int y0, int y1)
{
    if (!s_built) build_columns();
    if (y1 <= y0) return;
    s_slice_y0 = y0;
    s_slice_y1 = y1;
    dst.startWrite();
    dst.setTextWrap(false, false);
    for (int i = 0; i < CELL_N; ++i) cell_paint(dst, i % CELL_COLS, i / CELL_COLS);
    blit_devices(dst);
    dst.endWrite();
    dst.clearClipRect();
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
    paint_all(botcorps_content());
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
