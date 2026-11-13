#pragma once
/* MOCK 2026-11-13 morpheus-mix: crisp vector device emblems (replaces the 16x16 nearest-neighbour icon art on the pillars).
 * Each emblem is a few geometric primitives on a 16x16 canvas; rasterised by coverage (6x6 supersamples per pixel) into 3 alpha
 * masks: body, detail and a 1 px halo, then alpha-blended into RGB565 by the caller. Built once per (device, size) and cached. */
#include <math.h>
#include <stdint.h>
#include <string.h>
struct MoEP { char op, ty; float a, b, c, d, e, f, g; };   /* op '+' body, '-' cut body, 'd' detail. ty: C circle, R ring, A arc, L capsule, B round box, T triangle, E ellipse */
#define MO_EMB_N 20
#define MO_EMB_MAXS 48
#define MO_EMB_MAXP 20
/* index = MO_DEV order: 0 WIFI 1 BLUETOOTH 2 DEAUTH 3 HACK 4 FLOCK 5 DRONE 6 AIR TAG 7 KEYFOB 8 META 9 SKIMMER 10 FLIPPER 11 SAMSUNG
 *                       12 AXON 13 RAVEN 14 ALPR 15 CAMERA 16 RING 17 GOOGLE TAG 18 EVIL TWIN 19 IBEACON */
static const MoEP MO_EMB[MO_EMB_N][MO_EMB_MAXP] = {
 /* WIFI: three fan arcs + dot */
 {{'+','C',8,13.2f,1.7f},{'+','A',8,13.2f,4.0f,1.8f,45,135,1},{'+','A',8,13.2f,7.0f,1.8f,45,135,1},{'+','A',8,13.2f,10.0f,1.8f,47,133,1}},
 /* BLUETOOTH rune */
 {{'+','L',7.6f,1.4f,7.6f,14.6f,1.8f},{'+','L',7.6f,1.4f,12.0f,5.0f,1.8f},{'+','L',12.0f,5.0f,4.0f,11.6f,1.8f},{'+','L',7.6f,14.6f,12.0f,11.0f,1.8f},{'+','L',12.0f,11.0f,4.0f,4.4f,1.8f}},
 /* DEAUTH: Wi-Fi fan struck by an X */
 {{'+','C',6.5f,13.4f,1.5f},{'+','A',6.5f,13.4f,3.8f,1.7f,40,140,1},{'+','A',6.5f,13.4f,6.8f,1.7f,40,140,1},{'+','A',6.5f,13.4f,9.8f,1.7f,52,128,1},
  {'-','C',12.4f,11.2f,3.6f},{'d','L',10.2f,9.0f,14.6f,13.4f,1.8f},{'d','L',14.6f,9.0f,10.2f,13.4f,1.8f}},
 /* HACK: open padlock */
 {{'+','B',2.6f,7.4f,13.4f,15.0f,1.6f},{'+','A',8.6f,4.8f,3.4f,1.8f,0,180,1},{'+','L',5.2f,4.8f,5.2f,7.6f,1.8f},{'+','L',12.0f,4.8f,12.0f,5.4f,1.8f},
  {'-','C',8,10.4f,1.45f},{'-','L',8,10.4f,8,13.2f,1.2f}},
 /* FLOCK: pole-mounted plate camera */
 {{'+','L',3.6f,6.4f,3.6f,15.0f,1.6f},{'+','L',1.6f,15.0f,6.2f,15.0f,1.4f},{'+','L',3.6f,6.4f,6.8f,6.4f,1.4f},{'+','B',6.0f,3.4f,14.6f,8.8f,1.4f},{'+','L',5.6f,2.6f,15.2f,2.6f,1.0f},
  {'-','C',12.2f,6.1f,1.9f},{'d','C',12.2f,6.1f,1.1f},{'d','A',12.2f,6.1f,4.6f,1.0f,-60,-20,1},{'d','A',12.2f,6.1f,6.8f,1.0f,-58,-24,1}},
 /* DRONE: quad, 4 rotors */
 {{'+','L',3.6f,3.6f,12.4f,12.4f,1.6f},{'+','L',12.4f,3.6f,3.6f,12.4f,1.6f},{'+','R',3.4f,3.4f,2.5f,1.3f},{'+','R',12.6f,3.4f,2.5f,1.3f},{'+','R',3.4f,12.6f,2.5f,1.3f},{'+','R',12.6f,12.6f,2.5f,1.3f},
  {'+','B',5.8f,5.8f,10.2f,10.2f,1.3f},{'d','C',8,8,1.0f}},
 /* AIR TAG: disc with a rim gap and an inner ring */
 {{'+','C',8,8,7.2f},{'-','R',8,8,5.7f,0.8f},{'d','R',8,8,3.0f,1.1f},{'d','C',8,8,1.0f}},
 /* KEYFOB (Tile): rounded square, corner hole + key ring, T mark */
 {{'+','B',4.2f,4.2f,15.0f,15.0f,3.0f},{'+','R',3.8f,3.8f,3.0f,1.2f},{'-','C',6.8f,6.8f,1.35f},{'d','L',8.6f,9.4f,12.8f,9.4f,1.3f},{'d','L',10.7f,9.4f,10.7f,12.8f,1.3f}},
 /* META: smart glasses, capture LED */
 {{'+','R',4.4f,9.2f,3.2f,1.6f},{'+','R',11.6f,9.2f,3.2f,1.6f},{'+','A',8,9.6f,1.5f,1.2f,20,160,0},{'+','L',0.9f,7.4f,0.9f,10.6f,1.3f},{'+','L',15.1f,7.4f,15.1f,10.6f,1.3f},
  {'+','L',0.9f,7.4f,15.1f,7.4f,1.2f},{'d','C',13.9f,4.6f,1.1f},{'d','R',13.9f,4.6f,2.3f,0.6f}},
 /* SKIMMER: card with stripe, chip and a radio wave */
 {{'+','B',0.8f,3.4f,15.2f,13.2f,1.6f},{'-','B',0.0f,5.4f,16.0f,7.4f,0},{'d','B',2.6f,9.0f,6.4f,11.8f,0.6f},{'d','A',9.4f,10.4f,1.9f,1.0f,-55,55,1},{'d','A',9.4f,10.4f,3.8f,1.0f,-50,50,1}},
 /* FLIPPER: pocket tool, screen + d-pad */
 {{'+','B',0.6f,4.0f,15.4f,12.6f,3.4f},{'-','B',2.6f,5.8f,8.8f,10.8f,0.7f},{'d','B',3.6f,6.8f,7.8f,9.8f,0.4f},{'-','R',11.9f,8.3f,2.0f,0.7f},{'d','C',11.9f,8.3f,1.0f},{'+','L',13.0f,4.0f,14.2f,1.4f,1.0f}},
 /* SAMSUNG SmartTag: tall rounded tag, top hole, button ring */
 {{'+','B',3.0f,1.6f,13.0f,14.8f,4.2f},{'-','C',8,4.4f,1.45f},{'-','R',8,10.0f,2.6f,0.8f},{'d','C',8,10.0f,1.4f}},
 /* AXON: body camera with clip, lens and record dot */
 {{'+','B',3.2f,3.2f,12.8f,15.0f,1.6f},{'+','L',6.0f,1.4f,10.0f,1.4f,1.6f},{'+','L',8,1.4f,8,3.4f,1.4f},{'-','C',8,7.6f,3.0f},{'d','C',8,7.6f,1.9f},{'d','C',10.4f,12.6f,1.0f},{'d','L',4.8f,12.6f,7.6f,12.6f,0.9f}},
 /* RAVEN: acoustic gunshot sensor on a mast, sound arcs both sides */
 {{'+','L',8,2.8f,8,6.4f,4.2f},{'+','A',8,6.0f,3.2f,1.1f,200,340,0},{'+','L',8,9.2f,8,14.6f,1.4f},{'+','L',4.8f,14.8f,11.2f,14.8f,1.4f},
  {'-','L',6.6f,4.6f,9.4f,4.6f,0.6f},{'d','A',8,4.6f,5.4f,1.1f,-38,38,1},{'d','A',8,4.6f,5.4f,1.1f,142,218,1},{'d','A',8,4.6f,7.4f,1.0f,-30,30,1},{'d','A',8,4.6f,7.4f,1.0f,150,210,1}},
 /* ALPR: licence plate with characters + reader beam */
 {{'+','B',0.6f,5.6f,15.4f,13.6f,1.6f},{'-','B',1.8f,6.8f,14.2f,12.4f,0.9f},{'d','L',3.6f,8.4f,3.6f,10.8f,1.1f},{'d','L',6.2f,8.4f,6.2f,10.8f,1.1f},{'d','L',9.0f,8.4f,9.0f,10.8f,1.1f},{'d','L',11.8f,8.4f,11.8f,10.8f,1.1f},
  {'+','T',5.0f,0.6f,11.0f,0.6f,8,4.6f}},
 /* CAMERA: ceiling dome cam */
 {{'+','B',1.0f,1.6f,15.0f,4.4f,1.0f},{'+','C',8,4.4f,6.2f},{'-','B',-1,-3,17,1.6f,0},{'-','R',8,7.6f,3.0f,0.8f},{'d','C',8,7.6f,1.7f}},
 /* RING: doorbell, camera dot + button ring */
 {{'+','B',4.4f,0.8f,11.6f,15.2f,2.6f},{'-','R',8,4.2f,1.9f,0.7f},{'d','C',8,4.2f,1.0f},{'-','R',8,10.6f,2.4f,1.0f},{'d','C',8,10.6f,1.2f}},
 /* GOOGLE TAG: find-my-device pin */
 {{'+','C',8,6.4f,5.4f},{'+','T',3.1f,8.8f,12.9f,8.8f,8,15.6f},{'-','C',8,6.4f,2.4f},{'d','C',8,6.4f,1.2f}},
 /* EVIL TWIN: two access points side by side; the rogue twin (detail ink, outlined) wears horns */
 {{'+','B',0.6f,9.0f,7.2f,14.8f,1.2f},{'+','L',3.9f,9.0f,3.9f,4.2f,1.3f},{'+','A',3.9f,4.2f,2.4f,0.9f,35,145,1},{'-','C',2.5f,11.9f,0.75f},{'-','C',5.1f,11.9f,0.75f},
  {'d','L',9.4f,9.6f,14.8f,9.6f,1.2f},{'d','L',9.4f,14.2f,14.8f,14.2f,1.2f},{'d','L',9.4f,9.6f,9.4f,14.2f,1.2f},{'d','L',14.8f,9.6f,14.8f,14.2f,1.2f},{'d','C',11.0f,11.9f,0.75f},{'d','C',13.2f,11.9f,0.75f},
  {'d','L',12.1f,9.0f,12.1f,4.2f,1.3f},{'d','T',8.6f,9.2f,10.6f,9.2f,8.2f,5.2f},{'d','T',13.6f,9.2f,15.6f,9.2f,16.0f,5.2f}},
 /* IBEACON: retail shopping bag (tapered body, two loop handles) with a beacon dot + wave */
 {{'+','T',3.2f,6.0f,12.8f,6.0f,1.4f,15.2f},{'+','T',12.8f,6.0f,14.6f,15.2f,1.4f,15.2f},{'+','A',8,6.2f,3.0f,1.2f,0,180,1},{'-','C',5.0f,7.6f,0.7f},{'-','C',11.0f,7.6f,0.7f},
  {'d','C',8,11.4f,1.4f},{'d','A',8,11.4f,3.6f,1.0f,50,130,1}},
};
static inline float mo_e_seg(float px, float py, float ax, float ay, float bx, float by)
{ const float dx = bx - ax, dy = by - ay, l = dx * dx + dy * dy; float t = l > 0 ? ((px - ax) * dx + (py - ay) * dy) / l : 0; t = t < 0 ? 0 : t > 1 ? 1 : t; const float qx = ax + dx * t - px, qy = ay + dy * t - py; return sqrtf(qx * qx + qy * qy); }
static bool mo_e_in(const MoEP &p, float x, float y)
{
    switch (p.ty) {
    case 'C': { const float dx = x - p.a, dy = y - p.b; return dx * dx + dy * dy < p.c * p.c; }
    case 'E': { const float dx = (x - p.a) / p.c, dy = (y - p.b) / p.d; return dx * dx + dy * dy < 1; }
    case 'R': { const float d = sqrtf((x - p.a) * (x - p.a) + (y - p.b) * (y - p.b)); return fabsf(d - p.c) < p.d * 0.5f; }
    case 'A': { const float dx = x - p.a, dy = p.b - y, d = sqrtf(dx * dx + dy * dy); float ang = atan2f(dy, dx) * 57.29578f;
        const float a0 = p.e, a1 = p.f; while (ang < a0) ang += 360; while (ang >= a0 + 360) ang -= 360;
        if (ang <= a1 && fabsf(d - p.c) < p.d * 0.5f) return true;
        if (p.g > 0) { for (int k = 0; k < 2; ++k) { const float t = (k ? a1 : a0) / 57.29578f, cx = p.a + p.c * cosf(t), cy = p.b - p.c * sinf(t); if ((x - cx) * (x - cx) + (y - cy) * (y - cy) < p.d * p.d * 0.25f) return true; } }
        return false; }
    case 'L': return mo_e_seg(x, y, p.a, p.b, p.c, p.d) < p.e * 0.5f;
    case 'B': { const float r = p.e, cx = x < p.a + r ? p.a + r : x > p.c - r ? p.c - r : x, cy = y < p.b + r ? p.b + r : y > p.d - r ? p.d - r : y;
        if (x < p.a || x > p.c || y < p.b || y > p.d) return false; return r <= 0 || (x - cx) * (x - cx) + (y - cy) * (y - cy) <= r * r; }
    case 'T': { auto s = [](float ax, float ay, float bx, float by, float cx, float cy) { return (ax - cx) * (by - cy) - (bx - cx) * (ay - cy); };
        const float d1 = s(x, y, p.a, p.b, p.c, p.d), d2 = s(x, y, p.c, p.d, p.e, p.f), d3 = s(x, y, p.e, p.f, p.a, p.b);
        return !(((d1 < 0) || (d2 < 0) || (d3 < 0)) && ((d1 > 0) || (d2 > 0) || (d3 > 0))); }
    }
    return false;
}
struct MoEmb { int S; uint8_t body[MO_EMB_MAXS * MO_EMB_MAXS], det[MO_EMB_MAXS * MO_EMB_MAXS], halo[(MO_EMB_MAXS + 2) * (MO_EMB_MAXS + 2)]; };
static MoEmb *mo_emb_cache[MO_EMB_N][MO_EMB_MAXS + 1];
static const MoEmb *mo_emb_get(int i, int S)
{
    if (i < 0 || i >= MO_EMB_N) return nullptr; if (S < 4) S = 4; if (S > MO_EMB_MAXS) S = MO_EMB_MAXS;
    if (mo_emb_cache[i][S]) return mo_emb_cache[i][S];
    MoEmb *E = new MoEmb; memset(E, 0, sizeof *E); E->S = S; const int Q = 6; const float k = 16.0f / S;
    for (int y = 0; y < S; ++y) for (int x = 0; x < S; ++x) { int nb = 0, nd = 0;
        for (int a = 0; a < Q; ++a) for (int b = 0; b < Q; ++b) { const float u = (x + (b + 0.5f) / Q) * k, v = (y + (a + 0.5f) / Q) * k; bool B = false, D = false;
            for (int p = 0; p < MO_EMB_MAXP && MO_EMB[i][p].op; ++p) { const MoEP &P = MO_EMB[i][p]; if (!mo_e_in(P, u, v)) continue; if (P.op == '+') B = true; else if (P.op == '-') B = false; else D = true; }
            nb += B && !D; nd += D; }
        E->body[y * S + x] = (uint8_t)(nb * 255 / (Q * Q)); E->det[y * S + x] = (uint8_t)(nd * 255 / (Q * Q)); }
    /* halo: max coverage in the 3x3 neighbourhood (1 px dilation of body + detail), on an (S+2)^2 grid offset by -1 */
    const int H = S + 2;
    for (int y = 0; y < H; ++y) for (int x = 0; x < H; ++x) { int m = 0;
        for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) { const int yy = y - 1 + dy, xx = x - 1 + dx; if (yy < 0 || xx < 0 || yy >= S || xx >= S) continue;
            const int c = E->body[yy * S + xx] + E->det[yy * S + xx]; if (c > m) m = c; }
        E->halo[y * H + x] = (uint8_t)(m > 255 ? 255 : m); }
    mo_emb_cache[i][S] = E; return E;
}
