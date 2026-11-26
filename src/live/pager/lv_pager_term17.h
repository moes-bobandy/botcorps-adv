/* MOCK 2026-11-26 (CYD-17, sim-only, NOT in any bin): each home big-bot pager page swaps ONLY its small role panel
 * (the locked 152x71 plate at (45,220)-(196,290)) for a terminal-prompt readout of that bot's name + role, typed out.
 * Off by default: LV_CYD17_PANEL_TERM is 0, so the firmware and v1.2.22 renders are byte-identical. The mock sim builds with -DLV_CYD17_PANEL_TERM=1.
 * Look = TPS "F2" (mock 0d64cf3, lv_term_ps.h): 5x7 LV_GLCD mixed case, 6 px pitch, racing digits snap + lock cell by cell,
 * 7-role shade ramp (T1 bloom / MID / BODY / TAG / HOT / LEAD) taken from the LIVE theme per pixel: lv_theme_at(lv_theme, x, y)
 * + sec_legible + lv_mix3 (same functions as 0d64cf3; multi-colour themes resolve per pixel). No hardcoded ink colour.
 * No role field exists in source: every "role:" line is DERIVED from these sources (see docs/mocks/cyd17-panel-terminal/README.md):
 *   BOTpheus   lv_morpheus_v13.h (DOJO CONSTRUCT, MO_N 10 pillars, tap = push-in + 3 Dojo folders, search bar) + old panel caption DOJO/CONSTRUCT (lv_pager_fx.h)
 *   HACKbot    lv_core.h `go hackbot` -> LV_SC_DL "HACKbot game"; lv_scanbot_download.h (find ONE file, LV_DL_NT 5 towers, tap file = download, lv_dl_tms 90000)
 *   GIBSONbot  lv_core.h `bots` ("GIBSONbot home") + LV_CS {"home","GIBSONbot"}; lv_radar.h (3D tower scene, tower height = rssi: lv_rd_level, lv_rd_lock)
 *   TERMINALbot lv_core.h `bots` ("TERMINALbot term"); lv_inside_towers.h Relay Ring (10 device bots, itw_nt = 10); botcorps_ui.cpp:7615 core "scan lock zap purge restart"
 *   CONTROLbot lv_core.h `bots` ("CONTROLbot ctl"); only used by the opt-in LV_C17_CTL extra, not in the main set.
 * Mock-only RAM note: a 21 KB panel buffer is used here for the glow pass; a firmware port would draw row-wise. */
#pragma once
#ifndef LV_CYD17_PANEL_TERM
#define LV_CYD17_PANEL_TERM 0
#endif
#ifndef LV_CYD19_LOGBOT
#define LV_CYD19_LOGBOT 0   /* CYD-19: also needed here, the LOGbot log panel reuses the CYD-17 F2 glyph/ink helpers */
#endif
#ifndef LV_C17_CTL
#define LV_C17_CTL 0   /* 1 = also overpaint the CONTROLbot panel (hides its live controls; extra mock only) */
#endif
#if LV_CYD17_PANEL_TERM || LV_CYD19_LOGBOT
#define LV_C17_MS    45        /* ms per typed cell */
#define LV_C17_PAUSE 4         /* extra cells of pause at end of each line */
#define LV_C17_X     52
#define LV_C17_Y     227
#define LV_C17_ROW   10
#define LV_C17_NL    6

enum { C17_T1, C17_MID, C17_BODY, C17_TAG, C17_HOT, C17_LEAD, C17_NONE = -1 };
static uint32_t lv_c17_legible(uint32_t c) { return sec_legible(c); }
static uint16_t lv_c17_role_px(int role, uint32_t T)
{
    if (role == C17_T1) return lv_hex(lv_mix3(T, 0x000000, 79, 100));
    const uint32_t Tl = lv_c17_legible(T);
    switch (role) {
    case C17_MID:  return lv_hex(lv_mix3(Tl, 0x000000, 40, 100));
    case C17_BODY: return lv_hex(lv_mix3(Tl, 0xffffff, 1, 8));
    case C17_TAG:  return lv_hex(lv_mix3(Tl, 0xffffff, 2, 5));
    case C17_HOT:  return lv_hex(Tl);
    case C17_LEAD: return lv_hex(lv_mix3(Tl, 0xffffff, 4, 5));
    default:       return lv_hex(Tl);
    }
}
static uint16_t lv_c17_buf[LV_CB_PH][LV_CB_PW];
static uint32_t lv_c17_key = 0xFFFFFFFFu; static int lv_c17_keypg = -1;
static int lv_c17_lastpg = -1; static uint32_t lv_c17_t0 = 0;

static inline uint16_t lv_c17_maxc(uint16_t a, uint16_t b)
{
    uint16_t r = (a >> 11) > (b >> 11) ? (a & 0xF800) : (b & 0xF800);
    uint16_t g = ((a >> 5) & 63) > ((b >> 5) & 63) ? (a & 0x07E0) : (b & 0x07E0);
    uint16_t bl = (a & 31) > (b & 31) ? (a & 31) : (b & 31);
    return r | g | bl;
}
static inline void lv_c17_put(int role, int px, int py)
{
    const int bx = px - LV_CB_PX0, by = py - LV_CB_PY0;
    if (bx < 2 || bx > LV_CB_PW - 3 || by < 2 || by > LV_CB_PH - 3) return;   /* keep the plate frame */
    uint16_t c = lv_c17_role_px(role, lv_theme_at(lv_theme, px, py));
    if (py & 1) c = lv_dim(c, 7, 8);   /* F2 CRT odd-scanline modulation */
    lv_c17_buf[by][bx] = lv_c17_maxc(lv_c17_buf[by][bx], c);
}
static void lv_c17_char(int x, int y, char ch, int fg, int g1, int g2)
{
    const unsigned uc = (unsigned char)ch;
    if (uc <= 32 || uc > 126) return;
    const uint8_t *col5 = &LV_GLCD[(uc - 32) * 5];
    for (int pass = 0; pass < 3; ++pass)   /* g2 diagonal halo, g1 cross bloom, then fg */
        for (int c = 0; c < 5; ++c) for (int r = 0; r < 7; ++r) if (col5[c] >> r & 1) {
            const int px = x + c, py = y + r;
            if (pass == 0 && g2 >= 0) { lv_c17_put(g2, px - 1, py - 1); lv_c17_put(g2, px + 1, py - 1); lv_c17_put(g2, px - 1, py + 1); lv_c17_put(g2, px + 1, py + 1); }
            if (pass == 1 && g1 >= 0) { lv_c17_put(g1, px - 1, py); lv_c17_put(g1, px + 1, py); lv_c17_put(g1, px, py - 1); lv_c17_put(g1, px, py + 1); }
            if (pass == 2) lv_c17_put(fg, px, py);
        }
}
#endif
#if LV_CYD17_PANEL_TERM
struct LvC17Page { const char *l[LV_C17_NL]; uint8_t kind[LV_C17_NL]; };   /* kind 0 name, 1 role, 2 body, 3 prompt */
/* Index = LV_PG_* page id (BOTPHEUS 0, HACKBOT 1, GIBSON 2, TERMINAL 3, CTL 4). Max 23 chars per line. */
static const LvC17Page LV_C17_PG[5] = {
    { { "> BOTpheus", "> role: dojo construct", "  10 device pillars", "  tap pillar: 3 folders", "  search bar on top", "> _" },        { 0, 1, 2, 2, 2, 3 } },
    { { "> HACKbot", "> role: hack game", "  1 file in 5 towers", "  tap file: download", "  trace timer: 90 s", "> _" },          { 0, 1, 2, 2, 2, 3 } },
    { { "> GIBSONbot", "> role: home radar", "  3D tower scan view", "  tap tower: lock", "  tower height = RSSI", "> _" },               { 0, 1, 2, 2, 2, 3 } },
    { { "> TERMINALbot", "> role: term", "  sweep 10 device bots", "  scan lock zap purge", "  restart", "> _" },                          { 0, 1, 2, 2, 2, 3 } },
    { { "> CONTROLbot", "> role: ctl", "  radar sleep bright", "  recal rotate bar", "  exit", "> _" },                                   { 0, 1, 2, 2, 2, 3 } },
};

static void lv_c17_render(int page, int rel)
{
    for (int by = 0; by < LV_CB_PH; ++by) {   /* locked plate frame (same call as the v1.2.22 panels), black field */
        uint16_t tmp[240]; memset(tmp, 0, sizeof tmp);
        lv_pg_pan_plate(tmp, LV_CB_PY0 + by, lv_pg_fx_acc(), lv_rgb(0, 0, 0));
        memcpy(lv_c17_buf[by], tmp + LV_CB_PX0, LV_CB_PW * 2);
    }
    const LvC17Page &P = LV_C17_PG[page];
    const int cyc = rel > 0 ? rel / 90 : 0;
    int n = 0, cur = 0;
    for (int i = 0; i < LV_C17_NL; ++i) {   /* current row = first line that is not fully locked */
        const int L = (int)strlen(P.l[i]);
        if (rel < (n + L) * LV_C17_MS) { cur = i; break; }
        n += L + LV_C17_PAUSE; cur = i + 1;
    }
    n = 0;
    for (int i = 0; i < LV_C17_NL; ++i) {
        const char *s = P.l[i]; const int L = (int)strlen(s);
        const int y = LV_C17_Y + i * LV_C17_ROW;
        for (int c = 0; c < L; ++c) {
            if (s[c] == ' ') continue;
            const int x = LV_C17_X + c * 6;
            const int dt = rel - (n + c) * LV_C17_MS;
            if (dt < 0) {
                if (i != cur) continue;   /* only the line being typed races digits; later lines stay dark */
                const uint32_t h = (uint32_t)(cyc * 131u + i * 37u + c * 17u);
                const int wave = (cyc + i + ((c * 3) % 7)) % 7;
                lv_c17_char(x, y, (char)('0' + (h % 10u)), wave == 0 ? C17_HOT : wave <= 2 ? C17_MID : C17_T1, wave == 0 ? C17_T1 : C17_NONE, C17_NONE);
            } else if (dt < 240) lv_c17_char(x, y, s[c], C17_LEAD, C17_MID, C17_T1);       /* snap flare */
            else if (dt < 520)   lv_c17_char(x, y, s[c], C17_TAG, C17_T1, C17_NONE);
            else lv_c17_char(x, y, s[c], P.kind[i] == 1 ? C17_HOT : P.kind[i] == 2 ? C17_BODY : C17_TAG, C17_T1, C17_NONE);
        }
        n += L + LV_C17_PAUSE;
    }
}
static void lv_c17_row(uint16_t *row, int y, int page)
{
    if (y < LV_CB_PY0 || y >= LV_CB_PY0 + LV_CB_PH || page < 0 || page > 4) return;
    if (page != lv_c17_lastpg) { lv_c17_lastpg = page; lv_c17_t0 = lv_now; lv_c17_key = 0xFFFFFFFFu; }
    if (lv_c17_key != (uint32_t)lv_now || lv_c17_keypg != page) { lv_c17_key = (uint32_t)lv_now; lv_c17_keypg = page; lv_c17_render(page, (int)(lv_now - lv_c17_t0)); }
    memcpy(row + LV_CB_PX0, lv_c17_buf[y - LV_CB_PY0], LV_CB_PW * 2);
}
#endif
