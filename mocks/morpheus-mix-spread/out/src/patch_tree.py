#!/usr/bin/env python3
"""morpheus-mix tree patch (2026-11-14 spread: 18 devices, WIFI + BLUETOOTH removed; spread placement; three fixed style heights). Base: /workspace/cyd-morpheus-v6-wt src/live/lv_morpheus_v13.h (v13.1) + lv_inside_towers.h.
Applies: towers-3 renames (v13.1 skin / folder kept, not called) + mo_mix.inc include, 20 devices (MO_N), 8 new from the SquachWatch README,
threat -> look tables, ONE fixed height per style (MO_HFIX, replaces the engine rssi height + breathing in mo11_drift), vector emblems.
Usage: patch_tree.py TREE"""
import json, os, sys, re
D = os.path.dirname(os.path.abspath(__file__)); TREE = sys.argv[1]
BASE = '/workspace/cyd-morpheus-v6-wt/src/live/'
J = json.load(open(D + '/layout18.json'))
s = open(BASE + 'lv_morpheus_v13.h').read(); t = open(BASE + 'lv_inside_towers.h').read()
def rep(a, b, cnt=1):
    global s
    n = s.count(a); assert n == cnt, (a[:70], n); s = s.replace(a, b)
f = lambda v: ', '.join(('%g' % x if '.' in '%g' % x else '%g.0' % x) + 'f' for x in v)
# ---- towers-3: v13.1 skin + folder renamed (kept for reference, not called); mo_mix.inc provides mo11_tower_fx / mo12_folder
rep('static void mo11_tower_fx(LvBand &B, const LvTower &T)\n{', 'static void mo13_tower_fx_v131(LvBand &B, const LvTower &T)   /* v13.1 skin, kept for reference, not called */\n{')
rep('static void mo12_folder(LvBand &B, int i, int j, int ys, float ph, int tl)   /* tl = ms since this folder landed (LOADING bar) */',
    'static void mo13_folder_v131(LvBand &B, int i, int j, int ys, float ph, int tl)   /* v13.1 folder, kept for reference, not called */')
rep('/* ---- v12 line-item search bar', '#include "../../../src/mo_mix.inc"\n/* ---- v12 line-item search bar')
# ---- header comment + includes
rep('#if LV_MO_INSIDE\nstruct MoDev', '''#if LV_MO_INSIDE
/* MOCK 2026-11-14 morpheus-mix spread (on v13.1 + morpheus-towers-3 looks): 18 device pillars (WIFI + BLUETOOTH removed), look per
 * device by threat level (HIGH -> T1 code glass, MEDIUM -> T3 sentinel lantern, LOW -> T2 construct white light), ONE fixed height per
 * style (MO_HFIX: only three heights), spread placement, crisp vector emblems (src/mo_emblem.h). Mock only, EX.DATA. */
#include "../../../src/mo_emblem.h"
#define MO_N 18
struct MoDev''')
rep('static const MoDev MO_DEV[12] = {', 'static const MoDev MO_DEV[MO_N] = {   /* 2026-11-14: WIFI + BLUETOOTH removed */')
m = re.search(r'\n    \{"WIFI", [^\n]*\n    \{"BLUETOOTH", [^\n]*', s); assert m; s = s.replace(m.group(0), '')
rep('''    {"SAMSUNG", "Samsung Galaxy SmartTag trackers.", "SmartTag Bluetooth adverts (FD5A/FD59).", "Run an unknown tag scan in SmartThings."},
};''', '''    {"SAMSUNG", "Samsung Galaxy SmartTag trackers.", "SmartTag Bluetooth adverts (FD5A/FD59).", "Run an unknown tag scan in SmartThings."},
    /* morpheus-mix: the 8 SquachWatch README devices */
    {"AXON", "Axon police body cams and Taser gear.", "Axon maker MAC prefix and AB3 Wi-Fi names.", "Know your rights. Ask if you are filmed."},
    {"RAVEN", "Raven gunshot and sound sensors on poles.", "Raven BLE service UUIDs 0x3100 to 0x3500.", "Know where they are. Ask what is kept."},
    {"ALPR", "Plate reader cams (Motorola, Genetec).", "Camera maker MAC prefixes on Wi-Fi.", "Ask your city who gets the plate data."},
    {"CAMERA", "Wi-Fi IP cams (Wyze, Verkada, Tuya).", "Camera maker MAC prefixes on Wi-Fi.", "Check rentals for cams. Cover lenses."},
    {"RING", "Amazon Ring doorbells and cameras.", "Ring and Amazon maker MAC prefixes.", "Say no to video share asks you do not want."},
    {"GOOGLE TAG", "Google Find My Device trackers.", "FMDN adverts (FEAA), Chipolo, Pebblebee.", "Run your phone's unknown tracker scan."},
    {"EVIL TWIN", "A fake hotspot copying a real Wi-Fi name.", "Same name from two APs, lock type differs.", "Check the lock. Forget open copies."},
    {"IBEACON", "Apple iBeacon adverts used by shops.", "BLE adverts 4C 00 02 15 + UUID major minor.", "Turn off Bluetooth or beacon scans."},
};''')
rep('static const int MO_ICON[12] = {LV_IC_AP, LV_IC_BLE, -1, LV_IC_LAPTOP, LV_IC_CCTV, LV_IC_DRONE, LV_IC_AIRTAG, LV_IC_KEYFOB, LV_IC_GLASSES, LV_IC_SKIMMER, LV_IC_FLIPPER, LV_IC_GALAXYTAG};',
    'static const int MO_ICON[MO_N] = {-1, LV_IC_LAPTOP, LV_IC_CCTV, LV_IC_DRONE, LV_IC_AIRTAG, LV_IC_KEYFOB, LV_IC_GLASSES, LV_IC_SKIMMER, LV_IC_FLIPPER, LV_IC_GALAXYTAG, -1, -1, -1, -1, -1, -1, -1, -1};   /* old art (v13.1 only); mix pillars use mo_emblem.h */')
THREAT = [0,0,2,2,1,1,1,0,0,1, 1,2,2,1,2,1,0,2]; TLOOK = [1,3,2]
HF = J['H']   # fixed world heights [T1, T3, T2]
rep('static const int MO_KIND[12] = {0, 3, 0, 0, 1, 1, 2, 2, 1, 3, 3, 2};', '''static const int MO_KIND[MO_N] = {0, 0, 1, 1, 2, 2, 1, 3, 3, 2, 1, 1, 1, 1, 1, 2, 0, 3};
/* threat (0 HIGH, 1 MEDIUM, 2 LOW) -> look (1 T1 code glass, 3 T3 sentinel lantern, 2 T2 construct white light); provisional split */
static const int MO_THREAT[MO_N] = {%s};
static const char *const MO_TNAME[3] = {"HIGH", "MEDIUM", "LOW"};
static const int MO_TLOOK[3] = {1, 3, 2};
static const int MO_LOOKOF[MO_N] = {%s};
/* ONE fixed world height per style, by look (1 T1 code glass tallest, 3 T3 lantern middle, 2 T2 white light shortest): only three heights.
 * Replaces the engine's rssi height and the idle breathing for the pillars (mo11_drift). */
static const float MO_HFIX[4] = {1.0f, %.3ff, %.3ff, %.3ff};''' % (', '.join(map(str, THREAT)), ', '.join(str(TLOOK[x]) for x in THREAT), HF[0], HF[2], HF[1]))
# whole tables for the 18 (sx / z0: the spread placement; drift + rssi: each device's v1 values, WIFI / BLUETOOTH rows dropped)
for k, nm in (('sx','MO11_SX'),('z0','MO11_Z0'),('ax','MO11_AX'),('wx','MO11_WX'),('az','MO11_AZ'),('wz','MO11_WZ'),('wh','MO11_WH')):
    m = re.search(r'static const float %s\[12\] = \{([^}]*)\};' % nm, s); assert m, nm; assert len(J[k]) == 18, k
    s = s.replace(m.group(0), 'static const float %s[MO_N] = {%s};' % (nm, f(J[k])))
if J.get('drift_law') == 'v5c':   # v5c (Mobandy 2:05 PM ET 'not locked in movement'): own phase / speed / amplitude in X, Z + a lateral sway per tower (src/gibson_depth_drift.py)
    tabs = '\n'.join('static const float %s[MO_N] = {%s};' % (nm, f(J[k])) for k, nm in (('px','MO11_PX'),('pz','MO11_PZ'),('as','MO11_AS'),('ws','MO11_WS'),('ps','MO11_PS')))
    rep('static float mo11_dx(int i, float t) { return mo11_k(i) * MO11_AX[i] * (MO11_Z0[i] / 2.0f) * sinf(MO11_WX[i] * t + i * 2.399f); }',
        tabs + '\nstatic float mo11_dx(int i, float t) { return mo11_k(i) * (MO11_AX[i] * (MO11_Z0[i] / 2.0f) * sinf(MO11_WX[i] * t + MO11_PX[i]) + MO11_AS[i] * MO11_Z0[i] * sinf(MO11_WS[i] * t + MO11_PS[i])); }   /* v5c: own phase + lateral sway */')
    rep('static float mo11_dz(int i, float t) { return mo11_kz(i) * MO11_AZ[i] * (MO11_Z0[i] / 3.0f) * sinf(MO11_WZ[i] * t + i * 1.713f + 0.9f); }',
        'static float mo11_dz(int i, float t) { return mo11_kz(i) * MO11_AZ[i] * (MO11_Z0[i] / 3.0f) * sinf(MO11_WZ[i] * t + MO11_PZ[i]); }   /* v5c: own phase; depth from the Gibson map of the EX.DATA rssi */')
m = re.search(r'static const int8_t MO11_RSSI\[12\] = \{([^}]*)\};', s)
s = s.replace(m.group(0), 'static const int8_t MO11_RSSI[MO_N] = {%s};' % ', '.join(map(str, J['rssi'])))
rep('static LvTower mo11_tw[12]; static bool mo11_have[12]; static float mo11_cam = 0; static int MO_ORD[12], mo11_nord = 0;',
    'static LvTower mo11_tw[MO_N]; static bool mo11_have[MO_N]; static float mo11_cam = 0; static int MO_ORD[MO_N], mo11_nord = 0;')
rep('static float mo11_dxv[12], mo11_dzv[12], mo11_hk[12];', 'static float mo11_dxv[MO_N], mo11_dzv[MO_N], mo11_hk[MO_N];')
# drift: identical v11 law (dx, dz) for all 18; one fixed height per style, no breathing
rep('''    const int i = T.i; if (i < 0 || i >= 12) return; const float t = mo_t();
    const float Z0 = MO11_Z0[i]''', '''    const int i = T.i; if (i < 0 || i >= MO_N) return; const float t = mo_t();
    T.H = MO_HFIX[MO_LOOKOF[i]];   /* morpheus-mix spread: one fixed height per style (three heights total) */
    const float Z0 = MO11_Z0[i]''')
rep('hk = 1.0f + MO11_AH * sinf(MO11_WH[i] * t + i * 0.77f);', 'hk = 1.0f;   /* spread: no idle height breathing (heights stay exactly three) */')
rep('if (itw_mo_sel >= 0) snprintf(t, sizeof t, "PILLAR %d/12  LOCKED", itw_mo_sel + 1); else snprintf(t, sizeof t, "12 PILLARS  EX.DATA");',
    'if (itw_mo_sel >= 0) snprintf(t, sizeof t, "PILLAR %d/%d  LOCKED", itw_mo_sel + 1, MO_N); else snprintf(t, sizeof t, "%d PILLARS  EX.DATA", MO_N);')
rep('else { snprintf(a, sizeof a, "PILLAR OPEN  EX.DATA");', 'else { snprintf(a, sizeof a, "PILLAR OPEN  THREAT: %s  EX.DATA", MO_TNAME[MO_THREAT[itw_mo_sel]]);')
rep('''static void mo_chip_icon(LvBand &B, int i, int x, int y)   /* the device icon, monochrome: parchment body + green lines on the charcoal chip */
{
    for (int r = 0; r < 16; ++r) for (int c = 0; c < 16; ++c) { const int k = mo_src(i, r, c); if (k) b_pix(B, x + c, y + r, k == 2 ? lv_hex(MO11_GH) : lv_hex(0xe8dec8)); }
}''', '''static void mo_emb_draw(LvBand &B, int i, int x0, int y0, int S, uint32_t bc, uint32_t dc, uint32_t hc, int a, int ha, int yhi);   /* mo_mix.inc */
static void mo_chip_icon(LvBand &B, int i, int x, int y)   /* morpheus-mix: the device's 16 px vector emblem, parchment body + green detail on the charcoal chip */
{
    mo_emb_draw(B, i, x, y, 16, 0xe8dec8, MO11_GH, 0, 256, 0, 9999);
}''')
rep('    for (int i = 0; i < 12; ++i) mo11_have[i] = false;\n    for (int n = 0; n < lv_rd_n; ++n) { const LvTower &T = lv_rd_tw[n]; if (T.i < 0 || T.i >= 12) continue;',
    '    for (int i = 0; i < MO_N; ++i) { mo11_have[i] = false; mo_eS[i] = 0; }\n    for (int n = 0; n < lv_rd_n; ++n) { const LvTower &T = lv_rd_tw[n]; if (T.i < 0 || T.i >= MO_N) continue;')
rep('mo_build(); itw_nt = 12;', 'mo_build(); itw_nt = MO_N;')
rep('''    for (int i = 0; i < 12; ++i) {
        snprintf(itw_lab[i]''', '''    for (int i = 0; i < MO_N; ++i) {
        snprintf(itw_lab[i]''')
rep('w.nf = 12;', 'w.nf = MO_N;')
rep('"DOJO CONSTRUCT  12 PILLARS"', '"DOJO CONSTRUCT  18 PILLARS"')
rep('''    for (int i = 0; i < 12; ++i) if (mo11_have[i]) { const LvTower &T = mo11_tw[i];''', '''    for (int i = 0; i < MO_N; ++i) if (mo11_have[i]) { const LvTower &T = mo11_tw[i];''')
rep('T.byt, T.byb, mo_hn[i], mo11_dxv[i]', 'T.byt, T.byb, i < 12 ? mo_hn[i] : 0, mo11_dxv[i]')
rep('''    if (mo_txt_bad) printf("  MOWHY %s\\n", mo_txt_why);''', '''    for (int i = 0; i < MO_N; ++i) if (mo11_have[i]) printf("  MOEMB %d look=%d threat=%s hs=%.3f e=[%d,%d,%d]\\n", i, MO_LOOKOF[i], MO_TNAME[MO_THREAT[i]], MO_HFIX[MO_LOOKOF[i]], mo_ex[i], mo_ey[i], mo_eS[i]);
    if (mo_txt_bad) printf("  MOWHY %s\\n", mo_txt_why);''')
assert 'mo11_tower_fx(LvBand &B, const LvTower &T)\n{' not in s
if J.get('tw', 1.35) != 1.35:   # v4 (Mobandy 9:27 AM ET: bigger emblems need wider faces): pillar width scale from layout18.json
    rep('#define MO11_TW 1.35f    /* lv_rd_skin_tw: pillars a little broader than Gibson\'s needles (touch) */', '#define MO11_TW %gf    /* lv_rd_skin_tw (v4 2026-11-14: was 1.35; wider faces so the idle emblems can be ~20-28 px) */' % J['tw'])
open(TREE + '/src/live/lv_morpheus_v13.h', 'w').write(s)
# lv_inside_towers.h: 16 -> 24 slots
n0 = t.count('[16]'); t = t.replace('[16][14]', '[24][14]').replace('[16]', '[24]')
for a in ('for (int i = 0; i < 16; ++i) { itw_seen[i] = false; itw_lseen[i] = false; }', 'for (int i = 0; i < 16; ++i) itw_lseen[i] = false;'):
    assert t.count(a) == 1, a; t = t.replace(a, a.replace('< 16', '< 24'))
open(TREE + '/src/live/lv_inside_towers.h', 'w').write(t)
print('patched', TREE, 'H T1/T3/T2 =', HF, '[16] arrays ->24:', n0)
# ---- v5d (Mobandy 2:34 PM ET 'Shorten the near towers'): per-tower fixed depth height factor MO11_HK (layout18.json hk, src/v5d_heights.py)
if 'hk' in J:
    s = open(TREE + '/src/live/lv_morpheus_v13.h').read()
    a = '    T.H = MO_HFIX[MO_LOOKOF[i]];   /* morpheus-mix spread: one fixed height per style (three heights total) */'
    assert s.count(a) == 1
    s = s.replace(a, 'static_assert(MO_N == 18, "hk table");\n    T.H = MO_HFIX[MO_LOOKOF[i]] * MO11_HK[i];   /* v5d: group height x a fixed depth factor (near towers shorter), no breathing */')
    a = 'static float mo11_dz(int i, float t)'
    assert s.count(a) == 1
    s = s.replace(a, 'static const float MO11_HK[MO_N] = {%s};   /* v5d depth height factor */\n' % f(J['hk']) + a)
    open(TREE + '/src/live/lv_morpheus_v13.h', 'w').write(s); print('v5d: MO11_HK applied')
# ---- v5d open sequence (Mobandy 2:36 - 2:38 PM ET): Gibson lock (lv_radar.h lv_rd_cg_brackets timing + lv_rd_box / lv_rd_brackets /
# lv_rd_rect_blend called directly), then the HACKbot search (lv_dl_drop key bar + 11-band tower meter, lvi_* calls), then the HACKbot
# folder drop (lv_dl_drop geometry + cubic ease + 21-frame stagger, 26-frame drop); folders only after the search is done.
if 'hk' in J and J.get('open') == 'v5d':   # v5e: REJECTED 3:23 PM ET, off (v5c open state restored)
    s = open(TREE + '/src/live/lv_morpheus_v13.h').read()
    def r1(a, b):
        global s
        assert s.count(a) == 1, a[:60]; s = s.replace(a, b)
    r1('#define MO12_BAR0 120       /* search bar wipes in, then types the device */', '#define MO12_LOCK_MS 1000    /* v5d: Gibson lock - hold brackets close in for 1 s (lv_rd_cg_brackets), then the snap */\n#define MO12_SWEEP_FR 3      /* v5d: HACKbot tower meter, one band per 3 frames (30 fps) */\n#define MO12_BAR0 1200      /* v5d: HACKbot search bar after the lock snap (was 120) */')
    r1('#define MO12_DROP0 300      /* folders: DOWNLOAD\'s eased stagger (110 ms apart, 380 ms each) */', '#define MO12_DROP0 mo12_drop0()   /* v5d: folders only after the search is done (src/v5d_open.inc.txt) */')
    r1('#define MO12_GAP 110', '#define MO12_GAP 700        /* v5d: HACKbot stagger 21 frames */')
    r1('#define MO12_DROP_MS 380', '#define MO12_DROP_MS 867    /* v5d: HACKbot drop 26 frames */')
    r1('static const int MO12_FX = 92, MO12_FW = 143, MO12_FY0 = 60, MO12_FDY = 62, MO12_FH = 46;', 'static const int MO12_FX = 90, MO12_FW = 145, MO12_FY0 = 78, MO12_FDY = 50, MO12_FH = 40;   /* v5d: HACKbot folder x 90 w 145, first at 78; 40 tall, 50 apart */')
    a = s.index('static void mo12_search(LvBand &B, const LvTower &T, int i, int el)'); b = s.index('\n/* ---- per frame: the Gibson layout on the Morpheus world')
    s = s[:a] + open(D + '/v5d_search.inc.txt').read() + s[b:]
    r1('const int end = MO12_FY0 + j * MO12_FDY, start = LV_RD_Y0 - 50; ys[j] = start + (int)((end - start) * mo_ease(u));',
       'const float uu = u < 0 ? 0 : u > 1 ? 1 : u; const int end = MO12_FY0 + j * MO12_FDY, start = j == 0 ? MO12_FY0 - 16 : MO12_FY0 + (j - 1) * MO12_FDY + 6; ys[j] = start + (int)((end - start) * (1.f - (1.f - uu) * (1.f - uu) * (1.f - uu)));   /* v5d: HACKbot lv_dl_drop ease */')
    r1('printf("  MOFOLD nland=%d typed=%d ys=", mo12_nland, mo12_typed);', 'printf("  MOFOLD nland=%d typed=%d lit=%d drop0=%d ys=", mo12_nland, mo12_typed, mo12_lit, mo12_drop0());')
    open(TREE + '/src/live/lv_morpheus_v13.h', 'w').write(s); print('v5d: open sequence applied')
# ---- v5e (Mobandy 3:23-3:25 PM ET): v5c open state restored; ONLY the file-search step changes: the scrolls drop after the search
# has typed the whole device name (search first, then the files), not 300 ms after the tap while it is still typing
s = open(TREE + '/src/live/lv_morpheus_v13.h').read()
_a = "#define MO12_DROP0 300      /* folders: DOWNLOAD's eased stagger (110 ms apart, 380 ms each) */"
if _a in s:
    s = s.replace(_a, "#define MO12_DROP0 (MO12_BAR0 + MO12_WIPE_MS + (itw_mo_sel >= 0 ? (int)strlen(MO_DEV[itw_mo_sel].name) : 0) * MO12_TYPE_MS + 200)   /* v5e: folders drop 200 ms after the search finished typing the device (DOWNLOAD's eased stagger kept) */")
    open(TREE + '/src/live/lv_morpheus_v13.h', 'w').write(s); print('v5e: search then files')
# ---- v5f: folder squares 50 px tall (room for the instrument row under the fact line); stagger pitch unchanged (62)
s = open(TREE + '/src/live/lv_morpheus_v13.h').read()
if 'MO12_FDY = 62, MO12_FH = 46;' in s:
    s = s.replace('MO12_FDY = 62, MO12_FH = 46;', 'MO12_FDY = 62, MO12_FH = 50;'); open(TREE + '/src/live/lv_morpheus_v13.h', 'w').write(s); print('v5f: folder h 50')
