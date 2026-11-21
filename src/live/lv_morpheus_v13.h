#pragma once
/* MOCK 2026-11-13 v13 - MORPHEUSbot "DOJO CONSTRUCT" (built from v12; v12 / v11 kept on disk, not included).
 * Mobandy 2026-11-13: v12 rejected ("close-up pillar looks wrong", "folders don't look like the download game", "make the damn definition
 * better", "the towers look like shit ... the idle room towers too, redo them", no lock bracket / select effect, red pill removed).
 * - Engine motion unchanged: lv_rd_layout / lv_rd_place / mo11_drift tables, camera 0.5 sin(0.45 t), room, rain, chrome.
 * - NEW SKIN (mo11_tower_fx, every pillar, idle + close-up): opaque lacquer monolith, lit top + perspective side face from the
 *   lv_rd_place back corners, crisp 1 px parchment / timber / green edges, recessed icon plate with an integer-scaled (8/16/32/48 px)
 *   nearest-neighbour device icon, shoji window cassettes (lit paper / dark / Matrix-green panes) on the front + perspective ribs on the side.
 * - TAP: no pill, flood or brackets; straight into the push-in (400 ms). Search bar on top of the tower; 3 DOWNLOAD-structured Dojo
 *   folders (SCROLL n tab, icon + big file name, big size, 70x7 bar + action line; DROP0 300, GAP 110, DROP 380 ms). EX.DATA only. */
#if LV_MO_INSIDE
/* MOCK 2026-11-14 morpheus-mix spread (on v13.1 + morpheus-towers-3 looks): 18 device pillars (WIFI + BLUETOOTH removed), look per
 * device by threat level (HIGH -> T1 code glass, MEDIUM -> T3 sentinel lantern, LOW -> T2 construct white light), ONE fixed height per
 * style (MO_HFIX: only three heights), spread placement, crisp vector emblems (src/mo_emblem.h). Mock only, EX.DATA. */
#include "botpheus/mo_emblem.h"
#define MO_N 18
struct MoDev { const char *name, *what, *spot, *prot; };
static const MoDev MO_DEV[MO_N] = {   /* 2026-11-14: WIFI + BLUETOOTH removed */
    {"DEAUTH", "Fake messages that kick you off Wi-Fi.", "Passive watch for deauth and disassoc floods.", "Use WPA3 or protected frames (802.11w)."},
    {"HACK", "Weak Wi-Fi anyone can read.", "Access points that are OPEN or use WEP.", "Stay off them. Lock yours with WPA2/WPA3."},
    {"FLOCK", "Flock cameras that read license plates.", "Flock Wi-Fi names, Penguin/FS Ext Battery.", "Know where they are. Ask how long data is kept."},
    {"DRONE", "DJI, Tello, Skydio and Anafi drones.", "Drone Wi-Fi names, Bluetooth and Remote ID.", "Note time and place. Report misuse to police."},
    {"AIR TAG", "Apple AirTag, a coin-size Find My tracker.", "Find My adverts (Apple 0x004C, type 0x12).", "Check bags and car. Heed phone tag alerts."},
    {"KEYFOB", "Keyring trackers like Tile.", "Tile Bluetooth IDs FEED and FEEC.", "Scan for tags you do not own."},
    {"META", "Meta smart glasses with a camera.", "Bluetooth company ID 0x01AB or 0x058E.", "Watch for the capture light. Ask before filming."},
    {"SKIMMER", "Hidden card readers on pumps and ATMs.", "Serial modules HC-05/06/08 and RNBT.", "Tap to pay. Wiggle the reader. Cover the keypad."},
    {"FLIPPER", "Flipper Zero, a pocket radio and NFC tool.", "Bluetooth advert names that say Flipper.", "Update your phone. Dismiss surprise pop-ups."},
    {"SAMSUNG", "Samsung Galaxy SmartTag trackers.", "SmartTag Bluetooth adverts (FD5A/FD59).", "Run an unknown tag scan in SmartThings."},
    /* morpheus-mix: the 8 SquachWatch README devices */
    {"AXON", "Axon police body cams and Taser gear.", "Axon maker MAC prefix and AB3 Wi-Fi names.", "Know your rights. Ask if you are filmed."},
    {"RAVEN", "Raven gunshot and sound sensors on poles.", "Raven BLE service UUIDs 0x3100 to 0x3500.", "Know where they are. Ask what is kept."},
    {"ALPR", "Plate reader cams (Motorola, Genetec).", "Camera maker MAC prefixes on Wi-Fi.", "Ask your city who gets the plate data."},
    {"CAMERA", "Wi-Fi IP cams (Wyze, Verkada, Tuya).", "Camera maker MAC prefixes on Wi-Fi.", "Check rentals for cams. Cover lenses."},
    {"RING", "Amazon Ring doorbells and cameras.", "Ring and Amazon maker MAC prefixes.", "Say no to video share asks you do not want."},
    {"GOOGLE TAG", "Google Find My Device trackers.", "FMDN adverts (FEAA), Chipolo, Pebblebee.", "Run your phone's unknown tracker scan."},
    {"EVIL TWIN", "A fake hotspot copying a real Wi-Fi name.", "Same name from two APs, lock type differs.", "Check the lock. Forget open copies."},
    {"IBEACON", "Apple iBeacon adverts used by shops.", "BLE adverts 4C 00 02 15 + UUID major minor.", "Turn off Bluetooth or beacon scans."},
};
/* the locked description page's icon per device (docs/mocks/morpheus-glossary/README.md "Icons"); -1 = DEV_DEAUTH 12x12 below */
static const int MO_ICON[MO_N] = {-1, LV_IC_LAPTOP, LV_IC_CCTV, LV_IC_DRONE, LV_IC_AIRTAG, LV_IC_KEYFOB, LV_IC_GLASSES, LV_IC_SKIMMER, LV_IC_FLIPPER, LV_IC_GALAXYTAG, -1, -1, -1, -1, -1, -1, -1, -1};   /* old art (v13.1 only); mix pillars use mo_emblem.h */
static const char *const MO_DEAUTH[12] = { "...aaaaaa...", ".aa......aa.", "a..aaaaaa..a", "..aa....aa..", "...a.aa.a...", ".....aa.....",
    ".....aa.....", "........c..c", ".........cc.", ".........cc.", "........c..c", "............" };   /* DEV_ROW[DEV_DEAUTH], src/ui/botcorps_ui.cpp */
/* kinds: 0 NETWORKS, 1 CAMERAS, 2 TRACKERS, 3 BLE TOOLS (text only: v10 has no colour-coded categories) */
static const int MO_KIND[MO_N] = {0, 0, 1, 1, 2, 2, 1, 3, 3, 2, 1, 1, 1, 1, 1, 2, 0, 3};
/* threat (0 HIGH, 1 MEDIUM, 2 LOW) -> look (1 T1 code glass, 3 T3 sentinel lantern, 2 T2 construct white light); provisional split */
static const int MO_THREAT[MO_N] = {0, 0, 2, 2, 1, 1, 1, 0, 0, 1, 1, 2, 2, 1, 2, 1, 0, 2};
static const char *const MO_TNAME[3] = {"HIGH", "MEDIUM", "LOW"};
static const int MO_TLOOK[3] = {1, 3, 2};
static const int MO_LOOKOF[MO_N] = {1, 1, 2, 2, 3, 3, 3, 1, 1, 3, 3, 2, 2, 3, 2, 3, 1, 2};
/* ONE fixed world height per style, by look (1 T1 code glass tallest, 3 T3 lantern middle, 2 T2 white light shortest): only three heights.
 * Replaces the engine's rssi height and the idle breathing for the pillars (mo11_drift). */
static const float MO_HFIX[4] = {1.0f, 2.600f, 1.612f, 2.080f};
#define MO_G 0x00b843    /* Matrix green (rain body, brackets, flood) */
#define MO_GH 0x10d858   /* Matrix green highlight */
#define MO_INK 0x2a323b  /* graphite */
static const char *const MO_KNAME[4] = {"NETWORKS", "CAMERAS", "TRACKERS", "BLE TOOLS"};
static const char *const MO_SEC[3] = {"WHAT IT IS", "HOW IT IS SPOTTED", "HOW TO PROTECT YOURSELF"};
/* ---- v11 Dojo palette (parchment / timber / lacquer / Matrix green; no category colours) ---- */
#define MO11_MAT_A 0xe8dec8   /* tatami / parchment mat, light */
#define MO11_MAT_B 0xdfd3b8   /* tatami mat, the alternate weave */
#define MO11_SEAM  0x5c4a32   /* timber seam (grid rows + lanes) */
#define MO11_BEAM  0x3b2e1e   /* dark timber: posts, beams, horizon rail, pillar edges */
#define MO11_LAQ   0x2a2119   /* dark lacquer pillar face */
#define MO11_PAPER 0xf6f0e2   /* backlit shoji paper */
#define MO11_CHAR  0x1e1a16   /* charcoal chrome */
#define MO11_INK   0x2a2118   /* text ink on parchment */
#define MO11_G     0x00a83c   /* Matrix green (rain body, brackets, flood, accents) */
#define MO11_GH    0x10d858   /* Matrix green highlight */
/* ---- world: 12 pillars, each its OWN independent 3D drift (X, Z, breathing H) on top of a scattered base layout ---- */
/* base (picked offline for touch: tools opt2.py, segment-Voronoi tap cells over 30 drifting samples): screen column at camera 0, depth Z (12 steps 1.65 .. 4.35) and example rssi (12 distinct, -42 .. -92: Gibson's rssi -> level -> H law) */
static const float MO11_SX[MO_N] = {173.939f, 32.000f, 97.950f, 72.277f, 57.763f, 197.367f, 112.303f, 214.246f, 127.717f, 121.640f, 165.545f, 175.832f, 27.175f, 123.927f, 127.046f, 74.091f, 33.057f, 206.000f};
static const float MO11_Z0[MO_N] = {3.5786f, 1.7143f, 2.5786f, 2.5500f, 4.1214f, 3.5429f, 2.9071f, 2.1714f, 1.9429f, 4.5714f, 2.0571f, 3.4786f, 3.9500f, 1.8286f, 4.0429f, 2.1857f, 3.0214f, 3.6143f};
static const int8_t MO11_RSSI[MO_N] = {-79, -42, -58, -54, -85, -74, -62, -50, -46, -92, -48, -72, -82, -44, -88, -52, -64, -77};   /* EX.DATA */
/* v1.2.10 (Mobandy 2026-11-19: the Dojo only ever showed EX.DATA): the live scan behind each pillar. Each pillar whose device
 * the firmware can already detect uses that hunt's own matcher (lv_hunt_match, the same rule the HUNT bots use) over the
 * real finds in lv_f; DEAUTH uses the deauth watch count. The other 8 pillars have no detector in this firmware yet and keep
 * their EX.DATA labels. Refreshed once per Dojo frame, before the render swaps the example world in. Pillar layout,
 * heights and motion are unchanged; only the numbers and labels for a pillar that is really seen switch to LIVE. */
static const int MO_HUNT[MO_N] = {LV_H_DEAUTH, LV_H_HACK, LV_H_FLOCK, LV_H_DRONE, LV_H_AIRTAG, LV_H_KEYFOB, LV_H_META, LV_H_SKIMMER, LV_H_FLIPPER, LV_H_SAMSUNG,
                                  -1, -1, -1, -1, -1, -1, -1, -1};
static uint8_t mo_live_n[MO_N]; static int8_t mo_live_db[MO_N]; static int mo_live_tot = 0, mo_live_np = 0;
static void mo_live_update(void)
{
    mo_live_tot = mo_live_np = 0;
    for (int i = 0; i < MO_N; ++i) {
        int n = 0, best = -127; const int h = MO_HUNT[i];
        if (h == LV_H_DEAUTH) { if (lv_deauth_have && lv_deauth_n) { n = lv_deauth_n > 255 ? 255 : (int)lv_deauth_n; best = 0; } }
        else if (h >= 0) for (int k = 0; k < lv_nf; ++k) if (lv_hunt_match(h, lv_f[k])) { ++n; if (lv_f[k].rssi > best) best = lv_f[k].rssi; }
        mo_live_n[i] = (uint8_t)(n > 255 ? 255 : n); mo_live_db[i] = (int8_t)(n ? best : MO11_RSSI[i]);
        if (n) { mo_live_tot += n; ++mo_live_np; }
    }
}
static inline bool mo_live(int i) { return i >= 0 && i < MO_N && mo_live_n[i] && MO_HUNT[i] != LV_H_DEAUTH; }   /* a real dBm to show */
static inline int mo_db(int i) { return mo_live(i) ? mo_live_db[i] : MO11_RSSI[i]; }
/* independent drift: amplitude + rate (rad/s, all distinct) + phase per pillar and per axis */
static const float MO11_AX[MO_N] = {0.281f, 0.282f, 0.276f, 0.31f, 0.244f, 0.31f, 0.231f, 0.329f, 0.269f, 0.248f, 0.279f, 0.237f, 0.353f, 0.329f, 0.33f, 0.22f, 0.238f, 0.282f};
static const float MO11_WX[MO_N] = {0.323f, 0.587f, 0.247f, 0.278f, 0.801f, 0.899f, 0.855f, 0.726f, 0.539f, 0.195f, 0.927f, 0.167f, 0.678f, 0.776f, 0.627f, 0.368f, 0.829f, 0.652f};
static const float MO11_AZ[MO_N] = {0.261f, 0.226f, 0.293f, 0.333f, 0.328f, 0.317f, 0.273f, 0.311f, 0.29f, 0.242f, 0.333f, 0.283f, 0.25f, 0.186f, 0.314f, 0.2f, 0.234f, 0.315f};
static const float MO11_WZ[MO_N] = {0.613f, 0.262f, 0.201f, 0.311f, 0.353f, 0.239f, 0.181f, 0.54f, 0.728f, 0.673f, 0.59f, 0.128f, 0.373f, 0.639f, 0.698f, 0.56f, 0.285f, 0.151f};
static const float MO11_WH[MO_N] = {0.62f, 0.94f, 0.77f, 0.66f, 0.88f, 0.59f, 0.98f, 0.74f, 0.69f, 0.91f, 0.73f, 0.96f, 0.68f, 0.8f, 0.61f, 0.64f, 0.9f, 0.86f};
#define MO11_AH 0.06f    /* breathing: H * (1 + 0.06 sin) */
#define MO11_TW 1.35f    /* lv_rd_skin_tw: pillars a little broader than Gibson's needles (touch) */
#define MO11_TH 1.0f     /* lv_rd_skin_th: Gibson's own height law */
#define MO11_FA 4        /* lv_rd_skin_fa: lacquer face blend 4/8 */
#define MO_CAM_MS 520
#define MO_PILL_MS 480
#define MO_FLOOD_MS 420
#define MO_DROP0_MS 520
#define MO_DROP_GAP 120
#define MO_DROP_MS 420
#define MO_FY0 35       /* field rows (below the header bar, above the footer terminal) */
#define MO_FY1 285
static const LvRect MO_BTN = {170, 247, 239, 285};   /* "< TOWERS" finger target 70x39 (drawn 63x34 at 173,249) */
static int itw_mo_sel = -1, itw_mo_phase = 0; static uint32_t itw_mo_t0 = 0; static bool itw_mo_nav = false;
static int mo_last = -1; static uint32_t mo_close_t = 0; static bool mo_closing = false;
/* v1.2.11 (Mobandy 2026-11-20 "the screen changes to a tower and folders instead of the tower advancing"): the Dojo
 * animates on its own clock mo_vt. It follows lv_now, except that while the push / folder drop / close plays, one frame
 * may advance it by at most MO_DT_PUSH / MO_DT_DROP ms. A slow CYD frame (the first open frame is ~0.7 s while the
 * close-up emblems build) used to swallow the whole 400 ms push, so the pillar jumped straight to the close-up. At the
 * locked 30 ms cadence (sim / fast frames) the cap never binds and every frame is the locked frame. */
#define MO_DT_PUSH 50
#define MO_DT_DROP 100
static LvSelClk mo_clk;   /* lv_select.h (LOCKED shared selection) */
static inline uint32_t mo_vnow(void) { return lv_sel_clk_now(mo_clk, lv_now); }   /* the Dojo clock, read outside the frame (taps) */
/* v1.2.11 (Mobandy 2026-11-20 "None of the folders are responsive or interactive"): a landed folder is a button.
 * Tap = press flash + it opens (green frame, its real facts in the footer); tap it again or another one to switch. */
static int mo_fold_sel = -1;
static const char *mo_fold_line(int i, int j, int k);
static int mo_txt_bad = 0, mo_txt_n = 0; static char mo_txt_why[160] = ""; static long mo_txt_tot = 0, mo_txt_lines = 0;
static LvTower mo11_tw[MO_N]; static bool mo11_have[MO_N]; static float mo11_cam = 0; static int MO_ORD[MO_N], mo11_nord = 0;   /* this frame's lv_rd_tw, by device */
static float mo11_dxv[MO_N], mo11_dzv[MO_N], mo11_hk[MO_N];   /* this frame's drift, by device (dump) */
/* ---- ghost icon sprites (built on enter, sized to each pillar's face width at its base depth) ---- */
#define MO_SP 44
static uint8_t mo_hg[12][MO_SP * MO_SP], mo_hb[12][MO_SP * MO_SP], mo_hl[12][MO_SP * MO_SP]; static int mo_hs[12], mo_hn[12]; static float mo_hsv[12];
static int mo_src(int i, int r, int c)
{
    if (MO_ICON[i] < 0) { const int rr = r * 12 / 16, cc = c * 12 / 16; return MO_DEAUTH[rr][cc] == '.' ? 0 : 2; }
    const char ch = LV_ICON_ART[MO_ICON[i]][r][c]; return ch == '.' ? 0 : ch == 'k' ? 2 : 1;
}
static void mo_build(void)
{
    /* fw-3locks-1116: the 15.5 KB build scratch is heap-borrowed for the build only (was static .bss; DRAM overflowed). Same values. */
    float *const C = (float *)malloc(2 * sizeof(float) * MO_SP * MO_SP); if (!C) return; float *const T = C + MO_SP * MO_SP;
    for (int i = 0; i < 12; ++i) {
        const float Z = MO11_Z0[i], fw = 2 * LV_RD_F * 0.12f * MO11_TW / Z; float s = (fw - 4) / 16; s = s < 0.5f ? 0.5f : s > 1.0f ? 1.0f : s; mo_hsv[i] = s;
        const int n = (int)(16 * s + 0.5f), pad = 3, S = n + 2 * pad; mo_hs[i] = S; mo_hn[i] = n;
        for (int y = 0; y < S; ++y) for (int x = 0; x < S; ++x) {
            int nb = 0, nl = 0;
            for (int a = 0; a < 4; ++a) for (int b = 0; b < 4; ++b) {
                const float u = (x - pad + (b + 0.5f) / 4) / s, v = (y - pad + (a + 0.5f) / 4) / s;
                if (u < 0 || v < 0 || u >= 16 || v >= 16) continue;
                const int k = mo_src(i, (int)v, (int)u); nb += k == 1; nl += k == 2;
            }
            mo_hb[i][y * S + x] = (uint8_t)(nb * 255 / 16); mo_hl[i][y * S + x] = (uint8_t)(nl * 255 / 16); C[y * S + x] = (nb + nl) / 16.0f;
        }
        for (int y = 0; y < S; ++y) for (int x = 0; x < S; ++x) { float a = 0; for (int d = -2; d <= 2; ++d) { const int xx = x + d; if (xx >= 0 && xx < S) a += C[y * S + xx]; } T[y * S + x] = a / 5; }
        for (int y = 0; y < S; ++y) for (int x = 0; x < S; ++x) { float a = 0; for (int d = -2; d <= 2; ++d) { const int yy = y + d; if (yy >= 0 && yy < S) a += T[yy * S + x]; } C[y * S + x] = a / 5; }
        for (int k = 0; k < S * S; ++k) { const float g = C[k] * 2.0f; mo_hg[i][k] = (uint8_t)(g > 1 ? 255 : g * 255); }
    }
    free(C);
}
/* ---- pixel helpers ---- */
static inline void mo11_bl(LvBand &B, int x, int y, uint16_t c, int a)   /* alpha blend /256 (also used by the skin-1 hooks) */
{
    if (a <= 0 || x < B.x0 || x >= B.x0 + B.w || y < B.y0 || y >= B.y0 + B.h) return;
    uint16_t &p = B.px[(y - B.y0) * B.w + (x - B.x0)];
    if (a >= 256) { p = c; return; }   /* v1.2.11 speed: every opaque pillar/UI pixel was doing a 3-channel blend by 256/256 (exactly c) */
    /* perf-all 2026-11-21: one-entry memo; runs of the same (under, ink, alpha) skip the 3-channel blend (lv_mix is a pure function) */
    static uint32_t mk = 0xffffffffu; static int ma = -1; static uint16_t mv = 0;
    const uint32_t k = (uint32_t)p << 16 | c; if (k != mk || a != ma) { mk = k; ma = a; mv = lv_mix(p, c, a, 256); }
    p = mv;
}
static int mo_cy0 = -10000;   /* scene clip top: folders never paint over the header (y < 46) */
static inline void mo_bl(LvBand &B, int x, int y, uint16_t c, int a) { if (y >= mo_cy0) mo11_bl(B, x, y, c, a); }
static inline uint16_t mo_mixh(uint32_t a, uint32_t b, int k, int n) { return lv_mix(lv_hex(a), lv_hex(b), k, n); }
static void mo_hspan(LvBand &B, int y, int xa, int xb, uint16_t c, int a) { if (y < B.y0 || y >= B.y0 + B.h) return; for (int x = xa; x <= xb; ++x) mo_bl(B, x, y, c, a); }
static void mo_quad(LvBand &B, const float *px, const float *py, uint16_t c, int a)   /* convex quad fill */
{
    float ymin = py[0], ymax = py[0]; for (int k = 1; k < 4; ++k) { if (py[k] < ymin) ymin = py[k]; if (py[k] > ymax) ymax = py[k]; }
    int y0 = (int)ceilf(ymin), y1 = (int)floorf(ymax); if (y0 < B.y0) y0 = B.y0; if (y1 > B.y0 + B.h - 1) y1 = B.y0 + B.h - 1;
    for (int y = y0; y <= y1; ++y) {
        float xa = 1e9f, xb = -1e9f;
        for (int k = 0; k < 4; ++k) { const float ax = px[k], ay = py[k], bx = px[(k + 1) & 3], by = py[(k + 1) & 3];
            if ((y < ay && y < by) || (y > ay && y > by)) continue;
            const float x = ay == by ? (ax < bx ? ax : bx) : ax + (bx - ax) * (y - ay) / (by - ay); if (x < xa) xa = x; if (x > xb) xb = x;
            if (ay == by) { const float x2 = ax > bx ? ax : bx; if (x2 > xb) xb = x2; } }
        if (xa <= xb) mo_hspan(B, y, (int)lv_lroundf(xa), (int)lv_lroundf(xb), c, a);
    }
}
static void mo_aline(LvBand &B, float x0, float y0, float x1, float y1, uint16_t c, int a)   /* anti-aliased line (Wu-style, mostly vertical/horizontal) */
{
    if ((y0 < B.y0 - 2 && y1 < B.y0 - 2) || (y0 > B.y0 + B.h + 1 && y1 > B.y0 + B.h + 1)) return;   /* v1.2.11 speed: wholly outside this band */
    const float dx = x1 - x0, dy = y1 - y0; const int n = (int)(fabsf(dx) > fabsf(dy) ? fabsf(dx) : fabsf(dy)) + 1;
    const float ylo = (float)(B.y0 - 2), yhi = (float)(B.y0 + B.h + 1);   /* perf-all: samples whose 2 rows miss the band draw nothing */
    for (int k = 0; k <= n; ++k) { const float t = (float)k / n, x = x0 + dx * t, y = y0 + dy * t; if (y < ylo || y > yhi) continue;
        if (fabsf(dx) > fabsf(dy)) { const int yi = (int)floorf(y); const float f = y - yi; mo_bl(B, (int)lv_lroundf(x), yi, c, (int)(a * (1 - f))); mo_bl(B, (int)lv_lroundf(x), yi + 1, c, (int)(a * f)); }
        else { const int xi = (int)floorf(x); const float f = x - xi; mo_bl(B, xi, (int)lv_lroundf(y), c, (int)(a * (1 - f))); mo_bl(B, xi + 1, (int)lv_lroundf(y), c, (int)(a * f)); } }
}


static inline float mo_ease(float u) { return lv_sel_ease(u); }   /* v1.2.11: the shared lv_select.h ease (same formula) */
static inline float mo_px(float X, float Z) { return 120 + LV_RD_F * (X - lv_rd_camx) / Z; }   /* the radar's projection (lv_rd_place px/py) */
static inline float mo_py(float Y, float Z) { return LV_RD_HOR + LV_RD_F * (LV_RD_CAMH - Y) / Z; }
static uint32_t mo_tw0 = 0;   /* this world's t0 (= lv_rd_t0 while swapped; also valid in the tap handler, outside the swap) */
static float mo_t(void) { return (lv_now - mo_tw0) / 1000.0f; }


/* ---- v12 open state: the close-up push (all of it is gated on a tapped / closing pillar; idle never reaches it) ---- */
#define MO12_PUSH0 0       /* the push starts while the flood is still rising (pill 0..480, flood 480..900) */
#define MO12_PUSH_MS 400     /* camera + pillar ease to the close-up */
#define MO12_CLOSE_MS 560    /* < TOWERS: the same ease back to idle */
#define MO12_ZC 0.72f        /* close-up depth of the front face */
#define MO12_SX 38.0f        /* close-up front-face centre column (the DOWNLOAD game parks its tower at x 50) */
#define MO12_YT 60.0f        /* close-up front-top row (the search bar sits above it, rows 38..52) */
#define MO12_CAMA 0.02f      /* open camera: Gibson's sway law, smaller */
#define MO12_BAR0 120       /* search bar wipes in, then types the device */
#define MO12_WIPE_MS 180
#define MO12_TYPE_MS 45
#define MO12_DROP0 (MO12_BAR0 + MO12_WIPE_MS + (itw_mo_sel >= 0 ? (int)strlen(MO_DEV[itw_mo_sel].name) : 0) * MO12_TYPE_MS + 200)   /* v5e: folders drop 200 ms after the search finished typing the device (DOWNLOAD's eased stagger kept) */
#define MO12_GAP 110
#define MO12_DROP_MS 380
#define MO12_LOAD_MS 500     /* a landed folder's LOADING bar */
static const int MO12_FX = 92, MO12_FW = 143, MO12_FY0 = 60, MO12_FDY = 62, MO12_FH = 50;   /* folder j: tab top FY0 + j*FDY, body 8 below, 36 tall */
static const int MO12_BX = 8, MO12_BY = 38, MO12_BW = 229, MO12_BH = 15;                      /* search bar 229x15 at (8,38) */
static float mo12_camc = 0;   /* open camera centre (picked at tap) */
static LvBand *mo12_B = nullptr;   /* the band being drawn (for the room wash inside the tower loop) */
static float mo12_p(int i)   /* close-up progress of pillar i: 0 idle .. 1 close-up (eased) */
{
    if (i < 0) return 0;
    if (i == itw_mo_sel) return mo_ease(((int)(lv_now - itw_mo_t0) - MO12_PUSH0) / (float)MO12_PUSH_MS);
    if (mo_closing && i == mo_last) return 1.0f - mo_ease((lv_now - mo_close_t) / (float)MO12_CLOSE_MS);
    return 0;
}
static float mo12_cam(float t) { return mo12_camc + MO12_CAMA * sinf(t * 0.45f); }

/* ---- independent drift (lv_rd_place hook, skin 1): every pillar its own X / Z / H path; the sort in lv_rd_place re-orders far -> near ---- */
static float mo11_k(int i) { (void)i; return 1.0f; }    /* v12: every pillar keeps its full idle drift; the close-up is a blend on top */
static const float MO11_PX[MO_N] = {4.974f, 0.836f, 0.518f, 5.273f, 0.932f, 3.105f, 0.22f, 5.384f, 2.198f, 4.591f, 3.912f, 3.58f, 0.577f, 4.965f, 1.645f, 1.769f, 5.765f, 0.14f};
static const float MO11_PZ[MO_N] = {5.122f, 2.672f, 0.408f, 4.291f, 0.204f, 5.359f, 2.034f, 2.391f, 3.717f, 4.919f, 1.292f, 1.043f, 4.811f, 5.31f, 3.69f, 0.426f, 6.153f, 1.889f};
static const float MO11_AS[MO_N] = {0.051f, 0.058f, 0.05f, 0.04f, 0.059f, 0.052f, 0.037f, 0.049f, 0.068f, 0.068f, 0.04f, 0.06f, 0.048f, 0.05f, 0.054f, 0.064f, 0.048f, 0.036f};
static const float MO11_WS[MO_N] = {1.958f, 1.053f, 1.308f, 1.249f, 1.984f, 1.166f, 0.999f, 1.723f, 1.852f, 1.801f, 1.087f, 1.527f, 1.417f, 1.91f, 1.636f, 1.591f, 0.956f, 1.487f};
static const float MO11_PS[MO_N] = {1.912f, 3.194f, 4.48f, 4.229f, 4.51f, 1.918f, 5.198f, 0.913f, 1.143f, 5.049f, 1.627f, 2.121f, 5.394f, 3.06f, 1.851f, 2.074f, 1.554f, 2.251f};
static float mo11_dx(int i, float t) { return mo11_k(i) * (MO11_AX[i] * (MO11_Z0[i] / 2.0f) * sinf(MO11_WX[i] * t + MO11_PX[i]) + MO11_AS[i] * MO11_Z0[i] * sinf(MO11_WS[i] * t + MO11_PS[i])); }   /* v5c: own phase + lateral sway */   /* amplitude grows with depth: far pillars visibly travel too */
static float mo11_kz(int i) { (void)i; return 1.0f; }
static const float MO11_HK[MO_N] = {1.0677f, 0.6022f, 0.7461f, 0.6937f, 1.1396f, 0.995f, 0.805f, 0.6506f, 0.6192f, 1.1913f, 0.6332f, 0.9639f, 1.1062f, 0.6087f, 1.1668f, 0.6708f, 0.8361f, 1.0397f};   /* v5d depth height factor */
static float mo11_dz(int i, float t) { return mo11_kz(i) * MO11_AZ[i] * (MO11_Z0[i] / 3.0f) * sinf(MO11_WZ[i] * t + MO11_PZ[i]); }   /* v5c: own phase; depth from the Gibson map of the EX.DATA rssi */
static void mo11_drift(LvTower &T)
{
    const int i = T.i; if (i < 0 || i >= MO_N) return; const float t = mo_t();
static_assert(MO_N == 18, "hk table");
    T.H = MO_HFIX[MO_LOOKOF[i]] * MO11_HK[i];   /* v5d: group height x a fixed depth factor (near towers shorter), no breathing */
    const float Z0 = MO11_Z0[i], X0 = (MO11_SX[i] - 120) * Z0 / LV_RD_F;
    const float dx = mo11_dx(i, t), dz = mo11_dz(i, t), hk = 1.0f;   /* spread: no idle height breathing (heights stay exactly three) */
    T.X = X0 + dx; T.Z = Z0 + dz; T.H *= hk;
#ifdef MO_EB2
    { static const bool NEAR5[MO_N] = {false, true, false, false, false, false, false, true, true, false, true, false, false, true, false, false, false, false};   /* the nearest 25% by Gibson depth */
      const float kq = NEAR5[i] ? 1.0f + (MO_EB2_KZ - 1.0f) * (1.0f - mo12_p(i)) : 1.0f;   /* eases back to 1 while it is pulled up */
      T.X = T.X * kq + lv_rd_camx * (1.0f - kq); T.Z *= kq; T.H *= kq; mo_eb2_k[i] = kq; }   /* EB3: camera sway compensated -> same on-screen sway as EB (no extra parallax off the edges) */
#endif
    mo11_dxv[i] = dx; mo11_dzv[i] = dz; mo11_hk[i] = hk;
    const float p = mo12_p(i);
    if (p > 0) {   /* v12 push: forward along the camera ray (1/Z eased, so the growth reads evenly) to the close-up, keeping a little breathing */
        const float c = lv_rd_camx, Zi = T.Z, Xi = T.X, Hi = T.H;
        const float Zc = MO12_ZC * (1.0f + 0.012f * sinf(0.9f * t + i));
        const float Hc = (LV_RD_CAMH + (LV_RD_HOR - MO12_YT) * MO12_ZC / LV_RD_F) * (1.0f + 0.012f * sinf(MO11_WH[i] * t + i * 0.77f));
        const float Xc = mo12_camc + (MO12_SX - 120) * MO12_ZC / LV_RD_F + 0.010f * sinf(MO11_WX[i] * t + i * 2.399f);
        (void)Zi; (void)Xi; (void)Hi;
        lv_sel_pull(c, Xc, Zc, Hc, p, LV_RD_F, T.X, T.Z, T.H);   /* v1.2.13: the shared lv_select.h pull-up (same arithmetic) */
    }
}
/* ---- skin-1 scene hooks (lv_scene_radar keeps its own sky / grid / rain / tower code; these add the dojo) ---- */
static void mo11_tatami(LvBand &B, int y, float Z, float ph)   /* per floor row, before Gibson's seam rows + lanes: two-tone woven mats, one mat per grid cell */
{
    if (y < B.y0 || y >= B.y0 + B.h) return;
    const int fade = Z > 7 ? 0 : Z > 4 ? (int)(256 * (7 - Z) / 3) : 256; if (!fade) return;
    const float zs = Z + ph; const int zr = (int)floorf(zs); const float zf = zs - zr;
    const uint16_t a = lv_hex(MO11_MAT_A), b = lv_hex(MO11_MAT_B);
    for (int x = 0; x < 240; ++x) {
        const float X = (x - 120) * Z / LV_RD_F + lv_rd_camx; const int xc = lv_ifloor(X * 2.0f);
        const bool alt = ((xc + zr) & 1) != 0;
        int k = alt ? 256 : 0;
        if (Z < 4 && (((int)(zf * 16)) & 1)) k = alt ? 200 : 90;   /* rush weave across the mat */
        if (k) mo11_bl(B, x, y, b, k * fade / 256);
    }
}
static void mo11_horizon(LvBand &B, int shift)   /* Gibson's skyline, transformed: the dojo's far shoji wall + timber posts + ceiling rafters */
{
    const int top = LV_RD_HOR - 30, beam = top - 3;
    if (!b_yhit(B, LV_RD_Y0 - 2, LV_RD_HOR + 1)) return;   /* v1.2.11 speed: wall, beams, rafters all lie in rows LV_RD_Y0..LV_RD_HOR (lines clip to the band) */
    const uint16_t dark = lv_hex(MO11_BEAM), paper = lv_hex(MO11_PAPER), seam = lv_hex(MO11_SEAM);
    for (int y = b_ylo(B, top); y < LV_RD_HOR && y < B.y0 + B.h; ++y) for (int x = 0; x < 240; ++x) mo11_bl(B, x, y, paper, 200);   /* backlit paper band */
    for (int k = -12; k < 36; ++k) {   /* Gibson's skyline slots (x = k*11 - 6 - shift) become shoji panels; extended past 24 so a far-pillar pan never leaves a gap */
        const int x0 = k * 11 - 6 - shift, w = 9; if (x0 < -11 || x0 > 240) continue;
        for (int yy = top + 4; yy < LV_RD_HOR - 2; yy += 6) { if (yy < B.y0 || yy >= B.y0 + B.h) continue; for (int x = x0 + 1; x < x0 + w; ++x) mo11_bl(B, x, yy, seam, 90); }   /* kumiko lattice (perf-all: band rows only) */
        for (int y = b_ylo(B, top); y < LV_RD_HOR && y < B.y0 + B.h; ++y) { mo11_bl(B, x0 + 4, y, seam, 70); mo11_bl(B, x0, y, dark, ((k + 36) % 3) == 0 ? 230 : 120); }
        if (((k + 36) % 3) == 0) for (int y = b_ylo(B, top); y < LV_RD_HOR && y < B.y0 + B.h; ++y) mo11_bl(B, x0 + 1, y, dark, 160);   /* heavy timber post every 3 panels */
    }
    for (int y = b_ylo(B, beam); y < top && y < B.y0 + B.h; ++y) for (int x = 0; x < 240; ++x) mo11_bl(B, x, y, dark, 235);   /* kamoi head beam */
    if (b_yhit(B, LV_RD_HOR - 2, LV_RD_HOR - 2)) for (int x = 0; x < 240; ++x) mo11_bl(B, x, LV_RD_HOR - 2, dark, 140);                       /* sill */
    /* ceiling rafters (world Y 2.25) receding from the camera to the wall at Z 4.8, with Gibson's camera parallax */
    const float Yc = 2.25f, Zw = 4.8f;
    for (int r = -6; r <= 6; ++r) {
        const float X = r * 0.6f; const float z0 = 1.6f;
        const float xa = mo_px(X, z0), ya = mo_py(Yc, z0), xb = mo_px(X, Zw), yb = mo_py(Yc, Zw);
        mo_aline(B, xa, ya < LV_RD_Y0 ? LV_RD_Y0 : ya, xb, yb, dark, 150);
        (void)ya;
    }
    for (int q = 0; q < 6; ++q) {   /* cross beams */
        const float Z = 1.9f + q * 0.55f; const int y = (int)lv_lroundf(mo_py(Yc, Z)); if (y < LV_RD_Y0 || y >= beam || !b_yhit(B, y, y + 1)) continue;
        for (int x = 0; x < 240; ++x) { mo11_bl(B, x, y, dark, 150 - q * 14); mo11_bl(B, x, y + 1, lv_hex(0xd8c6a0), 60 - q * 6); }
    }
}

static void mo_halo(LvBand &B, int i, int cx, int cy, int fade, int boost)   /* frosted-parchment ghost of the device icon inside the pillar */
{
    const int S = mo_hs[i], x0 = cx - S / 2, y0 = cy - S / 2; if (y0 >= B.y0 + B.h || y0 + S <= B.y0) return;
    const uint16_t cg = boost ? lv_hex(0xd8ffe6) : lv_hex(0xf6f0e2), cb = boost ? lv_hex(0xeafff1) : lv_hex(0xe8dec8), cl = boost ? lv_hex(0x003a16) : lv_hex(0xfffaf0);
    const float br = 0.85f + 0.15f * sinf((lv_now / 1000.0f) * 2.0f + i * 0.9f);
    const int kg = (int)(70 * br) * fade / 256, kb = 120 * fade / 256, kl = (boost ? 220 : 200) * fade / 256;
    for (int y = 0; y < S; ++y) { const int yy = y0 + y; if (yy < B.y0 || yy >= B.y0 + B.h) continue;
        for (int x = 0; x < S; ++x) { const int k = y * S + x, xx = x0 + x;
            if (mo_hg[i][k]) mo_bl(B, xx, yy, cg, mo_hg[i][k] * kg / 255);
            if (mo_hb[i][k]) mo_bl(B, xx, yy, cb, mo_hb[i][k] * kb / 255);
            if (mo_hl[i][k]) mo_bl(B, xx, yy, cl, mo_hl[i][k] * kl / 255); } }
}
static void mo_pill(LvBand &B, int cx, int topy, float u)   /* glossy red pill: short ease-in fall + one small bounce */
{
    const float e = 1.f - (1.f - u) * (1.f - u); int y = (int)(34 + (topy - 5 - 34) * e);
    if (u > 0.82f) { const float b = (u - 0.82f) / 0.18f; y -= (int)(7 * (b - b * b) * 4); }
    for (int r = -4; r <= 4; ++r) for (int c = -8; c <= 8; ++c) { const float d = (c * c) / 64.f + (r * r) / 16.f; if (d > 1.15f) continue;
        const uint16_t col = (r < -1 && c < 2) ? lv_hex(0xff8a8a) : (r > 1 ? lv_hex(0xa00818) : lv_hex(0xe4121f));
        mo_bl(B, cx + c, y + r, col, d > 0.9f ? (int)(256 * (1.15f - d) / 0.25f) : 256); }
    mo_bl(B, cx - 3, y - 2, lv_hex(0xffffff), 230); mo_bl(B, cx - 2, y - 2, lv_hex(0xffe0e0), 200);
    for (int r = 0; r < 2; ++r) mo_hspan(B, topy - 1 + r, cx - 7 + r * 2, cx + 7 - r * 2, lv_hex(0x5a2a22), (int)(50 * e));
}
static void mo_brackets(LvBand &B, const LvTower &T, float u)   /* Matrix green corner ticks on the pillar's own 3D corners (front face + receding top edges) */
{
    const float p = 3 + 4 * (1 - u), L = 7; const uint16_t c = lv_hex(MO11_G); const int a = (int)(250 * (u < 0.3f ? u / 0.3f : 1));
    const float x0 = T.x0 - p, x1 = T.x1 + p, yt = T.yt - p, yb = T.yb + p;
    mo_aline(B, x0, yt, x0 + L, yt, c, a); mo_aline(B, x0, yt, x0, yt + L, c, a); mo_aline(B, x1, yt, x1 - L, yt, c, a); mo_aline(B, x1, yt, x1, yt + L, c, a);
    mo_aline(B, x0, yb, x0 + L, yb, c, a); mo_aline(B, x0, yb, x0, yb - L, c, a); mo_aline(B, x1, yb, x1 - L, yb, c, a); mo_aline(B, x1, yb, x1, yb - L, c, a);
    /* receding ticks along the top face's side edges toward the back-top corners (the pillar's own perspective) */
    const float bx0 = T.bx0 - p, bx1 = T.bx1 + p, byt = T.byt - p;
    auto tick = [&](float ax, float ay, float bx, float by) { const float dx = bx - ax, dy = by - ay, n = sqrtf(dx * dx + dy * dy); if (n < 1) return; const float s = fminf(1.0f, 5.0f / n); mo_aline(B, ax, ay, ax + dx * s, ay + dy * s, c, a); };
    tick(x0, yt, bx0, byt); tick(x1, yt, bx1, byt);
    mo_aline(B, bx0, byt, bx0 + 4, byt, c, a * 3 / 4); mo_aline(B, bx1, byt, bx1 - 4, byt, c, a * 3 / 4);
}
static int mo_wrap(const char *s, char out[3][24])   /* word wrap to 22 columns of the 5x7 font */
{
    int n = 0; out[0][0] = out[1][0] = out[2][0] = 0; char w[48]; const char *p = s;
    while (*p && n < 3) {
        while (*p == ' ') ++p; int k = 0; while (p[k] && p[k] != ' ' && k < 47) { w[k] = p[k]; ++k; } w[k] = 0; if (!k) break;
        const int L = (int)strlen(out[n]); if (L && L + 1 + k > 22) { if (++n >= 3) break; }
        if (strlen(out[n])) strcat(out[n], " "); strcat(out[n], w); p += k;
    }
    return n + 1;
}


static void mo_audit(int x, int y, int w, int h, int bx0, int by0, int bx1, int by1, const char *what)   /* chrome text audit (top band only) */
{
    if (y > 33 && by0 < 34) return;
    mo_txt_n++; mo_txt_lines++;
    if (x < bx0 || y < by0 || x + w - 1 > bx1 || y + h - 1 > by1) { mo_txt_bad++; mo_txt_tot++; snprintf(mo_txt_why, sizeof mo_txt_why, "%s x=%d y=%d w=%d h=%d box=[%d,%d,%d,%d]", what, x, y, w, h, bx0, by0, bx1, by1); }
}
static void mo_chrome(LvBand &B)   /* header bar rows 24..34: charcoal + parchment ink + green // */
{
    const uint16_t ink = lv_hex(0xe8dec8); char t[32];
    b_fill(B, 0, 24, 240, 1, lv_hex(MO11_BEAM)); b_fill(B, 0, 25, 240, 9, lv_hex(MO11_CHAR)); b_fill(B, 0, 34, 240, 1, lv_hex(MO11_G));
    int x = b_text35(B, 3, 27, LV_BOTPHEUS_NAME ? "BOTPHEUS" : "MORPHEUS", ink); x = b_text35(B, x + 4, 27, "//", lv_hex(MO11_GH)); const int xe = b_text35(B, x + 4, 27, "DOJO CONSTRUCT", ink);
    if (itw_mo_sel >= 0) snprintf(t, sizeof t, "PILLAR %d/%d  LOCKED", itw_mo_sel + 1, MO_N); else if (mo_live_np) snprintf(t, sizeof t, "%d LIVE  %d PILLARS", mo_live_np, MO_N); else snprintf(t, sizeof t, "%d PILLARS  EX.DATA", MO_N);
    const int rx = 237 - lv_w35(t); b_text35(B, rx, 27, t, lv_hex(0xbfb294));
    if (B.y0 == 0) { mo_audit(3, 27, xe - 3, 5, 0, 25, rx - 4, 33, "hdrL"); mo_audit(rx, 27, lv_w35(t), 5, xe + 4, 25, 239, 33, "hdrR"); }
}
static void mo_emb_draw(LvBand &B, int i, int x0, int y0, int S, uint32_t bc, uint32_t dc, uint32_t hc, int a, int ha, int yhi);   /* mo_mix.inc */
static void mo_chip_icon(LvBand &B, int i, int x, int y)   /* morpheus-mix: the device's 16 px vector emblem, parchment body + green detail on the charcoal chip */
{
    mo_emb_draw(B, i, x, y, 16, 0xe8dec8, MO11_GH, 0, 256, 0, 9999);
}
static void mo_panel(LvBand &B)   /* open pillar only: rows 247..285, parchment + timber, green < TOWERS */
{
    const uint16_t ink = lv_hex(MO11_INK), sub = lv_hex(MO11_SEAM);
    b_fill(B, 2, 247, 236, 39, lv_hex(MO11_PAPER)); b_frame(B, 2, 247, 236, 39, lv_hex(MO11_BEAM)); b_frame(B, 4, 249, 232, 35, lv_hex(0xd8ccb0));
    const int i = itw_mo_sel;
    b_fill(B, 7, 251, 22, 22, lv_hex(MO11_CHAR)); b_frame(B, 7, 251, 22, 22, lv_hex(MO11_BEAM)); b_fill(B, 7, 272, 22, 1, lv_hex(MO11_G));
    mo_chip_icon(B, i, 10, 254);
    const int nx = b_text(B, 34, 252, MO_DEV[i].name, ink, 2);
    char t[40];
    if (mo_live(i)) snprintf(t, sizeof t, "%s  %d DBM  LIVE %d SEEN", MO_KNAME[MO_KIND[i]], mo_live_db[i], mo_live_n[i]);
    else if (MO_HUNT[i] == LV_H_DEAUTH && mo_live_n[i]) snprintf(t, sizeof t, "%s  LIVE %d FRAMES", MO_KNAME[MO_KIND[i]], mo_live_n[i]);
    else snprintf(t, sizeof t, "%s  %d DBM  EX.DATA", MO_KNAME[MO_KIND[i]], MO11_RSSI[i]);
    b_text35(B, 34, 277, t, sub);
    const uint16_t c = lv_hex(MO11_G); b_fill(B, 173, 249, 63, 34, lv_hex(MO11_CHAR)); b_frame(B, 173, 249, 63, 34, c); b_frame(B, 175, 251, 59, 30, lv_hex(0x0a5a28));
    b_text(B, 173 + (63 - 8 * 6 + 1) / 2, 262, "< TOWERS", lv_hex(MO11_GH));
    if (B.y0 == 0) { mo_audit(34, 252, nx - 34, 14, 30, 249, 170, 275, "pname"); mo_audit(34, 277, lv_w35(t), 5, 30, 276, 170, 283, "psub"); mo_audit(173 + (63 - 8 * 6 + 1) / 2, 262, 47, 7, 176, 252, 233, 280, "ptowers"); }
}
static void mo_terminal(LvBand &B)   /* footer terminal rows 287..319, x 1..238 (full width, 1 px margins): charcoal, parchment + green */
{
    const int x0 = 1, y0 = 287, x1 = 238, y1 = 319; const uint16_t g = lv_hex(0xbfb294), ink = lv_hex(0xe8dec8);
    b_fill(B, x0, y0, x1 - x0 + 1, y1 - y0 + 1, lv_hex(MO11_CHAR)); b_frame(B, x0, y0, x1 - x0 + 1, y1 - y0 + 1, lv_hex(MO11_SEAM));
    char a[48], b[48];
    if (itw_mo_sel < 0) { snprintf(a, sizeof a, "HEIGHT = EXAMPLE SIGNAL  EX.DATA"); snprintf(b, sizeof b, "TAP A PILLAR = OPEN IT"); }
    else if (mo_fold_sel >= 0) { snprintf(a, sizeof a, "%s", mo_fold_line(itw_mo_sel, mo_fold_sel, 0)); snprintf(b, sizeof b, "%s", mo_fold_line(itw_mo_sel, mo_fold_sel, 1)); }
    else if (mo_live_n[itw_mo_sel]) { snprintf(a, sizeof a, "PILLAR OPEN  THREAT: %s  LIVE SCAN", MO_TNAME[MO_THREAT[itw_mo_sel]]); snprintf(b, sizeof b, "LOADED %s  3 SCROLLS", MO_DEV[itw_mo_sel].name); }
    else { snprintf(a, sizeof a, "PILLAR OPEN  THREAT: %s  EX.DATA", MO_TNAME[MO_THREAT[itw_mo_sel]]); snprintf(b, sizeof b, "LOADED %s  3 SCROLLS", MO_DEV[itw_mo_sel].name); }
    b_text35(B, x0 + 4, y0 + 4, a, g); b_text35(B, x0 + 4, y0 + 11, b, ink);
    b_fill(B, x0 + 2, y0 + 18, x1 - x0 - 3, 1, lv_hex(0x3a3228));
    int x = b_text(B, x0 + 4, y0 + 22, ">", lv_hex(MO11_GH)); const char *pr = itw_mo_sel < 0 ? "FREE YOUR MIND" : mo_fold_sel >= 0 ? "TAP SCROLL AGAIN = CLOSE IT" : "< TOWERS TO GO BACK"; x = b_text(B, x + 3, y0 + 22, pr, lv_hex(MO11_GH));
    if ((lv_now / 500) & 1) b_fill(B, x + 1, y0 + 29, 5, 1, lv_hex(MO11_GH));
    if (B.y0 == 0) { mo_audit(x0 + 4, y0 + 4, lv_w35(a), 5, x0 + 2, y0 + 2, x1 - 2, y0 + 17, "term1"); mo_audit(x0 + 4, y0 + 11, lv_w35(b), 5, x0 + 2, y0 + 2, x1 - 2, y0 + 17, "term2"); mo_audit(x0 + 4, y0 + 22, x - x0 - 4 + 6, 7, x0 + 2, y0 + 20, x1 - 2, y1 - 1, "term3"); }
}
static int mo11_fade(void)   /* the other pillars wash toward the parchment while a pillar is open */
{
    if (itw_mo_sel >= 0) return (int)(256 - 136 * mo_ease((lv_now - itw_mo_t0) / 400.0f));
    if (mo_closing) return (int)(120 + 136 * mo_ease((lv_now - mo_close_t) / (float)MO_CAM_MS));
    return 256;
}
static float mo_flood(void) { return 0; }   /* v13: no green flood */
/* ---- skin-1 hooks inside lv_scene_radar's own tower loop ---- */
static void mo12_wash(LvBand &B, float p)   /* v12: the room behind the close-up washes toward the parchment (field rows 35..246) */
{
    const int a = (int)(110 * p); if (a <= 0) return; const uint16_t pap = lv_hex(MO11_PAPER);
    /* perf-all 2026-11-21: the same per-pixel blend as mo_hspan -> mo_bl -> mo11_bl, walked directly over the band's own rows and columns
     * (same clips: field rows, mo_cy0, band rect), with a one-entry memo because the room behind is long runs of one colour. */
    int ya = LV_RD_Y0 + 1; if (ya < B.y0) ya = B.y0; if (ya < mo_cy0) ya = mo_cy0; int yb = 246; if (yb > B.y0 + B.h - 1) yb = B.y0 + B.h - 1;
    const int xa = B.x0 > 0 ? B.x0 : 0, xb = B.x0 + B.w - 1 < 239 ? B.x0 + B.w - 1 : 239;
    uint16_t lk = 0, lv = 0; bool have = false;
    for (int y = ya; y <= yb; ++y) { uint16_t *q = B.px + (y - B.y0) * B.w + (xa - B.x0);
        for (int x = xa; x <= xb; ++x, ++q) { if (a >= 256) { *q = pap; continue; } if (!have || *q != lk) { have = true; lk = *q; lv = lv_mix(lk, pap, a, 256); } *q = lv; } }
}
static void mo11_ink(const LvTower &T, uint16_t &face, uint16_t &edge)
{
    const int br = 3 + lv_rd_level(lv_f[T.i]) * 5 / 60;   /* Gibson's brightness by signal, 3..8 of 8: stronger = inkier timber */
    face = lv_hex(MO11_LAQ); edge = lv_mix(lv_hex(MO11_MAT_A), lv_hex(MO11_BEAM), br, 8);
    if (T.i == itw_mo_sel) { const float p = mo12_p(T.i); if (p > 0 && mo12_B) mo12_wash(*mo12_B, p);
        if (mo_flood() > 0) { const int k = (int)(256 * p); face = lv_mix(lv_hex(0x0a7a34), lv_hex(MO11_LAQ), k, 256); edge = lv_mix(lv_hex(0x005a20), lv_hex(MO11_G), k, 256); } else edge = lv_hex(MO11_BEAM); return; }
    if (mo_closing && T.i == mo_last) { const float p = mo12_p(T.i); if (p > 0) { if (mo12_B) mo12_wash(*mo12_B, p); edge = lv_mix(edge, lv_hex(MO11_G), (int)(256 * p), 256); return; } }
    const int f = mo11_fade(); if (f < 256) { face = lv_mix(lv_hex(MO11_MAT_A), face, f, 256); edge = lv_mix(lv_hex(MO11_MAT_A), edge, f, 256); }
}
/* v12 close-up body: solid lacquer front, timber side face (the near side toward the screen centre), Matrix-green silhouette edges */
/* ---- v13.1 HD monolith skin (every pillar; drawn over Gibson's own corners from lv_rd_place, far -> near, so it never moves anything) ---- */
static void mo13_px(LvBand &B, int x, int y, uint16_t c, int a) { if (y >= LV_RD_Y0 + 1 && y <= LV_RD_Y1) mo_bl(B, x, y, c, a); }
static void mo13_line(LvBand &B, int x, int y, int xx, int yy, uint16_t c, int a)   /* crisp Bresenham, field-clipped */
{ if ((y < B.y0 && yy < B.y0) || (y >= B.y0 + B.h && yy >= B.y0 + B.h)) return;   /* v1.2.11 speed: wholly outside this band */
  int dx = abs(xx - x), sx = x < xx ? 1 : -1, dy = -abs(yy - y), sy = y < yy ? 1 : -1, e = dx + dy;
  const int ylo = B.y0, yhi = B.y0 + B.h;   /* perf-all: monotonic y; skip rows outside the band, stop once past it (same pixels) */
  for (;;) { if (y >= ylo && y < yhi) mo13_px(B, x, y, c, a); else if (sy > 0 ? y >= yhi : y < ylo) break;
    if (x == xx && y == yy) break; int e2 = 2 * e; if (e2 >= dy) { e += dy; x += sx; } if (e2 <= dx) { e += dx; y += sy; } } }
static void mo13_span(LvBand &B, int y, int xa, int xb, uint16_t c, int a) { if (y < LV_RD_Y0 + 1 || y > LV_RD_Y1) return; mo_hspan(B, y, xa, xb, c, a); }
static uint32_t mo13_icol(char ch)   /* LV_ICON_ART inks -> Dojo: outline ink, parchment body / shade / highlight, fixed inks Matrix green */
{ return ch == 'k' ? 0x1e1a16 : ch == 'a' ? 0xe8dec8 : ch == 'b' ? 0xb8a47c : ch == 'c' ? 0xffffff : 0x00a83c; }
static void mo13_icon(LvBand &B, int i, int x0, int y0, int S, int a)   /* integer nearest-neighbour: S = 8 (exact 2:1), 16, 32, 48 */
{
    for (int y = 0; y < S; ++y) for (int x = 0; x < S; ++x) { const int r = y * 16 / S, c = x * 16 / S; char ch;
        if (MO_ICON[i] < 0) ch = MO_DEAUTH[r * 12 / 16][c * 12 / 16] == '.' ? '.' : 'g'; else ch = LV_ICON_ART[MO_ICON[i]][r][c];
        if (ch != '.') mo13_px(B, x0 + x, y0 + y, lv_hex(mo13_icol(ch)), a); }
}
/* 3x5 kind glyphs for far pillars (too narrow for an 8 px icon): N networks, C cameras, T trackers, B BLE tools */
static const char *const MO13_GLY[4][5] = {{"#.#", "###", "###", "#.#", "#.#"}, {"###", "#..", "#..", "#..", "###"}, {"###", ".#.", ".#.", ".#.", ".#."}, {"##.", "#.#", "##.", "#.#", "##."}};
static void mo12_bigbuild(int i) { (void)i; }   /* no blurred ghost sprite */
static uint16_t mo13_mix(uint32_t a, uint32_t b, int k, int n) { return lv_mix(lv_hex(a), lv_hex(b), k < 0 ? 0 : k > n ? n : k, n); }
/* one shoji pane: 1 px lit top + left edge, two-tone inner shading (light upper half, deeper lower half), dark sill */
static void mo13_pane(LvBand &B, int xa, int ya, int xb, int yb, int kind, int a)
{
    if (xb < xa || yb < ya) return;
    const uint32_t lit[3] = {0xf2e6c8, 0x5a4a34, 0x2fbf6a}, low[3] = {0xd2c09a, 0x3a2e20, 0x158a44}, edge[3] = {0xfffaf0, 0x7a6448, 0x8affb8};
    for (int y = ya; y <= yb; ++y) mo13_span(B, y, xa, xb, lv_hex(y - ya <= (yb - ya) / 2 ? lit[kind] : low[kind]), a);
    if (yb - ya >= 2 && xb - xa >= 1) { mo13_span(B, ya, xa, xb, lv_hex(edge[kind]), a); mo13_line(B, xa, ya, xa, yb, lv_hex(edge[kind]), a); mo13_span(B, yb, xa, xb, lv_hex(0x1a140e), a); }
}
static void mo13_tower_fx_v131(LvBand &B, const LvTower &T)   /* v13.1 skin, kept for reference, not called */
{
    const int i = T.i; if (i < 0 || i >= 12) return;
    const bool open = i == itw_mo_sel || (mo_closing && i == mo_last); const float cp = open ? mo12_p(i) : 0.0f;
    const int a = open ? 256 : mo11_fade();   /* others wash toward the parchment while open */
    const int x0 = T.x0, x1 = T.x1, yt = T.yt, yb = T.yb, fw = x1 - x0;
    /* close-up: deepen the pillar along its own receding edges (x and y extrapolated on the same rays) so the side + cap read as real 3D */
    const float k = 1.0f + 1.4f * cp;
    const int bx0 = x0 + (int)lv_lroundf((T.bx0 - x0) * k), bx1 = x1 + (int)lv_lroundf((T.bx1 - x1) * k), byt = yt + (int)lv_lroundf((T.byt - yt) * k), byb = yb + (int)lv_lroundf((T.byb - yb) * k);
    const uint16_t PAP = lv_hex(MO11_PAPER), DK = lv_hex(0x100c09), G = lv_hex(MO11_G), GH = lv_hex(MO11_GH);
    const int pitch = fw >= 40 ? 7 : fw >= 14 ? 5 : 4;   /* pane row pitch (front + side share it so the rows line up round the corner) */
    const int S = fw >= 60 ? 48 : fw >= 38 ? 32 : fw >= 20 ? 16 : fw >= 12 ? 8 : 0, wy0 = S ? yt + 3 + S + 8 : yt + (fw >= 7 ? 10 : 3);
    /* side face: scanned per column; timber graded from the lit corner (front) to dark (back), perspective pane rows + posts */
    auto side = [&](int xf, int xbk) {
        const int dir = xbk > xf ? 1 : -1, n = abs(xbk - xf); if (n < 1) return;
        for (int s = 0; s <= n; ++s) { const int x = xf + dir * s; const float u = s / (float)n;
            const int top = (int)lv_lroundf(yt + (byt - yt) * u), bot = (int)lv_lroundf(yb + (byb - yb) * u);
            const uint16_t base = mo13_mix(0x7a6342, 0x241b12, s, n);   /* graded shading */
            for (int y = top; y <= bot; ++y) mo13_px(B, x, y, base, a); }
        /* perspective shoji ribs: one per pane row, a dark kumiko line + a lit line under it, both on the receding rays */
        if (n >= 3) for (int yf = wy0; yf < yb - 1; yf += 2 * pitch) { const float v = (yf - yt) / (float)(yb - yt);
            const int ya = yf, yb2 = (int)lv_lroundf(byt + (byb - byt) * v);
            mo13_line(B, xf + dir, ya, xbk, yb2, lv_hex(0x1a140e), a); mo13_line(B, xf + dir, ya + 1, xbk, yb2 + 1, lv_hex(0xa88e62), a * 2 / 3); }
        if (n >= 8) for (int q = 1; q <= 2; ++q) { const int s = n * q / 3, x = xf + dir * s; const float u = s / (float)n;   /* posts */
            mo13_line(B, x, (int)lv_lroundf(yt + (byt - yt) * u), x, (int)lv_lroundf(yb + (byb - yb) * u), lv_hex(0x1e1610), a); }
        mo13_line(B, xf + dir, yt + 1, xf + dir, yb - 1, lv_hex(0xd8c6a0), a);          /* lit corner edge */
        mo13_line(B, xbk, byt, xbk, byb, lv_hex(0x120e0a), a); mo13_line(B, xf, yt, xbk, byt, lv_hex(0xcbb894), a); mo13_line(B, xf, yb, xbk, byb, DK, a);
    };
    if (bx1 > x1) side(x1, bx1);
    if (bx0 < x0) side(x0, bx0);
    /* top cap: lit timber with an inset darker panel and a bright front lip */
    if (byt < yt) { const float X[4] = {(float)x0, (float)bx0, (float)bx1, (float)x1}, Y[4] = {(float)yt, (float)byt, (float)byt, (float)yt}; mo_quad(B, X, Y, lv_hex(0xa88e62), a);
        if (yt - byt >= 4) { const float cx[4] = {(float)x0 + 3, (float)bx0 + 2, (float)bx1 - 2, (float)x1 - 3}, cy[4] = {(float)yt - 1, (float)byt + 1, (float)byt + 1, (float)yt - 1}; mo_quad(B, cx, cy, lv_hex(0x6b5638), a); }
        mo13_line(B, bx0, byt, bx1, byt, lv_hex(0xcbb894), a); mo13_line(B, x0, yt, bx0, byt, PAP, a); mo13_line(B, x1, yt, bx1, byt, lv_hex(0xcbb894), a); }
    /* front face: lacquer with a bevel (2 px lit inner frame top / left, shaded right column blending into the side) */
    for (int y = yt; y <= yb; ++y) mo13_span(B, y, x0, x1, lv_hex(0x2e241a), a);
    if (fw >= 6) { mo13_line(B, x0 + 1, yt + 1, x0 + 1, yb - 1, lv_hex(0x7a6342), a); mo13_line(B, x0 + 1, yt + 1, x1 - 1, yt + 1, lv_hex(0x7a6342), a);
                   mo13_line(B, x1 - 1, yt + 1, x1 - 1, yb - 1, lv_hex(0x1e1610), a); if (fw >= 30) { mo13_line(B, x0 + 2, yt + 2, x0 + 2, yb - 1, lv_hex(0x4e3e2c), a); mo13_line(B, x1 - 2, yt + 2, x1 - 2, yb - 1, lv_hex(0x241c14), a); } }
    /* icon plate (integer-scaled icon) or, on far pillars, a crisp 3x5 kind glyph */
    if (S) { const int px0 = (x0 + x1) / 2 - S / 2 - 2, py0 = yt + 3;
        for (int y = py0; y < py0 + S + 4; ++y) mo13_span(B, y, px0, px0 + S + 3, DK, a);
        mo13_line(B, px0, py0 + S + 4, px0 + S + 3, py0 + S + 4, G, a); mo13_line(B, px0, py0 - 1, px0 + S + 3, py0 - 1, lv_hex(0x7a6342), a);
        mo13_icon(B, i, px0 + 2, py0 + 2, S, a); }
    else if (fw >= 7) { const int gx = (x0 + x1) / 2 - 2, gy = yt + 2;
        for (int y = gy; y < gy + 7; ++y) mo13_span(B, y, gx, gx + 4, DK, a);
        for (int r = 0; r < 5; ++r) for (int c = 0; c < 3; ++c) if (MO13_GLY[MO_KIND[i]][r][c] == '#') mo13_px(B, gx + 1 + c, gy + 1 + r, GH, a); }
    /* fine shoji grid: thin panes (cell ~4..8 px) with lit edges + inner shading; a few dark and green panes */
    if (fw >= 5) {
        const int ix0 = x0 + (fw >= 30 ? 4 : 2), ix1 = x1 - (fw >= 30 ? 4 : 2), iw = ix1 - ix0 + 1, cols = iw / (fw >= 40 ? 8 : 4) < 2 ? 2 : iw / (fw >= 40 ? 8 : 4);
        for (int y = wy0, row = 0; y + pitch - 1 < yb - 1; y += pitch, ++row) for (int c = 0; c < cols; ++c) {
            const int xa = ix0 + c * iw / cols, xb = ix0 + (c + 1) * iw / cols - 2;
            const int h = (c * 7 + row * 13 + i * 5 + (int)(lv_now / 1500) * (row == (i % 5))) % 11;
            mo13_pane(B, xa, y, xb, y + pitch - 2, h == 0 ? 2 : h <= 2 ? 1 : 0, a); }
    }
    /* crisp outer edges + Matrix-green seam with a moving bright pulse */
    mo13_line(B, x0, yt, x0, yb, PAP, a); mo13_line(B, x0, yt, x1, yt, PAP, a); mo13_line(B, x1, yt, x1, yb, lv_hex(0xb8a47c), a); mo13_line(B, x0, yb, x1, yb, DK, a);
    if (fw >= 8) { const int sx = x1 - (fw >= 30 ? 3 : 1); mo13_line(B, sx, yt + 2, sx, yb - 1, G, a * 3 / 4);
        const int hgt = yb - yt; if (hgt > 8) { const int py = yb - 2 - (int)(fmodf(lv_now / 1000.0f * (0.6f + 0.05f * i) + i * 0.37f, 1.0f) * (hgt - 6)); mo13_line(B, sx, py, sx, py + 3, GH, a); } }
}
/* ---- v12 Dojo folders (the DOWNLOAD / INFO folder structure: tab, icon, name line, info lines, a small bar; Dojo paper + timber + ink) ---- */
static const char *const MO12_TAB[4] = {"WHAT IT IS", "HOW IT IS SPOTTED", "HOW TO PROTECT YOURSELF", "SIGNAL  EX.DATA"};
static const char *const MO12_IC[4][8] = {   /* 8x8 ink glyphs: eye / signal waves / shield / meter */
    {"........", "..####..", ".#....#.", "#..##..#", "#..##..#", ".#....#.", "..####..", "........"},
    {"..####..", ".#....#.", "#..##..#", "..#..#..", "...##...", "........", "...##...", "...##..."},
    {"########", "#......#", "#..##..#", "#.####.#", "#..##..#", ".#....#.", "..#..#..", "...##..."},
    {"......##", "......##", "....####", "....####", "..######", "..######", "########", "########"}};
static int mo12_fys[4], mo12_fph100[4], mo12_typed = 0, mo12_nland = 0;   /* dump */
static inline void mo12_rect(LvBand &B, int x, int y, int w, int h, uint16_t c, int a) { for (int yy = b_ylo(B, y); yy < y + h && yy < B.y0 + B.h; ++yy) mo_hspan(B, yy, x, x + w - 1, c, a); }
static void mo12_line(LvBand &B, int x, int y, int xx, int yy, uint16_t c)   /* Bresenham (the INFO folders' own lines), clipped like mo_bl */
{ if ((y < B.y0 && yy < B.y0) || (y >= B.y0 + B.h && yy >= B.y0 + B.h)) return;   /* v1.2.11 speed: wholly outside this band */
  int dx = abs(xx - x), sx = x < xx ? 1 : -1, dy = -abs(yy - y), sy = y < yy ? 1 : -1, e = dx + dy;
  for (;;) { mo_bl(B, x, y, c, 256); if (x == xx && y == yy) break; int e2 = 2 * e; if (e2 >= dy) { e += dy; x += sx; } if (e2 <= dx) { e += dx; y += sy; } } }
static void mo12_aud(int x, int y, int w, int h, int bx0, int by0, int bx1, int by1, const char *what)   /* every v12 text line: inside its box, below the header, above the panel */
{
    mo_txt_n++; mo_txt_lines++;
    if (x < bx0 || y < by0 || x + w - 1 > bx1 || y + h - 1 > by1 || y < 36 || y + h - 1 > 246 || x < 0 || x + w - 1 > 239) { mo_txt_bad++; mo_txt_tot++; snprintf(mo_txt_why, sizeof mo_txt_why, "%s x=%d y=%d w=%d h=%d box=[%d,%d,%d,%d]", what, x, y, w, h, bx0, by0, bx1, by1); }
}
static int mo12_wrap(const char *s, int cols, char out[4][40])   /* word wrap to cols columns of the 3x5 font; returns lines (> 3 = does not fit) */
{
    int n = 0; for (int k = 0; k < 4; ++k) out[k][0] = 0; const char *p = s; char w[40];
    while (*p) {
        while (*p == ' ') ++p; int k = 0; while (p[k] && p[k] != ' ' && k < 39) { w[k] = p[k]; ++k; } w[k] = 0; if (!k) break;
        const int L = (int)strlen(out[n]); if (L && L + 1 + k > cols) { if (++n >= 4) return 5; }
        if (out[n][0]) strcat(out[n], " "); strcat(out[n], w); p += k;
    }
    return n + 1;
}
static void mo13_folder_v131(LvBand &B, int i, int j, int ys, float ph, int tl)   /* v13.1 folder, kept for reference, not called */
{
    const uint16_t tim = lv_hex(MO11_BEAM), lip = lv_hex(0x6b5638), pap = lv_hex(MO11_PAPER), fib = lv_hex(0xe9dfca), ink = lv_hex(MO11_INK), g = lv_hex(MO11_G), sub = lv_hex(MO11_SEAM);
    const int x = MO12_FX, w = MO12_FW, y = ys + 8, h = MO12_FH, tw = 60; const bool solid = ph >= 0.65f;
    if (solid) { mo12_rect(B, x + 6, y + 5, w, h, lv_hex(0x1e1a16), 110); mo12_rect(B, x + 2, y - 6, 60, 4, lv_hex(0x1e1a16), 70); }   /* v13.1 drop shadow */
    if (solid) mo12_rect(B, x + 3, y + 2, w, h, lip, 236);   /* 3D lip block */
    { const uint16_t ol = lv_hex(0x1e1a16); mo12_line(B, x + w, y + 2, x + w + 3, y + 5, ol); mo12_line(B, x + w + 3, y + 5, x + w + 3, y + h + 1, ol); mo12_line(B, x + 3, y + h + 1, x + w + 3, y + h + 1, ol); }
    if (solid) { mo12_rect(B, x, y, w, h, pap, 256); for (int yy = y + 3; yy < y + h - 1; yy += 4) mo_hspan(B, yy, x + 4, x + w - 2, fib, 256);   /* paper fibres (INFO's scanlines) */
                 mo12_rect(B, x, y - 7, tw, 8, tim, 256); for (int k = 0; k < 5; ++k) mo_bl(B, x + tw - 4 + k, y - 7 + k, tim, 256); }
    mo12_line(B, x, y - 8, x + tw - 5, y - 8, tim); mo12_line(B, x + tw - 5, y - 8, x + tw, y - 3, tim); mo12_line(B, x + tw, y - 3, x + tw, y, tim); mo12_line(B, x, y - 8, x, y + h - 1, tim);
    mo12_line(B, x + tw, y, x + w - 1, y, tim); mo12_line(B, x + w - 1, y, x + w - 1, y + h - 1, tim); mo12_line(B, x, y + h - 1, x + w - 1, y + h - 1, tim);
    if (!solid) return;
    { const uint16_t ol = lv_hex(0x1e1a16);   /* v13.1 ink outline (top / left / tab), separates the folder from the parchment wall */
      mo12_line(B, x - 1, y - 9, x + tw - 4, y - 9, ol); mo12_line(B, x + tw - 4, y - 9, x + tw + 1, y - 4, ol); mo12_line(B, x + tw + 1, y - 4, x + tw + 1, y - 1, ol);
      mo12_line(B, x + tw + 1, y - 1, x + w, y - 1, ol); mo12_line(B, x - 1, y - 9, x - 1, y + h, ol); mo12_line(B, x - 1, y + h, x + 3, y + h, ol); }
    mo12_rect(B, x + 1, y + 2, 2, h - 4, g, 230);   /* Matrix green accent strip */
    /* v13 content: the DOWNLOAD folder rows (tab, icon + one big file name, big size on the right, 70x7 bar + action line), Dojo inks */
    static const char *const MO13_FN[12][3] = {
        {"ROUTER.NET", "BEACON.SIG", "WPA3.DEF"},   {"BT.RADIO", "ADVERT.SIG", "PAIR.DEF"},  {"DEAUTH.PKT", "FLOOD.SIG", "PMF.DEF"},
        {"OPEN.NET", "WEP.SIG", "WPA3.DEF"},        {"FLOCK.CAM", "SSID.SIG", "PLATE.DEF"},  {"DRONE.UAV", "REMOTEID.SIG", "REPORT.DEF"},
        {"AIRTAG.TRK", "FINDMY.SIG", "BAGCHK.DEF"}, {"TILE.TRK", "FEED.SIG", "TAGSCAN.DEF"}, {"META.CAM", "MAKERID.SIG", "LEDCHK.DEF"},
        {"SKIMMER.CRD", "HC05.SIG", "TAPPAY.DEF"},  {"FLIPPER.RF", "NAME.SIG", "POPUP.DEF"},  {"SMARTTAG.TRK", "FD5A.SIG", "TAGSCAN.DEF"}};
    static const char *const MO13_ACT[3] = {"WHAT IT IS", "HOW SPOTTED", "PROTECT"};
    char tab[16]; snprintf(tab, sizeof tab, "SCROLL %d", j + 1); b_text35(B, x + 5, y - 6, tab, pap);
    if (B.y0 == 0) mo12_aud(x + 5, y - 6, lv_w35(tab), 5, x + 1, y - 7, x + tw - 5, y - 1, "tab");
    static const int IC[3] = {0, 2, 3};   /* the INFO folder icon set (box / scope / shield-ish), drawn in ink */
    { const uint32_t ic = 0x2a2118; lvi_box(B, x + 5, y + 6, 8, 8, ic);
      if (IC[j] == 0) { lvi_box(B, x + 7, y + 8, 4, 4, ic); lvi_px(B, x + 8, y + 9, 0x00a83c); }
      if (IC[j] == 2) { lvi_box(B, x + 7, y + 8, 4, 4, ic); lvi_line(B, x + 5, y + 9, x + 12, y + 9, ic); }
      if (IC[j] == 3) { lvi_line(B, x + 6, y + 7, x + 11, y + 7, ic); lvi_line(B, x + 6, y + 7, x + 7, y + 12, ic); lvi_line(B, x + 11, y + 7, x + 9, y + 12, ic); } }
    const char *nm = MO13_FN[i][j]; b_text(B, x + 17, y + 7, nm, ink);
    const char *src = j == 0 ? MO_DEV[i].what : j == 1 ? MO_DEV[i].spot : MO_DEV[i].prot; unsigned hsh = 7; for (const char *q = nm; *q; ++q) hsh = hsh * 31 + (unsigned char)*q; const int kb10 = 6 + (int)strlen(src) % 9 + (int)(hsh % 31);   /* EX.DATA size */
    char sz[12]; snprintf(sz, sizeof sz, "%d.%dKB", kb10 / 10, kb10 % 10); const int sw = (int)strlen(sz) * 6 - 1, sx = x + w - 4 - sw;
    b_text(B, sx, y + 20, sz, g);
    b_text35(B, x + 5, y + 22, MO13_ACT[j], sub);
    const int bw = 70, fill = (bw - 4) * (tl < 0 ? 0 : tl > MO12_LOAD_MS ? MO12_LOAD_MS : tl) / MO12_LOAD_MS;
    b_frame(B, x + 4, y + 34, bw, 7, tim); if (fill > 0) mo12_rect(B, x + 6, y + 36, fill, 3, g, 256);
    char st[16]; if (tl >= MO12_LOAD_MS) snprintf(st, sizeof st, "LOADED"); else snprintf(st, sizeof st, "%d%%", tl < 0 ? 0 : 100 * tl / MO12_LOAD_MS);
    b_text35(B, x + w - 4 - lv_w35(st), y + 35, st, sub);
    if (B.y0 == 0) { mo12_aud(x + 17, y + 7, (int)strlen(nm) * 6 - 1, 7, x + 16, y + 3, x + w - 4, y + 17, "fname");
                     mo12_aud(sx, y + 20, sw, 7, x + 60, y + 18, x + w - 4, y + 30, "fsize");
                     mo12_aud(x + 5, y + 22, lv_w35(MO13_ACT[j]), 5, x + 2, y + 18, sx - 3, y + 30, "fact");
                     mo12_aud(x + w - 4 - lv_w35(st), y + 35, lv_w35(st), 5, x + bw + 6, y + 32, x + w - 3, y + h - 2, "fst"); }

}
#include "botpheus/mo_mix.inc"
/* ---- v12 line-item search bar across the top of the tower + its index line down to the folders ---- */
static void mo12_search(LvBand &B, const LvTower &T, int i, int el)
{
    const int t = el - MO12_BAR0; if (t < 0) return;
    const uint16_t ch = lv_hex(MO11_CHAR), tim = lv_hex(MO11_BEAM), g = lv_hex(MO11_G), gh = lv_hex(MO11_GH), pap = lv_hex(0xe8dec8);
    const int x0 = MO12_BX, y0 = MO12_BY, w = MO12_BW, h = MO12_BH;
    const int wv = (int)(w * mo_ease(t / (float)MO12_WIPE_MS));
    /* hanger: timber posts from the bar to the pillar's front-top corners + a green pointer at its seam */
    if (t >= MO12_WIPE_MS) { const int cx = (T.x0 + T.x1) / 2, yt = T.yt;
        if (yt > y0 + h) { mo_aline(B, (float)T.x0 + 4, (float)(y0 + h), (float)T.x0 + 4, (float)yt, tim, 220); mo_aline(B, (float)T.x1 - 4, (float)(y0 + h), (float)T.x1 - 4, (float)yt, tim, 220);
            for (int r = 0; r < 4; ++r) mo_hspan(B, y0 + h + r, cx - 3 + r, cx + 3 - r, g, 240); } }
    mo12_rect(B, x0, y0, wv, h, ch, 256);
    if (wv < w) { mo12_rect(B, x0 + wv - 2, y0, 2, h, gh, 256); return; }
    b_frame(B, x0, y0, w, h, tim); mo_hspan(B, y0 + 1, x0 + 1, x0 + w - 2, ((lv_now / 500) & 1) ? g : lv_hex(0x2e5a34), 256); mo_hspan(B, y0 + h - 2, x0 + 1, x0 + w - 2, lv_hex(0x3a3228), 256);
    const char *nm = MO_DEV[i].name; const int L = (int)strlen(nm), ty = t - MO12_WIPE_MS;
    int typed = ty < 0 ? 0 : ty / MO12_TYPE_MS; if (typed > L) typed = L; mo12_typed = typed;
    int x = b_text35(B, x0 + 5, y0 + 5, "SEARCH", gh); x = b_text35(B, x + 3, y0 + 5, "//", lv_hex(0x8a7a5a));
    char q[24]; snprintf(q, sizeof q, "%.*s", typed, nm); const int nx = x + 4, ne = b_text(B, nx, y0 + 4, q, pap);
    if (typed < L || ((lv_now / 400) & 1)) mo12_rect(B, ne + 1, y0 + 4, 5, 7, gh, 256);   /* blinking green block cursor (solid while typing) */
    char r[24]; snprintf(r, sizeof r, "%d/3 SCROLLS", mo12_nland); const int rx = x0 + w - 5 - lv_w35(r); b_text35(B, rx, y0 + 5, r, lv_hex(0xbfb294));
    if (B.y0 == 0) { mo12_aud(x0 + 5, y0 + 5, x - 4 - (x0 + 5), 5, x0 + 2, y0 + 2, x0 + w - 3, y0 + h - 3, "sbarL"); mo12_aud(nx, y0 + 4, L * 6 + 6, 7, x0 + 2, y0 + 2, rx - 4, y0 + h - 3, "sbarQ");
                     mo12_aud(rx, y0 + 5, lv_w35(r), 5, nx + L * 6 + 8, y0 + 2, x0 + w - 3, y0 + h - 3, "sbarR"); }
}
static void mo12_index(LvBand &B, int nland, const int *ys)   /* the line items: a green index line from the bar down the folder column, a tick + node per landed folder */
{
    if (nland <= 0) return; const uint16_t g = lv_hex(MO11_G); const int xl = MO12_FX - 3, yb = ys[nland - 1] + 12;
    for (int y = MO12_BY + MO12_BH; y <= yb; ++y) mo_bl(B, xl, y, g, 230);
    for (int j = 0; j < nland; ++j) { const int yy = ys[j] + 12; mo_hspan(B, yy, xl, MO12_FX - 1, g, 230); mo12_rect(B, xl - 1, yy - 1, 3, 3, lv_hex(MO11_GH), 256); }
}

/* ---- per frame: the Gibson layout on the Morpheus world (called while swapped, skin 1 set so lv_rd_place runs mo11_drift) ---- */
static void mo11_layout(void)
{
    const float t = mo_t();
#ifdef MO_EB2
    if (itw_mo_sel >= 0) {   /* EB2: the open pillar's real example threat owns the top banner (HIGH = Gibson alert flash) */
        char m[LV_TKLEN];
        if (mo_live(itw_mo_sel)) snprintf(m, sizeof m, "%s  THREAT: %s  LIVE %d SEEN  BEST %d DBM", MO_DEV[itw_mo_sel].name, MO_TNAME[MO_THREAT[itw_mo_sel]], mo_live_n[itw_mo_sel], mo_live_db[itw_mo_sel]);
        else if (mo_live_n[itw_mo_sel]) snprintf(m, sizeof m, "%s  THREAT: %s  LIVE %d FRAMES", MO_DEV[itw_mo_sel].name, MO_TNAME[MO_THREAT[itw_mo_sel]], mo_live_n[itw_mo_sel]);
        else snprintf(m, sizeof m, "%s  THREAT: %s  EX.DATA", MO_DEV[itw_mo_sel].name, MO_TNAME[MO_THREAT[itw_mo_sel]]);
        if (strcmp(m, lv_tk_msg)) { snprintf(lv_tk_msg, sizeof lv_tk_msg, "%s", m); lv_tk_off = LV_R_TICK.x1 - LV_R_TICK.x0 - 4; }
        lv_tk_alert = MO_THREAT[itw_mo_sel] == 0; }
    else {   /* EB3: idle banner = the real example threat count over the 18 example devices */
        int nh = 0, nm = 0, nl = 0; for (int k = 0; k < MO_N; ++k) { if (MO_THREAT[k] == 0) ++nh; else if (MO_THREAT[k] == 1) ++nm; else ++nl; }
        char m[LV_TKLEN];
        if (mo_live_np) snprintf(m, sizeof m, "LIVE SCAN: %d FINDS IN %d PILLARS - TAP A PILLAR", mo_live_tot, mo_live_np);
        else if (lv_job != LV_J_NONE) snprintf(m, sizeof m, "SCANNING... %d THREATS: %d HIGH %d MED %d LOW  EX.DATA", nh + nm + nl, nh, nm, nl);
        else snprintf(m, sizeof m, "%d THREATS: %d HIGH %d MED %d LOW - TAP A PILLAR  EX.DATA", nh + nm + nl, nh, nm, nl);
        if (strcmp(m, lv_tk_msg)) { snprintf(lv_tk_msg, sizeof lv_tk_msg, "%s", m); lv_tk_off = LV_R_TICK.x1 - LV_R_TICK.x0 - 4; }
        lv_tk_alert = false; }
#endif
    lv_rd_hunting = false; lv_rd_sel = -1; lv_rd_glow = false; lv_rd_holding = false;
    if (itw_mo_sel < 0 && !mo_closing) { lv_rd_lock = false; lv_rd_layout(); }   /* idle: EXACTLY the Gibson radar's layout (camera law + place + clamp) */
    else {
        const float idle = lv_rainmode == 0 ? 0.5f * sinf(t * 0.45f) : 0.0f;
        if (itw_mo_sel >= 0) {   /* the push: Gibson's camera eases to the open framing (DOWNLOAD's camera-to-the-tower move) */
            const float u = mo12_p(itw_mo_sel);
            lv_rd_camx = idle + (mo12_cam(t) - idle) * u;
            lv_rd_lock = true; lv_rd_lock_t = itw_mo_t0; memcpy(lv_rd_lockmac, lv_f[itw_mo_sel].mac, 6);
            lv_rd_place();
        } else {
            const float u = mo_ease((lv_now - mo_close_t) / (float)MO12_CLOSE_MS);
            lv_rd_camx = mo12_cam(t) + (idle - mo12_cam(t)) * u; lv_rd_lock = false;
            lv_rd_place(); if (u >= 1) mo_closing = false;
        }
    }
    itw_capture(); mo11_cam = lv_rd_camx; mo11_nord = 0;
    for (int i = 0; i < MO_N; ++i) { mo11_have[i] = false; mo_eS[i] = 0; }
    for (int n = 0; n < lv_rd_n; ++n) { const LvTower &T = lv_rd_tw[n]; if (T.i < 0 || T.i >= MO_N) continue; mo11_tw[T.i] = T; mo11_have[T.i] = true; MO_ORD[mo11_nord++] = T.i;
        int xa, xb, ya, yb; lv_rd_box(T, 3, xa, xb, ya, yb);
        itw_hx0[T.i] = xa < 0 ? 0 : xa; itw_hx1[T.i] = xb > 239 ? 239 : xb; itw_hy0[T.i] = ya < MO_FY0 ? MO_FY0 : ya; itw_hy1[T.i] = yb > MO_FY1 ? MO_FY1 : yb;
        itw_seen[T.i] = itw_hx1[T.i] >= itw_hx0[T.i] && itw_hy1[T.i] >= itw_hy0[T.i]; }
}
static void mo11_overlay(LvBand &B)   /* skin-1 hook: called by lv_scene_radar after its tower loop (instead of the Gibson fx / header / panel) */
{
    if (B.y0 == 0) { mo_txt_bad = mo_txt_n = 0; mo12_typed = 0; for (int j = 0; j < 4; ++j) { mo12_fys[j] = -999; mo12_fph100[j] = -1; } }
    const int sel = itw_mo_sel; const uint32_t el = lv_now - itw_mo_t0;
    if (sel >= 0 && mo11_have[sel]) {
        const LvTower &T = mo11_tw[sel];
        /* v13: no select effect (Mobandy: no lock bracket) */
        /* v13: red pill removed */
        int ys[4], nland = 0; float ph[4];
        for (int j = 0; j < 3; ++j) { const bool go = lv_sel_drop((int)el, MO12_DROP0, MO12_GAP, MO12_DROP_MS, j, LV_RD_Y0 - 50, MO12_FY0 + j * MO12_FDY, ys[j], ph[j]);   /* lv_select.h drop law */
            if (go && ph[j] >= LV_SEL_LAND) nland = j + 1; }
        mo12_nland = nland;
        mo12_index(B, nland, ys);
        for (int j = 2; j >= 0; --j) { const int tt = (int)el - MO12_DROP0 - j * MO12_GAP; if (tt < 0) continue;
            mo_cy0 = 35; mo12_folder(B, sel, j, ys[j], ph[j] > 1 ? 1 : ph[j], tt - MO12_DROP_MS); mo_cy0 = -10000;
            if (B.y0 == 0) { mo12_fys[j] = ys[j]; mo12_fph100[j] = (int)(100 * (ph[j] > 1 ? 1 : ph[j])); } }
        if (mo_fold_sel >= 0 && mo_fold_sel < nland) {   /* v1.2.11: the opened folder wears a 2 px green frame */
            const int fy = ys[mo_fold_sel] + 8; const uint16_t g = lv_hex(MO11_GH);
            if (b_yhit(B, fy - 13, fy + MO12_FH + 3)) { b_frame(B, MO12_FX - 4, fy - 13, MO12_FW + 8, MO12_FH + 17, g); b_frame(B, MO12_FX - 3, fy - 12, MO12_FW + 6, MO12_FH + 15, g); } }
        mo12_search(B, T, sel, (int)el);
        mo_panel(B);
    }
    mo_terminal(B); mo_chrome(B);
    lv_draw_backbot(B, LV_R_BACKBOT.x0, LV_R_BACKBOT.y0);
}
static void itw_mo_scene(LvBand &B, void *)
{
    lv_hc_swap();
    lv_rd_skin_tw = MO11_TW; lv_rd_skin_th = MO11_TH;
    const int rain = lv_rain; lv_rain = 1;   /* the Construct always shows the code rain */
    lv_rd_skin = 1;
    /* v1.2.11 speed: the layout is a function of the frame clock only, so it runs once per frame (first band, y 24)
     * instead of once per band; every later band of the same frame reuses the same towers / camera (0 px gate). */
    if (B.y0 <= 24) mo11_layout();           /* skin 1 during layout: lv_rd_place runs mo11_drift */
    mo12_B = &B;
    lv_rd_skin_fa = MO11_FA; lv_scene_radar(B, 0); lv_rd_skin = 0; lv_rd_skin_fa = 3; lv_rd_skin_tw = 1.0f; lv_rd_skin_th = 1.0f;
    mo12_B = nullptr;
    lv_rain = rain;
    lv_hc_swap();
}
static void itw_mo_enter(void)
{
    mo_build(); itw_nt = MO_N; itw_mo_sel = -1; itw_mo_phase = 0; itw_mo_nav = false; mo_closing = false; mo_last = -1;
    lv_sel_clk_reset(mo_clk, lv_now); mo_fold_sel = -1;
    LvHcWorld &w = lv_hc_w; memset(&w, 0, sizeof w);
    for (int i = 0; i < MO_N; ++i) {
        snprintf(itw_lab[i], sizeof itw_lab[i], "%s", MO_DEV[i].name); itw_icon[i] = MO_ICON[i];
        LvFind &f = w.f[i]; const uint8_t mac[6] = {0x4D, 0x11, (uint8_t)(i + 1), 0x00, 0xD0, (uint8_t)(0x10 + i)}; memcpy(f.mac, mac, 6);
        f.rssi = MO11_RSSI[i]; f.kind = (uint8_t)(i & 1); f.mfg = 0xFFFF; snprintf(f.name, sizeof f.name, "%s", MO_DEV[i].name);
    }
    w.nf = MO_N; w.t0 = lv_now; w.sel = -1; mo_tw0 = lv_now;
    snprintf(w.tl[0], sizeof w.tl[0], "DOJO CONSTRUCT  18 PILLARS");
    mo_live_update();
    if (lv_nf == 0 && lv_job == LV_J_NONE) lv_job_start(lv_scanmix == 1 ? LV_J_WIFI : lv_scanmix == 2 ? LV_J_BLE : LV_J_ALL, -1);   /* v1.2.10: a live feed, as GIBSON's lv_radar_enter */
}
static void itw_mo_frame(void)
{
    mo_emb_mark_frame();
    mo_live_update();   /* v1.2.10: real finds -> pillars, before the render swaps the example world in */
    const uint32_t real = lv_now; uint32_t cap = LV_SEL_NOCAP;
    if (itw_mo_sel >= 0 || mo_closing) {   /* v1.2.11: cap the step while the camera / folders move (lv_select.h clock) */
        const uint32_t el = mo_clk.vt - itw_mo_t0;
        cap = mo_closing || el < MO12_PUSH_MS ? MO_DT_PUSH : el < (uint32_t)(MO12_DROP0 + 2 * MO12_GAP + MO12_DROP_MS) ? MO_DT_DROP : LV_SEL_NOCAP; }
    lv_now = lv_sel_clk_step(mo_clk, real, cap);
    if (itw_mo_sel >= 0) { const uint32_t el = lv_now - itw_mo_t0;
        itw_mo_phase = el < MO12_PUSH_MS ? 1 : el < MO12_DROP0 + 2 * MO12_GAP + MO12_DROP_MS ? 2 : 3; }
    /* v1.2.5: dirty-rect the field only (y=24..319). Never clear or repaint the ticker band;
     * a full-frame blit was overwriting the banner every tick, then itw_top black-filled it. */
    lv_render(lv_blit_dirty_hw, {0, 24, 239, 319}, itw_mo_scene, 0);   /* perf-all: changed rows only (was lv_blit_hw: every row, every frame) */
    lv_now = real;
    itw_top();
}
static int mo_hit(int x, int y)   /* v11 tap assist for drifting pillars: every field point belongs to the pillar whose centre line (its front-face
                                     centre column, from lv_rd_box top to bottom) is nearest, front-first on ties. Hidden / far pillars always keep a cell. */
{
    if (y < MO_FY0 || y > MO_FY1) return -1;
    int best = -1; long long bd = 1LL << 40;   /* fw-3locks-1116: long is 32-bit on the ESP32 (1L << 40 overflowed -> no pillar ever hit); same values on the host */
    for (int k = mo11_nord - 1; k >= 0; --k) { const int i = MO_ORD[k]; if (!itw_seen[i]) continue;
        const int dx2 = 2 * x - itw_fx0[i] - itw_fx1[i], dy = y < itw_hy0[i] ? itw_hy0[i] - y : y > itw_hy1[i] ? y - itw_hy1[i] : 0;
        const long long d = (long long)dx2 * dx2 + 4LL * dy * dy; if (d < bd) { bd = d; best = i; } }
    return best;
}
static void mo12_plan(int i)   /* at tap: the open camera centre = DOWNLOAD's lv_dl_cam_for law (this pillar's spot at the close-up column), kept within +-1.2 */
{
    const LvTower &T = mo11_tw[i]; float c = T.X - (MO12_SX - 120) * T.Z / LV_RD_F; if (c > 1.2f) c = 1.2f; if (c < -1.2f) c = -1.2f;
    mo12_camc = c; mo12_bigbuild(i);
}
static int mo_fold_hit(int x, int y)   /* v1.2.11: landed folder j under (x,y), else -1. Boxes: x 88..238 (folder +-4),
                                          y split at the midpoints between the landed folders (51..115 / 116..177 / 178..242). */
{
    if (itw_mo_sel < 0 || x < MO12_FX - 4 || x > MO12_FX + MO12_FW + 3) return -1;
    const int el = (int)(mo_vnow() - itw_mo_t0);
    static const LvRect R[3] = {{(int16_t)(MO12_FX - 4), (int16_t)(MO12_FY0 - 9), (int16_t)(MO12_FX + MO12_FW + 3), (int16_t)(MO12_FY0 + MO12_FDY - 7)},
                                {(int16_t)(MO12_FX - 4), (int16_t)(MO12_FY0 + MO12_FDY - 6), (int16_t)(MO12_FX + MO12_FW + 3), (int16_t)(MO12_FY0 + 2 * MO12_FDY - 7)},
                                {(int16_t)(MO12_FX - 4), (int16_t)(MO12_FY0 + 2 * MO12_FDY - 6), (int16_t)(MO12_FX + MO12_FW + 3), (int16_t)(MO12_FY0 + 2 * MO12_FDY + 8 + MO12_FH + 2)}};
    const int j = lv_sel_hit(R, 3, x, y);   /* lv_select.h: landed folders only */
    return j >= 0 && lv_sel_landed(el, MO12_DROP0, MO12_GAP, MO12_DROP_MS, j) ? j : -1;
}
static const char *itw_mo_tap(int x, int y)
{
    if (y <= 23) { lv_go_soon(LV_SC_HOME, 0); return "mo-ticker"; }
    if (y >= 287) { lv_term_from_mo = true; lv_go_soon(LV_SC_TERM, 0); return "mo-terminal"; }
    if (itw_mo_sel >= 0) {
        if (lv_in(MO_BTN, x, y)) { mo_last = itw_mo_sel; mo_closing = true; mo_close_t = mo_vnow(); itw_mo_sel = -1; itw_mo_phase = 0; mo_fold_sel = -1; return "mo-towers"; }
        const int j = mo_fold_hit(x, y);
        if (j >= 0) {   /* v1.2.11: folder = button (press flash on the whole folder, then open / close it) */
            const int ty = MO12_FY0 + j * MO12_FDY;
            lv_press({(int16_t)(MO12_FX - 4), (int16_t)(ty - 2), (int16_t)(MO12_FX + MO12_FW + 3), (int16_t)(ty + 8 + MO12_FH + 2)});
            mo_fold_sel = mo_fold_sel == j ? -1 : j;
            static const char *const T[3] = {"mo-scroll1", "mo-scroll2", "mo-scroll3"}; return T[j];
        }
        return "mo-open";
    }
    if (mo_closing) return "mo-camera-moving";
    const int i = mo_hit(x, y);
    if (i >= 0) { mo12_plan(i); itw_mo_sel = i; itw_mo_phase = 1; itw_mo_t0 = mo_vnow(); mo_fold_sel = -1; return MO_DEV[i].name; }
    return "mo-none";
}
static const char *mo_fold_line(int i, int j, int k)   /* footer lines for an opened folder: its real name + fact */
{
    static char L[48];
    if (k == 0) snprintf(L, sizeof L, "SCROLL %d OPEN  %s  %s", j + 1, MO_DEV[i].name, MO_FOLD_NAME[i][j]);
    else snprintf(L, sizeof L, "%s", MO_FOLD_ACT[i][j]);
    return L;
}
static void mo_dump(void)
{
    const int cu = itw_mo_sel >= 0 ? itw_mo_sel : mo_closing ? mo_last : -1;
    printf("  MOCAM camx=%.4f sel=%d closing=%d lock=%d txt_n=%d txt_bad=%d txt_lines_all=%ld txt_bad_all=%ld t=%.3f p=%.3f camc=%.4f ord=", mo11_cam, itw_mo_sel, mo_closing ? 1 : 0, itw_mo_sel >= 0 ? 1 : 0, mo_txt_n, mo_txt_bad, mo_txt_lines, mo_txt_tot, mo_t(), mo12_p(cu), mo12_camc);
    for (int k = 0; k < mo11_nord; ++k) printf("%d%s", MO_ORD[k], k < mo11_nord - 1 ? "," : "\n");
    for (int i = 0; i < MO_N; ++i) if (mo11_have[i]) { const LvTower &T = mo11_tw[i];
        printf("  MOTW %d X=%.4f Z=%.4f H=%.4f f=[%d,%d,%d,%d] b=[%d,%d,%d,%d] halo=%d dx=%.4f dz=%.4f hk=%.4f\n", i, T.X, T.Z, T.H, T.x0, T.x1, T.yt, T.yb, T.bx0, T.bx1, T.byt, T.byb, i < 12 ? mo_hn[i] : 0, mo11_dxv[i], mo11_dzv[i], mo11_hk[i]); }
    for (int i = 0; i < MO_N; ++i) if (mo11_have[i]) printf("  MOEMB %d look=%d threat=%s hs=%.3f e=[%d,%d,%d]\n", i, MO_LOOKOF[i], MO_TNAME[MO_THREAT[i]], MO_HFIX[MO_LOOKOF[i]], mo_ex[i], mo_ey[i], mo_eS[i]);
    if (mo_txt_bad) printf("  MOWHY %s\n", mo_txt_why);
    if (cu >= 0 && mo11_have[cu]) { const LvTower &T = mo11_tw[cu]; printf("  MOCU %d p=%.3f f=[%d,%d,%d,%d] b=[%d,%d,%d,%d] fw=%d\n", cu, mo12_p(cu), T.x0, T.x1, T.yt, T.yb, T.bx0, T.bx1, T.byt, T.byb, T.x1 - T.x0); }
    if (itw_mo_sel >= 0) { printf("  MOFOLD nland=%d typed=%d ys=", mo12_nland, mo12_typed); for (int j = 0; j < 3; ++j) printf("%d/%d%s", mo12_fys[j], mo12_fph100[j], j < 2 ? "," : "\n"); }
}
#endif
