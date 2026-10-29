/* Sim-only CYDpaint parts dump (hub8 05f29ea). Included at the end of sim_ui_poke.cpp so the firmware
 * statics are reachable. Firmware source is NOT modified. Every part is painted by the firmware's own
 * drawing function twice, on two different key colours; a pixel is kept (opaque) only when both renders
 * agree, so the background is exactly transparent. Output: DIR/<name>@<THEME>.png (RGBA, cropped tight)
 * plus DIR/index.tsv (name theme x y w h file). */
#include <zlib.h>
#include <functional>
#include <string>
#include <vector>
extern void sim_set_millis(uint32_t t);
static void sp_chunk(FILE *f, const char *t, const std::vector<uint8_t> &d)
{
    uint8_t b[4]; const uint32_t n = (uint32_t)d.size();
    b[0] = n >> 24; b[1] = n >> 16; b[2] = n >> 8; b[3] = n; fwrite(b, 1, 4, f);
    uLong c = crc32(0, (const Bytef *)t, 4); if (n) c = crc32(c, d.data(), n);
    fwrite(t, 1, 4, f); if (n) fwrite(d.data(), 1, n, f);
    b[0] = c >> 24; b[1] = c >> 16; b[2] = c >> 8; b[3] = c; fwrite(b, 1, 4, f);
}
static bool sp_png(const char *path, const uint8_t *rgba, int w, int h)
{
    FILE *f = fopen(path, "wb"); if (!f) return false;
    static const uint8_t sig[8] = { 0x89, 'P', 'N', 'G', 13, 10, 26, 10 }; fwrite(sig, 1, 8, f);
    std::vector<uint8_t> hd = { (uint8_t)(w >> 24), (uint8_t)(w >> 16), (uint8_t)(w >> 8), (uint8_t)w,
                                (uint8_t)(h >> 24), (uint8_t)(h >> 16), (uint8_t)(h >> 8), (uint8_t)h, 8, 6, 0, 0, 0 };
    sp_chunk(f, "IHDR", hd);
    std::vector<uint8_t> raw; raw.reserve((size_t)(w * 4 + 1) * h);
    for (int y = 0; y < h; ++y) { raw.push_back(0); raw.insert(raw.end(), rgba + (size_t)y * w * 4, rgba + (size_t)(y + 1) * w * 4); }
    uLongf n = compressBound(raw.size()); std::vector<uint8_t> z(n);
    compress2(z.data(), &n, raw.data(), raw.size(), 9); z.resize(n);
    sp_chunk(f, "IDAT", z); sp_chunk(f, "IEND", {}); fclose(f); return true;
}
static FILE *s_sp_idx = nullptr;
static std::string s_sp_dir;
static uint32_t s_sp_ms = 9000;
static uint8_t s_sp_key = 0;   /* bob 0, no blink, TERM cursor on */
/* paint fn on key A and key B; keep agreeing pixels within crop (cx,cy,cw,ch; cw<=0 = whole panel) */
static void sp_part(const std::string &name, const char *th, const std::function<void(Gfx)> &fn, int cx = 0, int cy = 0, int cw = 0, int ch = 0)
{
    lgfx::LovyanGFX &d = botcorps_disp();
    const int W = d.width(), H = d.height();
    if (cw <= 0) { cx = 0; cy = 0; cw = W; ch = H; }
    std::vector<lgfx::rgb565_t> a((size_t)W * H), b((size_t)W * H);
    const uint8_t K[2] = { botcorps_pal_fit(0x07E0), botcorps_pal_fit(0xF81F) };
    for (int pass = 0; pass < 2; ++pass) {
        Gfx g(d);
        g.clearClipRect();
        sim_set_millis(s_sp_ms);
        s_sp_key = K[pass];
        g.fillScreen(K[pass]);
        fn(g);
        g.clearClipRect();
        d.readRect(0, 0, W, H, pass ? b.data() : a.data());
    }
    int x0 = W, y0 = H, x1 = -1, y1 = -1;
    for (int y = cy; y < cy + ch && y < H; ++y) for (int x = cx; x < cx + cw && x < W; ++x) {
        const size_t i = (size_t)y * W + x;
        if (a[i].raw == b[i].raw) { if (x < x0) x0 = x; if (x > x1) x1 = x; if (y < y0) y0 = y; if (y > y1) y1 = y; }
    }
    if (x1 < 0) { fprintf(stderr, "[parts] EMPTY %s %s\n", name.c_str(), th); return; }
    const int w = x1 - x0 + 1, h = y1 - y0 + 1;
    std::vector<uint8_t> o((size_t)w * h * 4, 0);
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
        const size_t i = (size_t)(y0 + y) * W + x0 + x; uint8_t *p = &o[((size_t)y * w + x) * 4];
        if (a[i].raw == b[i].raw) { p[0] = a[i].R8(); p[1] = a[i].G8(); p[2] = a[i].B8(); p[3] = 255; }
    }
    const std::string fn2 = name + "@" + th + ".png";
    sp_png((s_sp_dir + "/" + fn2).c_str(), o.data(), w, h);
    fprintf(s_sp_idx, "%s\t%s\t%d\t%d\t%d\t%d\t%s\n", name.c_str(), th, x0, y0, w, h, fn2.c_str());
}
static void sp_themes(const std::string &name, const std::function<void(Gfx)> &fn, int cx = 0, int cy = 0, int cw = 0, int ch = 0)
{
    const int keep = s_color;
    for (int t = 0; t < THEME_N; ++t) { s_color = t; s_cb_col = accent(); sp_part(name, THEMES[t].sname, fn, cx, cy, cw, ch); }
    s_color = keep; s_cb_col = accent();
}
void sim_parts(const char *dir, int kmin, int kmax)
{
    s_sp_dir = dir;
    const bool P = botcorps_cyd_orient != 0;
    s_sp_idx = fopen((s_sp_dir + (P ? "/index-P.tsv" : "/index-L.tsv")).c_str(), "w");
    const int keep_cur = s_cursor; const bool keep_term = s_hub_term;
    const char *O = P ? "P" : "L";
    auto nm = [&](const char *n) { return std::string(n) + "." + O; };
    s_cursor = -1;
    /* ---- chips: drawBotChipK at every k (portrait panel only; both chip geometries fit up to k40) ---- */
    if (P) {
        static const char *const BN[7] = { "gib", "term", "scan", "ctrl", "boot", "wifi", "ble" };
        for (int bot = 0; bot < 7; ++bot) for (int geo = 0; geo < 2; ++geo) for (int k = kmin; k <= kmax; ++k)
            for (int inf = 0; inf < 2; ++inf) {
                const char *info = nullptr;
                if (inf) { if (bot == MB_GIB) info = "JACK IN"; else if (bot == MB_TERM) info = "TTY0"; else if (bot == MB_SCAN) info = "ALL RADIO"; else if (bot == MB_CTRL) info = "*"; else continue; }
                const bool gP = geo == 0;
                if (mb_h(gP, k) + 8 > 320 || mb_w(gP, k) + 8 > 240) continue;
                char n[64]; snprintf(n, sizeof n, "chip.%s%s.%s.k%d", BN[bot], inf ? ".info" : "", gP ? "P" : "L", k);
                sp_themes(n, [=](Gfx g) { drawBotChipK(g, 4, 4, gP, bot, k, info ? (info[0] == '*' ? theme().sname : info) : nullptr); });
            }
    }
    /* ---- shared board (edge.inc) ---- */
    sp_themes(nm("topbar"), [](Gfx g) { e_bar(g); });
    sp_themes(nm("topbar.status"), [](Gfx g) { chip_mini(g, 14, 12, 14, 12, false, 0); });
    sp_themes(nm("topbar.swatches"), [](Gfx g) { static const uint8_t DIM[12] = { 9, 9, 9, 12, 76, 100, 33, 13, 76, 13, 100, 69 };
        for (int i = 0; i < 12; ++i) g.fillRect(30 + i * 8, 9, 5, 5, DIM[i]); });
    sp_themes(nm("topbar.crown"), [](Gfx g) { gib_crown(g, e_crown_x(), 3); });
    sp_themes(nm("topbar.link"), [](Gfx g) { const int W = EW(); const uint8_t ac = accent(); g.drawCircle(W - 40, 12, 3, ac); g.fillRect(W - 41, 11, 3, 3, ac);
        txt(g, "12", W - 34, 6, PAL_WHITE, lgfx::textdatum::top_left); });
    sp_themes(nm("topbar.alerts"), [](Gfx g) { txt(g, "0", EW() - 12, 9, accent(), lgfx::textdatum::top_left); });
    sp_themes(nm("frame"), [](Gfx g) { e_frame(g); });
    sp_themes(nm("mount"), [](Gfx g) { cb_ring(g, 8, EH() - 9); });
    sp_themes(nm("headerL"), [](Gfx g) { e_headers_hub(g); }, 0, 26, EW() / 2, EH() - 26);
    sp_themes(nm("headerR"), [](Gfx g) { e_headers_hub(g); }, EW() / 2, 26, EW() / 2, EH() - 26);
    sp_themes(nm("board"), [](Gfx g) { g.fillScreen((uint8_t)PAL_FIELD); e_board(g); e_frame(g); });
    for (int kind = 0; kind < 3; ++kind) for (int on = 0; on < 2; ++on) {
        char n[48]; snprintf(n, sizeof n, "pad.k%d.%s", kind, on ? "lit" : "off");
        sp_themes(nm(n), [=](Gfx g) { cb_pad(g, 20, 40, kind, on != 0, false); });
    }
    sp_themes(nm("pad.k2.tip"), [](Gfx g) { cb_pad(g, 20, 40, 2, true, true); });
    /* ---- home + TERMINALbot boards (hubbot.inc) ---- */
    for (int ht = 0; ht < 2; ++ht) {
        s_hub_term = ht != 0; hb_layout_sync();
        const char *bn = ht ? "term" : "home";
        char n[48];
        snprintf(n, sizeof n, "%s.copper", bn); sp_themes(nm(n), [](Gfx g) { hb_copper(g); });
        snprintf(n, sizeof n, "%s.decor", bn); sp_themes(nm(n), [](Gfx g) { hb_rich(g); });
    }
    s_hub_term = false; hb_layout_sync();
    /* single decor pieces, from the home decor layer pass (exact firmware pixels, cut by rect) */
    sp_themes(nm("via"), [](Gfx g) { cb_via(g, 60, 60, cbmix(botcorps_rgb565(cb_trace()), 0xFFFF, 40)); });
    sp_themes(nm("pour"), [](Gfx g) { const uint8_t pour = cbmix(botcorps_rgb565(cb_trace()), 0x0000, 55); cbf(g, 60, 60, 5, 1, pour); cbf(g, 62, 58, 1, 5, pour); });
    sp_themes(nm("smd.h"), [](Gfx g) { const uint8_t smd = sh_rgb(0x2a, 0x2e, 0x36), pad = sh_rgb(0xb8, 0x9a, 0x3c); cbf(g, 60, 60, 10, 4, smd); cbf(g, 60, 60, 2, 4, pad); cbf(g, 68, 60, 2, 4, pad); });
    sp_themes(nm("smd.v"), [](Gfx g) { const uint8_t smd = sh_rgb(0x2a, 0x2e, 0x36), pad = sh_rgb(0xb8, 0x9a, 0x3c); cbf(g, 60, 60, 4, 8, smd); cbf(g, 60, 60, 2, 8, pad); cbf(g, 62, 60, 2, 8, pad); });
    sp_themes(nm("testpad"), [](Gfx g) { const uint8_t smd = sh_rgb(0x2a, 0x2e, 0x36), pad = sh_rgb(0xb8, 0x9a, 0x3c); cbf(g, 58, 58, 5, 5, pad); cbf(g, 59, 59, 3, 3, smd); });
    for (const char *s : { "3V3", "GND", "NET-H" }) {
        char n[48]; snprintf(n, sizeof n, "silk.%s", s);
        sp_themes(nm(n), [=](Gfx g) { if (botcorps_cyd_orient) tiny_text(g, s, 60, 60, cb_silk()); else { const int keep = s_px; use_px(g, 6); txt(g, s, 60, 60, cb_silk(), lgfx::textdatum::top_left); s_px = keep; } });
    }
    /* ---- terminal plate (3-line Adv plate + right column), per board silk ---- */
    for (const char *silk : { "TERMINALBOT", "CONTROLBOT" }) {
        char n[48]; snprintf(n, sizeof n, "plate.%s", silk);
        sp_themes(nm(n), [=](Gfx g) { e_plate(g, HB_TBOT[0], HB_TBOT[1], HB_TBOT[2], HB_TBOT[3], silk, false); });
    }
    /* SELECT chip grid (cl open) and CORE sheet: painted over the plate geometry */
    sp_themes(nm("cmdgrid"), [](Gfx g) { e_plate(g, HB_TBOT[0], HB_TBOT[1], HB_TBOT[2], HB_TBOT[3], HB_TBOT_SILK, false);
        g.fillScreen(s_sp_key); botcorps_lut_term(true); g.fillScreen(s_sp_key); cyd_cl_open(); cyd_cl_paint(g); cyd_cl_close(); botcorps_lut_term(false); });
    sp_themes(nm("coresheet"), [](Gfx g) { e_plate(g, HB_TBOT[0], HB_TBOT[1], HB_TBOT[2], HB_TBOT[3], HB_TBOT_SILK, false);
        g.fillScreen(s_sp_key); botcorps_lut_term(true); g.fillScreen(s_sp_key); s_sheet = true; s_sheet_i = 0; cyd_adv_paint_sheet(g); s_sheet = false; botcorps_lut_term(false); });
    /* ---- CONTROLbot board parts ---- */
    sp_themes(nm("ctl.powerstrip"), [](Gfx g) { cb_power_strip(g); });
    if (P) sp_themes(nm("ctl.meters"), [](Gfx g) { cb_meters(g, CB_P_MET); });
    sp_themes(nm("ctl.orient"), [](Gfx g) { cb_orient(g); });
    for (int c : { 1, 2, 5, 7 }) { char n[48]; snprintf(n, sizeof n, "ctl.chip%d", c); sp_themes(nm(n), [=](Gfx g) { cb_chip(g, c, false); }); }
    for (int c : { 3, 4, 6 }) { char n[48]; snprintf(n, sizeof n, "ctl.part%d", c); sp_themes(nm(n), [=](Gfx g) { cb_part(g, c); }); }
    sp_themes(nm("ctl.copper"), [](Gfx g) { const uint8_t trace = cb_trace(), via = cbmix(botcorps_rgb565(trace), 0xFFFF, 35); cb_copper(g, trace, via); });
    /* ---- hub10: DIAG board pieces + Defend field banners ---- */
    {
        int PL[4]; e_diag_geo(PL);
        int R[4]; e_diag_panel(R);
        int cx, cy, cs; e_diag_chip(&cx, &cy, &cs);
        sp_themes(nm("diag.probe"), [=](Gfx g) { e_probe_chip(g, 20, 40, cs); });
        /* hub10: true re-render sizes for probe (firmware e_probe_chip takes s) */
        for (int ps : {12, 16, 20, 24, 28, 32, 40, 48, 64, 80, 96}) {
            char pn[48]; snprintf(pn, sizeof pn, "diag.probe.s%d", ps);
            sp_themes(nm(pn), [=](Gfx g) { e_probe_chip(g, 20, 40, ps); });
        }
        sp_themes(nm("diag.panel"), [=](Gfx g) {
            int sx, sy, sw, sh;
            e_plate_shell(g, 10, 10, R[2], R[3], "PROBE", false, &sx, &sy, &sw, &sh);
        });
        sp_themes(nm("diag.plate"), [=](Gfx g) { e_plate(g, 10, 10, PL[2], PL[3], "DIAG", false); });
        sp_themes(nm("diag.board"), [](Gfx g) {
            g.fillScreen((uint8_t)PAL_FIELD); e_board(g); e_frame(g);
            /* probe + panel + plate in place (same as paint_diag_e without live probe text) */
            int PL2[4]; e_diag_geo(PL2); int R2[4]; e_diag_panel(R2); int cx2, cy2, cs2; e_diag_chip(&cx2, &cy2, &cs2);
            const uint8_t tr = cb_trace(); const int mid = EW() / 2;
            for (int d : { -8, 8 }) {
                g.fillRect(mid + d, cy2 + cs2 + 3, 1, R2[1] - 2 - (cy2 + cs2 + 3), tr);
                g.fillRect(mid + d, R2[1] + R2[3] + 2, 1, PL2[1] - 2 - (R2[1] + R2[3] + 2), tr);
            }
            e_probe_chip(g, cx2, cy2, cs2);
            int sx, sy, sw, sh; e_plate_shell(g, R2[0], R2[1], R2[2], R2[3], "PROBE", false, &sx, &sy, &sw, &sh);
            e_plate(g, PL2[0], PL2[1], PL2[2], PL2[3], "DIAG", false);
        });
    }
    /* Defend GCHAT field banners — paint via cdl helpers with a forced state */
    sp_themes(nm("banner.charging"), [](Gfx g) {
        int fx, fy, fw, fh; cdl_rect(&fx, &fy, &fw, &fh);
        const uint8_t bc = accent();
        const int bx = fx + 4, by = fy + CDL_HDR + 4, bw = fw - 8, bh = 12;
        g.fillRect(bx, by, bw, bh, bc);
        g.drawRect(bx, by, bw, bh, (uint8_t)PAL_WHITE);
        const int keep = s_px; use_px(g, 6);
        txt(g, "CHARGING 50%", bx + bw / 2, by + 2, (uint8_t)PAL_BLACK, lgfx::textdatum::top_center);
        s_px = keep;
    });
    sp_themes(nm("banner.roundover"), [](Gfx g) {
        int bx, by, bw, bh; cdl_over_box(&bx, &by, &bw, &bh);
        const uint8_t bc = accent();
        g.fillRect(bx, by, bw, bh, (uint8_t)PAL_DEEP);
        g.drawRect(bx, by, bw, bh, (uint8_t)PAL_WHITE);
        g.drawRect(bx + 1, by + 1, bw - 2, bh - 2, bc);
        const int keep = s_px; use_px(g, 6);
        txt(g, "ROUND OVER", bx + bw / 2, by + 5, (uint8_t)PAL_WHITE, lgfx::textdatum::top_center);
        txt(g, "SCORE 12 HI 20", bx + bw / 2, by + 16, bc, lgfx::textdatum::top_center);
        cdl_btn_paint(g, 0, "RESTART", false);
        cdl_btn_paint(g, 1, "X", false);
        s_px = keep;
    });
    sp_themes(nm("banner.idle"), [](Gfx g) {
        int fx, fy, fw, fh; cdl_rect(&fx, &fy, &fw, &fh);
        const uint8_t bc = accent();
        const int bw = fw - 16, bh = 30, bx = fx + 8, by = fy + CDL_HDR + 8;
        g.fillRect(bx, by, bw, bh, (uint8_t)PAL_DEEP);
        g.drawRect(bx, by, bw, bh, bc);
        const int keep = s_px; use_px(g, 6);
        txt(g, "NO ROUND", bx + bw / 2, by + 4, (uint8_t)PAL_WHITE, lgfx::textdatum::top_center);
        txt(g, "TAP: RESTART", bx + bw / 2, by + 17, bc, lgfx::textdatum::top_center);
        s_px = keep;
    });
    s_cursor = keep_cur; s_hub_term = keep_term; hb_layout_sync();
    fclose(s_sp_idx); s_sp_idx = nullptr; s_dirty = true;
    fprintf(stderr, "[parts] done %s\n", O);
}

/* hub10: chip label pad check — every bot × geo × k4..k42 × info on/off.
 * Dumps DIR/chip.<bot>[.info].<P|L>.k<k>.ovl.txt and prints PASS/FAIL summary.
 * Rule: each TEXT inside BODY with pad = max(2, mb_S(3,k)); name/info gap handled by layout. */
void sim_ovl_clear(void); void sim_ovl_dump(const char *);
static int s_cl_min1[2][2][2], s_cl_minfull[2][2][2];
void sim_chiplabel(const char *dir)
{
    memset(s_cl_min1, 0xff, sizeof s_cl_min1); memset(s_cl_minfull, 0xff, sizeof s_cl_minfull);
    static const char *const BN[7] = { "gib", "term", "scan", "ctrl", "boot", "wifi", "ble" };
    const int keep_cur = s_cursor;
    s_cursor = -1;
    int bad = 0, ok = 0, skip = 0;
    char path[512];
    for (int bot = 0; bot < 7; ++bot) for (int geo = 0; geo < 2; ++geo) for (int k = 4; k <= 42; ++k)
        for (int inf = 0; inf < 2; ++inf) {
            const char *info = nullptr;
            if (inf) {
                if (bot == MB_GIB) info = "JACK IN";
                else if (bot == MB_TERM) info = "TTY0";
                else if (bot == MB_SCAN) info = "ALL RADIO";
                else if (bot == MB_CTRL) info = theme().sname;  /* hub11: live theme name, never stale AMBER */
                else { ++skip; continue; }
            }
            const bool gP = geo == 0;
            if (mb_h(gP, k) + 8 > 320 || mb_w(gP, k) + 8 > 240) { ++skip; continue; }
            sim_ovl_clear();
            lgfx::LovyanGFX &d = botcorps_disp();
            Gfx g(d);
            g.clearClipRect();
            /* Keep panel size matching chip geometry (P chips on P panel). */
            g.fillScreen(PAL_FIELD);
            drawBotChipK(g, 4, 4, gP, bot, k, info);
            snprintf(path, sizeof path, "%s/chip.%s%s.%s.k%d.ovl.txt", dir, BN[bot], inf ? ".info" : "", gP ? "P" : "L", k);
            sim_ovl_dump(path);
            /* Inline pad check against BODY (silk window) and CHIP. */
            FILE *f = fopen(path, "r");
            if (!f) { fprintf(stderr, "[chiplabel] DUMPFAIL %s\n", path); ++bad; continue; }
            struct Box { char kind[16]; int x, y, w, h; char name[64]; };
            Box body = {}, chip = {};
            Box texts[8]; int nt = 0;
            char line[256];
            while (fgets(line, sizeof line, f)) {
                Box b = {};
                if (sscanf(line, "%15s\t%d\t%d\t%d\t%d\t%63[^\n]", b.kind, &b.x, &b.y, &b.w, &b.h, b.name) < 5) continue;
                if (!strcmp(b.kind, "BODY")) body = b;
                else if (!strcmp(b.kind, "CHIP")) chip = b;
                else if (!strcmp(b.kind, "TEXT") && nt < 8) texts[nt++] = b;
            }
            fclose(f);
            const int pad = mb_pad(k);
            int v = 0;
            if (body.w <= 0) { fprintf(stderr, "[chiplabel] NOBODY chip.%s%s.%s.k%d\n", BN[bot], inf ? ".info" : "", gP ? "P" : "L", k); ++bad; continue; }
            for (int i = 0; i < nt; ++i) {
                Box &t = texts[i];
                if (t.x < body.x + pad || t.y < body.y + pad ||
                    t.x + t.w > body.x + body.w - pad || t.y + t.h > body.y + body.h - pad) {
                    fprintf(stderr, "[chiplabel] PAD chip.%s%s.%s.k%d text='%s' %d,%d %dx%d body %d,%d %dx%d pad=%d\n",
                            BN[bot], inf ? ".info" : "", gP ? "P" : "L", k, t.name, t.x, t.y, t.w, t.h,
                            body.x, body.y, body.w, body.h, pad);
                    ++v;
                }
                if (chip.w > 0 && (t.x < chip.x || t.y < chip.y || t.x + t.w > chip.x + chip.w || t.y + t.h > chip.y + chip.h)) {
                    fprintf(stderr, "[chiplabel] OVF chip.%s%s.%s.k%d text='%s' outside CHIP\n",
                            BN[bot], inf ? ".info" : "", gP ? "P" : "L", k, t.name);
                    ++v;
                }
            }
            /* name↔info gap >= 2 when both present */
            if (nt >= 2) {
                Box a = texts[0], b = texts[1];
                if (a.y > b.y) { Box tmp = a; a = b; b = tmp; }
                const int gap = b.y - (a.y + a.h);
                if (gap < 2) {
                    fprintf(stderr, "[chiplabel] GAP chip.%s%s.%s.k%d name↔info gap=%d\n",
                            BN[bot], inf ? ".info" : "", gP ? "P" : "L", k, gap);
                    ++v;
                }
            }
            /* hub12 (8:58 steering): TERM/CTRL must read TERMINALBOT / CONTROLBOT — one line, or the
             * two-line NAME/BOT form. Short names (TERMBOT, TERM, CTRLBOT, CTRL) are banned at k>=9. */
            if (bot == MB_TERM || bot == MB_CTRL) {
                const char *full = bot == MB_TERM ? "TERMINALBOT" : "CONTROLBOT";
                const char *top  = bot == MB_TERM ? "TERMINAL" : "CONTROL";
                const char *sh1  = bot == MB_TERM ? "TERMBOT" : "CTRLBOT";
                const char *sh2  = bot == MB_TERM ? "TERM" : "CTRL";
                bool one = false, two_a = false, two_b = false, shortn = false;
                for (int i = 0; i < nt; ++i) {
                    if (!strcmp(texts[i].name, full)) one = true;
                    if (!strcmp(texts[i].name, top)) two_a = true;
                    if (!strcmp(texts[i].name, "BOT")) two_b = true;
                    if (!strcmp(texts[i].name, sh1) || !strcmp(texts[i].name, sh2)) shortn = true;
                }
                const bool fulls = one || (two_a && two_b);
                const int bi = bot == MB_TERM ? 0 : 1;
                if (fulls) {
                    int &m1 = s_cl_min1[bi][gP ? 0 : 1][inf], &m2 = s_cl_minfull[bi][gP ? 0 : 1][inf];
                    if (one && (m1 < 0 || k < m1)) m1 = k;
                    if (m2 < 0 || k < m2) m2 = k;
                }
                if (k >= 9 && (shortn || !fulls)) {
                    fprintf(stderr, "[chiplabel] NAME chip.%s%s.%s.k%d %s\n", BN[bot], inf ? ".info" : "",
                            gP ? "P" : "L", k, shortn ? "short name shown" : "full name missing");
                    ++v;
                }
            }
            if (v) bad++; else ok++;
        }
    s_cursor = keep_cur;
    for (int bi = 0; bi < 2; ++bi) for (int o = 0; o < 2; ++o) for (int inf = 0; inf < 2; ++inf)
        fprintf(stderr, "[chiplabel] MINK %s %s%s full-name(any)=k%d one-line=k%d\n", bi ? "CONTROLBOT" : "TERMINALBOT",
                o ? "L" : "P", inf ? "+info" : "", s_cl_minfull[bi][o][inf], s_cl_min1[bi][o][inf]);
    fprintf(stderr, "[chiplabel] PASS %d  FAIL %d  SKIP %d\n", ok, bad, skip);
}

/* hub10 proof: all 7 bots at one k (P geo), with info where the board shows it */
void sim_chipsheet(int k16)
{
    if (k16 < 4) k16 = 4;
    static const char *const INFO[7] = { "JACK IN", "TTY0", "ALL RADIO", "*", nullptr, nullptr, nullptr }; /* * = theme().sname */
    lgfx::LovyanGFX &d = botcorps_disp();
    Gfx g(d);
    g.fillScreen(PAL_FIELD);
    e_frame(g);
    const int keep = s_cursor; s_cursor = -1;
    const int W = EW(), H = EH();
    int x = 6, y = 28, row_h = 0;
    char lab[32]; snprintf(lab, sizeof lab, "k%d", k16);
    { const int keepp = s_px; use_px(g, 6); txt(g, lab, 8, 8, PAL_WHITE, lgfx::textdatum::top_left); s_px = keepp; }
    for (int bot = 0; bot < 7; ++bot) {
        const int w = mb_w(true, k16), h = mb_h(true, k16);
        if (x + w > W - 4) { x = 6; y += row_h + 4; row_h = 0; }
        if (y + h > H - 4) break;
        drawBotChipK(g, x, y, true, bot, k16, INFO[bot] && INFO[bot][0] == '*' ? theme().sname : INFO[bot]);
        x += w + 4; if (h > row_h) row_h = h;
    }
    s_cursor = keep;
    s_dirty = true;
}
/* hub12 SCAN gold hard check: paint drawBotChipK(MB_SCAN) for every k / geometry / info / theme / bob / blink
 * phase, read the real panel pixels inside the face rect (same mb_geo/mb_lab_plan maths as drawBotChipK) and
 * require hue 30..50 deg and R>=G>=B for every pixel except the pure-black eye pixels. */
void sim_facecheck(const char *tsv, int kmin, int kmax)
{
    lgfx::LovyanGFX &d = botcorps_disp();
    const int W = d.width(), H = d.height();
    FILE *f = fopen(tsv, "w");
    fprintf(f, "geo\tk\tinfo\ttheme\tms\tfs\tface_px\teye_px\tbad\thue_min\thue_max\n");
    const int keep_cur = s_cursor; s_cursor = -1;
    long tot = 0, totbad = 0, toteye = 0; double hmin = 999, hmax = -1;
    static const uint32_t MS[3] = { 9000, 9900, 18200 + 20 };  /* bob0, bob1, blink */
    std::vector<lgfx::rgb565_t> px((size_t)W * H);
    for (int geo = 0; geo < 2; ++geo) for (int k = kmin; k <= kmax; ++k) for (int inf = 0; inf < 2; ++inf)
    for (int t = 0; t < THEME_N; ++t) for (int mi = 0; mi < 3; ++mi) {
        const bool gP = geo == 0;
        if (mb_h(gP, k) + 8 > H || mb_w(gP, k) + 8 > W) continue;
        s_color = t; s_cb_col = accent();
        const char *info = inf ? "ALL RADIO" : nullptr;
        Gfx g(d); g.clearClipRect(); sim_set_millis(MS[mi]);
        g.fillScreen((uint8_t)PAL_FIELD);
        drawBotChipK(g, 4, 4, gP, MB_SCAN, k, info);
        d.readRect(0, 0, W, H, px.data());
        const MbLab lab = mb_lab_plan(gP, MB_SCAN, k, info);
        int dcx, dcy, D, rty; mb_geo(gP, &dcx, &dcy, &D, &rty, k, lab.info || lab.nm2);
        const int bob = ((millis() / 900u) % 2u) ? 1 : 0;
        const int fs = D - mb_S(12, k), cx = 4 + dcx, fy = 4 + dcy + bob - 1;
        const int x0 = cx - fs / 2, y0 = fy - fs / 2;
        long n = 0, bad = 0, eye = 0; double lo = 999, hi = -1;
        for (int y = y0; y < y0 + fs; ++y) for (int x = x0; x < x0 + fs; ++x) {
            const lgfx::rgb565_t p = px[(size_t)y * W + x];
            const int R = p.R8(), G = p.G8(), B = p.B8();
            if (p.raw == 0) { ++eye; continue; }
            ++n;
            const int mx = R > G ? (R > B ? R : B) : (G > B ? G : B), mn = R < G ? (R < B ? R : B) : (G < B ? G : B);
            double h = -1;
            if (mx != mn && mx == R) h = 60.0 * (double)(G - B) / (double)(mx - mn);
            if (h >= 0) { if (h < lo) lo = h; if (h > hi) hi = h; }
            if (!(R >= G && G >= B) || h < 30.0 || h > 50.0) {
                if (bad < 3) fprintf(stderr, "[facecheck] BAD geo=%s k=%d t=%d (%d,%d) rgb=%d,%d,%d h=%.1f\n", gP ? "P" : "L", k, t, x - x0, y - y0, R, G, B, h);
                ++bad;
            }
        }
        tot += n; totbad += bad; toteye += eye; if (lo < hmin) hmin = lo; if (hi > hmax) hmax = hi;
        fprintf(f, "%s\t%d\t%d\t%s\t%u\t%d\t%ld\t%ld\t%ld\t%.1f\t%.1f\n", gP ? "P" : "L", k, inf, THEMES[t].sname, (unsigned)MS[mi], fs, n, eye, bad, lo, hi);
    }
    fclose(f);
    s_cursor = keep_cur; s_color = 0; s_cb_col = accent();
    fprintf(stderr, "[facecheck] face_px=%ld eye_px=%ld bad=%ld hue=[%.1f,%.1f] %s\n", tot, toteye, totbad, hmin, hmax, totbad ? "FAIL" : "PASS");
}

/* hub12 TERMINALbot face check: every face pixel must be either an allowed mark (eyes + >_ from
 * e_term_marks, must be black) or the silver ramp {41,110,146,186,223,255}; anything else = stray. */
void sim_termcheck(const char *tsv, int kmin, int kmax)
{
    lgfx::LovyanGFX &d = botcorps_disp();
    const int W = d.width(), H = d.height();
    FILE *f = fopen(tsv, "w");
    fprintf(f, "geo\tk\tinfo\tms\tfs\tface_px\tmark_px\tstray\tmark_miss\n");
    const int keep_cur = s_cursor; s_cursor = -1;
    static const uint8_t RAMP[6] = { 41, 110, 146, 186, 223, 255 };
    uint16_t ok565[6]; for (int i = 0; i < 6; ++i) ok565[i] = botcorps_rgb565(RAMP[i]);
    long tot = 0, stray = 0, miss = 0, marks = 0;
    static const uint32_t MS[3] = { 9000, 9900, 18200 + 20 };
    std::vector<lgfx::rgb565_t> px((size_t)W * H);
    for (int geo = 0; geo < 2; ++geo) for (int k = kmin; k <= kmax; ++k) for (int inf = 0; inf < 2; ++inf)
    for (int mi = 0; mi < 3; ++mi) {
        const bool gP = geo == 0;
        if (mb_h(gP, k) + 8 > H || mb_w(gP, k) + 8 > W) continue;
        const char *info = inf ? "TTY0" : nullptr;
        Gfx g(d); g.clearClipRect(); sim_set_millis(MS[mi]);
        g.fillScreen((uint8_t)PAL_FIELD);
        drawBotChipK(g, 4, 4, gP, MB_TERM, k, info);
        d.readRect(0, 0, W, H, px.data());
        const MbLab lab = mb_lab_plan(gP, MB_TERM, k, info);
        int dcx, dcy, D, rty; mb_geo(gP, &dcx, &dcy, &D, &rty, k, lab.info || lab.nm2);
        const int bob = ((millis() / 900u) % 2u) ? 1 : 0;
        const int fs = D - mb_S(12, k), cx = 4 + dcx, fy = 4 + dcy + bob - 1;
        const int x0 = cx - fs / 2, y0 = fy - fs / 2;
        int R[8][4]; const int nr = e_term_marks(x0, y0, fs, e_blink_now(), R);
        long n = 0, st = 0, ms = 0, mk = 0;
        for (int y = y0; y < y0 + fs; ++y) for (int x = x0; x < x0 + fs; ++x) {
            bool inm = false;
            for (int q = 0; q < nr; ++q) if (x >= R[q][0] && x < R[q][0] + R[q][2] && y >= R[q][1] && y < R[q][1] + R[q][3]) inm = true;
            const uint16_t v = px[(size_t)y * W + x].raw;
            /* readRect returns byte-swapped rgb565_t.raw? compare via channels */
            const lgfx::rgb565_t p = px[(size_t)y * W + x];
            ++n;
            if (inm) { ++mk; if (v != 0) { if (ms < 3) fprintf(stderr, "[termcheck] MISS %s k=%d (%d,%d)\n", gP ? "P" : "L", k, x - x0, y - y0); ++ms; } continue; }
            bool ok = false;
            for (int i = 0; i < 6; ++i) {
                const uint16_t c = ok565[i];
                const int r8 = ((c >> 11) & 31) * 255 / 31, g8 = ((c >> 5) & 63) * 255 / 63, b8 = (c & 31) * 255 / 31;
                if (abs(p.R8() - r8) <= 8 && abs(p.G8() - g8) <= 8 && abs(p.B8() - b8) <= 8) ok = true;
            }
            if (!ok) { if (st < 5) fprintf(stderr, "[termcheck] STRAY %s k=%d inf=%d (%d,%d) rgb=%d,%d,%d\n", gP ? "P" : "L", k, inf, x - x0, y - y0, p.R8(), p.G8(), p.B8()); ++st; }
        }
        tot += n; stray += st; miss += ms; marks += mk;
        fprintf(f, "%s\t%d\t%d\t%u\t%d\t%ld\t%ld\t%ld\t%ld\n", gP ? "P" : "L", k, inf, (unsigned)MS[mi], fs, n, mk, st, ms);
    }
    fclose(f);
    s_cursor = keep_cur;
    fprintf(stderr, "[termcheck] face_px=%ld mark_px=%ld stray=%ld mark_miss=%ld %s\n", tot, marks, stray, miss, (stray || miss) ? "FAIL" : "PASS");
}

/* hub12: decor A/B toggle for the clearance check */
void sim_decor_off(int v) { s_e_decor_off = v != 0; }

/* hub12 SIM-ONLY TERMINALbot layout variants (Mobandy 8:51-8:58 PM ET). Firmware HN_TERM_P/L and HT_RT_*
 * are untouched; this painter only exists in the sim. Slots = HB4_* {x, y, k16}; w/h from mb_w/mb_h.
 * No chip gets a ring: lit=false everywhere. C's "active" TERM = enlarged + accent ACTIVE info line. */
struct SimTV { int s[5][3]; int nr; Pt r[6][4]; int rn[6]; const char *tinfo; };
static const SimTV SIM_TV[4][2] = {
 { /* A */
  { { {32,51,8}, {33,157,13}, {166,51,8}, {78,30,16}, {157,183,6} }, 6,
    { {{74,95},{77,95}}, {{162,95},{165,95}}, {{112,150},{112,200},{101,200}}, {{126,150},{126,205},{156,205}},
      {{52,111},{52,156}}, {{186,111},{186,182}} }, {2,2,3,3,2,2}, "TTY0" },
  { { {42,28,12}, {42,104,12}, {222,28,12}, {123,30,16}, {236,122,6} }, 5,
    { {{98,62},{122,62}}, {{221,62},{198,62}}, {{98,140},{110,140},{110,110},{122,110}},
      {{197,110},{216,110},{216,140},{235,140}}, {{250,100},{250,121}} }, {2,2,4,4,2}, "TTY0" } },
 { /* B */
  { { {34,30,7}, {76,30,10}, {134,30,7}, {78,135,16}, {177,30,5} }, 4,
    { {{52,84},{52,118},{88,118},{88,134}}, {{102,106},{102,134}}, {{152,84},{152,134}}, {{190,69},{190,190},{163,190}} },
    {4,2,2,3}, "TTY0" },
  { { {70,30,10}, {137,30,10}, {204,30,10}, {130,96,13}, {256,30,6} }, 4,
    { {{93,91},{93,138},{129,138}}, {{160,91},{160,95}}, {{227,91},{227,138},{191,138}}, {{270,67},{270,160},{191,160}} },
    {3,2,3,3}, "TTY0" } },
 { /* C */
  { { {32,51,8}, {33,157,13}, {166,51,8}, {128,142,15}, {102,64,7} }, 6,
    { {{74,85},{101,85}}, {{165,85},{140,85}}, {{112,118},{112,200},{102,200}}, {{130,118},{130,141}},
      {{52,111},{52,156}}, {{186,111},{186,141}} }, {2,2,3,2,2,2}, "ACTIVE" },
  { { {42,28,12}, {42,104,12}, {222,28,12}, {222,104,12}, {142,54,8} }, 4,   /* L: no height for k13 above the plate */
    { {{98,62},{141,62}}, {{221,62},{180,62}}, {{98,140},{110,140},{110,90},{141,90}}, {{180,90},{200,90},{200,140},{221,140}} },
    {2,2,4,4}, "ACTIVE" } },
 { /* D */
  { { {162,40,8}, {124,108,13}, {40,168,8}, {33,30,16}, {170,210,6} }, 4,
    { {{118,70},{161,70}}, {{110,151},{123,164}}, {{61,151},{61,167}}, {{96,151},{96,232},{169,232}} }, {2,2,2,3}, "TTY0" },
  { { {126,30,9}, {176,92,12}, {240,30,9}, {36,30,16}, {250,138,6} }, 4,
    { {{111,56},{125,56}}, {{111,100},{175,100}}, {{111,88},{261,88},{261,85}}, {{100,127},{100,170},{249,170}} },
    {2,2,3,3}, "TTY0" } },
};
void sim_termvar(int v)
{
    if (v < 0 || v > 3) return;
    const bool P = botcorps_cyd_orient != 0;
    const SimTV &T = SIM_TV[v][P ? 0 : 1];
    sim_ovl_clear();
    lgfx::LovyanGFX &d = botcorps_disp();
    Gfx g(d); g.clearClipRect();
    const int keep = s_cursor; s_cursor = -1;
    hb_layout_sync();                                   /* HB_TBOT etc. for this orientation */
    for (int i = 0; i < 5; ++i) {
        HB_CHIPR[i][0] = T.s[i][0]; HB_CHIPR[i][1] = T.s[i][1];
        HB_CHIPR[i][2] = mb_w(P, T.s[i][2]); HB_CHIPR[i][3] = mb_h(P, T.s[i][2]); HB_KS[i] = T.s[i][2];
    }
    HB_MOD = HB_CHIPR[HB4_TERM]; HB_KMID = HB_KS[HB4_TERM];
    s_hb_sel = -1; s_cb_col = accent();
    g.fillScreen((uint8_t)PAL_FIELD);
    e_board(g);
    {   /* decor with explicit keep-outs: every chip, every trace segment (+5 px), the plate */
        ERect K[40]; int nk = 0;
        for (int i = 0; i < 5; ++i) K[nk++] = { (int16_t)HB_CHIPR[i][0], (int16_t)HB_CHIPR[i][1], (int16_t)HB_CHIPR[i][2], (int16_t)HB_CHIPR[i][3] };
        for (int i = 0; i < T.nr; ++i) for (int q = 0; q + 1 < T.rn[i]; ++q) {
            const Pt a = T.r[i][q], c = T.r[i][q + 1];
            const int x0 = a.x < c.x ? a.x : c.x, y0 = a.y < c.y ? a.y : c.y, x1 = a.x > c.x ? a.x : c.x, y1 = a.y > c.y ? a.y : c.y;
            if (nk < 40) K[nk++] = { (int16_t)(x0 - 5), (int16_t)(y0 - 5), (int16_t)(x1 - x0 + 12), (int16_t)(y1 - y0 + 12) };
        }
        e_decor(g, K, nk, HB_TBOT[1] - 6);
    }
    for (int i = 0; i < T.nr; ++i) e_route2(g, T.r[i], T.rn[i]);
    static const int M[5] = { MB_BOOT, MB_SCAN, MB_CTRL, MB_TERM, MB_GIB };
    for (int i = 0; i < 5; ++i)
        drawBotChipK(g, HB_CHIPR[i][0], HB_CHIPR[i][1], P, M[i], HB_KS[i],
                     i == HB4_TERM ? T.tinfo : (i == HB4_SCAN && HB_KS[i] >= 12 ? "ALL RADIO" : nullptr), false);
    e_plate(g, HB_TBOT[0], HB_TBOT[1], HB_TBOT[2], HB_TBOT[3], HB_TBOT_SILK, false);
    e_finish(g);
    s_cursor = keep;
    hb_layout_sync();                                   /* restore firmware tables */
}

/* hub13 SIM-ONLY TERMINALbot device board (Mobandy 4:33-4:35 AM ET Oct 27). TERMINALbot = shared drawBotChipK
 * (MB_TERM, k12, no info, lit=false -> no border) in the TOP-LEFT corner; the 10 glossary device chips fan out
 * from him, each on its own e_route2 copper trace. No BOOT/SCAN/CONTROL/GIBSON. Firmware untouched. */
struct SimTDc { int x, y, type, n; Pt r[8]; };
struct SimTD { SimTDc c[10]; };
static const SimTD SIM_TD_P = { {
  { 110, 56, 0, 2, { {95,72}, {110,72} } },
  { 164, 34, 1, 2, { {95,44}, {164,44} } },
  { 164, 80, 2, 2, { {95,98}, {164,98} } },
  { 164, 126, 3, 4, { {95,104}, {102,104}, {138,140}, {164,140} } },
  { 164, 172, 4, 3, { {95,113}, {102,113}, {164,175} } },
  { 164, 218, 5, 4, { {92,119}, {92,160}, {162,230}, {164,230} } },
  { 100, 218, 6, 3, { {87,119}, {87,196}, {109,218} } },
  { 35, 218, 7, 3, { {82,119}, {82,234}, {72,234} } },
  { 35, 174, 8, 3, { {77,119}, {77,190}, {72,190} } },
  { 35, 130, 9, 2, { {53,119}, {53,130} } },
} };
static const SimTD SIM_TD_L = { {
  { 100, 30, 0, 2, { {89,46}, {100,46} } },
  { 148, 30, 1, 3, { {89,69}, {161,69}, {161,63} } },
  { 196, 30, 2, 3, { {89,75}, {209,75}, {209,63} } },
  { 244, 30, 3, 3, { {89,81}, {257,81}, {257,63} } },
  { 244, 88, 4, 2, { {89,95}, {244,95} } },
  { 244, 143, 5, 5, { {87,101}, {87,105}, {93,111}, {222,111}, {254,143} } },
  { 193, 143, 6, 5, { {81,101}, {81,111}, {87,117}, {179,117}, {205,143} } },
  { 142, 143, 7, 5, { {75,101}, {75,117}, {81,123}, {134,123}, {154,143} } },
  { 91, 143, 8, 5, { {69,101}, {69,123}, {75,129}, {89,129}, {103,143} } },
  { 40, 143, 9, 2, { {52,101}, {52,143} } },
} };
static const int TD_W = 38, TD_H = 34, TD_K = 12;
static void sim_dev_chip(Gfx g, int x, int y, int type)
{
    const uint8_t gold = 177, goldd = 104;
    cbf(g, x, y, TD_W, TD_H, cbp(0x18E3));
    for (int py = y + 6; py <= y + TD_H - 6; py += 5) {           /* gold edge pins, flush to the package */
        cbf(g, x, py, 2, 2, gold); cbf(g, x, py, 1, 2, goldd);
        cbf(g, x + TD_W - 2, py, 2, 2, gold); cbf(g, x + TD_W - 1, py, 1, 2, goldd);
    }
    const int sx = x + 2, sy = y + 2, sw = TD_W - 4, sh = TD_H - 4;  /* shaded silver silk window */
    for (int r = 0; r < sh; ++r) {
        int a = sw / 3 - r / 3, b = sw + sw / 3 - r / 3;
        if (a < 0) a = 0; if (b < 0) b = 0; if (b > sw) b = sw;
        cbf(g, sx, sy + r, a, 1, 178); cbf(g, sx + a, sy + r, b - a, 1, 110); cbf(g, sx + b, sy + r, sw - b, 1, 73);
    }
    cbf(g, sx, sy, sw, 1, 186); cbf(g, sx, sy, 1, sh, 186);
    const int D = 18, dx = x + (TD_W - D) / 2, dy = y + 4;         /* black die, steel rim, silver pad ticks */
    for (int k = 0; k < 4; ++k) {
        const int i = 3 + 4 * k;
        cbf(g, dx - 1, dy + i, 1, 1, PAL_SOFT); cbf(g, dx + D, dy + i, 1, 1, PAL_SOFT);
    }
    cbf(g, dx, dy, D, D, PAL_BLACK);
    cbf(g, dx, dy, D, 1, PAL_STEEL); cbf(g, dx, dy + D - 1, D, 1, PAL_STEEL);
    cbf(g, dx, dy, 1, D, PAL_STEEL); cbf(g, dx + D - 1, dy, 1, D, PAL_STEEL);
    gl_icon(g, type, dx + 3, dy + 3, false);
    const char *nm = GL_LABEL[type];
    const int tw = tiny_w(nm), ty = y + TD_H - 9;
    tiny_text(g, nm, x + (TD_W - tw) / 2, ty, sh_rgb(0x10, 0x12, 0x18));
    OVL_BOX("BODY", nm, sx, sy, sw, sh);
    OVL_BOX("LABEL", nm, x + (TD_W - tw) / 2, ty, tw, 5);
    OVL_BOX("CHIP", nm, x, y, TD_W, TD_H);
}
void sim_termdev(void)
{
    const bool P = botcorps_cyd_orient != 0;
    const SimTD &T = P ? SIM_TD_P : SIM_TD_L;
    const int tx = P ? 33 : 34, ty = 30;
    sim_ovl_clear();
    lgfx::LovyanGFX &d = botcorps_disp();
    Gfx g(d); g.clearClipRect();
    const int keep = s_cursor; s_cursor = -1;
    hb_layout_sync();
    s_hb_sel = -1; s_cb_col = accent();
    g.fillScreen((uint8_t)PAL_FIELD);
    e_board(g);
    {
        ERect K[96]; int nk = 0;
        K[nk++] = { (int16_t)(tx - 3), (int16_t)(ty - 3), (int16_t)(mb_w(P, TD_K) + 6), (int16_t)(mb_h(P, TD_K) + 6) };
        for (int i = 0; i < 10; ++i) {
            const SimTDc &c = T.c[i];
            K[nk++] = { (int16_t)(c.x - 3), (int16_t)(c.y - 3), (int16_t)(TD_W + 6), (int16_t)(TD_H + 6) };
            for (int q = 0; q + 1 < c.n && nk < 96; ++q) {
                const Pt a = c.r[q], b = c.r[q + 1];
                const int x0 = a.x < b.x ? a.x : b.x, y0 = a.y < b.y ? a.y : b.y, x1 = a.x > b.x ? a.x : b.x, y1 = a.y > b.y ? a.y : b.y;
                K[nk++] = { (int16_t)(x0 - 5), (int16_t)(y0 - 5), (int16_t)(x1 - x0 + 12), (int16_t)(y1 - y0 + 12) };
            }
        }
        e_decor(g, K, nk, HB_TBOT[1] - 6);
    }
    for (int i = 0; i < 10; ++i) e_route2(g, T.c[i].r, T.c[i].n);
    for (int i = 0; i < 10; ++i) sim_dev_chip(g, T.c[i].x, T.c[i].y, T.c[i].type);
    drawBotChipK(g, tx, ty, P, MB_TERM, TD_K, "", false);
    e_plate(g, HB_TBOT[0], HB_TBOT[1], HB_TBOT[2], HB_TBOT[3], HB_TBOT_SILK, false);
    e_finish(g);
    s_cursor = keep;
    hb_layout_sync();
}

/* hub13-v3 SIM-ONLY TERMINALbot board (Mobandy 12:20-12:23 PM ET Oct 27, portrait only).
 * TERMINALBOT = drawMainBotChip(MB_TERM) at the home GIBSONBOT seat (78,30) k16. The 10 device bots from the
 * LL19 reference are drawn by sim_botchip_named(): a line-for-line copy of drawBotChipK's frame / side+bottom
 * pins / power-bar header / shaded silk window / die + pad ticks / cb_face / label code, with only the name
 * (and face colour = the bot's BOT_PAL colour) supplied by the caller. Firmware untouched. */
static void sim_botchip_named(Gfx g, int mx, int my, bool P, int k16, const char *name, uint8_t col)
{
#define MS(v) mb_S((v), k16)
    const uint8_t gold = 177, goldd = 104, steel = PAL_STEEL;
    const int mw = mb_w(P, k16), mh = mb_h(P, k16);
    cbf(g, mx, my, mw, mh, cbp(0x18E3));
    const int pstep = MS(6) < 3 ? 3 : MS(6);
    for (int y = my + MS(22); y < my + mh - MS(4); y += pstep) {
        cbf(g, mx - MS(2), y, MS(4), MS(3), gold); cbf(g, mx - MS(2), y, 1, MS(3), goldd);
        cbf(g, mx + mw - MS(2), y, MS(4), MS(3), gold); cbf(g, mx + mw - MS(2) + MS(4) - 1, y, 1, MS(3), goldd);
    }
    for (int x = mx + MS(7); x < mx + mw - MS(6); x += pstep) cbf(g, x, my + mh - MS(2), MS(3), MS(4), gold);
    {
        const int leg = MS(3) < 2 ? 2 : MS(3), cap = leg + 1, per = cap + leg, hh = MS(11), hy = my + MS(3);
        const int span = mw - MS(8), n = (span - MS(4)) / per, used = n * per - (per - cap);
        int ax = mx + (mw - used) / 2;
        for (int i = 0; i < n; ++i) {
            cbf(g, ax, hy, 1, hh, gold); cbf(g, ax, hy, cap, 1, gold);
            cbf(g, ax + leg, hy, 1, hh, gold); cbf(g, ax + leg, hy + hh - 1, cap, 1, gold);
            ax += per;
        }
    }
    const int sx = mx + MS(3), sy = my + MS(18), sw = mw - 2 * MS(3), sh = mh - MS(22);
    for (int r = 0; r < sh; ++r) {
        int a = sw / 3 - r / 3, b = sw + sw / 3 - r / 3;
        if (a < 0) a = 0;
        if (b < 0) b = 0;
        if (b > sw) b = sw;
        cbf(g, sx, sy + r, a, 1, 178); cbf(g, sx + a, sy + r, b - a, 1, 110); cbf(g, sx + b, sy + r, sw - b, 1, 73);
    }
    cbf(g, sx, sy, sw, 1, 186); cbf(g, sx, sy, 1, sh, 186);
    /* label plan: mb_lab_plan's generic branch with the caller's name (Font0 6x8, else tiny 4x5), pad >= 3 */
    int pad = mb_pad(k16); if (pad < 3) pad = 3;
    const int gap = mb_gap(k16), avail = sw - 2 * pad, fit = avail > 2 ? avail - 2 : avail;
    const int silk_top = MS(18), silk_bot = mh - MS(4);
    const int dmin = k16 >= 12 ? 12 : (k16 >= 8 ? 8 : 6);
    /* one font for the whole set: the painter's tiny 4x5 fallback (the 7-letter names cannot take Font0 at k7) */
    int cw = 4, nh = 5; (void)fit; (void)dmin;
    int dcx, dcy, D, rty; mb_geo(P, &dcx, &dcy, &D, &rty, k16, false);
    int lty = silk_bot - pad - nh; if (lty < silk_top + gap) lty = silk_top + gap;
    { const int face_bot = dcy + D / 2, min_ty = face_bot + gap, max_ty = silk_bot - pad - nh;
      rty = lty; if (rty < min_ty) rty = min_ty; if (rty > max_ty) rty = max_ty; }
    const int cx = mx + dcx, cy = my + dcy;
    const int tk = MS(2);
    for (int k = 0; k < 5; ++k) {
        const int i = (D - 4 * (D / 6) - 1) / 2 + (D / 6) * k;
        cbf(g, cx - D / 2 - tk, cy - D / 2 + i, tk, 1, PAL_SOFT); cbf(g, cx + D / 2, cy - D / 2 + i, tk, 1, PAL_SOFT);
        cbf(g, cx - D / 2 + i, cy - D / 2 - tk, 1, tk, PAL_SOFT); cbf(g, cx - D / 2 + i, cy + D / 2, 1, tk, PAL_SOFT);
    }
    cbf(g, cx - D / 2, cy - D / 2, D, D, PAL_BLACK);
    cbf(g, cx - D / 2, cy - D / 2, D, 1, steel); cbf(g, cx - D / 2, cy + D / 2 - 1, D, 1, steel);
    cbf(g, cx - D / 2, cy - D / 2, 1, D, steel); cbf(g, cx + D / 2 - 1, cy - D / 2, 1, D, steel);
    const int bob = ((millis() / 900u) % 2u) ? 1 : 0;
    const int fs = D - MS(12), fy = cy + bob - 1;
    { const uint8_t keep = s_cb_col; s_cb_col = col; cb_face(g, cx, fy, fs); s_cb_col = keep; }
    const int ty = my + rty;
    const uint8_t ink = sh_rgb(0x10, 0x12, 0x18);
    int tw;
    if (cw >= 6) { const int keep = s_px; use_px(g, 6); txt(g, name, cx, ty, ink, lgfx::textdatum::top_center); s_px = keep; tw = (int)strlen(name) * 6; }
    else { tw = tiny_w(name); tiny_text(g, name, cx - tw / 2, ty, ink, avail); }
    OVL_BOX("BODY", name, sx, sy, sw, sh);
    OVL_BOX("LABEL", name, mx + pad, ty, mw - 2 * pad, nh);
    OVL_BOX("TEXT", name, cw >= 6 ? cx - tw / 2 : cx - tw / 2, ty, tw, nh);
    OVL_BOX("CHIP", name, mx, my, mw, mh);
#undef MS
}
struct SimV3 { const char *nm; int bot, x, y, n; Pt r[6]; };
static const int V3_K = 7;   /* 37 x 53: smallest k where a 7-letter tiny name keeps >= 3 px pad */
static const SimV3 SIM_V3[10] = {
    /* left column (reference left column order) */
    { "DEAUTH",  2, 34, 30,  2, { {72,56}, {77,56} } },
    { "HACK",    3, 34, 87,  2, { {72,113}, {77,113} } },
    { "FLOCK",   4, 34, 144, 4, { {72,170}, {84,170}, {90,164}, {90,152} } },
    { "DRONE",   5, 34, 201, 6, { {72,213}, {75,213}, {75,190}, {80,185}, {96,185}, {96,152} } },
    /* right column (reference right column order) */
    { "META",    8, 169, 30,  2, { {168,56}, {163,56} } },
    { "SKIMMER", 9, 169, 87,  2, { {168,113}, {163,113} } },
    { "FLIPPER",10, 169, 144, 4, { {168,170}, {156,170}, {150,164}, {150,152} } },
    { "SAMSUNG",11, 169, 201, 6, { {168,213}, {165,213}, {165,190}, {160,185}, {144,185}, {144,152} } },
    /* bottom centre, under TERMINALBOT (reference top-right pair) */
    { "AIR TAG", 6, 80, 201, 2, { {110,201}, {110,152} } },
    { "KEYFOB",  7, 123, 201, 2, { {130,201}, {130,152} } },
};
void sim_termv3(void)
{
    const bool P = botcorps_cyd_orient != 0;
    if (!P) return;                                     /* portrait only (Mobandy 12:22 PM ET) */
    sim_ovl_clear();
    lgfx::LovyanGFX &d = botcorps_disp();
    Gfx g(d); g.clearClipRect();
    const int keep = s_cursor; s_cursor = -1;
    hb_layout_sync();
    s_hb_sel = -1; s_cb_col = accent();
    g.fillScreen((uint8_t)PAL_FIELD);
    e_board(g);
    for (int i = 0; i < 10; ++i) e_route2(g, SIM_V3[i].r, SIM_V3[i].n);
    for (int i = 0; i < 10; ++i) sim_botchip_named(g, SIM_V3[i].x, SIM_V3[i].y, true, V3_K, SIM_V3[i].nm, bot_color(SIM_V3[i].bot));
    drawMainBotChip(g, 78, 30, true, MB_TERM, "");
    e_plate(g, HB_TBOT[0], HB_TBOT[1], HB_TBOT[2], HB_TBOT[3], HB_TBOT_SILK, false);
    e_finish(g);
    s_cursor = keep;
    hb_layout_sync();
}

/* hub13 entrance proof (sim only): dump the palette index -> RGB565 and the s_mb_inv mapping
 * mb_iv(i) (nearest palette entry to the bitwise-inverted RGB565) that BOOT is painted through. */
void sim_ivdump(const char *path)
{
    FILE *f = fopen(path, "w");
    if (!f) return;
    for (int i = 0; i < 256; ++i)
        fprintf(f, "%d %04x %d %04x\n", i, (unsigned)botcorps_rgb565((uint8_t)i), (int)mb_iv(i), (unsigned)botcorps_rgb565(mb_iv(i)));
    fclose(f);
}
