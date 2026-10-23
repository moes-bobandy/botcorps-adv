/*
 * v1019n (#6 / #7): BACKGROUNDS = the four former SCREEN-SAVER animations (BOTS / GIBSON / FLYBY /
 * BOUNCE), ported from the approved saver mock (BOTcorps-Adv-FINAL-v1 SCREENSAVERS) with their saver
 * look kept. When one is selected it RUNS AS THE LIVE BACKGROUND and REPLACES the rain on that screen
 * (the rain is not drawn at all). BG_RAIN (default) = our one BOTcorps rain. There is no idle timer:
 * nothing here ever takes over the screen on its own.
 *   BOTS   : the bot crew falls in strands (own colours, fading bot trail) + the ivory Gibson chip
 *            dropping through them (saver "crew" look)
 *   GIBSON : dark teal board grid, six device chips, every bot (own colour + name) walks the grid to a
 *            device along an L circuit trace; the Gibson chip drifts and bounces
 *   FLYBY  : black space, star streams from the centre, bots fly at the viewer growing in whole pixels
 *   BOUNCE : one bot DVD-style, new colour (and shape in MIXED) on every wall hit, corner hit flashes
 *            the frame; the Gibson chip bounces too
 * Drawn through the same 8x8 cell grid as the rain: covers, the radar disc and the band slices work
 * unchanged, a step repaints only the cells an object left or entered. Bots follow the BOT-RAIN colour
 * toggle (MULTI = own colour, SOLID = that ink); the Gibson chip is always ivory + gold pins.
 * Included by botcorps_rain.cpp only (uses its statics).
 */
#include <math.h>

enum { BG_RAIN = 0, BG_BOTS = 1, BG_GIBSON = 2, BG_FLYBY = 3, BG_BOUNCE = 4, BG_STYLES = 5 };
static int s_bg = BG_RAIN;
/* the Terminal / prompt keeps its fixed device-icon rain (item 15): never a background */
static bool bg_on(void) { return s_bg != BG_RAIN && !s_no_bots; }

static const int BG_W = 320, BG_H = 240;
static const int FP = 8;                            /* positions in 1/8 px */
static uint16_t bg565(int r, int g, int b) { return (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)); }
/* a -> b by num/den, RGB565 */
static uint16_t bg_mix(uint16_t a, uint16_t b, int num, int den)
{
    const int ar = (a >> 11) & 31, ag = (a >> 5) & 63, ab = a & 31;
    const int br = (b >> 11) & 31, bgg = (b >> 5) & 63, bb = b & 31;
    return (uint16_t)(((ar + (br - ar) * num / den) << 11) | ((ag + (bgg - ag) * num / den) << 5) | (ab + (bb - ab) * num / den));
}
static int bg_shape(int i) { return s_shape > 0 ? (s_shape - 1) % 4 : ((i % 4) + 4) % 4; }
static const uint8_t BG_SW[4] = { 7, 9, 7, 5 }, BG_SH[4] = { 7, 6, 9, 5 };
/* bot ink: own colour (MULTI) or the BOT-RAIN solid ink */
static uint16_t bg_bot565(int b) { return s_mode > 0 ? SOLID_INK[s_mode - 1] : BOTCORPS_BOT_COLOR[b % 12]; }
/* saver pool (mock SV_PAL: the rain palette without GIBSONbot's ivory) */
static const uint8_t BG_POOL[13] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13 };
static uint16_t bg_pool565(int k) { return s_mode > 0 ? SOLID_INK[s_mode - 1] : PALETTE[BG_POOL[((k % 13) + 13) % 13]]; }
static const char *const BG_NAME[12] = { "WIFI", "BLUETOOTH", "DEAUTH", "HACK", "FLOCK", "DRONE",
                                         "AIRTAG", "KEYFOB", "META", "SKIMMER", "FLIPPER", "SAMSUNGTAG" };

/* ---- scene state (one style live at a time) ---- */
struct bg_chip_t { int16_t x, y; int16_t vx, vy; uint8_t T; };
struct bg_crew_t { int16_t x, y; uint8_t v, b; };
static const int BG_CREW = 22;                      /* BOTS: falling bot strands */
struct bg_gbot_t { int16_t x, y; uint8_t target, speed; };
struct bg_dev_t { int16_t x, y; uint8_t hop, pat; };
struct bg_fly_t { int16_t p; int8_t ox, oy; uint8_t col, k; };
struct bg_star_t { uint8_t a; uint16_t r; };
static bg_chip_t s_gm;                              /* the ivory Gibson chip (BOTS / GIBSON / BOUNCE) */
static bool s_gm_on = false;
static union {
    struct { bg_crew_t c[BG_CREW]; } crew;
    struct { bg_gbot_t b[12]; bg_dev_t d[6]; } gib;
    struct { bg_fly_t f[6]; bg_star_t st[56]; uint8_t kc; } fly;
    struct { int16_t x, y, vx, vy; uint8_t k, col, flash; } bn;
} s_bs;
static bool s_bg_built = false;
/* dirty cell marks, one bit per 8x8 cell */
static uint8_t s_bg_dirty[(40 * 30 + 7) / 8];

static void bg_mark_cell(int c, int r)
{
    if (c < 0 || r < 0 || c >= CELL_COLS || r >= CELL_ROWS) return;
    const int i = r * CELL_COLS + c;
    s_bg_dirty[i >> 3] |= (uint8_t)(1u << (i & 7));
}
static void bg_mark(int x, int y, int w, int h)      /* world px rect */
{
    if (w <= 0 || h <= 0) return;
    const int c0 = x < 0 ? 0 : x / CELL, r0 = y < 0 ? 0 : y / CELL;
    const int c1 = (x + w - 1) / CELL, r1 = (y + h - 1) / CELL;
    for (int r = r0; r <= r1 && r < CELL_ROWS; ++r)
        for (int c = c0; c <= c1 && c < CELL_COLS; ++c) bg_mark_cell(c, r);
}

/* ---- bounding boxes (world px) ---- */
static void chip_box(const bg_chip_t &m, int *x, int *y, int *w, int *h)
{ *x = m.x / FP - m.T / 2; *y = m.y / FP - m.T / 2; *w = m.T; *h = m.T; }
static void spr_box(int cx, int cy, int shape, int z, int *x, int *y, int *w, int *h)
{ *w = BG_SW[shape] * z; *h = BG_SH[shape] * z; *x = cx - *w / 2; *y = cy - *h / 2; }
static void crew_box(const bg_crew_t &c, int *x, int *y, int *w, int *h)
{ *x = c.x - 5; *y = c.y / FP - 5 - 4 * 9; *w = 10; *h = 10 + 4 * 9; }
static void gbot_box(int i, int *x, int *y, int *w, int *h)
{
    const bg_gbot_t &b = s_bs.gib.b[i];
    int sx, sy, sw, sh;
    spr_box(b.x / FP, b.y / FP, bg_shape(i), 3, &sx, &sy, &sw, &sh);
    const int lw = (int)strlen(BG_NAME[i]) * 6;
    const int lx = b.x / FP - lw / 2;
    *x = sx < lx ? sx : lx; *w = (sx + sw > lx + lw ? sx + sw : lx + lw) - *x;
    *y = sy; *h = sh + 3 + 8;
}
static void fly_geom(const bg_fly_t &f, int *cx, int *cy, int *z, float *sc)
{
    const float p = f.p / 1000.0f;
    *sc = expf(p * logf(11.0f));
    *z = (int)floorf(*sc); if (*z < 1) *z = 1;
    *cx = (int)lroundf(BG_W / 2 + f.ox * (0.5f + *sc * 0.3f));
    *cy = (int)lroundf(BG_H / 2 + f.oy * (0.5f + *sc * 0.3f));
}
static void star_xy(const bg_star_t &s, int *x, int *y)
{
    const float a = s.a * (6.2831853f / 256.0f), r = s.r / (float)FP;
    *x = (int)lroundf(BG_W / 2 + cosf(a) * r * 1.25f);
    *y = (int)lroundf(BG_H / 2 + sinf(a) * r);
}
static void bn_box(int *x, int *y, int *w, int *h)
{ spr_box(s_bs.bn.x / FP, s_bs.bn.y / FP, bg_shape(s_bs.bn.k), 4, x, y, w, h); }

/* mark everything an object covers now (called before AND after a step) */
static void bg_mark_scene(void)
{
    int x, y, w, h;
    if (s_gm_on) { chip_box(s_gm, &x, &y, &w, &h); bg_mark(x - 1, y - 1, w + 2, h + 2); }
    switch (s_bg) {
    case BG_BOTS:
        for (int i = 0; i < BG_CREW; ++i) { crew_box(s_bs.crew.c[i], &x, &y, &w, &h); bg_mark(x, y, w, h); }
        break;
    case BG_GIBSON:
        for (int i = 0; i < 12; ++i) {
            gbot_box(i, &x, &y, &w, &h); bg_mark(x, y, w, h);
            const bg_gbot_t &b = s_bs.gib.b[i]; const bg_dev_t &d = s_bs.gib.d[b.target];
            const int bx = b.x / FP, by = b.y / FP;
            bg_mark(bx < d.x ? bx : d.x, by, abs(d.x - bx) + 1, 1);
            bg_mark(d.x, by < d.y ? by : d.y, 1, abs(d.y - by) + 1);
        }
        for (int i = 0; i < 6; ++i) bg_mark(s_bs.gib.d[i].x - 10, s_bs.gib.d[i].y - 7, 21, 15);
        break;
    case BG_FLYBY:
        for (int i = 0; i < 6; ++i) {
            int cx, cy, z; float sc; fly_geom(s_bs.fly.f[i], &cx, &cy, &z, &sc);
            spr_box(cx, cy, bg_shape(s_bs.fly.f[i].k), z, &x, &y, &w, &h); bg_mark(x, y, w, h);
        }
        for (int i = 0; i < 56; ++i) { star_xy(s_bs.fly.st[i], &x, &y); bg_mark(x, y, 2, 2); }
        break;
    case BG_BOUNCE:
        bn_box(&x, &y, &w, &h); bg_mark(x, y, w, h);
        if (s_bs.bn.flash) { bg_mark(0, 0, BG_W, 1); bg_mark(0, BG_H - 1, BG_W, 1); bg_mark(0, 0, 1, BG_H); bg_mark(BG_W - 1, 0, 1, BG_H); }
        break;
    default: break;
    }
}

static int bg_sg(void) { return rnd_n(2) ? 1 : -1; }
static void gm_make(int x, int y, int vx, int vy, int T)
{ s_gm.x = (int16_t)(x * FP); s_gm.y = (int16_t)(y * FP); s_gm.vx = (int16_t)vx; s_gm.vy = (int16_t)vy; s_gm.T = (uint8_t)T; s_gm_on = true; }
static void crew_spawn(bg_crew_t &c, bool scatter)
{
    c.x = (int16_t)(rnd_n(CELL_COLS) * CELL + 4);
    c.y = (int16_t)((scatter ? rnd_n(BG_H + 40) : -rnd_n(60)) * FP);
    c.v = (uint8_t)(16 + rnd_n(24));             /* 2..5 px per step */
    c.b = (uint8_t)rnd_n(12);
}
static void dev_hop(bg_dev_t &d)
{
    d.hop++;
    const int fx = (d.hop * 37 + d.pat * 21) % 100, fy = (d.hop * 53 + d.pat * 17) % 100;
    d.x = (int16_t)(BG_W * (12 + fx * 76 / 100) / 100);
    d.y = (int16_t)(BG_H * (14 + fy * 62 / 100) / 100);
}
static void fly_spawn(bg_fly_t &f, int p, int k)
{ f.p = (int16_t)p; f.ox = (int8_t)(rnd_n(93) - 46); f.oy = (int8_t)(rnd_n(73) - 36); f.col = (uint8_t)rnd_n(13); f.k = (uint8_t)k; }

static void bg_build(void)
{
    memset(&s_bs, 0, sizeof(s_bs));
    memset(s_bg_dirty, 0, sizeof(s_bg_dirty));
    s_gm_on = false;
    switch (s_bg) {
    case BG_BOTS:                                     /* mock makeMascot('crew'): falls at 62 px/s */
        for (int i = 0; i < BG_CREW; ++i) crew_spawn(s_bs.crew.c[i], true);
        gm_make(BG_W * 62 / 100, BG_H * 20 / 100, 0, 45, 22);
        break;
    case BG_GIBSON:
        for (int i = 0; i < 6; ++i) {
            bg_dev_t &d = s_bs.gib.d[i];
            d.pat = (uint8_t)i; d.hop = 0;
            d.x = (int16_t)(BG_W * (18 + ((i * 37) % 100) * 64 / 100) / 100);
            d.y = (int16_t)(BG_H * (22 + ((i * 53) % 100) * 50 / 100) / 100);
        }
        for (int i = 0; i < 12; ++i) {
            bg_gbot_t &b = s_bs.gib.b[i];
            b.x = (int16_t)((40 + (i % 4) * 80 + (i % 3) * 6) * FP);
            b.y = (int16_t)((40 + (i / 4) * 62 + (i % 2) * 8) * FP);
            b.speed = (uint8_t)(6 + (i % 4) * 2);     /* 0.75..1.5 px per step (mock 9..17 px/s) */
            b.target = (uint8_t)(i % 6);
        }
        gm_make(BG_W * 62 / 100, BG_H * 38 / 100, bg_sg() * 24, bg_sg() * 19, 30);
        break;
    case BG_FLYBY:
        for (int i = 0; i < 6; ++i) fly_spawn(s_bs.fly.f[i], i * 1000 / 6, rnd_n(4));
        for (int i = 0; i < 56; ++i) { s_bs.fly.st[i].a = (uint8_t)rnd_n(256); s_bs.fly.st[i].r = (uint16_t)((4 + rnd_n(150)) * FP); }
        break;
    case BG_BOUNCE:
        s_bs.bn.x = (int16_t)((40 + rnd_n(160)) * FP); s_bs.bn.y = (int16_t)((30 + rnd_n(120)) * FP);
        s_bs.bn.vx = (int16_t)(bg_sg() * 66); s_bs.bn.vy = (int16_t)(bg_sg() * 53);
        s_bs.bn.k = (uint8_t)rnd_n(4); s_bs.bn.col = (uint8_t)rnd_n(13);
        gm_make(BG_W * 62 / 100, BG_H * 38 / 100, bg_sg() * 47, bg_sg() * 42, 30);
        break;
    default: break;
    }
    s_bg_built = true;
}

static void gm_step(void)
{
    if (!s_gm_on) return;
    s_gm.x = (int16_t)(s_gm.x + s_gm.vx); s_gm.y = (int16_t)(s_gm.y + s_gm.vy);
    const int h = s_gm.T / 2 + 1;
    if (s_bg == BG_BOTS) {
        if (s_gm.y / FP - h > BG_H) { s_gm.y = (int16_t)(-h * FP); s_gm.x = (int16_t)((h + rnd_n(BG_W - 2 * h)) * FP); }
        return;
    }
    if (s_gm.x < h * FP) { s_gm.x = (int16_t)(h * FP); s_gm.vx = (int16_t)abs(s_gm.vx); }
    else if (s_gm.x > (BG_W - h) * FP) { s_gm.x = (int16_t)((BG_W - h) * FP); s_gm.vx = (int16_t)-abs(s_gm.vx); }
    if (s_gm.y < h * FP) { s_gm.y = (int16_t)(h * FP); s_gm.vy = (int16_t)abs(s_gm.vy); }
    else if (s_gm.y > (BG_H - h) * FP) { s_gm.y = (int16_t)((BG_H - h) * FP); s_gm.vy = (int16_t)-abs(s_gm.vy); }
}

static void bg_sim(void)
{
    gm_step();
    switch (s_bg) {
    case BG_BOTS:
        for (int i = 0; i < BG_CREW; ++i) {
            bg_crew_t &c = s_bs.crew.c[i];
            c.y = (int16_t)(c.y + c.v);
            if (c.y / FP - 4 * 9 - 6 > BG_H) crew_spawn(c, false);
        }
        break;
    case BG_GIBSON: {
        for (int i = 0; i < 12; ++i) {
            bg_gbot_t &b = s_bs.gib.b[i];
            bg_dev_t &d = s_bs.gib.d[b.target];
            const int dx = d.x * FP - b.x, dy = d.y * FP - b.y, st = b.speed;
            if (abs(dx) > 2 * FP) b.x = (int16_t)(b.x + (dx > 0 ? 1 : -1) * (st < abs(dx) ? st : abs(dx)));
            else if (abs(dy) > 2 * FP) b.y = (int16_t)(b.y + (dy > 0 ? 1 : -1) * (st < abs(dy) ? st : abs(dy)));
            else { dev_hop(d); b.target = (uint8_t)((b.target + 1 + (i % 3)) % 6); }
            if (b.x < 16 * FP) b.x = 16 * FP; if (b.x > (BG_W - 16) * FP) b.x = (int16_t)((BG_W - 16) * FP);
            if (b.y < 24 * FP) b.y = 24 * FP; if (b.y > (BG_H - 34) * FP) b.y = (int16_t)((BG_H - 34) * FP);
        }
        for (int i = 0; i < 12; ++i)                   /* keep bots (and name tags) from stacking */
            for (int j = i + 1; j < 12; ++j) {
                bg_gbot_t &a = s_bs.gib.b[i], &c = s_bs.gib.b[j];
                const int dx = (c.x - a.x) / FP, dy = (c.y - a.y) / FP;
                if (abs(dx) < 30 && abs(dy) < 34) {
                    const int push = ((34 - abs(dy)) * FP) / 4 + 1, sg = dy >= 0 ? 1 : -1;
                    int ay = a.y - sg * push, cy = c.y + sg * push;
                    if (ay < 24 * FP) ay = 24 * FP; if (ay > (BG_H - 34) * FP) ay = (BG_H - 34) * FP;
                    if (cy < 24 * FP) cy = 24 * FP; if (cy > (BG_H - 34) * FP) cy = (BG_H - 34) * FP;
                    a.y = (int16_t)ay; c.y = (int16_t)cy;
                }
            }
        break;
    }
    case BG_FLYBY:
        for (int i = 0; i < 6; ++i) {
            bg_fly_t &f = s_bs.fly.f[i];
            f.p = (int16_t)(f.p + 27);                 /* 3.3 s per fly-by */
            if (f.p >= 1000) { s_bs.fly.kc++; fly_spawn(f, 0, s_bs.fly.kc); }
        }
        for (int i = 0; i < 56; ++i) {
            bg_star_t &st = s_bs.fly.st[i];
            st.r = (uint16_t)(st.r + 4 + st.r / 10);
            if (st.r > 210 * FP) { st.a = (uint8_t)rnd_n(256); st.r = (uint16_t)((3 + rnd_n(8)) * FP); }
        }
        break;
    case BG_BOUNCE: {
        s_bs.bn.x = (int16_t)(s_bs.bn.x + s_bs.bn.vx); s_bs.bn.y = (int16_t)(s_bs.bn.y + s_bs.bn.vy);
        const int sh = bg_shape(s_bs.bn.k);
        const int hw = BG_SW[sh] * 4 / 2, hh = BG_SH[sh] * 4 / 2;
        bool hx = false, hy = false;
        if (s_bs.bn.x < hw * FP) { s_bs.bn.x = (int16_t)(hw * FP); s_bs.bn.vx = (int16_t)abs(s_bs.bn.vx); hx = true; }
        else if (s_bs.bn.x > (BG_W - hw) * FP) { s_bs.bn.x = (int16_t)((BG_W - hw) * FP); s_bs.bn.vx = (int16_t)-abs(s_bs.bn.vx); hx = true; }
        if (s_bs.bn.y < hh * FP) { s_bs.bn.y = (int16_t)(hh * FP); s_bs.bn.vy = (int16_t)abs(s_bs.bn.vy); hy = true; }
        else if (s_bs.bn.y > (BG_H - hh) * FP) { s_bs.bn.y = (int16_t)((BG_H - hh) * FP); s_bs.bn.vy = (int16_t)-abs(s_bs.bn.vy); hy = true; }
        if (s_bs.bn.flash) s_bs.bn.flash--;
        if (hx || hy) {
            uint8_t c; do { c = (uint8_t)rnd_n(13); } while (c == s_bs.bn.col);
            s_bs.bn.col = c; s_bs.bn.k++;
            if (hx && hy) s_bs.bn.flash = 6;           /* 0.5 s */
            const int s2 = bg_shape(s_bs.bn.k), w2 = BG_SW[s2] * 2, h2 = BG_SH[s2] * 2;
            if (s_bs.bn.x < w2 * FP) s_bs.bn.x = (int16_t)(w2 * FP); if (s_bs.bn.x > (BG_W - w2) * FP) s_bs.bn.x = (int16_t)((BG_W - w2) * FP);
            if (s_bs.bn.y < h2 * FP) s_bs.bn.y = (int16_t)(h2 * FP); if (s_bs.bn.y > (BG_H - h2) * FP) s_bs.bn.y = (int16_t)((BG_H - h2) * FP);
        }
        break;
    }
    default: break;
    }
}

/* ---- painting (world coords; dst y = world y - slice y0) ---- */
static void bg_rect(lgfx::LovyanGFX &p, int x, int y, int w, int h, uint8_t ink)
{ botcorps_pal_rect(p, x, y - s_slice_y0, w, h, ink); }
static void bg_spr(lgfx::LovyanGFX &p, int cx, int cy, int shape, int z, uint16_t c565)
{ botcorps_sprite_draw(p, cx, cy - s_slice_y0, shape, z, botcorps_pal_fit(c565), false); }

/* the ivory Gibson chip: exactly the UI's chip_mini(bot 12) parts - black die, steel silk outline, 5 GOLD
 * castellated pins per side, square ivory face with the slim Matrix shades and steel eyes */
static void bg_gib_chip(lgfx::LovyanGFX &p, int cx, int cy, int T)
{
    const int x = cx - T / 2, y = cy - T / 2, D = T - 2, dx = x + 1, dy = y + 1;
    const uint8_t gold = 177;
    bg_rect(p, dx, dy, D, D, PAL_BLACK);
    bg_rect(p, dx, dy, D, 1, PAL_STEEL); bg_rect(p, dx, dy + D - 1, D, 1, PAL_STEEL);
    bg_rect(p, dx, dy, 1, D, PAL_STEEL); bg_rect(p, dx + D - 1, dy, 1, D, PAL_STEEL);
    for (int k = 0; k < 5; ++k) {
        const int i = (D * (k + 1)) / 6;
        bg_rect(p, x, dy + i, 2, 1, gold); bg_rect(p, x + T - 2, dy + i, 2, 1, gold);
        bg_rect(p, dx + i, y, 1, 2, gold); bg_rect(p, dx + i, y + T - 2, 1, 2, gold);
    }
    const int s = D - 4, fx = cx - s / 2, fy = cy - s / 2;
    const int lip = s >= 12 ? 2 : 1;
    bg_rect(p, fx, fy, s, s, PAL_IVORY);
    bg_rect(p, fx, fy, s, lip, PAL_WHITE);
    bg_rect(p, fx, fy + s - lip, s, lip, PAL_CHIN);
    const int ew = s / 5 > 2 ? s / 5 : 2, gap = s / 4 > ew ? s / 4 : ew;
    const int eyc0 = fy + (s * 22) / 100, eyc = eyc0 > fy + lip + 1 ? eyc0 : fy + lip + 1;
    const int lx = fx + gap, rx = fx + s - gap - ew;
    bg_rect(p, lx, eyc, ew, ew, PAL_STEEL); bg_rect(p, rx, eyc, ew, ew, PAL_STEEL);
    const int pad = s >= 24 ? 2 : 1, lw = ew + pad, l0 = lx - pad, r0 = rx, tl = s >= 24 ? 3 : 1;
    const int lh = ew * 3 / 4 + pad > 2 ? ew * 3 / 4 + pad : 2, ly = eyc + (ew - lh) / 2;
    const int rl = l0 - tl >= fx ? l0 - tl : fx, rr = r0 + lw + tl <= fx + s ? r0 + lw + tl : fx + s;
    bg_rect(p, rl, ly - 1, rr - rl, 1, PAL_BLACK);
    bg_rect(p, l0, ly, lw, lh, PAL_BLACK); bg_rect(p, r0, ly, lw, lh, PAL_BLACK);
    if (lh >= 4) {
        bg_rect(p, l0, ly + lh - 1, 1, 1, PAL_IVORY); bg_rect(p, l0 + lw - 1, ly + lh - 1, 1, 1, PAL_IVORY);
        bg_rect(p, r0, ly + lh - 1, 1, 1, PAL_IVORY); bg_rect(p, r0 + lw - 1, ly + lh - 1, 1, 1, PAL_IVORY);
    }
    if (lw >= 6) { bg_rect(p, l0 + 1, ly + 1, 2, 1, PAL_STEEL); bg_rect(p, r0 + 1, ly + 1, 2, 1, PAL_STEEL); }
}

static const char BG_DEV_PAT[6][5][6] = {        /* mock DEV_PAT 5x5 chip marks */
    { "#...#", ".#.#.", "..#..", ".#.#.", "#...#" }, { ".###.", "#...#", "#.#.#", "#...#", ".###." },
    { "#####", "#...#", "#.#.#", "#...#", "#####" }, { "..#..", ".###.", "#####", ".###.", "..#.." },
    { "#.#.#", ".###.", "#####", ".###.", "#.#.#" }, { "#....", "##...", "###..", "####.", "#####" },
};

static bool bg_hit(int cx0, int cy0, int x, int y, int w, int h)
{ return x < cx0 + CELL && x + w > cx0 && y < cy0 + CELL && y + h > cy0; }

static void bg_paint_cell(lgfx::LovyanGFX &p, int col, int row)
{
    const int x0 = col * CELL, y0 = row * CELL;
    int x, y, w, h;
    switch (s_bg) {
    case BG_GIBSON: {
        static const uint16_t BASE = 0x0042;           /* #050a12 */
        const uint8_t base = botcorps_pal_fit(BASE);
        const uint8_t line = botcorps_pal_fit(bg565(4, 39, 41));
        const uint8_t hl = botcorps_pal_fit(bg565(28, 84, 73));
        const uint8_t dot = botcorps_pal_fit(bg565(22, 64, 58));
        bg_rect(p, x0, y0, CELL, CELL, base);
        for (int gx = (x0 / 20) * 20; gx < x0 + CELL; gx += 20) if (gx >= x0) bg_rect(p, gx, y0, 1, CELL, line);
        for (int gy = (y0 / 20) * 20; gy < y0 + CELL; gy += 20) if (gy >= y0) bg_rect(p, x0, gy, CELL, 1, line);
        if (172 >= y0 && 172 < y0 + CELL) bg_rect(p, x0, 172, CELL, 1, hl);
        for (int gx = ((x0 + 1) / 20) * 20; gx <= x0 + CELL; gx += 20)
            for (int gy = ((y0 + 1) / 20) * 20; gy <= y0 + CELL; gy += 20)
                if (bg_hit(x0, y0, gx - 1, gy - 1, 3, 3)) bg_rect(p, gx - 1, gy - 1, 3, 3, dot);
        for (int i = 0; i < 12; ++i) {                 /* L circuit traces, bot colour at 38% */
            const bg_gbot_t &b = s_bs.gib.b[i]; const bg_dev_t &d = s_bs.gib.d[b.target];
            const int bx = b.x / FP, by = b.y / FP;
            const uint8_t tc = botcorps_pal_fit(bg_mix(BASE, bg_bot565(i), 38, 100));
            const int hx = bx < d.x ? bx : d.x, vy = by < d.y ? by : d.y;
            if (bg_hit(x0, y0, hx, by, abs(d.x - bx) + 1, 1)) bg_rect(p, hx, by, abs(d.x - bx) + 1, 1, tc);
            if (bg_hit(x0, y0, d.x, vy, 1, abs(d.y - by) + 1)) bg_rect(p, d.x, vy, 1, abs(d.y - by) + 1, tc);
        }
        const uint8_t dl = botcorps_pal_fit(bg565(0x9d, 0xcf, 0x72)), df = botcorps_pal_fit(bg565(0x10, 0x24, 0x1c));
        const uint8_t dm = botcorps_pal_fit(bg565(0xd7, 0xe6, 0xcf));
        for (int i = 0; i < 6; ++i) {                  /* device chips */
            const bg_dev_t &d = s_bs.gib.d[i];
            const int dx = d.x - 8, dy = d.y - 7;
            if (!bg_hit(x0, y0, dx - 2, dy, 21, 15)) continue;
            bg_rect(p, dx, dy, 17, 15, dl); bg_rect(p, dx + 1, dy + 1, 15, 13, df);
            for (int ry = 0; ry < 5; ++ry) for (int rx = 0; rx < 5; ++rx)
                if (BG_DEV_PAT[d.pat % 6][ry][rx] == '#') bg_rect(p, dx + 6 + rx, dy + 5 + ry, 1, 1, dm);
            for (int k = 0; k < 3; ++k) { bg_rect(p, dx - 2, dy + 3 + k * 4, 2, 1, dl); bg_rect(p, dx + 17, dy + 3 + k * 4, 2, 1, dl); }
        }
        int ord[12];                                   /* bots back to front by y */
        for (int i = 0; i < 12; ++i) ord[i] = i;
        for (int i = 1; i < 12; ++i) { const int v = ord[i]; int j = i - 1;
            while (j >= 0 && s_bs.gib.b[ord[j]].y > s_bs.gib.b[v].y) { ord[j + 1] = ord[j]; --j; } ord[j + 1] = v; }
        p.setFont(&fonts::Font0); p.setTextSize(1);
        for (int n = 0; n < 12; ++n) {
            const int i = ord[n];
            gbot_box(i, &x, &y, &w, &h);
            if (!bg_hit(x0, y0, x, y, w, h)) continue;
            const bg_gbot_t &b = s_bs.gib.b[i];
            const int sh = bg_shape(i), bx = b.x / FP, by = b.y / FP;
            bg_spr(p, bx, by, sh, 3, bg_bot565(i));
            const int lw = (int)strlen(BG_NAME[i]) * 6;
            botcorps_pal_text(p, botcorps_pal_fit(bg_mix(bg_bot565(i), 0, 35, 100)));
            p.setCursor(bx - lw / 2, by - BG_SH[sh] * 3 / 2 + BG_SH[sh] * 3 + 3 - s_slice_y0);
            p.print(BG_NAME[i]);
        }
        break;
    }
    case BG_FLYBY: {
        bg_rect(p, x0, y0, CELL, CELL, botcorps_pal_fit(bg565(3, 6, 10)));
        for (int i = 0; i < 56; ++i) {
            const bg_star_t &st = s_bs.fly.st[i];
            star_xy(st, &x, &y);
            const int sz = st.r > 100 * FP ? 2 : 1;
            if (x < 0 || y < 0 || x >= BG_W || y >= BG_H || !bg_hit(x0, y0, x, y, sz, sz)) continue;
            int k = st.r * 100 / (120 * FP); if (k > 100) k = 100;
            bg_rect(p, x, y, sz, sz, botcorps_pal_fit(bg_mix(0, bg565(0x8f, 0xb0, 0xa6), 25 + 75 * k / 100, 100)));
        }
        int ord[6];
        for (int i = 0; i < 6; ++i) ord[i] = i;
        for (int i = 1; i < 6; ++i) { const int v = ord[i]; int j = i - 1;
            while (j >= 0 && s_bs.fly.f[ord[j]].p > s_bs.fly.f[v].p) { ord[j + 1] = ord[j]; --j; } ord[j + 1] = v; }
        for (int n = 0; n < 6; ++n) {
            const bg_fly_t &f = s_bs.fly.f[ord[n]];
            int cx, cy, z; float sc; fly_geom(f, &cx, &cy, &z, &sc);
            const int sh = bg_shape(f.k);
            spr_box(cx, cy, sh, z, &x, &y, &w, &h);
            if (!bg_hit(x0, y0, x, y, w, h)) continue;
            bg_spr(p, cx, cy, sh, z, bg_mix(0, bg_pool565(f.col), 30 + 70 * f.p / 1000, 100));
        }
        break;
    }
    case BG_BOUNCE: {
        bg_rect(p, x0, y0, CELL, CELL, botcorps_pal_fit(bg565(5, 7, 10)));
        const uint8_t fr = botcorps_pal_fit(s_bs.bn.flash ? bg565(0xe8, 0xee, 0xf2) : bg565(0x1a, 0x22, 0x30));
        if (y0 == 0) bg_rect(p, x0, 0, CELL, 1, fr);
        if (y0 + CELL == BG_H) bg_rect(p, x0, BG_H - 1, CELL, 1, fr);
        if (x0 == 0) bg_rect(p, 0, y0, 1, CELL, fr);
        if (x0 + CELL == BG_W) bg_rect(p, BG_W - 1, y0, 1, CELL, fr);
        bn_box(&x, &y, &w, &h);
        if (bg_hit(x0, y0, x, y, w, h)) bg_spr(p, s_bs.bn.x / FP, s_bs.bn.y / FP, bg_shape(s_bs.bn.k), 4, bg_pool565(s_bs.bn.col));
        break;
    }
    case BG_BOTS: default: {
        bg_rect(p, x0, y0, CELL, CELL, PAL_FIELD);
        for (int i = 0; i < BG_CREW; ++i) {
            const bg_crew_t &c = s_bs.crew.c[i];
            crew_box(c, &x, &y, &w, &h);
            if (!bg_hit(x0, y0, x, y, w, h)) continue;
            const uint16_t own = bg_bot565(c.b);
            static const int FADE[5] = { 100, 100, 72, 46, 22 };
            for (int t = 4; t >= 0; --t) {            /* trail first, the lead bot on top */
                const int by = c.y / FP - t * 9;
                if (!bg_hit(x0, y0, c.x - 5, by - 5, 10, 10)) continue;
                bg_spr(p, c.x, by, bg_shape(c.b), 1, bg_mix(0, own, FADE[t], 100));
            }
        }
        break;
    }
    }
    if (s_gm_on) {
        chip_box(s_gm, &x, &y, &w, &h);
        if (bg_hit(x0, y0, x - 1, y - 1, w + 2, h + 2)) bg_gib_chip(p, s_gm.x / FP, s_gm.y / FP, s_gm.T);
    }
}

/* one cell, clipped, if it is open (not under chrome / radar / outside the slice) */
static void bg_draw_cell(lgfx::LovyanGFX &panel, int col, int row)
{
    const int x = col * CELL, wy = row * CELL;
    if (wy + CELL <= s_slice_y0 || wy >= s_slice_y1) return;
    if (!cell_open(x, wy)) return;
    if (!cell_clip(panel, x, wy)) return;   /* v1019x: cell clip (and the draw window, when on) */
    bg_paint_cell(panel, col, row);
    panel.clearClipRect();
}

static void bg_step(lgfx::LovyanGFX *panel)
{
    if (!s_bg_built) bg_build();
    bg_mark_scene();
    bg_sim();
    bg_mark_scene();
    for (int i = 0; i < CELL_COLS * CELL_ROWS; ++i) {
        if (!(s_bg_dirty[i >> 3] & (1u << (i & 7)))) continue;
        if (panel) bg_draw_cell(*panel, i % CELL_COLS, i / CELL_COLS);
    }
    memset(s_bg_dirty, 0, sizeof(s_bg_dirty));
}

static void bg_paint_all(lgfx::LovyanGFX &panel)
{
    if (!s_bg_built) bg_build();
    panel.setTextWrap(false, false);
    const int r0 = s_slice_y0 / CELL, r1 = (s_slice_y1 + CELL - 1) / CELL;
    for (int row = r0; row < r1 && row < CELL_ROWS; ++row)
        for (int col = 0; col < CELL_COLS; ++col) bg_draw_cell(panel, col, row);
}
