/* cyd-sim RAIN HARNESS (sim-only, not firmware). Built as target `cydrain`.
 * Boots the firmware's own setup() headless (palette LUT, panel, fonts), then takes over the
 * firmware rain engine (this TU #includes src/rain/botcorps_rain.cpp; the cydrain target excludes
 * that file from its firmware source list so there is exactly one copy) and renders the FULL-SCREEN
 * 240x320 portrait rain, one firmware step (STEP_MS=90) per frame, on a clear background:
 *   - every frame is drawn twice (over black and over white) through botcorps_rain_draw() onto the
 *     real sim panel and read back; pixels equal in both = rain, differing = transparent;
 *   - writes frames/fNNNN.png (RGBA) and atlas.json: every lit cell's key (level + glyph) -> 8x8 RGBA
 *     tile, asserting a key always renders identically (so a JS port can blit the firmware tiles).
 * The engine state is reset to the firmware's power-on RNG seed before build_columns(), so a
 * bit-exact JS port of build/step reproduces the same sequence.
 * usage: cydrain --type device|bots --frames N --out DIR */
#include "rain/botcorps_rain.cpp"
#include <zlib.h>
#include <string>
#include <map>
#include <vector>
#include <sys/stat.h>

extern void setup(void);
extern bool sim_vclock;
extern std::map<std::string, uint32_t> sim_nvs;
extern void sim_nvs_load(void);
SimTouch g_sim_touch;

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

static void grab(std::vector<lgfx::rgb565_t> &px, uint16_t bg)
{
    g_cyd.startWrite(); g_cyd.clearClipRect(); g_cyd.fillScreen(bg); g_cyd.endWrite();
    botcorps_rain_draw(g_cyd, 0, 320);
    g_cyd.startWrite(); g_cyd.clearClipRect(); g_cyd.endWrite();
    g_cyd.readRect(0, 0, 240, 320, px.data());
}

/* key of a lit cell, mirroring draw_cell() for MULTI mode, no strand/gibson/no_bots */
static std::string cell_key(int col, int row, int lv, int lead)
{
    if (lv == 0) return "0";
    const uint8_t gl = *glyph_at(col, row);
    char b[16];
    if (lv == 1 && (lead & 3) == 1) snprintf(b, sizeof b, "%d:B%d", lv, (lead >> 2) % 12);
    else if (lv >= 2 && gl >= 0x80 && gl < 0x80 + 12) snprintf(b, sizeof b, "%d:B%d", lv, gl & 0x0F);
    else if (lv == 1 && (lead & 3) == 2) snprintf(b, sizeof b, "%d:D%d", lv, (lead >> 2) % DEV_N);
    else if (lv >= 2 && gl >= 0xA0) snprintf(b, sizeof b, "%d:D%d", lv, (gl & 0x0F) % DEV_N);
    else { char ch = (char)gl; if (gl >= 0x80) ch = (gl & 1) ? '1' : '0'; snprintf(b, sizeof b, "%d:C%c", lv, ch); }
    return b;
}

int main(int argc, char **argv)
{
    std::string type = "device", out = ".";
    int frames = 400;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--type" && i + 1 < argc) type = argv[++i];
        else if (a == "--frames" && i + 1 < argc) frames = atoi(argv[++i]);
        else if (a == "--out" && i + 1 < argc) out = argv[++i];
    }
    static const uint16_t c[8] = {300, 300, 300, 3900, 3900, 300, 3900, 3900};
    sim_nvs["botcorps/tcal"] = 1; sim_nvs["botcorps/cv"] = 2;
    for (int k = 0; k < 8; ++k) sim_nvs["botcorps/c" + std::to_string(k)] = c[k];
    sim_nvs["botcorps/orient"] = 1;
    sim_vclock = true;
    setup();
    fprintf(stderr, "[rain] setup done t=%lu panel %dx%d\n", (unsigned long)millis(), g_cyd.width(), g_cyd.height());

    /* take over: plain full-screen rain, MULTI colour mode, no covers/hub/gibson/strand */
    botcorps_rain_set_draw(false);
    botcorps_rain_set_covers(nullptr, 0, false);
    s_no_bots = false; s_gibson = false; s_hub_on = false; s_faint = false; botcorps_rain_mock_strand(false); botcorps_rain_mock_strand_dx(0);
    s_bg = BG_RAIN; s_mode = 0;
    s_view_w = 240; s_view_h = 320; s_cell_cols = 30; s_cell_rows = 40;
    s_win_on = false; s_win_nex = 0; s_win_x = 0; s_win_y = 0; s_win_w = 240; s_win_h = 320;
    s_slice_y0 = 0; s_slice_y1 = 320; s_radar = false; s_ncover = 0;
    s_mix = type == "bots" ? BOTCORPS_RAIN_MIX_BOTS : BOTCORPS_RAIN_MIX_FIELD;
    s_rng = 0xA5C3E1u;     /* firmware power-on seed */
    s_built = false;
    build_columns();

    mkdir(out.c_str(), 0755);
    mkdir((out + "/frames").c_str(), 0755);
    std::vector<lgfx::rgb565_t> pb(240 * 320), pw(240 * 320);
    std::vector<uint8_t> rgba(240 * 320 * 4);
    std::map<std::string, std::vector<uint8_t>> atlas;
    long conflicts = 0, lit_total = 0;
    for (int f = 0; f < frames; ++f) {
        grab(pb, 0x0000); grab(pw, 0xFFFF);
        for (int i = 0; i < 240 * 320; ++i) {
            const bool same = pb[i].R8() == pw[i].R8() && pb[i].G8() == pw[i].G8() && pb[i].B8() == pw[i].B8();
            rgba[i * 4 + 0] = same ? pb[i].R8() : 0; rgba[i * 4 + 1] = same ? pb[i].G8() : 0;
            rgba[i * 4 + 2] = same ? pb[i].B8() : 0; rgba[i * 4 + 3] = same ? 255 : 0;
        }
        char p[512]; snprintf(p, sizeof p, "%s/frames/f%04d.png", out.c_str(), f);
        write_png_rgba(p, rgba.data(), 240, 320);
        for (int col = 0; col < 30; ++col) for (int row = 0; row < 40; ++row) {
            int lead = 0; const int lv = cell_level(col, row, &lead);
            ++lit_total;
            std::vector<uint8_t> t(64 * 4);
            for (int y = 0; y < 8; ++y) for (int x = 0; x < 8; ++x)
                memcpy(&t[(y * 8 + x) * 4], &rgba[((row * 8 + y) * 240 + col * 8 + x) * 4], 4);
            const std::string k = cell_key(col, row, lv, lead);
            auto it = atlas.find(k);
            if (it == atlas.end()) atlas[k] = t;
            else if (it->second != t) { if (conflicts < 5) fprintf(stderr, "[rain] CONFLICT key %s f%d c%d r%d\n", k.c_str(), f, col, row); ++conflicts; }
        }
        step_columns(nullptr);     /* one firmware rain step = STEP_MS (90 ms) */
        delay(STEP_MS);
    }
    std::string ap = out + "/atlas.json";
    FILE *af = fopen(ap.c_str(), "w");
    fprintf(af, "{\"type\":\"%s\",\"mix\":%d,\"frames\":%d,\"step_ms\":%d,\"cell\":%d,\"cols\":30,\"rows\":40,\"seed\":%u,\"conflicts\":%ld,\"tiles\":{",
            type.c_str(), (int)s_mix, frames, STEP_MS, CELL, 0xA5C3E1u, conflicts);
    bool first = true;
    for (auto &kv : atlas) {
        fprintf(af, "%s\"%s\":[", first ? "" : ",", kv.first.c_str()); first = false;
        for (size_t i = 0; i < kv.second.size(); ++i) fprintf(af, "%s%u", i ? "," : "", kv.second[i]);
        fprintf(af, "]");
    }
    fprintf(af, "}}\n"); fclose(af);
    fprintf(stderr, "[rain] type=%s mix=%d frames=%d tiles=%zu lit=%ld conflicts=%ld -> %s\n",
            type.c_str(), (int)s_mix, frames, atlas.size(), lit_total, conflicts, out.c_str());
    fflush(stderr);
    _exit(conflicts ? 1 : 0);
}
