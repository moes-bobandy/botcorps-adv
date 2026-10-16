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
    0x9F0B, 0xFDC9, 0x36B8, 0xAC7F, 0xFB72, 0x5D3F, 0xF689,
    0xFA69, 0xE2FF, 0xCF89, 0xF79C, 0xFC43, 0x6BBF, 0x5FF8,
};
static const char DEV_GLYPHS[] = "*/#+@%&$X";
static int s_shape = 0;

static const uint16_t SOLID_INK[6] = {
    0x26E8, 0x269C, 0x331F, 0xB21F, 0xF586, 0xFFFF,
};

static const char CODE_GLYPHS[] = "01<>[]{}:/\\|+-=*#";
static const int CODE_N = 18;
#ifdef BOTCORPS_CYD
/* v11 intro mock is lighter than Adv EXT rain: 14 px pitch, every third column off. */
static const int GAP = 14;
#else
static const int GAP = 6;
#endif
static const int STEP_MS = 140;
static const int RAIN_COLS = 56;
static const int CELL = 8;
static const int CELL_COLS = 40;
static const int CELL_ROWS = 30;
static const int CELL_N = CELL_COLS * CELL_ROWS;
/* Mock step() keeps a fading frame. The trail is a ring of stamps in the Tier 0 cell planes. */
static const int TRAIL = 20;
static const int STAMP_N = 56 * TRAIL;
static_assert(STAMP_N + 56 <= 1200, "rain trail fits the tier0 cell plane");

enum { KIND_OFF = 0, KIND_CODE = 1, KIND_BOT = 2, KIND_DEVICE = 3 };

static int s_mode = 0;
static int s_mix = BOTCORPS_RAIN_MIX_FIELD;
static int s_ncols = 0;
static int s_h = 240;
static uint32_t s_rng = 0xA5C3E1u;
static uint32_t s_last_ms = 0;
static bool s_built = false;
static bool s_hub_on = false;
static bool s_draw = false;
static bool s_logged = false;
static bool s_radar = false;
static botcorps_cover_t s_cover[24];
static int s_ncover = 0;
/* Planes 1 and 2, bytes 1120..1179, hold the Tab completion scratch.
 * They sit inside botcorps_t0_rain_cell so the nm Tier 0 total still counts them. */

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

static uint16_t scale_native(uint16_t c, int num, int den)
{
    int r, g, b;
    if (den <= 0) return c;
    unpack565(c, &r, &g, &b);
    return pack565(r * num / den, g * num / den, b * num / den);
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
    /* BOTS label uses the old CREW split: even columns bots, odd columns code. */
    if (s_mix == BOTCORPS_RAIN_MIX_BOTS) return (i % 2 == 0) ? KIND_BOT : KIND_CODE;
    if ((i % 3) == 0) return KIND_BOT;
    if ((i % 2) == 1) return KIND_DEVICE;
    return KIND_CODE;
}

static void build_columns(void)
{
    /* 54 columns, index 0..53. Column 53 starts at x=318 (a 2 px sliver).
     * Column 54 would start at x=324 and is fully off the 320 px panel. */
    s_ncols = (320 + GAP - 1) / GAP;
#ifndef BOTCORPS_CYD
    if (s_ncols > 54) s_ncols = 54;
    static_assert((320 + 6 - 1) / 6 == 54, "drop only the column at x=324");
#else
    if (s_ncols > RAIN_COLS) s_ncols = RAIN_COLS;
#endif
    memset(botcorps_t0_rain_cell, 0, sizeof(botcorps_t0_rain_cell));
    memset(botcorps_t0_rain_col, 0, sizeof(botcorps_t0_rain_col));
    for (int i = 0; i < s_ncols; ++i) {
        uint8_t *c = col_at(i);
        uint8_t kind = kind_for(i);
#ifdef BOTCORPS_CYD
        /* v11 intro: skip every third column so boot rain is not full Adv density. */
        if ((i % 3) == 2) kind = KIND_OFF;
#endif
        col_set_y(i, (int16_t)rnd_n(s_h));
        c[2] = kind;
        c[3] = (uint8_t)((i >> 1) & 3);
        /* Mock trail flag: random < 0.15. About 15% trail. Bots read this. */
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

static int s_slice_y0 = 0;
static int s_slice_y1 = 240;

/* Mock FADE is rgba(3,12,6,.18): each step keeps about 0.82 of the glyph. */
static const uint8_t FADE_KEEP[TRAIL] = {
    255, 210, 172, 141, 115, 94, 77, 63, 51, 41,
    33, 27, 22, 18, 14, 11, 9, 7, 5, 4
};

static uint8_t *stamp_y(void) { return botcorps_t0_rain_cell[0]; }
static uint8_t *stamp_k(void) { return botcorps_t0_rain_cell[1]; }
static uint8_t *stamp_p(void) { return botcorps_t0_rain_cell[2]; }
static uint8_t *ring_head(void) { return botcorps_t0_rain_cell[0] + STAMP_N; }

static bool rain_pixel_ok(int x, int y);
static bool icon_pixel_ok(int x, int y);
static bool code_is_head(int index, int y);

static uint8_t fade_ink(uint16_t native, int age);
/* Bot trails only. Shorter and fainter than the glyph ramp. Age 0 stays full. */
static uint8_t bot_trail_ink(uint16_t native, int age);

static uint8_t trail_ink(uint16_t native, int age)
{
    if (age >= TRAIL) return PAL_FIELD;
    if (age > 0 && FADE_KEEP[age] < 16) return PAL_FIELD;
    uint8_t idx = PAL_FIELD;
    if (botcorps_pal_ramp_for(native, age, &idx)) return idx;
    return fade_ink(native, age);
}

static uint8_t bot_trail_ink(uint16_t native, int age)
{
    if (age <= 0) return botcorps_pal_fit(native);
    if (age > 2) return PAL_FIELD;
    const int keep = (age == 1) ? 36 : 14;
    int fr, fg, fb;
    unpack565(COL_FIELD_N, &fr, &fg, &fb);
    return botcorps_pal_fit(blend565(native, fr, fg, fb, 256 - keep));
}

static uint8_t fade_ink(uint16_t native, int age)
{
    if (age <= 0) return botcorps_pal_fit(native);
    if (age >= TRAIL) return PAL_FIELD;
    const int keep = FADE_KEEP[age];
    if (keep < 16) return PAL_FIELD;
    int fr, fg, fb;
    unpack565(COL_FIELD_N, &fr, &fg, &fb);
    return botcorps_pal_fit(blend565(native, fr, fg, fb, 256 - keep));
}

static bool box_hits_chrome(int x, int y, int w, int h)
{
    for (int dy = 0; dy < h; dy += 2) {
        for (int dx = 0; dx < w; dx += 2) {
            const int px = x + dx;
            const int py = y + dy;
            if (px < 0 || py < 0 || px >= 320 || py >= 240) continue;
            if (!rain_pixel_ok(px, py)) return true;
        }
    }
    return false;
}

static void push_stamp(int col, int y, uint8_t kind, uint8_t payload)
{
    if (col < 0 || col >= 56) return;
    uint8_t *head = ring_head();
    const int slot = head[col] % TRAIL;
    const int idx = col * TRAIL + slot;
    stamp_y()[idx] = (y < 0 || y > 239) ? 255 : (uint8_t)y;
    stamp_k()[idx] = kind;
    stamp_p()[idx] = payload;
    head[col] = (uint8_t)((slot + 1) % TRAIL);
}

static const uint8_t STAMP_REDRAW = 0x40;
static const uint8_t STAMP_DONE = 0x80;

struct rect_t { int x, y, w, h; };

/* DejaVu12 bitmap box relative to the cursor. Ascent is 9. */
struct dbox_t { int8_t x, y, w, h; };
static const dbox_t DEV_BOX[9] = {
    { 1, 1, 5, 6 },   /* * */
    { 0, 1, 4, 10 },  /* / */
    { 1, 2, 8, 8 },   /* # */
    { 1, 3, 7, 7 },   /* + */
    { 1, 1, 11, 11 }, /* @ */
    { 0, 1, 10, 9 },  /* % */
    { 1, 1, 9, 9 },   /* & */
    { 2, 1, 6, 11 },  /* $ */
    { 0, 1, 7, 9 },   /* X */
};

static bool rect_hit(rect_t a, rect_t b)
{
    return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h;
}

/* Screen x is 0..319. Negative x is clipped, never wrapped. */
static bool clamp_local(rect_t r, rect_t *o)
{
    int x0 = r.x;
    int x1 = r.x + r.w;
    if (x0 < 0) x0 = 0;
    if (x1 > 320) x1 = 320;
    int y0 = r.y;
    int y1 = r.y + r.h;
    const int sh = s_slice_y1 - s_slice_y0;
    if (y0 < 0) y0 = 0;
    if (y1 > sh) y1 = sh;
    if (x1 <= x0 || y1 <= y0) return false;
    o->x = x0;
    o->y = y0;
    o->w = x1 - x0;
    o->h = y1 - y0;
    return true;
}

static int stamp_slot(int col, int age)
{
    const int head = ring_head()[col] % TRAIL;
    int slot = head - 1 - age;
    while (slot < 0) slot += TRAIL;
    return col * TRAIL + slot;
}

/* World-space rectangles. Bots are 9 px centred on x+4 (x..x+8). */
static int stamp_foot(int col, int world_y, uint8_t rawk, uint8_t payload, rect_t *o)
{
    const int x = col * GAP;
    const int kind = rawk & 3;
    int n = 0;
    if (kind == KIND_CODE) {
        o[n++] = { x, world_y, 6, 8 };
    } else if (kind == KIND_BOT) {
        /* Sprite only. A 12x28 backing rect reads as a solid block. */
        o[n++] = { x, world_y, 9, 9 };
    } else if (kind == KIND_DEVICE && s_mix != BOTCORPS_RAIN_MIX_DEV) {
        const dbox_t b = DEV_BOX[payload % 9];
        o[n++] = { x + b.x, world_y + b.y, b.w, b.h };
    } else {
        o[n++] = { x, world_y, 12, 12 };
    }
    return n;
}

static void mark_redraw(int self, const rect_t *box, int nbox)
{
    for (int c = 0; c < s_ncols; ++c) {
        for (int age = 0; age < 14; ++age) {
            const int idx = stamp_slot(c, age);
            if (idx == self) continue;
            uint8_t k = stamp_k()[idx];
            if (k & STAMP_DONE) continue;
            if ((k & 3) == KIND_OFF) continue;
            const int wy = stamp_y()[idx];
            if (wy == 255) continue;
            rect_t fp[2];
            const int n = stamp_foot(c, wy, k, stamp_p()[idx], fp);
            bool hit = false;
            for (int i = 0; i < n && !hit; ++i) {
                for (int j = 0; j < nbox; ++j) {
                    if (rect_hit(fp[i], box[j])) { hit = true; break; }
                }
            }
            if (hit) stamp_k()[idx] = (uint8_t)(k | STAMP_REDRAW);
        }
    }
}

static void fill_clamped(lgfx::LovyanGFX &panel, rect_t world, uint8_t ink)
{
    rect_t local = { world.x, world.y - s_slice_y0, world.w, world.h };
    rect_t c;
    if (!clamp_local(local, &c)) return;
    botcorps_pal_rect(panel, c.x, c.y, c.w, c.h, ink);
}

static void clip_clamped(lgfx::LovyanGFX &panel, rect_t world)
{
    rect_t local = { world.x, world.y - s_slice_y0, world.w, world.h };
    rect_t c;
    if (!clamp_local(local, &c)) {
        panel.setClipRect(0, 0, 0, 0);
        return;
    }
    panel.setClipRect(c.x, c.y, c.w, c.h);
}

static bool foot_hits_slice(const rect_t *fp, int n)
{
    const int sh = s_slice_y1 - s_slice_y0;
    for (int i = 0; i < n; ++i) {
        const int y0 = fp[i].y - s_slice_y0;
        if (y0 < sh && y0 + fp[i].h > 0 && fp[i].x < 320 && fp[i].x + fp[i].w > 0) return true;
    }
    return false;
}

static void draw_stamp(lgfx::LovyanGFX &panel, int col, int age)
{
    const int idx = stamp_slot(col, age);
    uint8_t rawk = stamp_k()[idx];
    if (rawk & STAMP_DONE) return;
    if (rawk & STAMP_REDRAW) {
        rawk = (uint8_t)(rawk & ~STAMP_REDRAW);
        stamp_k()[idx] = rawk;
    }
    const uint8_t kind = rawk & 3;
    if (kind == KIND_OFF) return;
    const int world_y = stamp_y()[idx];
    if (world_y == 255) return;
    const int x = col * GAP;
    const int shape = (rawk >> 2) & 3;
    const uint8_t payload = stamp_p()[idx];
    rect_t fp[2];
    const int nfp = stamp_foot(col, world_y, rawk, payload, fp);
    int bottom = fp[0].y + fp[0].h;
    for (int i = 1; i < nfp; ++i) {
        const int b = fp[i].y + fp[i].h;
        if (b > bottom) bottom = b;
    }
    /* Non-trail bots (~85%) erase the small field plate, then stamp full brightness.
     * Trail bots (~15%) keep a two-step faint tail, not the glyph ramp. */
    if (kind == KIND_BOT && col_at(col)[4] == 0) {
        if (age != 0) {
            if (world_y < s_slice_y1 && bottom <= s_slice_y1)
                stamp_k()[idx] = (uint8_t)(rawk | STAMP_DONE);
            return;
        }
        /* 12x28 at (x-1, y-18). Clamped to this slice (the y=120 seam) and to x=0..319.
         * A plate that meets chrome is not wiped, so the clear cannot punch the UI. */
        const rect_t plate = { x - 1, world_y - 18, 12, 28 };
        const bool draw = foot_hits_slice(fp, nfp)
            && !box_hits_chrome(fp[nfp - 1].x, fp[nfp - 1].y, fp[nfp - 1].w, fp[nfp - 1].h);
        const bool wipe = foot_hits_slice(&plate, 1)
            && !box_hits_chrome(plate.x, plate.y, plate.w, plate.h);
        if (!draw && !wipe) return;
        /* One transaction so the field clear is not visible before the sprite. */
        panel.startWrite();
        if (wipe) {
            fill_clamped(panel, plate, PAL_FIELD);
            mark_redraw(idx, &plate, 1);
        }
        if (draw) {
            uint16_t native = PALETTE[0];
            if (payload >= 32 && payload < 44) native = BOTCORPS_BOT_COLOR[payload - 32];
            else if (s_mode != 0) native = SOLID_INK[s_mode - 1];
            else native = PALETTE[payload % 14];
            const int use = (s_shape > 0) ? (s_shape - 1) : shape;
            clip_clamped(panel, fp[nfp - 1]);
            botcorps_sprite_draw(panel, x + 4, world_y - s_slice_y0 + 4, use, 1,
                                 botcorps_pal_fit(native), false);
            panel.clearClipRect();
        }
        panel.endWrite();
        return;
    }
    if (!foot_hits_slice(fp, nfp)) return;
    if (box_hits_chrome(fp[nfp - 1].x, fp[nfp - 1].y, fp[nfp - 1].w, fp[nfp - 1].h)) return;
    const int ly = world_y - s_slice_y0;
    if (kind == KIND_CODE) {
        uint16_t base = code_is_head(col, world_y) ? COL_HEAD_N : COL_CODE_N;
        if (s_mode != 0) base = code_ink(code_is_head(col, world_y));
        const uint8_t ink = trail_ink(base, age);
        if (ink == PAL_FIELD) {
            fill_clamped(panel, fp[0], PAL_FIELD);
            mark_redraw(idx, fp, nfp);
            if (bottom <= s_slice_y1) stamp_k()[idx] = (uint8_t)(rawk | STAMP_DONE);
            return;
        }
        const char ch = CODE_GLYPHS[payload % CODE_N];
        char s[2] = { ch, 0 };
        clip_clamped(panel, fp[0]);
        panel.setFont(&fonts::Font0);
        panel.setTextSize(1);
        botcorps_pal_text(panel, ink);
        panel.setCursor(x, ly);
        panel.print(s);
        panel.clearClipRect();
        return;
    }
    if (kind == KIND_BOT) {
        uint16_t native = PALETTE[0];
        if (payload >= 32 && payload < 44) native = BOTCORPS_BOT_COLOR[payload - 32];
        else if (s_mode != 0) native = SOLID_INK[s_mode - 1];
        else native = PALETTE[payload % 14];
        const uint8_t ink = bot_trail_ink(native, age);
        if (ink == PAL_FIELD) {
            for (int i = 0; i < nfp; ++i) fill_clamped(panel, fp[i], PAL_FIELD);
            mark_redraw(idx, fp, nfp);
            if (bottom <= s_slice_y1) stamp_k()[idx] = (uint8_t)(rawk | STAMP_DONE);
            return;
        }
        const int use = (s_shape > 0) ? (s_shape - 1) : shape;
        clip_clamped(panel, fp[nfp - 1]);
        botcorps_sprite_draw(panel, x + 4, ly + 4, use, 1, ink, false);
        panel.clearClipRect();
        return;
    }
    if (s_mix != BOTCORPS_RAIN_MIX_DEV) {
        const uint8_t ink = trail_ink(0x9E6E, age);
        if (ink == PAL_FIELD) {
            fill_clamped(panel, fp[0], PAL_FIELD);
            mark_redraw(idx, fp, nfp);
            if (bottom <= s_slice_y1) stamp_k()[idx] = (uint8_t)(rawk | STAMP_DONE);
            return;
        }
        const char ch = DEV_GLYPHS[payload % 9];
        char s[2] = { ch, 0 };
        clip_clamped(panel, fp[0]);
        panel.setFont(&fonts::DejaVu12);
        panel.setTextSize(1);
        botcorps_pal_text(panel, ink);
        panel.setCursor(x, ly);
        panel.print(s);
        panel.clearClipRect();
        return;
    }
    const uint16_t body = (s_mode == 0) ? (uint16_t)0x9E6E : SOLID_INK[s_mode - 1];
    const uint8_t c1 = trail_ink(body, age);
    const uint8_t c2 = trail_ink(scale_native(body, 128, 256), age);
    const uint8_t c3 = trail_ink(scale_native(0xFFFF, 200, 256), age);
    if (c1 == PAL_FIELD) {
        fill_clamped(panel, fp[0], PAL_FIELD);
        mark_redraw(idx, fp, nfp);
        if (bottom <= s_slice_y1) stamp_k()[idx] = (uint8_t)(rawk | STAMP_DONE);
        return;
    }
    clip_clamped(panel, fp[0]);
    botcorps_glyph_blit(panel, x, ly, payload % 10, c1, c2, c3, icon_pixel_ok);
    panel.clearClipRect();
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
    if (!rain_pixel_ok(x, world_y)) return false;
    if (world_y < s_slice_y0 || world_y >= s_slice_y1) return false;
    return true;
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

static void step_columns(void)
{
    for (int i = 0; i < s_ncols; ++i) {
        uint8_t *c = col_at(i);
        if (c[2] == KIND_OFF) continue;
        if (rnd_n(100) < c[6]) continue;
        const int y = col_y(i);
        int step = GAP;
        if (c[2] == KIND_BOT) {
            uint8_t payload = c[5];
            if (c[7] > 0 && c[7] <= 12) payload = (uint8_t)(32 + (c[7] - 1));
            const uint8_t shape = (uint8_t)(c[3] & 3);
            push_stamp(i, y, (uint8_t)(KIND_BOT | (shape << 2)), payload);
        } else if (c[2] == KIND_DEVICE) {
            push_stamp(i, y, KIND_DEVICE, c[5]);
            if (s_mix == BOTCORPS_RAIN_MIX_DEV) {
                c[5] = (uint8_t)((c[5] + 1 + rnd_n(4)) % 10);
                step = 12;
            }
        } else {
            push_stamp(i, y, KIND_CODE, (uint8_t)rnd_n(CODE_N));
        }
        int ny = y + step;
        if (ny > s_h + step) {
            ny = -step - rnd_n(4) * GAP;
            if (c[2] == KIND_BOT) {
                c[5] = (uint8_t)pick_bot_color(i);
                c[3] = (uint8_t)((c[3] + 1) & 3);
            }
            if (c[2] == KIND_DEVICE) c[5] = (uint8_t)((c[5] + 1 + rnd_n(3)) % 10);
        }
        col_set_y(i, (int16_t)ny);
    }
}

static void paint_stamps(lgfx::LovyanGFX &panel)
{
    panel.setTextWrap(false, false);
    for (int col = 0; col < s_ncols; ++col) {
        for (int age = TRAIL - 1; age >= 0; --age) draw_stamp(panel, col, age);
    }
    for (int col = 0; col < s_ncols; ++col) {
        for (int age = TRAIL - 1; age >= 0; --age) {
            if (stamp_k()[stamp_slot(col, age)] & STAMP_REDRAW) draw_stamp(panel, col, age);
        }
    }
}

static void paint_dirty(lgfx::LovyanGFX &panel)
{
    panel.startWrite();
    paint_stamps(panel);
    panel.endWrite();
    panel.clearClipRect();
}

static void paint_all(lgfx::LovyanGFX &panel)
{
    paint_dirty(panel);
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
    step_columns();
    if (s_draw && botcorps_dual_ok() && botcorps_band_direct()) {
        s_slice_y0 = 0;
        s_slice_y1 = 240;
        botcorps_pal_hub_set(s_hub_on);
        paint_dirty(botcorps_content());
        botcorps_pal_hub_set(false);
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
    paint_stamps(dst);
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
