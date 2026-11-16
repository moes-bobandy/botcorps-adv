/* THREAT BRAIN (MOCK 2026-11-16) - neural-net layer for the LOCKED TERMINALbot screen.
 * Each of the 10 device bots is a node; thin glowing axons run through the dark gaps between the boards to the
 * TERMINALbot hub. A node's halo and lines brighten with its scan activity, and a pulse runs along its axon to the
 * hub on every detection. The layer is a post-pass over the firmware framebuffer and may ONLY change a pixel that is
 *   (a) outside every bot board / the hub / the pin rails / ticker / terminal box (TB_PROT), and
 *   (b) background (dark) in that exact frame,
 * so every bot, face, label, trace, rain glyph and text keeps its locked pixels. Colours = the selected CONTROLbot
 * theme ink (lv_theme_at). Activity comes from a scripted EXAMPLE feed (tb_feed.h), never live radio data.
 * Variant B NEURAL TRACKS (TB_VAR_TRACKS): no new lines at all - the LOCKED circuit traces between the boards become the
 * nerves: a glow hugs them (on dark px only), brighter near busy bots, and each detection sends a signal front climbing
 * the traces outward from the bot that caught it; when it reaches TERMINALbot the hub halo flashes.
 * Needs lv_core.h (lv_theme_at, LV_NHUNT, LV_H_*) and lv_font.h (LV_F35) included first. */
#pragma once
#include <math.h>
#include <string.h>

struct TbRect { int x0, y0, x1, y1; };
/* node boards in LV_H_* order (DEAUTH, META, HACK, SKIMMER, FLOCK, FLIPPER, DRONE, AIRTAG, KEYFOB, SAMSUNG) */
static const TbRect TB_NODE[LV_NHUNT] = {{32, 29, 72, 82}, {167, 29, 207, 82}, {32, 86, 72, 139}, {167, 86, 207, 139}, {32, 143, 72, 196},
                                         {167, 143, 207, 196}, {32, 200, 72, 253}, {78, 200, 118, 253}, {122, 200, 162, 253}, {167, 200, 207, 253}};
static const TbRect TB_HUB = {76, 28, 164, 152};
static const TbRect TB_AREA = {28, 26, 211, 257};   /* the net lives only inside this (pin rails, ticker, terminal box are outside) */

/* axons: node -> hub, through the dark gaps (1x panel px); a pulse runs start -> end */
struct TbPath { int n; int x[7], y[7]; };
static const TbPath TB_AXON[LV_NHUNT] = {
    {5, {52, 52, 74, 74, 76}, {82, 84, 84, 70, 70}},                  /* DEAUTH  down the row gap, up the left channel */
    {5, {187, 187, 165, 165, 164}, {82, 84, 84, 70, 70}},             /* META */
    {5, {52, 52, 74, 74, 76}, {139, 141, 141, 118, 118}},             /* HACK */
    {5, {187, 187, 165, 165, 164}, {139, 141, 141, 118, 118}},        /* SKIMMER */
    {4, {72, 96, 96, 96}, {172, 172, 160, 152}},                      /* FLOCK   into the field, up to the hub pins */
    {4, {167, 144, 144, 144}, {172, 172, 160, 152}},                  /* FLIPPER */
    {6, {52, 52, 75, 75, 88, 88}, {253, 256, 256, 186, 186, 152}},    /* DRONE   bottom bus, DRONE|AIR TAG gap, field */
    {3, {104, 104, 110}, {200, 180, 152}},                            /* AIR TAG */
    {3, {136, 136, 130}, {200, 180, 152}},                            /* KEYFOB */
    {6, {187, 187, 165, 165, 152, 152}, {253, 256, 256, 186, 186, 152}}, /* SAMSUNG */
};
/* synapses: dim cross-links (node a, node b, segment) that make it read as a web, not a bus */
struct TbSyn { int a, b, x0, y0, x1, y1; };
static const TbSyn TB_SYN[] = {
    {LV_H_DEAUTH, LV_H_HACK, 30, 56, 30, 112}, {LV_H_HACK, LV_H_FLOCK, 30, 112, 30, 170}, {LV_H_FLOCK, LV_H_DRONE, 30, 170, 30, 226},
    {LV_H_META, LV_H_SKIMMER, 209, 56, 209, 112}, {LV_H_SKIMMER, LV_H_FLIPPER, 209, 112, 209, 170}, {LV_H_FLIPPER, LV_H_SAMSUNG, 209, 170, 209, 226},
    {LV_H_DEAUTH, LV_H_DEAUTH, 30, 56, 32, 56}, {LV_H_HACK, LV_H_HACK, 30, 112, 32, 112}, {LV_H_FLOCK, LV_H_FLOCK, 30, 170, 32, 170}, {LV_H_DRONE, LV_H_DRONE, 30, 226, 32, 226},
    {LV_H_META, LV_H_META, 207, 56, 209, 56}, {LV_H_SKIMMER, LV_H_SKIMMER, 207, 112, 209, 112}, {LV_H_FLIPPER, LV_H_FLIPPER, 207, 170, 209, 170}, {LV_H_SAMSUNG, LV_H_SAMSUNG, 207, 226, 209, 226},
    {LV_H_HACK, LV_H_FLOCK, 74, 141, 74, 170}, {LV_H_SKIMMER, LV_H_FLIPPER, 165, 141, 165, 170},
    {LV_H_FLOCK, LV_H_AIRTAG, 96, 172, 104, 186}, {LV_H_FLIPPER, LV_H_KEYFOB, 144, 172, 136, 186}, {LV_H_AIRTAG, LV_H_KEYFOB, 104, 192, 136, 192},
    {LV_H_DRONE, LV_H_AIRTAG, 75, 198, 104, 198}, {LV_H_KEYFOB, LV_H_SAMSUNG, 136, 198, 165, 198}, {LV_H_AIRTAG, LV_H_KEYFOB, 118, 256, 122, 256},
    {LV_H_DRONE, LV_H_SAMSUNG, 75, 256, 165, 256},
};
static const int TB_NSYN = (int)(sizeof TB_SYN / sizeof TB_SYN[0]);

/* activity: a = 1 - exp(-E), E = sum over hits 0.45 * exp(-(t - ti) / 2.5 s) */
static float tb_activity(const float *hits, int nh, float t)
{
    float e = 0;
    for (int i = 0; i < nh; ++i) if (hits[i] <= t) e += 0.45f * expf(-(t - hits[i]) / 2.5f);
    return 1.f - expf(-e);
}

static float g_tbL[320][240][3];   /* additive light, 0..~2 per channel */
static inline bool tb_in(const TbRect &r, int x, int y) { return x >= r.x0 && x <= r.x1 && y >= r.y0 && y <= r.y1; }
static bool tb_prot(int x, int y)
{
    if (!tb_in(TB_AREA, x, y) || tb_in(TB_HUB, x, y)) return true;
    for (int i = 0; i < LV_NHUNT; ++i) if (tb_in(TB_NODE[i], x, y)) return true;
    return false;
}
static inline void tb_add(int x, int y, float k, float w, int theme)   /* k = light, w = whiteness (pulse heads) */
{
    if (x < 0 || x > 239 || y < 0 || y > 319 || k <= 0) return;
    const uint32_t c = lv_theme_at(theme, x, y);
    const float r = ((c >> 16) & 255) / 255.f, g = ((c >> 8) & 255) / 255.f, b = (c & 255) / 255.f;
    g_tbL[y][x][0] += k * (r + (1 - r) * w); g_tbL[y][x][1] += k * (g + (1 - g) * w); g_tbL[y][x][2] += k * (b + (1 - b) * w);
}
static void tb_line(int x0, int y0, int x1, int y1, float k, int theme)   /* 1 px core + soft 1 px glow */
{
    int dx = abs(x1 - x0), dy = -abs(y1 - y0), sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, e = dx + dy;
    for (;;) {
        tb_add(x0, y0, k, 0, theme);
        const float g = k * 0.28f;
        if (dx >= -dy) { tb_add(x0, y0 - 1, g, 0, theme); tb_add(x0, y0 + 1, g, 0, theme); } else { tb_add(x0 - 1, y0, g, 0, theme); tb_add(x0 + 1, y0, g, 0, theme); }
        if (x0 == x1 && y0 == y1) break;
        const int e2 = 2 * e;
        if (e2 >= dy) { e += dy; x0 += sx; }
        if (e2 <= dx) { e += dx; y0 += sy; }
    }
}
static void tb_neuron(int x, int y, float k, int theme)   /* junction neuron: bright core + plus */
{
    tb_add(x, y, k * 1.1f, 0.25f, theme);
    tb_add(x - 1, y, k * 0.6f, 0, theme); tb_add(x + 1, y, k * 0.6f, 0, theme); tb_add(x, y - 1, k * 0.6f, 0, theme); tb_add(x, y + 1, k * 0.6f, 0, theme);
}
static void tb_halo(const TbRect &r, float k, int theme)   /* glow ring 1..6 px outside a board */
{
    if (k <= 0.01f) return;
    for (int y = r.y0 - 6; y <= r.y1 + 6; ++y)
        for (int x = r.x0 - 6; x <= r.x1 + 6; ++x) {
            const int ddx = x < r.x0 ? r.x0 - x : x > r.x1 ? x - r.x1 : 0, ddy = y < r.y0 ? r.y0 - y : y > r.y1 ? y - r.y1 : 0;
            const int d = ddx > ddy ? ddx : ddy;
            if (d < 1) continue;
            const float f = 1.f - (d - 1) / 6.f;
            tb_add(x, y, k * f * f * 0.95f, 0, theme);
        }
}
static float tb_path_len(const TbPath &p)
{
    float L = 0; for (int i = 1; i < p.n; ++i) L += hypotf((float)(p.x[i] - p.x[i - 1]), (float)(p.y[i] - p.y[i - 1])); return L;
}
static bool tb_path_at(const TbPath &p, float s, float &x, float &y)
{
    if (s < 0) return false;
    for (int i = 1; i < p.n; ++i) {
        const float L = hypotf((float)(p.x[i] - p.x[i - 1]), (float)(p.y[i] - p.y[i - 1]));
        if (s <= L) { const float u = L > 0 ? s / L : 0; x = p.x[i - 1] + (p.x[i] - p.x[i - 1]) * u; y = p.y[i - 1] + (p.y[i] - p.y[i - 1]) * u; return true; }
        s -= L;
    }
    return false;
}
static const float TB_SPEED = 110.f;   /* pulse speed, px / s */

/* "EX.DATA" stamp in the dark bottom bus row (3x5 font), part of the layer */
static void tb_stamp(int x, int y, const char *s, float k, int theme)
{
    for (; *s; ++s, x += 4) {
        const int c = *s - 0x20; if (c < 0 || c >= 95) continue;
        const uint16_t g = LV_F35[c];
        for (int r = 0; r < 5; ++r) for (int q = 0; q < 3; ++q) if (g & (1 << (14 - (r * 3 + q)))) tb_add(x + q, y + r, k, 0.15f, theme);
    }
}

enum { TB_VAR_BRAIN = 0, TB_VAR_TRACKS = 1 };
static const TbRect TB_TRK = {72, 26, 167, 257};   /* B: the trace field between the board columns */
static inline int tb_rdist(const TbRect &r, int x, int y)   /* Chebyshev distance from a pixel to a rect (0 inside) */
{
    const int dx = x < r.x0 ? r.x0 - x : x > r.x1 ? x - r.x1 : 0, dy = y < r.y0 ? r.y0 - y : y > r.y1 ? y - r.y1 : 0;
    return dx > dy ? dx : dy;
}
static inline float tb_rdistf(const TbRect &a, const TbRect &b)   /* gap between two rects */
{
    const int dx = a.x1 < b.x0 ? b.x0 - a.x1 : b.x1 < a.x0 ? a.x0 - b.x1 : 0, dy = a.y1 < b.y0 ? b.y0 - a.y1 : b.y1 < a.y0 ? a.y0 - b.y1 : 0;
    return (float)(dx > dy ? dx : dy);
}
/* a LOCKED trace pixel: in the field, not on a board, lit, and the same hue as the theme ink */
static bool tb_trace_px(const uint16_t (*base)[240], int theme, int x, int y)
{
    if (!tb_in(TB_TRK, x, y) || tb_prot(x, y)) return false;
    const uint16_t v = base[y][x];
    const float r = (float)(((v >> 11) & 31) << 3), g = (float)(((v >> 5) & 63) << 2), b = (float)((v & 31) << 3);
    if (r < 90 && g < 90 && b < 90) return false;
    const uint32_t c = lv_theme_at(theme, x, y);
    const float R = (float)((c >> 16) & 255), G = (float)((c >> 8) & 255), B = (float)(c & 255);
    const float cs = (r * R + g * G + b * B) / (sqrtf(r * r + g * g + b * b) * sqrtf(R * R + G * G + B * B) + 1e-3f);
    return cs > 0.97f;
}
static uint8_t g_tbTr[320][240];
static void tb_tracks(const uint16_t (*base)[240], int theme, const float *a, const struct TbFeed *feed, float t, float &hub);
struct TbFeed { const float *hit[LV_NHUNT]; int nh[LV_NHUNT]; };
/* build the layer for time t (s) and composite: out = base, plus light only on allowed (dark, unprotected) pixels.
 * mask (optional) gets 1 for every pixel the layer is ALLOWED to touch this frame. */
static void tb_compose(const uint16_t (*base)[240], uint16_t (*out)[240], int theme, const TbFeed *feed, float t, bool stamp, uint8_t (*mask)[240], int var = TB_VAR_BRAIN)
{
    memset(g_tbL, 0, sizeof g_tbL);
    float a[LV_NHUNT];
    for (int n = 0; n < LV_NHUNT; ++n) a[n] = feed ? tb_activity(feed->hit[n], feed->nh[n], t) : 0.f;
    float hub = 0;
    if (var == TB_VAR_TRACKS) { tb_tracks(base, theme, a, feed, t, hub); goto composite; }
    /* lines: idle 0.20, hot 1.0 */
    for (int i = 0; i < TB_NSYN; ++i) {
        const TbSyn &s = TB_SYN[i]; const float aa = a[s.a] > a[s.b] ? a[s.a] : a[s.b];
        tb_line(s.x0, s.y0, s.x1, s.y1, 0.13f + 0.55f * aa, theme);
    }
    for (int n = 0; n < LV_NHUNT; ++n) {
        const TbPath &p = TB_AXON[n]; const float k = 0.20f + 0.80f * a[n];
        for (int i = 1; i < p.n; ++i) tb_line(p.x[i - 1], p.y[i - 1], p.x[i], p.y[i], k, theme);
        for (int i = 1; i < p.n - 1; ++i) tb_neuron(p.x[i], p.y[i], 0.30f + 0.9f * a[n], theme);
        tb_halo(TB_NODE[n], 0.85f * a[n], theme);
    }
    /* pulses + hub flash on arrival */
    if (feed)
        for (int n = 0; n < LV_NHUNT; ++n) {
            const float L = tb_path_len(TB_AXON[n]);
            for (int i = 0; i < feed->nh[n]; ++i) {
                const float dt = t - feed->hit[n][i]; if (dt < 0) continue;
                const float s = dt * TB_SPEED;
                if (s > L) { const float ta = dt - L / TB_SPEED; if (ta < 0.6f) hub += 0.9f * expf(-ta / 0.18f); continue; }
                float x, y;
                for (int k = 0; k <= 12; ++k) {   /* head + 12 px tail */
                    if (!tb_path_at(TB_AXON[n], s - k, x, y)) break;
                    const float f = k == 0 ? 1.7f : 1.1f * (1.f - k / 13.f);
                    const int xi = (int)lroundf(x), yi = (int)lroundf(y);
                    tb_add(xi, yi, f, k < 3 ? 0.65f : 0.2f, theme);
                    if (k == 0) { tb_add(xi - 1, yi, 0.8f, 0.4f, theme); tb_add(xi + 1, yi, 0.8f, 0.4f, theme); tb_add(xi, yi - 1, 0.8f, 0.4f, theme); tb_add(xi, yi + 1, 0.8f, 0.4f, theme); }
                }
            }
        }
composite:
    if (hub > 1.2f) hub = 1.2f;
    tb_halo(TB_HUB, 0.12f + hub, theme);
    if (stamp) tb_stamp(186, 253, "EX.DATA", 0.55f, theme);   /* example-data stamp (bottom-right dark strip) */
    for (int y = 0; y < 320; ++y)
        for (int x = 0; x < 240; ++x) {
            const uint16_t v = base[y][x]; out[y][x] = v;
            const int r = ((v >> 11) & 31) << 3, g = ((v >> 5) & 63) << 2, b = (v & 31) << 3;
            const bool ok = !tb_prot(x, y) && r <= 40 && g <= 40 && b <= 40 && !(r == 24 && g == 24 && b == 24);
            if (mask) mask[y][x] = ok;
            if (!ok) continue;
            float R = r + 255.f * g_tbL[y][x][0], G = g + 255.f * g_tbL[y][x][1], B = b + 255.f * g_tbL[y][x][2];
            if (R > 255) R = 255; if (G > 255) G = 255; if (B > 255) B = 255;
            out[y][x] = (uint16_t)((((int)R >> 3) << 11) | (((int)G >> 2) << 5) | ((int)B >> 3));
        }
}

/* B NEURAL TRACKS: glow on dark px within 2 px of a locked trace; signal fronts expand from the bot along the traces */
static const float TB_TRK_SPEED = 70.f;   /* px / s */
static void tb_tracks(const uint16_t (*base)[240], int theme, const float *a, const TbFeed *feed, float t, float &hub)
{
    for (int y = 0; y < 320; ++y) for (int x = 0; x < 240; ++x) g_tbTr[y][x] = tb_trace_px(base, theme, x, y);
    for (int y = TB_TRK.y0; y <= TB_TRK.y1; ++y)
        for (int x = TB_TRK.x0; x <= TB_TRK.x1; ++x) {
            if (g_tbTr[y][x]) continue;
            int d = 9;
            for (int yy = y - 2; yy <= y + 2; ++yy) for (int xx = x - 2; xx <= x + 2; ++xx) {
                if (yy < 0 || yy > 319 || xx < 0 || xx > 239 || !g_tbTr[yy][xx]) continue;
                const int dd = abs(xx - x) > abs(yy - y) ? abs(xx - x) : abs(yy - y); if (dd < d) d = dd;
            }
            if (d > 2) continue;
            const float fall = d == 1 ? 1.f : 0.38f;
            float act = 0, sig = 0;
            for (int n = 0; n < LV_NHUNT; ++n) {
                const float dn = (float)tb_rdist(TB_NODE[n], x, y);
                const float w = expf(-dn / 28.f); if (a[n] * w > act) act = a[n] * w;
                if (!feed) continue;
                for (int i = 0; i < feed->nh[n]; ++i) {
                    const float dt = t - feed->hit[n][i]; if (dt < 0 || dt > 2.5f) continue;
                    const float df = dt * TB_TRK_SPEED - dn;   /* front passed this px df px ago */
                    if (df >= -1.5f && df < 14.f) { const float k = df < 0 ? 1.f + df / 1.5f : 1.f - df / 14.f; if (k > sig) sig = k; }
                }
            }
            const float k = fall * (0.22f + 0.85f * act) + fall * 1.5f * sig;
            tb_add(x, y, k, sig > 0.5f ? 0.55f * sig : 0.f, theme);
        }
    if (!feed) return;
    for (int n = 0; n < LV_NHUNT; ++n) {
        const float arr = tb_rdistf(TB_NODE[n], TB_HUB) / TB_TRK_SPEED + 0.25f;
        for (int i = 0; i < feed->nh[n]; ++i) { const float ta = t - feed->hit[n][i] - arr; if (ta >= 0 && ta < 0.6f) hub += 0.9f * expf(-ta / 0.18f); }
    }
}
