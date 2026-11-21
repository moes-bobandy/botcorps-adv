// tools/livescan/pad_span_test.cpp : pad_octagon / pad_disc / pad_glow / pad_glow_oct (span versions) vs their *_ref originals.
// Random backgrounds, every band split, sizes 0..48, both pressed states, centres on and off the band edges. HOST TEST ONLY.
// Build from the repo root: g++ -O2 -std=gnu++17 -w -o /tmp/pad_span_test tools/livescan/pad_span_test.cpp -lz && /tmp/pad_span_test
#define main sim_main_unused
#include "sim_live.cpp"
#undef main
#include <random>
int main()
{
    std::mt19937 rng(1121); long cases = 0, bad = 0;
    static uint16_t A[240 * 48], Bf[240 * 48];
    for (int it = 0; it < 60000; ++it) {
        const int bh = 1 + rng() % 40, by0 = (int)(rng() % 330) - 5, rx = rng() % 49, ry = rng() % 49, r = rng() % 49;
        const int cx = (int)(rng() % 280) - 20, cy = by0 + (int)(rng() % 100) - 50, pr = rng() & 1, k = rng() % 9, shape = rng() % 4;
        const uint16_t f = rng(), ink = rng(), c = rng();
        for (int i = 0; i < 240 * bh; ++i) A[i] = Bf[i] = rng();
        LvBand Ba{A, 0, by0, 240, bh}, Bb{Bf, 0, by0, 240, bh};
        if (shape == 0) { pad_octagon(Ba, cx, cy, rx, ry, f, ink, pr); pad_octagon_ref(Bb, cx, cy, rx, ry, f, ink, pr); }
        if (shape == 1) { pad_disc(Ba, cx, cy, r, f, ink, pr); pad_disc_ref(Bb, cx, cy, r, f, ink, pr); }
        if (shape == 2) { pad_glow(Ba, cx, cy, r, c, k); pad_glow_ref(Bb, cx, cy, r, c, k); }
        if (shape == 3) { pad_glow_oct(Ba, cx, cy, rx, ry, c, k); pad_glow_oct_ref(Bb, cx, cy, rx, ry, c, k); }
        ++cases; if (memcmp(A, Bf, 240 * bh * 2)) { if (bad++ < 5) printf("DIFF shape %d rx %d ry %d r %d c %d,%d band %d+%d pr %d\n", shape, rx, ry, r, cx, cy, by0, bh, pr); }
    }
    printf("pad_span_test: %ld cases, %ld differ\n", cases, bad); return bad != 0;
}
