// cyd-sim: CYDpaint "Export C++" verification harness (sim-only; firmware untouched).
// Renders every exported screen with LovyanGFX into a 240x320 16-bit LGFX_Sprite and writes PNGs,
// so CYDpaint's canvas can be pixel-diffed against what the real drawing code produces.
//   usage: cydexport OUT_DIR [ui_idx] [theme_idx] [state 0|1|2] [ms]
#include <LovyanGFX.hpp>
#include <zlib.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "export/cydp_gen.h"   // written by cydpaint-preview.py: #include "cydpaint_<name>.h"
#ifndef CYDP_W                 // landscape exports define CYDP_W 320 / CYDP_H 240 (portrait: 240x320)
#define CYDP_W 240
#define CYDP_H 320
#endif

static void be32(std::vector<uint8_t> &v, uint32_t x) { for (int i = 3; i >= 0; --i) v.push_back((x >> (i * 8)) & 255); }
static void chunk(FILE *f, const char *type, const std::vector<uint8_t> &data)
{
    std::vector<uint8_t> b; be32(b, data.size()); fwrite(b.data(), 1, 4, f);
    std::vector<uint8_t> td(type, type + 4); td.insert(td.end(), data.begin(), data.end());
    fwrite(td.data(), 1, td.size(), f);
    std::vector<uint8_t> c; be32(c, crc32(0, td.data(), td.size())); fwrite(c.data(), 1, 4, f);
}
static bool write_png_rgba(const char *path, const uint8_t *rgba, int w, int h)
{
    FILE *f = fopen(path, "wb"); if (!f) return false;
    static const uint8_t sig[8] = {137, 80, 78, 71, 13, 10, 26, 10}; fwrite(sig, 1, 8, f);
    std::vector<uint8_t> ih; be32(ih, w); be32(ih, h); ih.push_back(8); ih.push_back(6); ih.push_back(0); ih.push_back(0); ih.push_back(0);
    chunk(f, "IHDR", ih);
    std::vector<uint8_t> raw; raw.reserve((w * 4 + 1) * h);
    for (int y = 0; y < h; ++y) { raw.push_back(0); raw.insert(raw.end(), rgba + y * w * 4, rgba + (y + 1) * w * 4); }
    uLongf n = compressBound(raw.size()); std::vector<uint8_t> z(n);
    compress2(z.data(), &n, raw.data(), raw.size(), 9); z.resize(n);
    chunk(f, "IDAT", z); chunk(f, "IEND", {}); fclose(f); return true;
}

// compile check: the same draw functions take the panel device too (never called here: no SDL window).
void cydp_compile_check_device(lgfx::LGFX_Device &d) { if (CYDP_SCREEN_COUNT) CYDP_SCREENS[0].draw(d, 0, nullptr); }

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "usage: %s OUT_DIR [ui] [theme] [state] [ms]\n", argv[0]); return 2; }
    std::string out = argv[1];
    int ui = argc > 2 ? atoi(argv[2]) : -1, th = argc > 3 ? atoi(argv[3]) : -1, st = argc > 4 ? atoi(argv[4]) : 0; uint32_t ms = argc > 5 ? strtoul(argv[5], 0, 10) : 0;
    if (th >= 0) cydpaint_set_theme(th);
    if (ui >= 0) cydpaint_set_ui(ui);
    lgfx::LGFX_Sprite sp; sp.setColorDepth(16);
    if (!sp.createSprite(CYDP_W, CYDP_H)) { fprintf(stderr, "createSprite failed\n"); return 1; }
    std::vector<uint8_t> states(CYDP_SP_COUNT ? CYDP_SP_COUNT : 1, (uint8_t)st);
    std::vector<uint8_t> rgba(CYDP_W * CYDP_H * 4);
    for (int s = 0; s < CYDP_SCREEN_COUNT; ++s) {
        sp.clearClipRect(); sp.fillScreen(0);
        CYDP_SCREENS[s].draw(sp, ms, states.data());
        for (int y = 0; y < CYDP_H; ++y) for (int x = 0; x < CYDP_W; ++x) {
            uint16_t c = sp.readPixel(x, y); uint8_t r5 = c >> 11, g6 = (c >> 5) & 63, b5 = c & 31;
            uint8_t *p = &rgba[(y * CYDP_W + x) * 4]; p[0] = (r5 << 3) | (r5 >> 2); p[1] = (g6 << 2) | (g6 >> 4); p[2] = (b5 << 3) | (b5 >> 2); p[3] = 255; }
        char path[1024]; snprintf(path, sizeof path, "%s/screen%02d_st%d_ms%u.png", out.c_str(), s, st, (unsigned)ms);
        if (!write_png_rgba(path, rgba.data(), CYDP_W, CYDP_H)) { fprintf(stderr, "write %s failed\n", path); return 1; }
        printf("%s %s\n", path, CYDP_SCREENS[s].name);
    }
    // touch table smoke test: centre of every zone hits itself
    int bad = 0;
    for (int s = 0; s < CYDP_SCREEN_COUNT; ++s) for (int k = 0; k < CYDP_SCREENS[s].nzones; ++k) { const CydZone &z = CYDP_SCREENS[s].zones[k]; int h = cydpaint_hit(s, z.x + z.w / 2, z.y + z.h / 2); if (h < k) { bad++; fprintf(stderr, "zone %d/%d centre hit %d\n", s, k, h); } }
    printf("zones-ok %d\n", bad == 0);
    return 0;
}
