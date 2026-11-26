/* v3 NEW themes mock (Mobandy 2026-11-16 11:17 AM ET) + 2026-11-25 theme-refresh candidates (16..21):
 * EXT_* = the 12 CYD themes (0..11) + 4 v3 themes (12..15: CIRCUIT/SONAR/HOLO/CAMO) + 6 refresh candidates (16..21:
 * MATRX / STEEL / CARBN + AMBER / TEAL / VAPOR + 3 VAPOR-structure grids SUNST / TRON / ACID). Old themes untouched for 0-px default gate. */
#include <cmath>
#define EXT_NTHEME 25
static const uint32_t EXT_ACC[EXT_NTHEME]  = {0xb324fd, 0xdd00ac, 0x92ff02, 0x00ffff, 0x345efb, 0xfd6b00, 0xffda00, 0xff24fd, 0x20fe01, 0x00fe02, 0xfe0001, 0xff0052,
                                              0xe0913c, 0x3dffcf, 0xc9a8ff, 0x9aa36a,
                                              0x22e868, 0xa8c4e0, 0xff244c, 0xffb010, 0x00e0a8, 0x00e8ff, 0xff7a18, 0x40f0ff, 0xa8ff20};
static const uint32_t EXT_ACCD[EXT_NTHEME] = {0x53205d, 0x68004f, 0x4a6010, 0x00636d, 0x0e5166, 0x61480f, 0x6a5a00, 0x690d69, 0x0d690d, 0x086908, 0x690405, 0x690022,
                                              0x0a4a2c, 0x00444a, 0x4f3f72, 0x3a4a2a,
                                              0x0a5228, 0x3c4b5c, 0x5e0a1a, 0x683c00, 0x005c44, 0x3b1866, 0x691808, 0x0a3a5c, 0x2a6908};
static const uint32_t EXT_HDR[EXT_NTHEME]  = {0xb36afe, 0xdc4aac, 0xb9fb4f, 0x4afffc, 0x6891ff, 0xf4b24c, 0xffe86a, 0xff6afd, 0x6aff6b, 0x6afe6c, 0xff6a6b, 0xff6a96,
                                              0xffd27a, 0xa8ffe8, 0xffc2ee, 0xd8cc98,
                                              0x9effc0, 0xe8f4ff, 0xff889e, 0xffe082, 0x78ffd8, 0xff78e0, 0xffc878, 0xc0ffff, 0xe0ff88};
static bool ext_multi(int t) { return (t >= 7 && t <= 10) || (t >= 12 && t != 20); }
static uint32_t ext_lerp_ring(const uint32_t *P, int n, int u, int period)   /* smooth cyclic blend through n stops */
{
    u %= period; if (u < 0) u += period; const int seg = period / n, k = u / seg, f = u % seg;
    return lv_mix3(P[k % n], P[(k + 1) % n], f, seg);
}
static uint32_t ext_hash(int a, int b) { uint32_t h = (uint32_t)(a * 73856093) ^ (uint32_t)(b * 19349663); h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15; return h; }
static uint32_t ext_theme_at(int t, int x, int y)
{
    switch (t) {
    case 12: {   /* CIRCUIT: green solder mask, copper traces on a 24x32 lattice, gold vias at the joins */
        const int tx = x % 24, ty = y % 32;
        const bool via = (tx >= 9 && tx <= 13) && (ty >= 13 && ty <= 17);
        if (via) return 0xffd27a;
        if ((tx >= 10 && tx <= 12) || (ty >= 14 && ty <= 16)) return 0xe0913c;
        return lv_mix3(0x0f7a45, 0x0a4a2c, y, 319);
    }
    case 13: {   /* SONAR: deep-teal scope, concentric range rings, bright sweep wedge */
        const float dx = x - 120.f, dy = y - 160.f, r = std::sqrt(dx * dx + dy * dy);
        float a = std::atan2(dy, dx) * 57.2958f; a = std::fmod(a - 300.f + 720.f, 360.f);        /* 0 = leading edge of the sweep */
        uint32_t c = lv_mix3(0x00a6a0, 0x00343c, (int)(r > 200 ? 200 : r), 200);
        if (a > 300.f) c = lv_mix3(c, 0x9dffe8, (int)(a - 300.f), 60);                         /* wedge fades in toward the edge */
        if ((int)r % 28 < 3) c = 0x3dffcf;
        return c;
    }
    case 14: {   /* HOLO: iridescent foil, soft pastel sheen flowing on a wavy diagonal */
        static const uint32_t P[5] = {0x8ef6ff, 0xc9a8ff, 0xff9ee6, 0xfff1a8, 0x9effc4};
        return ext_lerp_ring(P, 5, x * 2 + y + (int)(28.f * std::sin(y / 23.f)), 400);
    }
    case 15: {   /* CAMO: digital woodland ops camo, 10 px blocks clumped by a coarse layer */
        static const uint32_t P[4] = {0x2c3a26, 0x55703a, 0x9aa36a, 0xcbbb8a};
        const int bx = x / 10, by = y / 10;
        return P[((ext_hash(bx / 3, by / 3) & 3) * 2 + (ext_hash(bx, by) & 3)) / 2 % 4];
    }
    case 16: {   /* MATRX: falling emerald-mint phosphor rain columns (replaces CIRCUIT) */
        const int col = x / 10;
        const int hy = 40 + (int)(ext_hash(col, 7) % 240);
        const int len = 140 + (int)(ext_hash(col, 13) % 120);
        const int d = (hy - y + 320) % 320;
        const int cell = 75 + (int)(ext_hash(col, y / 8) % 26);
        if (d < 14) return lv_mix3(0xe4fff2, 0x38ff78, d, 13);
        if (d < len) {
            const uint32_t base = lv_mix3(0x24f868, 0x089838, d - 14, len - 14);
            return lv_mix3(0x003814, base, cell, 100);
        }
        return lv_mix3(0x00521e, 0x00782c, cell, 100);
    }
    case 17: {   /* STEEL: anisotropic brushed titanium/chrome sheen (replaces NEON GRAPE) */
        static const uint32_t S[5] = {0x384a5e, 0x6c86a2, 0xb8d2ec, 0xf4f9ff, 0x8aa4c0};
        const int u = (x * 3 + y * 2) % 320;
        const int v = u < 160 ? u : 319 - u;
        const int grain = ((int)(ext_hash(y, 3) & 7) - 3) * 4;
        int k = v * 255 / 159 + grain;
        if (k < 0) k = 0; if (k > 255) k = 255;
        const int seg = 255 / 4, i = k / seg >= 4 ? 3 : k / seg;
        return lv_mix3(S[i], S[i + 1], k - i * seg, seg);
    }
    case 18: {   /* CARBN: 2x2 twill crimson carbon-fiber weave with ruby sheen (replaces SONAR) */
        const int cx = x / 6, cy = y / 6, w = (cx + cy) & 3;
        int d = (x * 2 + y) - 340; if (d < 0) d = -d;
        int sheen = 255 - d * 255 / 340; if (sheen < 0) sheen = 0;
        const bool warp = (w < 2);
        const bool hi = warp && ((y % 6) == 0 || (x % 6) == 0);
        if (hi) return lv_mix3(0xd82040, 0xff6884, sheen, 255);
        return warp ? lv_mix3(0xb8102e, 0xff284c, sheen, 255)
                    : lv_mix3(0x3a0814, 0x760e24, sheen, 255);
    }
    case 19: {   /* AMBER: warm amber phosphor CRT bloom + scanlines (bonus 1) */
        static const uint32_t S[4] = {0xfff0b0, 0xffbe24, 0xd87400, 0x6b3200};
        int dx = x - 120, dy = y - 160; if (dx < 0) dx = -dx; if (dy < 0) dy = -dy;
        int b = (dx * 5 + dy * 4) / 4; if (b > 255) b = 255;
        const int seg = 255 / 3, i = b / seg >= 3 ? 2 : b / seg;
        const uint32_t c = lv_mix3(S[i], S[i + 1], b - i * seg, seg);
        return (y % 4 == 3) ? lv_mix3(c, 0x1a0800, 38, 100) : c;
    }
    case 20: return EXT_ACC[20];   /* TEAL: solid cyber-teal (bonus 2) */
    case 21: {   /* VAPOR: neon horizon + receding perspective cyber-grid (bonus 3) */
        if (y < 146) return lv_mix3(0x00b4ff, 0xff28c8, y, 145);
        if (y <= 154) return 0xffe8ff;
        const int dy = y - 154;
        const bool hline = ((dy >= 8 && dy <= 10) || (dy >= 22 && dy <= 24) || (dy >= 40 && dy <= 42)
                         || (dy >= 64 && dy <= 66) || (dy >= 94 && dy <= 96) || (dy >= 130 && dy <= 132));
        const int gx = (x - 120) * 64 / (dy + 20);
        const bool vline = ((gx % 24 + 24) % 24) < 3;
        if (hline || vline) return lv_mix3(0xff48dc, 0x00f4ff, dy, 165);
        return lv_mix3(0x7018c0, 0x0078b8, dy, 165);
    }
    case 22: {   /* SUNST: sunset grid — orange-to-magenta sky, gold horizon, warm receding grid (VAPOR structure, palette only) */
        if (y < 146) return lv_mix3(0xff6a10, 0xff1878, y, 145);
        if (y <= 154) return 0xffe8a0;
        const int dy = y - 154;
        const bool hline = ((dy >= 8 && dy <= 10) || (dy >= 22 && dy <= 24) || (dy >= 40 && dy <= 42)
                         || (dy >= 64 && dy <= 66) || (dy >= 94 && dy <= 96) || (dy >= 130 && dy <= 132));
        const int gx = (x - 120) * 64 / (dy + 20);
        const bool vline = ((gx % 24 + 24) % 24) < 3;
        if (hline || vline) return lv_mix3(0xff6030, 0xff40c0, dy, 165);
        return lv_mix3(0x801020, 0x301040, dy, 165);
    }
    case 23: {   /* TRON: ice-cyan / electric-blue grid on deep navy (VAPOR structure, palette only) */
        if (y < 146) return lv_mix3(0x041848, 0x1860c0, y, 145);
        if (y <= 154) return 0xc8ffff;
        const int dy = y - 154;
        const bool hline = ((dy >= 8 && dy <= 10) || (dy >= 22 && dy <= 24) || (dy >= 40 && dy <= 42)
                         || (dy >= 64 && dy <= 66) || (dy >= 94 && dy <= 96) || (dy >= 130 && dy <= 132));
        const int gx = (x - 120) * 64 / (dy + 20);
        const bool vline = ((gx % 24 + 24) % 24) < 3;
        if (hline || vline) return lv_mix3(0x80ffff, 0x2080ff, dy, 165);
        return lv_mix3(0x041830, 0x020818, dy, 165);
    }
    case 24: {   /* ACID: lime-to-teal toxic grid (VAPOR structure, palette only) */
        if (y < 146) return lv_mix3(0xb0ff20, 0x00e0a0, y, 145);
        if (y <= 154) return 0xe8ffc0;
        const int dy = y - 154;
        const bool hline = ((dy >= 8 && dy <= 10) || (dy >= 22 && dy <= 24) || (dy >= 40 && dy <= 42)
                         || (dy >= 64 && dy <= 66) || (dy >= 94 && dy <= 96) || (dy >= 130 && dy <= 132));
        const int gx = (x - 120) * 64 / (dy + 20);
        const bool vline = ((gx % 24 + 24) % 24) < 3;
        if (hline || vline) return lv_mix3(0xc8ff20, 0x00ffc8, dy, 165);
        return lv_mix3(0x186820, 0x084040, dy, 165);
    }
    default: return lv_theme_at(t, x, y);
    }
}
