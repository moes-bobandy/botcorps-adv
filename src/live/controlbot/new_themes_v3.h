/* v3 NEW themes mock (Mobandy 2026-11-16 11:17 AM ET): EXT_* = the 12 CYD themes from the verbatim lv_core.h copy (unchanged)
 * + 4 NEW patterned candidates (12..15). Nothing in cyd_themes_lv_core_copy.h is edited; this header only appends. */
#include <cmath>
#define EXT_NTHEME 16
static const uint32_t EXT_ACC[EXT_NTHEME]  = {0xb324fd, 0xdd00ac, 0x92ff02, 0x00ffff, 0x345efb, 0xfd6b00, 0xffda00, 0xff24fd, 0x20fe01, 0x00fe02, 0xfe0001, 0xff0052,
                                              0xe0913c, 0x3dffcf, 0xc9a8ff, 0x9aa36a};
static const uint32_t EXT_ACCD[EXT_NTHEME] = {0x53205d, 0x68004f, 0x4a6010, 0x00636d, 0x0e5166, 0x61480f, 0x6a5a00, 0x690d69, 0x0d690d, 0x086908, 0x690405, 0x690022,
                                              0x0a4a2c, 0x00444a, 0x4f3f72, 0x3a4a2a};
static const uint32_t EXT_HDR[EXT_NTHEME]  = {0xb36afe, 0xdc4aac, 0xb9fb4f, 0x4afffc, 0x6891ff, 0xf4b24c, 0xffe86a, 0xff6afd, 0x6aff6b, 0x6afe6c, 0xff6a6b, 0xff6a96,
                                              0xffd27a, 0xa8ffe8, 0xffc2ee, 0xd8cc98};
static bool ext_multi(int t) { return (t >= 7 && t <= 10) || t >= 12; }
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
    default: return lv_theme_at(t, x, y);
    }
}
