/* ---------- SCANbot game: HOT COLD HUNT (MOCK 2026-11-11, Mobandy: "Hot/Cold Hunt in the scan tower style") ----------
 * A NEW screen (LV_SC_HCH). It draws the GIBSON radar 1:1 by calling the radar's own scene (lv_scene_radar: sky, skyline,
 * 3D neon grid camera, rain, towers, lock brackets, D4 current, LOCKED A thermometer + V2 arc) and the radar's own top hunt
 * bar (lv_hunt_bar: frozen magenta/gold stripes + 11-bar meter + WARMER / COLDER words) with the radar globals swapped
 * for the game's world while it draws (lv_hc_swap), then writes the game text over the header strip, the panel and the
 * terminal legend in the same fonts / inks / frames. The towers are FAKE game nodes (names NODE 01.., MACs 5C:A9:..),
 * never scan data; the real scan list (lv_f) is untouched outside the draw calls. One node is the hidden target: tap a
 * tower to PROBE it; the probed tower locks (the radar's lock brackets + current) and its thermometer fills to the heat
 * = how close it stands to the target in the 3D city (world distance, 14 bands); the banner says COLD / WARM / HOT on the
 * first probe, then WARMER / COLDER against the last probe. Finding it fires the radar's lock finish (lock pop, full
 * meter, FOUND). Score = 1000 - 50 per probe - 10 per second. 8 towers; 8 probes in round 1, one fewer each round (min 4). LV_HC_GAME 0 = none. */
#if LV_HC_GAME
#include "lv_scanbot_face.h"
#define LV_HC_MAXT 12
#define LV_HC_PROBES lv_hc_nprobe()   /* 8 in round 1, one fewer each round, never below 4 */
enum { LV_HC_START, LV_HC_PLAY, LV_HC_FOUND, LV_HC_LOST, LV_HC_WIN, LV_HC_OVER };
/* the radar globals the game borrows while it draws (its own world: towers, lock, hunt meter, log) */
struct LvHcWorld {
    LvFind f[LV_MAXF]; int nf, sel; uint8_t selmac[6]; uint32_t t0; bool glow; uint32_t glow_t; bool holding;
    bool lock; uint32_t lock_t; uint8_t lockmac[6]; bool hunting; LvFind tgt; bool tgt_have; int trend, reads, miss;
    char tl[3][LV_RD_TW + 1]; uint32_t tl_t;
#if LV_RD_CHARGE
    bool cg; uint32_t cg_t, cg_rel_t; float cg_rel;
#if LV_RD_HUNT_METER
    LvRdHmEv hm_ev[LV_RD_HM_NEV]; int hm_nev, hm_lit, hm_pd; bool hm_on; uint32_t hm_ft;
#ifdef LV_RD_HM_SIM
    uint32_t hm_sim_t0;
#endif
#endif
#endif
#if LV_RD_INFO
    bool info;
#endif
};
static LvHcWorld lv_hc_w;
template <class T> static inline void lv_hc_sw(T &a, T &b) { T t = a; a = b; b = t; }
static void lv_hc_swap(void)   /* in <-> out; always called in pairs around radar code */
{
    LvHcWorld &w = lv_hc_w;
    for (int i = 0; i < LV_MAXF; ++i) lv_hc_sw(w.f[i], lv_f[i]);
    lv_hc_sw(w.nf, lv_nf); lv_hc_sw(w.sel, lv_rd_sel); for (int k = 0; k < 6; ++k) { lv_hc_sw(w.selmac[k], lv_rd_selmac[k]); lv_hc_sw(w.lockmac[k], lv_rd_lockmac[k]); }
    lv_hc_sw(w.t0, lv_rd_t0); lv_hc_sw(w.glow, lv_rd_glow); lv_hc_sw(w.glow_t, lv_rd_glow_t); lv_hc_sw(w.holding, lv_rd_holding);
    lv_hc_sw(w.lock, lv_rd_lock); lv_hc_sw(w.lock_t, lv_rd_lock_t); lv_hc_sw(w.hunting, lv_rd_hunting); lv_hc_sw(w.tgt, lv_rd_tgt);
    lv_hc_sw(w.tgt_have, lv_rd_tgt_have); lv_hc_sw(w.trend, lv_rd_trend); lv_hc_sw(w.reads, lv_rd_reads); lv_hc_sw(w.miss, lv_rd_miss);
    { char t[3][LV_RD_TW + 1]; memcpy(t, w.tl, sizeof t); memcpy(w.tl, lv_rd_tl, sizeof t); memcpy(lv_rd_tl, t, sizeof t); }
    lv_hc_sw(w.tl_t, lv_rd_tl_t);
#if LV_RD_CHARGE
    lv_hc_sw(w.cg, lv_rd_cg); lv_hc_sw(w.cg_t, lv_rd_cg_t); lv_hc_sw(w.cg_rel_t, lv_rd_cg_rel_t); lv_hc_sw(w.cg_rel, lv_rd_cg_rel);
#if LV_RD_HUNT_METER
    for (int i = 0; i < LV_RD_HM_NEV; ++i) lv_hc_sw(w.hm_ev[i], lv_rd_hm_ev[i]);
    lv_hc_sw(w.hm_nev, lv_rd_hm_nev); lv_hc_sw(w.hm_lit, lv_rd_hm_lit); lv_hc_sw(w.hm_pd, lv_rd_hm_pd); lv_hc_sw(w.hm_on, lv_rd_hm_on); lv_hc_sw(w.hm_ft, lv_rd_hm_ft);
#ifdef LV_RD_HM_SIM
    lv_hc_sw(w.hm_sim_t0, lv_rd_hm_sim_t0);
#endif
#endif
#endif
#if LV_RD_INFO
    lv_hc_sw(w.info, lv_rd_info);
#endif
}
/* game state */
static int lv_hc_round = 1;
static int lv_hc_nprobe(void) { return lv_hc_round < 5 ? 9 - lv_hc_round : 4; }
static int lv_hc_phase = LV_HC_START, lv_hc_n = 0, lv_hc_tgt = -1, lv_hc_probes = 0, lv_hc_pi = -1;
static int lv_hc_heat = 0, lv_hc_prev = -1, lv_hc_score = 0, lv_hc_best = -1;
static uint32_t lv_hc_t0 = 0, lv_hc_tend = 0, lv_hc_pt = 0; static bool lv_hc_shown = false;
static float lv_hc_X[LV_HC_MAXT], lv_hc_Z[LV_HC_MAXT];
struct LvHcHit { int i, xa, xb, ya, yb, cx; float Z; };
static LvHcHit lv_hc_hb[LV_HC_MAXT]; static int lv_hc_nhb = 0;
static const LvRect LV_HC_BTN_L = {18, 204, 117, 235}, LV_HC_BTN_R = {122, 204, 221, 235};   /* card buttons, drawn 100 x 32 */
static const uint16_t LV_HC_MAG = 0xF95A /* lv_hex(0xff2ad4) */;
static void lv_hc_log(const char *fmt, ...)
{
    char t[96]; va_list ap; va_start(ap, fmt); vsnprintf(t, sizeof t, fmt, ap); va_end(ap);
    lv_hc_swap(); lv_rd_log("%s", t); lv_hc_swap();
}
static int lv_hc_secs10(void) { return (int)(((lv_hc_phase == LV_HC_PLAY ? lv_now : lv_hc_tend) - lv_hc_t0) / 100); }   /* tenths */
static int lv_hc_rssi(int heat) { return -100 + (heat * 60 + 7) / 14; }   /* heat 0..14 -> the radar's 4..60 level scale (14 = -40) */
static float lv_hc_dmin = 0, lv_hc_dmax = 1;   /* nearest / farthest decoy from the target this round */
static float lv_hc_dist(int i) { const float dx = lv_hc_X[i] - lv_hc_X[lv_hc_tgt], dz = lv_hc_Z[i] - lv_hc_Z[lv_hc_tgt]; return sqrtf(dx * dx + dz * dz); }
static int lv_hc_heat_of(int i)   /* 3D city distance to the target, scaled per round: nearest decoy 12 (HOT), farthest 1 */
{
    if (i == lv_hc_tgt) return 14;
    const float u = lv_hc_dmax > lv_hc_dmin ? (lv_hc_dmax - lv_hc_dist(i)) / (lv_hc_dmax - lv_hc_dmin) : 1;
    int h = 1 + (int)lroundf(11 * u); return h < 1 ? 1 : h > 12 ? 12 : h;
}
static const char *lv_hc_abs(int h) { return h >= 14 ? "FOUND" : h >= 11 ? "HOT" : h >= 7 ? "WARM" : "COLD"; }
static uint32_t lv_hc_abs_col(int h) { return h >= 14 ? 0x30ff60 : h >= 11 ? 0xff3030 : h >= 7 ? 0xffb000 : 0x40c8ff; }
/* the round's city: fake nodes placed by the radar's own MAC-hash law, kept apart on screen so every tower is tappable */
static void lv_hc_new_round(void)
{
    LvHcWorld &w = lv_hc_w; memset(&w, 0, sizeof w);
    lv_hc_n = 0; const int want = 8;   /* 8 towers, ~24 px apart on screen: each keeps a wide tap zone */
    /* 8 screen slots 24 px apart (x -84..84 from centre): per slot the first MAC whose hash lands within 3 px of it */
    for (int slot = 0; slot < want; ++slot) for (int k = 0; k < 60000; ++k) {
        const uint8_t mac[6] = {0x5C, 0xA9, (uint8_t)lv_hc_round, (uint8_t)(k >> 8), (uint8_t)k, (uint8_t)(slot + 1)};
        const uint32_t h = lv_rd_hash(mac);
        const float Z = 1.6f + (((h >> 8) & 0xff) / 255.0f) * 3.2f, X = (((h & 0xff) / 255.0f) * 2.0f - 1.0f) * 0.72f * Z, x = 120 * X / Z;
        if (Z < 1.8f || Z > 4.5f || fabsf(x - (-84 + 24 * slot)) > 3) continue;
        LvFind &f = w.f[lv_hc_n]; snprintf(f.name, sizeof f.name, "NODE %02d", lv_hc_n + 1); memcpy(f.mac, mac, 6);
        f.rssi = (int8_t)(-86 + (int)((h >> 16) % 36)); f.kind = (h >> 24) % 3 == 0 ? LV_K_BLE : LV_K_WIFI; f.mfg = 0xFFFF;
        lv_hc_X[lv_hc_n] = X; lv_hc_Z[lv_hc_n] = Z; ++lv_hc_n; break;
    }
    w.nf = lv_hc_n; w.sel = -1; w.t0 = lv_now; w.miss = LV_RD_MISS_GRACE; w.tgt.rssi = -99;   /* 3 glyphs: the radar bar's dBm slot */ w.tgt_have = true;
    lv_hc_tgt = (int)((lv_rd_ch((uint32_t)lv_hc_round, 0x5ca9u) >> 8) % (uint32_t)lv_hc_n);
    lv_hc_dmin = 1e9f; lv_hc_dmax = 0; for (int i = 0; i < lv_hc_n; ++i) if (i != lv_hc_tgt) { const float d = lv_hc_dist(i); if (d < lv_hc_dmin) lv_hc_dmin = d; if (d > lv_hc_dmax) lv_hc_dmax = d; }
    lv_hc_probes = 0; lv_hc_pi = -1; lv_hc_heat = 0; lv_hc_prev = -1; lv_hc_shown = false; lv_hc_t0 = lv_hc_tend = lv_now;
}
static void lv_hc_lock(int i, int heat)   /* the radar's lock + hunt state on tower i, meter target = heat */
{
    LvHcWorld &w = lv_hc_w;
    w.lock = true; w.lock_t = lv_now; memcpy(w.lockmac, w.f[i].mac, 6); w.sel = -1; w.holding = false;
    w.hunting = true; w.tgt = w.f[i]; w.tgt.rssi = (int8_t)lv_hc_rssi(heat); w.tgt_have = true; w.miss = 0;
    w.reads = lv_hc_probes < 2 ? 1 : 2; w.trend = 0;
#if LV_RD_CHARGE && LV_RD_HUNT_METER
    w.hm_on = false;   /* the thermometer restarts empty and climbs one band per D4 packet */
#endif
}
static void lv_hc_play(void) { lv_hc_new_round(); lv_hc_phase = LV_HC_PLAY; lv_hc_t0 = lv_now;
    lv_hc_log("ROUND %d  %d NODES  ONE IS HIDDEN", lv_hc_round, lv_hc_n); lv_hc_log("TAP A TOWER TO PROBE  %d PROBES", LV_HC_PROBES); }
static void lv_hc_probe(int i)
{
    ++lv_hc_probes; lv_hc_prev = lv_hc_pi >= 0 ? lv_hc_heat : -1; lv_hc_pi = i; lv_hc_heat = lv_hc_heat_of(i);
    lv_hc_lock(i, lv_hc_heat);
    const char *tr = lv_hc_prev < 0 || lv_hc_heat >= 11 ? lv_hc_abs(lv_hc_heat) : lv_hc_heat > lv_hc_prev ? "WARMER" : lv_hc_heat < lv_hc_prev ? "COLDER" : "SAME";
    lv_hc_log("PROBE %d NODE %02d  HEAT %d/14  %s", lv_hc_probes, i + 1, lv_hc_heat, tr);
    if (i == lv_hc_tgt) {
        lv_hc_phase = LV_HC_FOUND; lv_hc_tend = lv_hc_pt = lv_now;
        const int s = 1000 - 50 * lv_hc_probes - lv_hc_secs10(); lv_hc_score = s < 0 ? 0 : s; if (lv_hc_score > lv_hc_best) lv_hc_best = lv_hc_score;
        lv_hc_log("NODE FOUND  SCORE %d", lv_hc_score);
    } else if (lv_hc_probes >= LV_HC_PROBES) { lv_hc_phase = LV_HC_LOST; lv_hc_tend = lv_hc_pt = lv_now; lv_hc_log("OUT OF PROBES"); }
}
static void lv_hc_enter(void)
{
    lv_hc_phase = LV_HC_START; lv_hc_new_round();
    lv_hc_log("SCANBOT // HOT COLD HUNT"); lv_hc_log("GAME NODES ARE FAKE, NOT SCAN DATA"); lv_hc_log("TAP PLAY");
}
static void lv_hc_leave(void) { lv_hc_phase = LV_HC_START; }
/* per frame: phase timers, then the radar layout for the game world (camera drift, towers) + generous hit boxes */
static void lv_hc_tick(void)
{
    if (lv_hc_phase == LV_HC_FOUND && (int32_t)(lv_now - lv_hc_pt) >= 3600) lv_hc_phase = LV_HC_WIN;   /* the meter climbs to 14 first */
    if (lv_hc_phase == LV_HC_LOST) {
        if (!lv_hc_shown && (int32_t)(lv_now - lv_hc_pt) >= 1200) { lv_hc_shown = true; lv_hc_lock(lv_hc_tgt, 14); lv_hc_log("IT WAS NODE %02d", lv_hc_tgt + 1); }
        if ((int32_t)(lv_now - lv_hc_pt) >= 4200) lv_hc_phase = LV_HC_OVER;
    }
    lv_hc_swap(); lv_rd_layout();
    lv_hc_nhb = 0;
    for (int n = 0; n < lv_rd_n && lv_hc_nhb < LV_HC_MAXT; ++n) {
        const LvTower &T = lv_rd_tw[n]; LvHcHit &h = lv_hc_hb[lv_hc_nhb++]; int xa, xb, ya, yb; lv_rd_box(T, 3, xa, xb, ya, yb);
        h.i = T.i; h.Z = T.Z; h.cx = (xa + xb) / 2; const int hw = (xb - xa) / 2 + 6 < 15 ? 15 : (xb - xa) / 2 + 6;   /* >= 31 px wide */
        h.xa = h.cx - hw; h.xb = h.cx + hw; h.ya = ya - 6; h.yb = yb + 6;
        if (h.ya < LV_RD_Y0) h.ya = LV_RD_Y0; if (h.yb > LV_RD_PTOP - 1) h.yb = LV_RD_PTOP - 1; if (h.xa < 0) h.xa = 0; if (h.xb > 239) h.xb = 239;
    }
    lv_hc_swap();
}
static int lv_hc_hit(int x, int y)   /* the tower whose (widened) box holds the finger, nearest centre wins */
{
    int best = -1, bd = 1 << 30;
    for (int k = 0; k < lv_hc_nhb; ++k) { const LvHcHit &h = lv_hc_hb[k]; if (x < h.xa || x > h.xb || y < h.ya || y > h.yb) continue;
        const int d = abs(x - h.cx) * 16 + (int)(h.Z * 4); if (d < bd) { bd = d; best = h.i; } }
    return best;
}
/* tiny 9x9 SCANbot face for the header strip, in the HOME face's own gold ramp (black frame, pale bevel, shine stripe) */
static void lv_hc_face9(LvBand &B, int x0, int y0)
{
    b_fill(B, x0, y0, 9, 9, 0x0000);
    for (int y = 0; y < 7; ++y) for (int x = 0; x < 7; ++x) {
        uint32_t c; const int d = x + y;
        if (y == 0 || x == 0) c = 0xffdfb5; else if (y == 6 || x == 6) c = 0x6b4d08; else c = d == 5 ? 0xf7d74a : d < 5 ? 0xf7ba4a : d < 9 ? 0xbd9652 : 0x9c7100;
        b_pix(B, x0 + 1 + x, y0 + 1 + y, lv_hex(c));
    }
    b_fill(B, x0 + 2, y0 + 3, 2, 2, 0x0000); b_fill(B, x0 + 5, y0 + 3, 2, 2, 0x0000);
}
static void lv_hc_face(LvBand &B, int x0, int y0)   /* the locked HOME SCANbot face, pixel for pixel */
{
    for (int y = 0; y < LV_HC_FACE_H; ++y) for (int x = 0; x < LV_HC_FACE_W; ++x) b_pix(B, x0 + x, y0 + y, LV_HC_FACE[y * LV_HC_FACE_W + x]);
}
static void lv_hc_button(LvBand &B, const LvRect &r, const char *l, uint16_t c, bool on)   /* the radar's panel-button style, 32 px tall */
{
    const int w = r.x1 - r.x0 + 1, h = r.y1 - r.y0 + 1;
    b_fill(B, r.x0, r.y0, w, h, on ? lv_dim(c, 3, 8) : 0x0000); b_frame(B, r.x0, r.y0, w, h, c); b_frame(B, r.x0 + 2, r.y0 + 2, w - 4, h - 4, lv_dim(c, 3, 8));
    b_text(B, r.x0 + (w - (int)strlen(l) * 6 + 1) / 2, r.y0 + (h - 7) / 2, l, on ? lv_hex(0xffffff) : c);
}
static void lv_hc_ctext(LvBand &B, int y, const char *s, uint16_t c) { b_text35(B, 120 - lv_w35(s) / 2, y, s, c); }
static void lv_hc_card(LvBand &B)
{
    const uint16_t W = lv_hex(0xffffff), G = lv_hex(0xa0a0a8), TAN = lv_hex(LV_TAN), CY = lv_hex(0x30e8ff);
    b_fill(B, 12, 48, 216, 193, 0x0000); b_frame(B, 12, 48, 216, 193, LV_HC_MAG); b_frame(B, 14, 50, 212, 189, lv_dim(LV_HC_MAG, 3, 8));
    lv_hc_face(B, 120 - LV_HC_FACE_W / 2, 56);
    char t[48];
    if (lv_hc_phase == LV_HC_START) {
        b_text(B, 120 - 78 + 1, 109, "HOT COLD HUNT", lv_hex(0x5a0040), 2); b_text(B, 120 - 78, 108, "HOT COLD HUNT", W, 2);
        lv_hc_ctext(B, 131, "A HIDDEN NODE IS IN THE TOWER CITY.", W);
        lv_hc_ctext(B, 139, "TAP A TOWER TO PROBE IT. THE METER", G);
        lv_hc_ctext(B, 147, "SAYS HOW CLOSE: COLD - WARM - HOT.", G);
        snprintf(t, sizeof t, "%d PROBES. SCORE = 1000 - 50/PROBE", LV_HC_PROBES); lv_hc_ctext(B, 155, t, G);
        lv_hc_ctext(B, 163, "- 10/SECOND.", G);
        lv_hc_ctext(B, 175, "GAME NODES ARE FAKE, NOT SCAN DATA", TAN);
        if (lv_hc_best >= 0) snprintf(t, sizeof t, "ROUND %d   %d NODES   BEST %d", lv_hc_round, lv_hc_n, lv_hc_best); else snprintf(t, sizeof t, "ROUND %d   %d NODES   BEST ---", lv_hc_round, lv_hc_n);
        lv_hc_ctext(B, 189, t, CY);
        lv_hc_button(B, LV_HC_BTN_L, "PLAY", LV_HC_MAG, true); lv_hc_button(B, LV_HC_BTN_R, "BACK", CY, false);
        return;
    }
    const bool win = lv_hc_phase == LV_HC_WIN; const int s10 = lv_hc_secs10();
    const char *title = win ? "NODE FOUND" : "NODE LOST"; const uint16_t tc = lv_hex(win ? 0x30ff60 : 0xff3030); const int tw = (int)strlen(title) * 12;
    b_text(B, 120 - tw / 2 + 1, 109, title, lv_hex(win ? 0x0a4a1a : 0x4a0a0a), 2); b_text(B, 120 - tw / 2, 108, title, tc, 2);
    if (win) { snprintf(t, sizeof t, "ROUND %d CLEAR", lv_hc_round); lv_hc_ctext(B, 131, t, W); }
    else { snprintf(t, sizeof t, "OUT OF PROBES. IT WAS NODE %02d", lv_hc_tgt + 1); lv_hc_ctext(B, 131, t, W); }
    snprintf(t, sizeof t, "TIME %d.%dS   PROBES %d/%d", s10 / 10, s10 % 10, lv_hc_probes, LV_HC_PROBES); lv_hc_ctext(B, 143, t, G);
    if (win) { snprintf(t, sizeof t, "1000 - 50X%d - %d = %d", lv_hc_probes, s10, lv_hc_score); lv_hc_ctext(B, 151, t, G);
               b_text(B, 120 - 9 * 6 / 2 - 3 * 6, 161, "SCORE", TAN); snprintf(t, sizeof t, "%d", lv_hc_score); b_text(B, 120 - 6 + 9, 159, t, W, 2); }
    if (lv_hc_best >= 0) { snprintf(t, sizeof t, "BEST %d", lv_hc_best); lv_hc_ctext(B, 179, t, CY); }
    if (win) snprintf(t, sizeof t, "PLAY AGAIN = ROUND %d, %d PROBES", lv_hc_round + 1, lv_hc_round + 1 < 5 ? 8 - lv_hc_round : 4);
    else snprintf(t, sizeof t, "PLAY AGAIN = RETRY ROUND %d", lv_hc_round);
    lv_hc_ctext(B, 189, t, G);
    lv_hc_button(B, LV_HC_BTN_L, "PLAY AGAIN", LV_HC_MAG, true); lv_hc_button(B, LV_HC_BTN_R, "BACK", CY, false);
}
/* the GIBSON radar scene for the game world, then the game's header / panel / legend / card over it */
static void lv_hc_scene(LvBand &B, void *)
{
    lv_hc_swap(); lv_scene_radar(B, 0);
    const uint16_t hdr = lv_ink(LV_HDR), W = lv_hex(0xffffff), G = lv_hex(0xa0a0a8), TAN = lv_hex(LV_TAN);
    char t[64];
    /* header strip (same 9-row strip, same ink): gold SCANbot + the game name, round / probes / clock on the right */
    b_fill(B, 0, 25, 240, 9, lv_hex(0x101018)); lv_hc_face9(B, 2, 25); b_text35(B, 14, 27, "SCANBOT // HOT COLD HUNT", hdr);
    const int s10 = lv_hc_secs10();
    if (lv_hc_phase == LV_HC_START) snprintf(t, sizeof t, "R%d  %d NODES  READY", lv_hc_round, lv_hc_n);
    else snprintf(t, sizeof t, "R%d  P%d/%d  %d.%dS", lv_hc_round, lv_hc_probes, LV_HC_PROBES, s10 / 10, s10 % 10);
    b_text35(B, 237 - lv_w35(t), 27, t, hdr);
    /* panel: the radar's locked panel frame (black, 1 px frame in the probed tower's colour) */
    if (lv_hc_phase != LV_HC_START) {
        const int py0 = LV_RD_PTOP; const bool have = lv_hc_pi >= 0; const int shown = (lv_hc_phase == LV_HC_LOST || lv_hc_phase == LV_HC_OVER) && lv_hc_shown ? lv_hc_tgt : lv_hc_pi;
        const uint16_t fc = have ? lv_rd_col(lv_f[shown], 8, 8) : LV_HC_MAG;
        b_fill(B, 2, py0, 236, 285 - py0, 0x0000); b_frame(B, 2, py0, 236, 285 - py0, fc);
        if (lv_hc_phase == LV_HC_FOUND || lv_hc_phase == LV_HC_WIN) snprintf(t, sizeof t, "TARGET NODE FOUND  ROUND %d CLEAR", lv_hc_round);
        else if (lv_hc_phase == LV_HC_LOST || lv_hc_phase == LV_HC_OVER) snprintf(t, sizeof t, lv_hc_shown ? "OUT OF PROBES  THIS WAS THE TARGET" : "OUT OF PROBES");
        else snprintf(t, sizeof t, "FIND THE HIDDEN NODE  %d PROBES LEFT", LV_HC_PROBES - lv_hc_probes);
        b_text35(B, 5, 251, t, G);
        if (have) {
            const int h = shown == lv_hc_pi ? lv_hc_heat : 14;
            snprintf(t, sizeof t, "NODE %02d", shown + 1); b_text(B, 5, 264, t, W);
            snprintf(t, sizeof t, "%d", h); b_text(B, 235 - ((int)strlen(t) + 4) * 6, 264, t, fc); b_text35(B, 235 - 4 * 6 + 4, 265, "/14", fc);
            const char *st = shown != lv_hc_pi ? "TARGET" : lv_hc_abs(h);
            snprintf(t, sizeof t, "HEAT %s  PROBES %d/%d  TIME %d.%dS", st, lv_hc_probes, LV_HC_PROBES, s10 / 10, s10 % 10); b_text35(B, 5, 275, t, TAN);
        } else { b_text(B, 5, 264, "TAP A TOWER TO PROBE", W); snprintf(t, sizeof t, "PROBES 0/%d  TIME %d.%dS", LV_HC_PROBES, s10 / 10, s10 % 10); b_text35(B, 5, 275, t, TAN); }
    }
    /* terminal: the radar's own 3-line terminal (game log); the legend line says what the screen means */
    b_fill(B, 14, 312, 203, 6, 0x0000);
    { const char *L = "TAP TOWER=PROBE  METER=HEAT  FAKE GAME NODES"; b_text35(B, 218 - 3 - lv_w35(L), 312, L, W); }
    if (lv_hc_phase == LV_HC_START || lv_hc_phase == LV_HC_WIN || lv_hc_phase == LV_HC_OVER) lv_hc_card(B);
    lv_draw_backbot(B, LV_R_BACKBOT.x0, LV_R_BACKBOT.y0);
    lv_hc_swap();
}
/* top bar: the radar's own hunt bar (frozen stripes, 11-bar meter) with the game's words */
static void lv_hc_top(LvBand &B, void *)
{
    lv_hc_swap(); lv_hunt_bar(B); lv_hc_swap();
    const uint16_t BG = lv_hex(0x1a0012), W = lv_hex(0xffffff); char t[24];
    b_fill(B, 3, 4, 80, 17, BG); b_fill(B, 150, 4, 87, 17, BG);
    if (lv_hc_phase == LV_HC_START) {
        b_text35(B, 3, 5, "SCANBOT GAME", LV_HC_MAG); b_text(B, 3, 12, "HOT COLD", W);
        b_text(B, 190 - 2 * 12, 6, "--", lv_hex(0x808080), 2); b_text35(B, 191, 15, "/14", lv_hex(0x808080));
        b_text35(B, 236 - lv_w35("READY"), 6, "READY", lv_hex(0xc0c0c0)); b_text35(B, 236 - lv_w35("TAP PLAY"), 14, "TAP PLAY", lv_hex(0xc0c0c0));
        return;
    }
    const int shown = lv_hc_phase >= LV_HC_LOST && lv_hc_shown ? lv_hc_tgt : lv_hc_pi;
    snprintf(t, sizeof t, "PROBE %d/%d", lv_hc_probes, LV_HC_PROBES); b_text35(B, 3, 5, t, LV_HC_MAG);
    if (shown >= 0) snprintf(t, sizeof t, "NODE %02d", shown + 1); else snprintf(t, sizeof t, "TAP TOWER");
    b_text(B, 3, 12, t, W);
    const int h = shown < 0 ? -1 : shown == lv_hc_pi ? lv_hc_heat : 14;
    if (h < 0) { b_text(B, 190 - 2 * 12, 6, "--", lv_hex(0x808080), 2); b_text35(B, 191, 15, "/14", lv_hex(0x808080)); }
    else { snprintf(t, sizeof t, "%d", h); b_text(B, 190 - (int)strlen(t) * 12, 6, t, W, 2); b_text35(B, 191, 15, "/14", W); }
    const char *w1, *w2; uint32_t k; char d[16];
    if (h < 0) { w1 = "PROBE"; w2 = "1ST"; k = 0xc0c0c0; }
    else if (shown != lv_hc_pi) { w1 = "LOST"; snprintf(d, sizeof d, "NODE %02d", shown + 1); w2 = "HERE"; k = 0xff3030; }
    else if (h >= 11 || lv_hc_prev < 0) { w1 = lv_hc_abs(h); k = lv_hc_abs_col(h); w2 = h >= 14 ? "LOCK" : lv_hc_prev < 0 ? "1ST" : h > lv_hc_prev ? "WARMER" : h < lv_hc_prev ? "COLDER" : "SAME"; }
    else if (h > lv_hc_prev) { w1 = "WARMER"; snprintf(d, sizeof d, "%+d", h - lv_hc_prev); w2 = d; k = 0xff8020; }
    else if (h < lv_hc_prev) { w1 = "COLDER"; snprintf(d, sizeof d, "%+d", h - lv_hc_prev); w2 = d; k = 0x40c8ff; }
    else { w1 = "SAME"; w2 = lv_hc_abs(h); k = 0xc0c0c0; }
    b_text35(B, 236 - lv_w35(w1), 6, w1, lv_hex(k)); b_text35(B, 236 - lv_w35(w2), 14, w2, lv_hex(k));
}
static void lv_hc_frame(void)
{
    lv_hc_tick();
    lv_render(lv_blit, LV_R_RADAR, lv_hc_scene, 0);
    lv_render(lv_blit, LV_R_TICK, lv_hc_top, 0);
}
static void lv_go_soon(int scr, const char *tty_cmd);
static const char *lv_hc_tap(int x, int y)
{
    if (lv_in(LV_R_BACKBOT_HIT, x, y)) { lv_go_soon(LV_SC_HOME, 0); return "hc-backbot-home"; }
    if (lv_hc_phase == LV_HC_START || lv_hc_phase == LV_HC_WIN || lv_hc_phase == LV_HC_OVER) {
        const LvRect L = {(int16_t)(LV_HC_BTN_L.x0 - 3), (int16_t)(LV_HC_BTN_L.y0 - 3), (int16_t)(LV_HC_BTN_L.x1 + 2), (int16_t)(LV_HC_BTN_L.y1 + 3)};
        const LvRect R = {(int16_t)(LV_HC_BTN_R.x0 - 2), (int16_t)(LV_HC_BTN_R.y0 - 3), (int16_t)(LV_HC_BTN_R.x1 + 3), (int16_t)(LV_HC_BTN_R.y1 + 3)};
        if (lv_in(L, x, y)) { const char *r = lv_hc_phase == LV_HC_START ? "hc-play" : "hc-play-again"; if (lv_hc_phase == LV_HC_WIN) ++lv_hc_round; lv_hc_play(); return r; }
        if (lv_in(R, x, y)) { lv_go_soon(LV_SC_HOME, 0); return "hc-back-home"; }
        return "hc-card";
    }
    if (lv_hc_phase != LV_HC_PLAY) return "hc-wait";
    if (y < LV_RD_Y0 || y >= LV_RD_PTOP) return "hc-none";
    const int i = lv_hc_hit(x, y);
    if (i < 0) return "hc-miss";
    if (i == lv_hc_pi) return "hc-same";
    lv_hc_probe(i); return lv_hc_phase == LV_HC_FOUND ? "hc-found" : "hc-probe";
}
#endif
