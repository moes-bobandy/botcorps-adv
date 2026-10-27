/* ============================================================================
 * CYD edge board (PICK.md idea A-fixed, out/cyd-bootbot-intro-mock): the shared painter for every CYD
 * screen, portrait 240x320 and landscape 320x240. The screen perimeter IS the dev-board's gold edge,
 * the top bar sits ON the board, full-length pin headers run down both sides (20 a side in portrait,
 * pitch 14 from y 32; landscape fits 14), and all text output lives in castellated, silk-labelled
 * plates wired into the copper. Ported from tools-bc/intro3_harness.cpp (e_frame / e_headers /
 * e_hub_plate), with the 240/320 constants generalised to the current orientation.
 * Included from botcorps_cyd_portrait.inc (BOTCORPS_CYD only).
 * ==========================================================================*/
struct Pt { int x, y; };
static const uint8_t GOLD = 177;
/* Metallic plated faces: gold (SCAN) / silver (TERMINAL). Shaded, never flat yellow.
 * hub12 SCAN gold: original base pal 177 PAL_GOLD_PIN (182,146,85). Ramp is built ONLY from static palette
 * slots whose sim pixel has hue 30..50 deg and R>=G>=B (no green / pink / gray):
 *   108 rim (82,69,24)  104 bronze (99,73,16)  140 lo (148,109,0)  177 base (181,146,82)
 *   213 bright (247,182,74)  249 sheen (247,211,74)  250 pale (255,219,173).
 * Light from top-left: diagonal bands (d = col + row), one diagonal sheen band, 2-px bevel rim. */
static void e_gold_face(Gfx g, int x, int y, int s)
{
    static const uint8_t RIM = 108, BRZ = 104, LO = 140, BASE = 177, BRT = 213, SHN = 249, PALE = 250;
    const int bw = s >= 12 ? 2 : 1;                    /* bevel width */
    const int sh0 = (s * 70) / 100, shw = s / 6 > 2 ? s / 6 : 2;   /* sheen band on d */
    for (int r = 0; r < s; ++r) {
        int i = 0;
        while (i < s) {
            const int d = i + r;
            uint8_t c; int run = 1;
            if (d < (s * 45) / 100) c = BRT;
            else if (d >= sh0 && d < sh0 + shw) c = (d == sh0 + shw / 2) ? PALE : SHN;
            else if (d >= (s * 165) / 100) c = BRZ;
            else if (d >= (s * 135) / 100) c = LO;
            else c = BASE;
            /* extend run while colour stays the same */
            while (i + run < s) {
                const int d2 = i + run + r; uint8_t c2;
                if (d2 < (s * 45) / 100) c2 = BRT;
                else if (d2 >= sh0 && d2 < sh0 + shw) c2 = (d2 == sh0 + shw / 2) ? PALE : SHN;
                else if (d2 >= (s * 165) / 100) c2 = BRZ;
                else if (d2 >= (s * 135) / 100) c2 = LO;
                else c2 = BASE;
                if (c2 != c) break; ++run;
            }
            cbf(g, x + i, y + r, run, 1, c);
            i += run;
        }
    }
    /* bevel: lit top/left (pale outer, bright inner), shaded bottom/right (rim outer, bronze inner) */
    for (int k = 0; k < bw; ++k) {
        const uint8_t lit = k == 0 ? PALE : BRT, dk = k == 0 ? RIM : BRZ;
        cbf(g, x + k, y + k, s - 2 * k, 1, lit);
        cbf(g, x + k, y + k, 1, s - 2 * k, lit);
        cbf(g, x + k, y + s - 1 - k, s - 2 * k, 1, dk);
        cbf(g, x + s - 1 - k, y + k, 1, s - 2 * k, dk);
    }
    /* specular glint (pale gold, never white) */
    const int gx = x + s / 5, gy = y + s / 5, gw = s / 4 > 2 ? s / 4 : 2;
    cbf(g, gx, gy, gw, 1, PALE);
    cbf(g, gx, gy, 1, gw / 2 > 1 ? gw / 2 : 1, PALE);
}
/* hub12 TERMINALbot silver (Mobandy 8:54 PM: stray pixel / broken > on the old face).
 * Face = shaded cool-silver fill + two eyes + ONE >_ prompt centred low. Ramp from static slots whose
 * sim pixel has B>=G>=R (no green / muddy gray): 41 rim (41,56,66) 110 lo (107,121,132)
 * 146 mid (140,154,165) 186 base (198,207,214) 223 hi (231,239,247) 255 glint.
 * e_term_marks() is the single source of the black marks (eyes + prompt): the painter fills them and
 * the sim face check builds its allowed-element mask from the same rects. */
static int e_term_marks(int x, int y, int s, bool blink, int R[][4])
{
    int n = 0;
    const int c = s / 5 > 2 ? s / 5 : 2, m = (s - 3 * c) / 2;          /* eyes (same as SCAN) */
    const int ey = y + c + (blink ? c - 1 : 0), eh = blink ? 1 : c;
    R[n][0] = x + m; R[n][1] = ey; R[n][2] = c; R[n][3] = eh; ++n;
    R[n][0] = x + m + 2 * c; R[n][1] = ey; R[n][2] = c; R[n][3] = eh; ++n;
    const int bw = s >= 12 ? 2 : 1, eye_bot = y + 2 * c;               /* first free row under the eyes */
    int u = s / 12 > 1 ? s / 12 : 1;
    while (u > 1 && y + s - bw - 1 - 5 * u < eye_bot + 1) --u;
    if (y + s - bw - 1 - 5 * u >= eye_bot + 1) {                      /* full chevron: 2u-wide stroke rows (edge-joined, no
                                                                          * diagonal-only dots) + 3-block cursor */
        const int gw = 8 * u, gx = x + (s - gw) / 2, gy = y + s - bw - 1 - 5 * u;
        static const int8_t OFS[5] = { 0, 1, 2, 1, 0 };
        for (int r = 0; r < 5; ++r) { R[n][0] = gx + OFS[r] * u; R[n][1] = gy + r * u; R[n][2] = 2 * u; R[n][3] = u; ++n; }
        R[n][0] = gx + 5 * u; R[n][1] = gy + 4 * u; R[n][2] = 3 * u; R[n][3] = u; ++n;
    } else {                                                           /* tiny faces: 3-row chevron + 2-px cursor */
        const int gx = x + (s - 5) / 2, gy0 = y + s - bw - 1 - 3, gy = gy0 > eye_bot ? gy0 : eye_bot;
        static const int8_t OFS[3] = { 0, 1, 0 };
        for (int r = 0; r < 3; ++r) { R[n][0] = gx + OFS[r]; R[n][1] = gy + r; R[n][2] = 1; R[n][3] = 1; ++n; }
        R[n][0] = gx + 3; R[n][1] = gy + 2; R[n][2] = 2; R[n][3] = 1; ++n;
    }
    return n;
}
static bool e_blink_now(void) { return (millis() / 2600u) % 7u == 0u && (millis() % 2600u) < 140u; }
static void e_silver_face(Gfx g, int x, int y, int s)
{
    static const uint8_t RIM = 41, LO = 110, MID = 146, BASE = 186, HI = 223, GL = 255;
    const int bw = s >= 12 ? 2 : 1;
    const int sh0 = (s * 70) / 100, shw = s / 6 > 2 ? s / 6 : 2;
    for (int r = 0; r < s; ++r) {
        int i = 0;
        while (i < s) {
            auto col = [&](int d) -> uint8_t {
                if (d < (s * 45) / 100) return HI;
                if (d >= sh0 && d < sh0 + shw) return HI;
                if (d >= (s * 165) / 100) return LO;
                if (d >= (s * 135) / 100) return MID;
                return BASE;
            };
            const uint8_t c = col(i + r); int run = 1;
            while (i + run < s && col(i + run + r) == c) ++run;
            cbf(g, x + i, y + r, run, 1, c);
            i += run;
        }
    }
    for (int k = 0; k < bw; ++k) {
        const uint8_t lit = k == 0 ? GL : HI, dk = k == 0 ? RIM : LO;
        cbf(g, x + k, y + k, s - 2 * k, 1, lit);
        cbf(g, x + k, y + k, 1, s - 2 * k, lit);
        cbf(g, x + k, y + s - 1 - k, s - 2 * k, 1, dk);
        cbf(g, x + s - 1 - k, y + k, 1, s - 2 * k, dk);
    }
    if (s >= 12) cbf(g, x + bw + 1, y + bw, s / 4, 1, GL);          /* specular glint on the lit bevel, clear of the eyes */
    int R[8][4]; const int n = e_term_marks(x, y, s, e_blink_now(), R);
    for (int q = 0; q < n; ++q) cbf(g, R[q][0], R[q][1], R[q][2], R[q][3], PAL_BLACK);
}
static void e_metal_face(Gfx g, int cx, int cy, int s, bool gold)
{
    const int x = cx - s / 2, y = cy - s / 2;
    if (!gold) { e_silver_face(g, x, y, s); return; }
    e_gold_face(g, x, y, s);
    /* eyes */
    const int c = s / 5 > 2 ? s / 5 : 2, m = (s - 3 * c) / 2;
    const bool bl = e_blink_now();
    cbf(g, x + m, y + c + (bl ? c - 1 : 0), c, bl ? 1 : c, PAL_BLACK);
    cbf(g, x + m + 2 * c, y + c + (bl ? c - 1 : 0), c, bl ? 1 : c, PAL_BLACK);
}

static int EW(void) { return botcorps_cyd_orient ? 240 : 320; }
static int EH(void) { return botcorps_cyd_orient ? 320 : 240; }
static uint8_t e_rule(void) { return cbmix(0x0841, botcorps_rgb565(accent()), 40); }
static int ri(float v) { return (int)(v + 0.5f); }

/* edge-to-edge: the perimeter is the board's gold edge; inner rule 2 px in; mounting holes at the bottom corners */
static void e_frame(Gfx g)
{
    const int W = EW(), H = EH();
    const uint8_t rule = e_rule();
    g.drawRect(0, 0, W, H, GOLD);
    cbf(g, 2, 1, 1, H - 3, rule); cbf(g, W - 3, 1, 1, H - 3, rule); cbf(g, 2, H - 3, W - 4, 1, rule);
    cbf(g, 2, 26, 1, 1, rule);
    cb_ring(g, 8, H - 9); cb_ring(g, W - 9, H - 9);
}

/* full-length headers (ESP32 DevKitC 38-pin style), pitch 14 from y 32: 20 a side in portrait, 14 in landscape */
static const int EPY0 = 32, EPP = 14;
static const char *const E_LP[20] = { "3V", "GND", "D23", "D22", "TX0", "RX0", "D21", "D19", "D18", "D5", "D17", "D16",
                                      "D4", "D0", "D2", "D15", "SD1", "SD0", "CLK", "GND" };
static const char *const E_RP[20] = { "EN", "VP", "VN", "D34", "D35", "D32", "D33", "D25", "D26", "D27", "D14", "D12",
                                      "GND", "D13", "SD2", "SD3", "CMD", "3V3", "GND", "5V" };
static int e_pn(void) { const int n = (EH() - 54) / EPP + 1; return n > 20 ? 20 : n; }
static int e_py(int k) { return EPY0 + k * EPP; }
static int e_kind(const char *n) { if (!strcmp(n, "GND")) return 1; if (n[0] == '3' || n[0] == '5') return 0; return 2; }
static void e_headers(Gfx g, const bool *litL, const bool *litR, const bool *tipL, const bool *tipR)
{
    const int W = EW(), N = e_pn();
    const uint8_t silkc = cb_silk();
    cbf(g, 4, EPY0 - 2, 11, (N - 1) * EPP + 11, PAL_BLACK);
    cbf(g, W - 15, EPY0 - 2, 11, (N - 1) * EPP + 11, PAL_BLACK);
    for (int k = 0; k < N; ++k) {
        const int y = e_py(k);
        cb_pad(g, 6, y, e_kind(E_LP[k]), litL[k], tipL[k]);
        cb_pad(g, W - 13, y, e_kind(E_RP[k]), litR[k], tipR[k]);
        const int lw = tiny_w(E_LP[k]), rw = tiny_w(E_RP[k]);
        cbf(g, 15, y, lw + 3, 7, PAL_BLACK); tiny_text(g, E_LP[k], 17, y + 1, litL[k] ? (uint8_t)PAL_WHITE : silkc);
        cbf(g, W - 18 - rw - 1, y, rw + 3, 7, PAL_BLACK); tiny_text(g, E_RP[k], W - 17 - rw, y + 1, litR[k] ? (uint8_t)PAL_WHITE : silkc);
    }
}
/* powered-board semantics (hub): right D32 D33 D25 D14 D12 lit + 400 ms white tip */
static void e_headers_hub(Gfx g)
{
    bool L[20] = {}, R[20] = {}, TL[20] = {}, TR[20] = {};
    const int tip = (int)((millis() / 400u) % 12u);
    for (int k = 0; k < 20; ++k) { const bool lr = (k == 5 || k == 6 || k == 7 || k == 10 || k == 11); R[k] = lr; TR[k] = lr && k == tip; }
    e_headers(g, L, R, TL, TR);
}

/* the shared top bar: Gibson status chip, 12 bot swatches, crown (= BACK) at W-94, link + counters.
 * No terminal glyph: there is no standalone Terminal screen on the CYD (the hub plate is the terminal). */
static int e_crown_x(void) { return EW() - 94; }
static void e_bar(Gfx g)
{
    const int W = EW();
    const uint8_t ac = accent();
    g.fillRect(0, 0, W, 26, PAL_BLACK);
    chip_mini(g, 14, 12, 14, 12, false, 0);
    static const uint8_t DIM[12] = { 9, 9, 9, 12, 76, 100, 33, 13, 76, 13, 100, 69 };
    for (int i = 0; i < 12; ++i) g.fillRect(30 + i * 8, 9, 5, 5, DIM[i]);
    gib_crown(g, e_crown_x(), 3);
    g.drawCircle(W - 40, 12, 3, ac);
    g.fillRect(W - 41, 11, 3, 3, ac);
    txt(g, "12", W - 34, 6, PAL_WHITE, lgfx::textdatum::top_left);
    txt(g, "0", W - 12, 9, ac, lgfx::textdatum::top_left);
    g.fillRect(0, 24, W, 1, sb_tone(3));
}
static void e_silk(Gfx g, const char *t, int x, int y) { cbf(g, x - 1, y - 1, tiny_w(t) + 2, 7, PAL_BLACK); tiny_text(g, t, x, y, cb_silk()); }

/* board base for a screen: headers + bottom silk; e_finish() draws the top bar and the edge last */
static void e_board(Gfx g)
{
    s_cb_col = accent();
    const int W = EW(), H = EH();
    /* header strips are solid board (no rain between the pin labels) */
    const uint8_t bg = (uint8_t)PAL_FIELD;
    g.fillRect(0, 26, 33, H - 26, bg);
    g.fillRect(W - 33, 26, 33, H - 26, bg);
    g.fillRect(0, H - 4, W, 4, bg);
    e_headers_hub(g);
}
#include "ui/botcorps_cyd_adv_plate.inc"

/* hub12: home-P board decor (same pour hatch / via grid / SMD / test pads / net silk density as
 * hb_rich) for every non-TERM edge board. No pixel readback on the panel, so each painter passes its
 * parts as explicit keep-out rects; decor stays EK px clear of every one. Paint before copper + parts. */
struct ERect { int16_t x, y, w, h; };
static bool s_e_decor_off = false;   /* sim A/B toggle (decor-clearance check) */
static bool e_kept(const ERect *K, int nk, int x, int y, int w, int h)
{
    enum { EK = 3 };
    for (int i = 0; i < nk; ++i)
        if (x < K[i].x + K[i].w + EK && x + w > K[i].x - EK && y < K[i].y + K[i].h + EK && y + h > K[i].y - EK) return true;
    return false;
}
static void e_decor(Gfx g, const ERect *K, int nk, int y1)
{
    if (s_e_decor_off) return;
    const int W = EW(), x0 = 30, x1 = W - 30, y0 = 34;
    const uint8_t tr = cb_trace();
    const uint8_t via = cbmix(botcorps_rgb565(tr), 0xFFFF, 40);
    const uint8_t pour = cbmix(botcorps_rgb565(tr), 0x0000, 55);
    const uint8_t silk = cb_silk();
    const uint8_t smd = sh_rgb(0x2a, 0x2e, 0x36);
    const uint8_t pad = sh_rgb(0xb8, 0x9a, 0x3c);
    for (int y = y0 + 4; y < y1 - 2; y += 14)          /* ground-pour hatch 18 x 14 */
        for (int x = x0 + 4; x + 5 <= x1; x += 18) {
            if (e_kept(K, nk, x, y - 2, 5, 5)) continue;
            cbf(g, x, y, 5, 1, pour); cbf(g, x + 2, y - 2, 1, 5, pour);
        }
    for (int y = y0 + 10; y < y1 - 1; y += 22)         /* via grid 26 x 22 */
        for (int x = x0 + 10; x < x1 - 1; x += 26) {
            if (e_kept(K, nk, x - 1, y - 1, 3, 3)) continue;
            cb_via(g, x, y, via);
        }
    /* SMD parts + test pads on a staggered 52 x 44 pocket grid (between the hatch/via lattices) */
    int n = 0;
    for (int y = y0 + 17; y < y1; y += 44)
        for (int x = x0 + 19 + ((y / 44) & 1) * 26; x < x1; x += 52, ++n) {
            const int kind = n % 3, w = kind == 0 ? 10 : kind == 1 ? 4 : 5, h = kind == 0 ? 4 : kind == 1 ? 8 : 5;
            if (x + w > x1 || y + h > y1 || e_kept(K, nk, x, y, w, h)) continue;
            if (kind == 2) { cbf(g, x, y, 5, 5, pad); cbf(g, x + 1, y + 1, 3, 3, smd); continue; }
            cbf(g, x, y, w, h, smd);
            if (kind == 0) { cbf(g, x, y, 2, h, pad); cbf(g, x + w - 2, y, 2, h, pad); }
            else { cbf(g, x, y, w, 2, pad); cbf(g, x, y + h - 2, w, 2, pad); }
        }
    /* net silk: portrait tiny, landscape Font0 (as hb_rich) */
    if (botcorps_cyd_orient) {
        if (!e_kept(K, nk, x0 + 2, y0 + 2, tiny_w("3V3"), 5)) tiny_text(g, "3V3", x0 + 2, y0 + 2, silk);
        if (!e_kept(K, nk, x1 - tiny_w("GND") - 2, y0 + 2, tiny_w("GND"), 5)) tiny_text(g, "GND", x1 - tiny_w("GND") - 2, y0 + 2, silk);
        if (!e_kept(K, nk, x0 + 2, y1 - 10, tiny_w("NET-H"), 5)) tiny_text(g, "NET-H", x0 + 2, y1 - 10, silk);
    } else {
        const int keep = s_px; use_px(g, 6);
        if (!e_kept(K, nk, x0 + 2, y0 + 2, 18, 8)) txt(g, "3V3", x0 + 2, y0 + 2, silk, lgfx::textdatum::top_left);
        if (!e_kept(K, nk, x1 - 20, y0 + 2, 18, 8)) txt(g, "GND", x1 - 2, y0 + 2, silk, lgfx::textdatum::top_right);
        s_px = keep;
    }
}
/* 2 px copper route with vias at the bends (home-P hb_copper language) */
static void e_route2(Gfx g, const Pt *p, int n)
{
    const uint8_t tr = cb_trace(), via = cbmix(botcorps_rgb565(tr), 0xFFFF, 40);
    for (int q = 0; q + 1 < n; ++q) {
        cb_line(g, p[q].x, p[q].y, p[q + 1].x, p[q + 1].y, tr);
        if (p[q].y == p[q + 1].y) cb_line(g, p[q].x, p[q].y + 1, p[q + 1].x, p[q + 1].y + 1, tr);
        else cb_line(g, p[q].x + 1, p[q].y, p[q + 1].x + 1, p[q + 1].y, tr);
    }
    for (int q = 1; q + 1 < n; ++q) cb_via(g, p[q].x, p[q].y, via);
}

static void e_finish(Gfx g)
{
    e_bar(g);
    e_frame(g);
    /* Adv OSK opens upward from the plate cmdline; paint after the frame so keys sit on top. */
    cyd_adv_paint_osk_overlay(g);
    if (s_sheet && s_pt_bw > 0) cyd_adv_paint_sheet(g);
}

/* 2 px power copper + animated head (intro) */
static int seg_len(Pt a, Pt b) { const int dx = abs(a.x - b.x), dy = abs(a.y - b.y); return dx > dy ? dx : dy; }
static void line2(Gfx g, Pt a, Pt b, uint8_t c)
{
    cb_line(g, a.x, a.y, b.x, b.y, c);
    if (a.y == b.y) cb_line(g, a.x, a.y + 1, b.x, b.y + 1, c);
    else cb_line(g, a.x + 1, a.y, b.x + 1, b.y, c);
}
static void path2(Gfx g, const Pt *p, int n, float prog, uint8_t base, uint8_t lit, bool head)
{
    int L = 0; for (int q = 0; q + 1 < n; ++q) L += seg_len(p[q], p[q + 1]);
    for (int q = 0; q + 1 < n; ++q) line2(g, p[q], p[q + 1], base);
    if (prog <= 0) return;
    int rem = ri(prog * L); Pt h = p[0];
    for (int q = 0; q + 1 < n && rem > 0; ++q) {
        const int sl = seg_len(p[q], p[q + 1]), k = rem < sl ? rem : sl;
        const Pt e = { p[q].x + (p[q + 1].x > p[q].x ? k : p[q + 1].x < p[q].x ? -k : 0), p[q].y + (p[q + 1].y > p[q].y ? k : p[q + 1].y < p[q].y ? -k : 0) };
        line2(g, p[q], e, lit); h = e; rem -= k;
    }
    if (head && prog < 1) cbf(g, h.x - (h.x >= EW() / 2 ? 2 : 1), h.y - 1, 4, 4, PAL_WHITE);
}
/* 1 px signal trace with vias at the bends (plate feeds on non-hub screens) */
static void e_trace(Gfx g, const Pt *p, int n)
{
    const uint8_t trace = cb_trace(), via = cbmix(botcorps_rgb565(trace), 0xFFFF, 35);
    for (int q = 0; q + 1 < n; ++q) cb_line(g, p[q].x, p[q].y, p[q + 1].x, p[q + 1].y, trace);
    for (int q = 1; q + 1 < n; ++q) cb_via(g, p[q].x, p[q].y, via);
}

/* ---- plates: the terminal language of the board (hb_terminalbot geometry per PICK.md §9) ---- */
/* shell only: substrate, gold edge + rule, castellations x = bx+3+12k top and bottom, silk legend, pads, black screen.
 * Returns the screen rect. Pads: RX TX IO0 EN at sx+52+26k when the plate is wide enough, else RX TX at the right. */
static void e_plate_shell(Gfx g, int bx, int by, int bw, int bh, const char *silk, bool pads_on,
                          int *osx, int *osy, int *osw, int *osh, int pad = 6)
{
    const uint8_t ac = accent(), silkc = cb_silk();
    cbf(g, bx, by, bw, bh, cbp(0x18E3));
    g.drawRect(bx, by, bw, bh, GOLD);
    g.drawRect(bx + 1, by + 1, bw - 2, bh - 2, e_rule());
    for (int x = bx + 3; x < bx + bw - 6; x += 12) { cbf(g, x, by - 2, 3, 3, GOLD); cbf(g, x, by + bh - 1, 3, 3, GOLD); }
    const int sx = bx + pad, sw = bw - 2 * pad, sy = by + 10, sh = bh - 13;
    tiny_text(g, silk, sx + 1, by + 3, cbmix(botcorps_rgb565(ac), 0xFFFF, 35));
    const int tip = (int)((millis() / 400u) % 4u);
    if (sw >= 52 + 26 * 3 + 20 && tiny_w(silk) < 48) {
        static const char *const TP[4] = { "RX", "TX", "IO0", "EN" };
        for (int k = 0; k < 4; ++k) { const int px = sx + 52 + k * 26; cb_pad(g, px, by + 2, 2, pads_on, pads_on && k == tip); tiny_text(g, TP[k], px + 9, by + 3, silkc); }
    } else {
        static const char *const TP[2] = { "RX", "TX" };
        for (int q = 0; q < 2; ++q) { const int px = sx + sw - 38 + q * 20; cb_pad(g, px, by + 2, 2, pads_on, pads_on && q == (tip & 1)); tiny_text(g, TP[q], px + 9, by + 3, silkc); }
    }
    g.fillRect(sx, sy, sw, sh, PAL_BLACK);
    g.drawRect(sx, sy, sw, sh, PAL_LINE);
    *osx = sx; *osy = sy; *osw = sw; *osh = sh;
}
/* one log line, cut to the plate width (no clip rect: the intro wipe owns the clip) */
static void e_log_line(Gfx g, const char *ln, int lx, int ly, int w)
{
    if (!ln || !ln[0]) return;
    const int maxc = w / 6;
    char b[64];
    int n = 0;
    if (ln[0] == '\x01' && ln[1]) { b[0] = ln[0]; b[1] = ln[1]; n = 2; }
    const int lim = n + maxc < (int)sizeof b - 1 ? n + maxc : (int)sizeof b - 1;
    for (int i = n; ln[i] && i < lim; ++i) b[n++] = ln[i];   /* n tracks i */
    b[n] = 0;
    pt_draw_log_line(g, b, lx, ly);
}
/* ---- Adv f1c08aa terminal plate (replaces b98559e old0/old12/abc keyboard) ---- */
static bool pt_overlay_rect(int *x, int *y, int *w, int *h)
{
    /* rain cover while Adv OSK / sheet / palette / attack is open */
    if (cyd_cl_cover(x, y, w, h)) return true;   /* pt346u command list */
    if (s_sheet && s_pt_bw > 0) {
        int by, hh, top, nshow;
        cyd_adv_sheet_box(&by, &hh, &top, &nshow);
        *x = s_adv_sx + 2; *y = by; *w = s_adv_sw - 4; *h = hh;
        return true;
    }
    if (s_term_ovl == 2 && s_pt_bw > 0) {   /* attack prompts open upward above the 3-line plate */
        cyd_adv_atk_box(x, y, w, h);
        return true;
    }
    if ((s_focus_cmds || s_term_ovl) && s_pt_bw > 0) {
        *x = s_adv_sx; *y = s_adv_sy; *w = s_adv_sw; *h = s_adv_sh;
        return true;
    }
    return false;
}
/* live terminal plate: silk shell + Adv prompt plate in the screen rect.
 * dark: shell + black screen only (intro staging). */
static void e_plate(Gfx g, int bx, int by, int bw, int bh, const char *silk, bool sel, bool dark = false)
{
    int sx, sy, sw, sh;
    OVL_PLATE_BEGIN();
    e_plate_shell(g, bx, by, bw, bh, silk, true, &sx, &sy, &sw, &sh, 2);
    if (sel) g.drawRect(bx - 2, by - 2, bw + 4, bh + 4, PAL_WHITE);
    if (!dark) cyd_adv_paint_plate(g, sx, sy, sw, sh);
    OVL_PLATE_END();
    OVL_BOX("PLATE", silk, bx, by - 2, bw, bh + 2);   /* incl. top castellations */
}
/* Owner 8:16 AM ET Oct 26: every CYD terminal plate is 3 lines (2 log lines + cmdline):
 * outer 49 -> screen sh 36 -> log rows at sy+4, sy+12, cmdline at sy+25. */
static int e_plate_h(void) { return 49; }

/* draw in a shifted frame: everything painted through .h lands (dx, dy) away (Gfx fill ops use the
 * global band x origin + the member y origin, raw blits use .ox/.oy) */
struct EShift {
    int sox;
    Gfx h;
    EShift(Gfx g, int dx, int dy) : sox(botcorps_band_ox()), h(g)
    {
        botcorps_band_set_ox(sox - dx);
        h.oy -= dy;
        h.ox -= dx;
    }
    ~EShift() { botcorps_band_set_ox(sox); }
};

/* CONTROLbot on the edge board: the scattered parts (ctlbot_layout per orientation) over the shared
 * board, headers and top bar; no own frame, no own pin rows.
 * Owner pt358u: same 3-line Adv plate as hub/scan (log/prompt/ret/CORE/≡ + chip grid + CORE sheet).
 * Chips stay put (only SCANbot is re-spaced); plate may cover lower parts — honest overlap. */
static void paint_ctlbot(Gfx g)
{
    const bool P = botcorps_cyd_orient != 0;
    e_board(g);
    const uint8_t trace = cb_trace(), via = cbmix(botcorps_rgb565(trace), 0xFFFF, 35);
#ifdef BOTCORPS_CYD
    {   /* hub12 board match: home-P decor density; keep-outs = every part + its silk labels, strip, meters, plate */
        ERect K[16]; int nk = 0;
        const CbR *RC = P ? CB_P_RECT : CB_L_RECT;
        for (int i = 0; i < 9; ++i) K[nk++] = { (int16_t)(RC[i].x - 14), (int16_t)(RC[i].y - 12), (int16_t)(RC[i].w + 28), (int16_t)(RC[i].h + (P ? 30 : 26)) };
        K[nk++] = { 28, 24, (int16_t)(EW() - 56), 14 };                       /* power strip */
        if (P) K[nk++] = { 34, 34, 80, 50 };                                   /* meters */
        if (P) K[nk++] = { 29, 257, 181, 56 }; else K[nk++] = { 32, 180, 256, 56 };
        e_decor(g, K, nk, P ? 255 : 178);
    }
#endif
    cb_copper(g, trace, via);
    cb_hilite(g);
    const int *M = P ? CB_P_MOD : CB_L_MOD;
    cb_module(g, M[0], M[1], M[2], M[3]);
    cb_meters(g, P ? CB_P_MET : CB_L_MET);
    cb_orient(g);
    /* decorative DC/USB/C1/Rs/BOOT/EN/serial removed (pt359u ctrltheme); theme = centre module */
    static const int CH[4] = { 1, 2, 5, 7 };
    for (int k = 0; k < 4; ++k) cb_chip(g, CH[k], s_cursor == CH[k]);
    cb_part(g, 3); cb_part(g, 4); cb_part(g, 6);
    cb_power_strip(g);   /* hub6: header now lives in the stretched strip (bottom silk sat in the plate keep-out) */   /* hub6: stretched top power bars (replaces the L top silk) */
    if (P) e_plate(g, 31, 261, 177, 49, "CONTROLBOT", false);
    else e_plate(g, 34, 184, 252, 49, "CONTROLBOT", false);
    e_finish(g);
}
/* the whole Control board is copper: rain is painted once under it and never stepped over the parts */
static void layout_covers_ctlbot(void)
{
    cover_add(0, 0, EW(), EH());
}
