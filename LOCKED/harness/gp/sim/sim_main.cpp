/* cyd-sim harness: runs the firmware's own setup()/loop() (src/main.cpp, BOTCORPS_CYD) against
 * LovyanGFX Panel_sdl. Modes:
 *   live window (default):   ./cydsim               (mouse = touch, wall clock)
 *   scripted:                 ./cydsim --script steps.txt --out DIR [--headless]
 * Script lines:  wait MS | until ABS_MS (loop until millis() >= ABS_MS) | tap X Y [HOLD_MS] | down X Y | up | shot NAME | orient p|l (NVS seed only, before boot)
 *                | clock MS (jump virtual clock) | echo TEXT | quit
 * Coordinates are screen pixels in the firmware's current rotation (portrait 240x320). */
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/sdl/Panel_sdl.hpp>
#include "display/botcorps_display.h"
#include <zlib.h>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <atomic>
#include <csignal>
#include <unistd.h>
#include <sys/stat.h>

extern void setup(void);
extern void loop(void);
extern bool sim_vclock, sim_pace, sim_serial_echo;
extern std::map<std::string, uint32_t> sim_nvs;
extern const char *sim_nvs_path;
extern void sim_nvs_load(void);
extern void sim_set_millis(uint32_t);
SimTouch g_sim_touch;

static std::string s_out = ".";
static std::vector<std::string> s_steps;
static bool s_headless = false;
static std::atomic<bool> s_snap_req{false};
static int s_snap_n = 0;
static int s_loops_per_frame = 1;

/* ---- PNG (RGB8) ---- */
static void be32(std::vector<uint8_t> &v, uint32_t x) { for (int i = 3; i >= 0; --i) v.push_back((x >> (i * 8)) & 255); }
static void chunk(FILE *f, const char *type, const std::vector<uint8_t> &data)
{
    std::vector<uint8_t> b; be32(b, data.size()); fwrite(b.data(), 1, 4, f);
    std::vector<uint8_t> td(type, type + 4); td.insert(td.end(), data.begin(), data.end());
    fwrite(td.data(), 1, td.size(), f);
    uint32_t crc = crc32(0, td.data(), td.size());
    b.clear(); be32(b, crc); fwrite(b.data(), 1, 4, f);
}
static bool write_png(const char *path, const uint8_t *rgb, int w, int h)
{
    FILE *f = fopen(path, "wb"); if (!f) return false;
    static const uint8_t sig[8] = {137, 80, 78, 71, 13, 10, 26, 10}; fwrite(sig, 1, 8, f);
    std::vector<uint8_t> ihdr; be32(ihdr, w); be32(ihdr, h);
    ihdr.insert(ihdr.end(), {8, 2, 0, 0, 0}); chunk(f, "IHDR", ihdr);
    std::vector<uint8_t> raw; raw.reserve((w * 3 + 1) * h);
    for (int y = 0; y < h; ++y) { raw.push_back(0); raw.insert(raw.end(), rgb + y * w * 3, rgb + (y + 1) * w * 3); }
    uLongf n = compressBound(raw.size()); std::vector<uint8_t> z(n);
    compress2(z.data(), &n, raw.data(), raw.size(), 9); z.resize(n);
    chunk(f, "IDAT", z); chunk(f, "IEND", {}); fclose(f); return true;
}

/* Read the panel in the current rotation as RGB565 and expand 5/6-bit with bit replication
 * (same expansion as tools/host/render_main.cpp rgb_of), so the PNG is exactly the panel's 565 data. */
static void snap(const std::string &name)
{
    const int w = g_cyd.width(), h = g_cyd.height();
    std::vector<lgfx::rgb565_t> px(w * h);
    g_cyd.readRect(0, 0, w, h, px.data());
    std::vector<uint8_t> rgb(w * h * 3);
    for (int i = 0; i < w * h; ++i) {
        rgb[i * 3 + 0] = px[i].R8();   /* (r5<<3)|(r5>>2), same as render_main.cpp rgb_of */
        rgb[i * 3 + 1] = px[i].G8();
        rgb[i * 3 + 2] = px[i].B8();
    }
    std::string p = s_out + "/" + name + ".png";
    write_png(p.c_str(), rgb.data(), w, h);
    fprintf(stderr, "[sim] t=%lu shot %s (%dx%d)\n", (unsigned long)millis(), p.c_str(), w, h);
}

#ifdef SIM_GP
/* gp: raw RGB888 dump of the whole panel (for fitting) and a PNG at an explicit path */
void sim_dump_raw(FILE *f)
{
    const int w = g_cyd.width(), h = g_cyd.height();
    std::vector<lgfx::rgb565_t> px(w * h); g_cyd.readRect(0, 0, w, h, px.data());
    std::vector<uint8_t> rgb(w * h * 3);
    for (int i = 0; i < w * h; ++i) { rgb[i * 3] = px[i].R8(); rgb[i * 3 + 1] = px[i].G8(); rgb[i * 3 + 2] = px[i].B8(); }
    fwrite(rgb.data(), 1, rgb.size(), f);
}
void sim_png(const char *path)
{
    const int w = g_cyd.width(), h = g_cyd.height();
    std::vector<lgfx::rgb565_t> px(w * h); g_cyd.readRect(0, 0, w, h, px.data());
    std::vector<uint8_t> rgb(w * h * 3);
    for (int i = 0; i < w * h; ++i) { rgb[i * 3] = px[i].R8(); rgb[i * 3 + 1] = px[i].G8(); rgb[i * 3 + 2] = px[i].B8(); }
    write_png(path, rgb.data(), w, h);
}
void sim_gp_rainfit(const char *path, int n, int mode);
void sim_gp_v8(const char *dir, int f0, int f1, const char *tl);
#endif
/* screen (current rotation) -> raw native point, inverse of Panel_Device::convertRawXY with identity affine */
static void screen_to_raw(int sx, int sy, int *rx, int *ry)
{
    const int r = g_cyd.getRotation() & 3;
    const int W = g_cyd.width(), H = g_cyd.height();
    const bool vflip = (1 << r) & 0b10010110;
    const int b = vflip ? (H - 1 - sy) : sy;
    const int a = (r & 2) ? (W - 1 - sx) : sx;
    if (r & 1) { *rx = b; *ry = a; } else { *rx = a; *ry = b; }
}

#ifdef SIM_POKE
void sim_poke_hub_zone(int z); void sim_poke_hterm(int); void sim_poke_hubvar(int); void sim_bootproof(int);
void sim_poke_cmd(const char*); void sim_poke_screen(int); int sim_peek_screen(void); int sim_peek_plate(void); void sim_peek_pt(int*);
void sim_peek_buf(char*, int); void sim_peek_keys(void); void sim_poke_clr(void); void sim_poke_put(const char*); int sim_peek_which(int,int); void sim_poke_nosheet(void); void sim_poke_osk(int); void sim_poke_sheet(int); void sim_poke_palette(int); void sim_poke_attack(int); void sim_poke_cl(int); void sim_peek_clkeys(void); void sim_parts(const char*, int, int); void sim_chiplabel(const char*); void sim_chipsheet(int); void sim_facecheck(const char*, int, int); void sim_termcheck(const char*, int, int); void sim_decor_off(int); void sim_termvar(int); void sim_termdev(void);
void sim_termv3(void); void sim_ivdump(const char *); void sim_peek_dl(void); int sim_dl_dev_xy(int, int*, int*);
#endif
static void run_for(uint32_t ms)
{
    const uint32_t t0 = millis();
    do { loop(); } while ((uint32_t)(millis() - t0) < ms);
}

void sim_ovl_clear(void); void sim_ovl_dump(const char *);
static void run_script(void)
{
    for (auto &line : s_steps) {
        std::istringstream ss(line);
        std::string op; ss >> op;
        if (op.empty() || op[0] == '#') continue;
        if (op == "wait") { int ms = 0; ss >> ms; run_for(ms); }
        else if (op == "until") { uint32_t t = 0; ss >> t; while ((int32_t)(millis() - t) < 0) loop(); }
        else if (op == "tap" || op == "down" || op == "dltap") {
            int x = 0, y = 0, hold = 80;
            if (op == "dltap") { int i = 0; ss >> i >> hold;
#ifdef SIM_POKE
                if (!sim_dl_dev_xy(i, &x, &y)) fprintf(stderr, "[sim] dltap %d: no device\n", i);
#endif
            } else ss >> x >> y >> hold;
            int rx, ry; screen_to_raw(x, y, &rx, &ry);
            g_sim_touch.rx = rx; g_sim_touch.ry = ry; g_sim_touch.on = true;
            fprintf(stderr, "[sim] t=%lu %s %d,%d\n", (unsigned long)millis(), op.c_str(), x, y);
            if (op != "down") { run_for(hold); g_sim_touch.on = false; run_for(60); }
        }
        else if (op == "lift") { int n = 1; ss >> n; g_sim_touch.on = false; for (int i = 0; i < n; ++i) loop(); g_sim_touch.on = true; fprintf(stderr, "[sim] t=%lu lift %d loops\n", (unsigned long)millis(), n); }
        else if (op == "up") { g_sim_touch.on = false; run_for(60); }
        else if (op == "shot") { std::string n; ss >> n; snap(n); }
        else if (op == "ovlclr") { sim_ovl_clear(); }
        else if (op == "ovl") { std::string n; ss >> n; sim_ovl_dump((s_out + "/" + n + ".ovl.txt").c_str()); }
        else if (op == "clock") { uint32_t t; ss >> t; sim_set_millis(t); }
        else if (op == "echo") { fprintf(stderr, "[sim] %s\n", line.c_str() + 5); }
#ifdef SIM_POKE
        else if (op == "poke") { std::string k; int v = 0; ss >> k >> v;
            if (k == "hubzone") { sim_poke_hub_zone(v); fprintf(stderr, "[sim] t=%lu poke hubzone %d\n", (unsigned long)millis(), v); }
            else if (k == "screen") { sim_poke_screen(v); }
            else if (k == "hterm") { sim_poke_hterm(v); }
            else if (k == "bootproof") { sim_bootproof(v); }
            else if (k == "hubvar") { sim_poke_hubvar(v); fprintf(stderr, "[sim] t=%lu poke hubvar %d\n", (unsigned long)millis(), v); }
            fprintf(stderr, "[sim] t=%lu state screen=%d plate=%d\n", (unsigned long)millis(), sim_peek_screen(), sim_peek_plate()); }
        else if (op == "cmd") { std::string rest; std::getline(ss, rest); size_t a = rest.find_first_not_of(' '); if (a != std::string::npos) rest = rest.substr(a); sim_poke_cmd(rest.c_str());
            fprintf(stderr, "[sim] t=%lu cmd '%s' -> screen=%d plate=%d\n", (unsigned long)millis(), rest.c_str(), sim_peek_screen(), sim_peek_plate()); }
        else if (op == "dl") { sim_peek_dl(); }
        else if (op == "buf") { char b[256]; sim_peek_buf(b, sizeof b); fprintf(stderr, "[sim] t=%lu buf %s\n", (unsigned long)millis(), b); }
        else if (op == "keys") { sim_peek_keys(); }
        else if (op == "clr") { sim_poke_clr(); }
        else if (op == "which") { int a = 0, b = 0; ss >> a >> b; fprintf(stderr, "[sim] which %d %d -> %d\n", a, b, sim_peek_which(a, b)); }
        else if (op == "nosheet") { sim_poke_nosheet(); }
        else if (op == "osk") { int v=1; ss>>v; sim_poke_osk(v); }
        else if (op == "sheet") { int v=1; ss>>v; sim_poke_sheet(v); }
        else if (op == "palette") { int v=1; ss>>v; sim_poke_palette(v); }
        else if (op == "attack") { int v=1; ss>>v; sim_poke_attack(v); }
        else if (op == "termdev") { sim_termdev(); }
        else if (op == "termv3") { sim_termv3(); }
        else if (op == "ivdump") { std::string n; ss >> n; sim_ivdump((s_out + "/" + n).c_str()); }
        else if (op == "termvar") { std::string v; ss >> v; sim_termvar(v.empty() ? -1 : v[0] - 'A'); }
        else if (op == "decoroff") { int v = 1; ss >> v; sim_decor_off(v); }
        else if (op == "parts") { std::string d; int a = 6, b = 40; ss >> d >> a >> b; sim_parts(d.c_str(), a, b); }
        else if (op == "chiplabel") { std::string d; ss >> d; sim_chiplabel(d.c_str()); }
        else if (op == "termcheck") { std::string d; int a = 4, b = 42; ss >> d >> a >> b; sim_termcheck(d.c_str(), a, b); }
        else if (op == "facecheck") { std::string d; int a = 6, b = 42; ss >> d >> a >> b; sim_facecheck(d.c_str(), a, b); }
        else if (op == "chipsheet") { int k = 16; ss >> k; sim_chipsheet(k); }
        else if (op == "cl") { int v=1; ss>>v; sim_poke_cl(v); }
        else if (op == "clkeys") { sim_peek_clkeys(); }
        else if (op == "put") { std::string rest; std::getline(ss, rest); size_t a = rest.find_first_not_of(' '); sim_poke_put(a == std::string::npos ? "" : rest.substr(a).c_str()); }
        else if (op == "state") { int a[5]; sim_peek_pt(a); fprintf(stderr, "[sim] t=%lu state screen=%d plate=%d pt=%d,%d,%d,%d mode=%d\n", (unsigned long)millis(), sim_peek_screen(), sim_peek_plate(), a[0], a[1], a[2], a[3], a[4]); }
#endif
#ifdef SIM_GP
        else if (op == "rainfit") { std::string p; int n = 100, m = 0; ss >> p >> n >> m; sim_gp_rainfit((s_out + "/" + p).c_str(), n, m); }
        else if (op == "gibv8") { std::string d, tl; int a = 0, b = 0; ss >> d >> a >> b >> tl; mkdir((s_out + "/" + d).c_str(), 0755); sim_gp_v8((s_out + "/" + d).c_str(), a, b, tl.c_str()); }
#endif
        else if (op == "quit") break;
        else if (op == "orient") {}
        else fprintf(stderr, "[sim] unknown step: %s\n", line.c_str());
    }
}

static int fw_thread(bool *running)
{
    setup();
    if (!s_steps.empty()) {
        run_script();
        fflush(stderr);
        _exit(0);
    }
    while (!running || *running) {
        loop();
        if (s_snap_req.exchange(false)) { char n[32]; snprintf(n, sizeof n, "live-%03d", s_snap_n++); snap(n); }
    }
    return 0;
}

static void on_usr1(int) { s_snap_req = true; }

int main(int argc, char **argv)
{
    int scale = 2;
    char orient = 'p';
    bool seed_cal = true;
    const char *script = nullptr;
    bool realtime = false;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto nx = [&]() { return i + 1 < argc ? argv[++i] : (char *)""; };
        if (a == "--script") script = nx();
        else if (a == "--out") s_out = nx();
        else if (a == "--headless") s_headless = true;
        else if (a == "--scale") scale = atoi(nx());
        else if (a == "--landscape") orient = 'l';
        else if (a == "--nvs") sim_nvs_path = nx();
        else if (a == "--no-cal-seed") seed_cal = false;
        else if (a == "--serial") sim_serial_echo = true;
        else if (a == "--pace") sim_pace = true;
        else if (a == "--realtime") realtime = true;
        else { fprintf(stderr, "usage: %s [--script F --out DIR [--headless] [--pace]] [--scale N] [--landscape] [--nvs FILE] [--serial] [--realtime] [--no-cal-seed]\n", argv[0]); return 2; }
    }
    /* NVS seed: a valid cv=2 8-point cal so setup() skips the first-boot calibration screen (the
     * values are never applied; touch is uncalibrated passthrough), and orient (portrait default). */
    if (seed_cal) {
        static const uint16_t c[8] = {300, 300, 300, 3900, 3900, 300, 3900, 3900};
        sim_nvs["botcorps/tcal"] = 1; sim_nvs["botcorps/cv"] = 2;
        for (int k = 0; k < 8; ++k) sim_nvs["botcorps/c" + std::to_string(k)] = c[k];
    }
    sim_nvs["botcorps/orient"] = orient == 'p' ? 1 : 0;
    sim_nvs_load();   /* a --nvs file overrides the seed */

    if (script) {
        std::ifstream f(script);
        if (!f) { fprintf(stderr, "cannot read %s\n", script); return 2; }
        std::string l; while (std::getline(f, l)) s_steps.push_back(l);
        mkdir(s_out.c_str(), 0755);
        if (s_steps.empty()) s_steps.push_back("# empty");
    }
    sim_vclock = script != nullptr && !realtime;
    signal(SIGUSR1, on_usr1);

    g_cyd.sim_panel().setScaling(scale, scale);
    g_cyd.sim_panel().setWindowTitle("BOTcorps CYD sim e9b388a (ESP32-2432S028R)");
    if (s_headless) { fw_thread(nullptr); return 0; }
    return lgfx::Panel_sdl::main(fw_thread);
}
