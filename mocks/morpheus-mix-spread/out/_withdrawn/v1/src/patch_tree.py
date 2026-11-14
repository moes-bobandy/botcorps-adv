#!/usr/bin/env python3
"""morpheus-mix tree patch. Base: /workspace/cyd-morpheus-v6-wt src/live/lv_morpheus_v13.h (v13.1) + lv_inside_towers.h.
Applies: towers-3 renames (v13.1 skin / folder kept, not called) + mo_mix.inc include, 20 devices (MO_N), 8 new from the SquachWatch README,
threat -> look tables, slight per-style height scale (MO_HSC_LOOK, applied in mo11_drift on the engine height), vector emblems.
Usage: patch_tree.py TREE [--nohs]   (--nohs builds the same tree with -DMO_HSC_OFF behaviour baked in, for the A/B gate)"""
import json, os, sys, re
D = os.path.dirname(os.path.abspath(__file__)); TREE = sys.argv[1]; NOHS = '--nohs' in sys.argv
BASE = '/workspace/cyd-morpheus-v6-wt/src/live/'
J = json.load(open(D + '/layout.json'))
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
/* MOCK 2026-11-13 morpheus-mix (on v13.1 + morpheus-towers-3 looks): 20 device pillars, look per device by threat level
 * (HIGH -> T1 code glass, MEDIUM -> T3 sentinel lantern, LOW -> T2 construct white light), slight per-style height scale on top of the
 * engine height (MO_HSC_LOOK), crisp vector emblems (src/mo_emblem.h). Mock only, EX.DATA. */
#include "../../../src/mo_emblem.h"
#define MO_N 20
struct MoDev''')
rep('static const MoDev MO_DEV[12] = {', 'static const MoDev MO_DEV[MO_N] = {')
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
    'static const int MO_ICON[MO_N] = {LV_IC_AP, LV_IC_BLE, -1, LV_IC_LAPTOP, LV_IC_CCTV, LV_IC_DRONE, LV_IC_AIRTAG, LV_IC_KEYFOB, LV_IC_GLASSES, LV_IC_SKIMMER, LV_IC_FLIPPER, LV_IC_GALAXYTAG, -1, -1, -1, -1, -1, -1, -1, -1};   /* old art (v13.1 only); mix pillars use mo_emblem.h */')
THREAT = [2,2,0,0,2,2,1,1,1,0,0,1, 1,2,2,1,2,1,0,2]; TLOOK = [1,3,2]
h1, h2 = (1.0, 1.0) if NOHS else (J['h1'], J['h2'])
rep('static const int MO_KIND[12] = {0, 3, 0, 0, 1, 1, 2, 2, 1, 3, 3, 2};', '''static const int MO_KIND[MO_N] = {0, 3, 0, 0, 1, 1, 2, 2, 1, 3, 3, 2, 1, 1, 1, 1, 1, 2, 0, 3};
/* threat (0 HIGH, 1 MEDIUM, 2 LOW) -> look (1 T1 code glass, 3 T3 sentinel lantern, 2 T2 construct white light); provisional split */
static const int MO_THREAT[MO_N] = {%s};
static const char *const MO_TNAME[3] = {"HIGH", "MEDIUM", "LOW"};
static const int MO_TLOOK[3] = {1, 3, 2};
static const int MO_LOOKOF[MO_N] = {%s};
/* slight per-style height scale on the engine height (T1 tallest, T3 unchanged, T2 shortest; tuned for idle emblem visibility) */
#ifdef MO_HSC_OFF
static const float MO_HSC_LOOK[4] = {1.0f, 1.0f, 1.0f, 1.0f};
#else
static const float MO_HSC_LOOK[4] = {1.0f, %.3ff, %.3ff, 1.0f};
#endif''' % (', '.join(map(str, THREAT)), ', '.join(str(TLOOK[x]) for x in THREAT), h1, h2))
for k, nm in (('sx','MO11_SX'),('z0','MO11_Z0'),('ax','MO11_AX'),('wx','MO11_WX'),('az','MO11_AZ'),('wz','MO11_WZ'),('wh','MO11_WH')):
    m = re.search(r'static const float %s\[12\] = \{([^}]*)\};' % nm, s); assert m, nm
    s = s.replace(m.group(0), 'static const float %s[MO_N] = {%s, %s};' % (nm, m.group(1), f(J[k])))
m = re.search(r'static const int8_t MO11_RSSI\[12\] = \{([^}]*)\};', s)
s = s.replace(m.group(0), 'static const int8_t MO11_RSSI[MO_N] = {%s, %s};' % (m.group(1), ', '.join(map(str, J['rssi']))))
rep('static LvTower mo11_tw[12]; static bool mo11_have[12]; static float mo11_cam = 0; static int MO_ORD[12], mo11_nord = 0;',
    'static LvTower mo11_tw[MO_N]; static bool mo11_have[MO_N]; static float mo11_cam = 0; static int MO_ORD[MO_N], mo11_nord = 0;')
rep('static float mo11_dxv[12], mo11_dzv[12], mo11_hk[12];', 'static float mo11_dxv[MO_N], mo11_dzv[MO_N], mo11_hk[MO_N];')
# drift: identical v11 law for all 20; slight style height on the engine height
rep('''    const int i = T.i; if (i < 0 || i >= 12) return; const float t = mo_t();
    const float Z0 = MO11_Z0[i]''', '''    const int i = T.i; if (i < 0 || i >= MO_N) return; const float t = mo_t();
    T.H *= MO_HSC_LOOK[MO_LOOKOF[i]];   /* morpheus-mix: slight per-style height (engine H law unchanged) */
    const float Z0 = MO11_Z0[i]''')
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
rep('"DOJO CONSTRUCT  12 PILLARS"', '"DOJO CONSTRUCT  20 PILLARS"')
rep('''    for (int i = 0; i < 12; ++i) if (mo11_have[i]) { const LvTower &T = mo11_tw[i];''', '''    for (int i = 0; i < MO_N; ++i) if (mo11_have[i]) { const LvTower &T = mo11_tw[i];''')
rep('T.byt, T.byb, mo_hn[i], mo11_dxv[i]', 'T.byt, T.byb, i < 12 ? mo_hn[i] : 0, mo11_dxv[i]')
rep('''    if (mo_txt_bad) printf("  MOWHY %s\\n", mo_txt_why);''', '''    for (int i = 0; i < MO_N; ++i) if (mo11_have[i]) printf("  MOEMB %d look=%d threat=%s hs=%.3f e=[%d,%d,%d]\\n", i, MO_LOOKOF[i], MO_TNAME[MO_THREAT[i]], MO_HSC_LOOK[MO_LOOKOF[i]], mo_ex[i], mo_ey[i], mo_eS[i]);
    if (mo_txt_bad) printf("  MOWHY %s\\n", mo_txt_why);''')
assert 'mo11_tower_fx(LvBand &B, const LvTower &T)\n{' not in s
open(TREE + '/src/live/lv_morpheus_v13.h', 'w').write(s)
# lv_inside_towers.h: 16 -> 24 slots
n0 = t.count('[16]'); t = t.replace('[16][14]', '[24][14]').replace('[16]', '[24]')
for a in ('for (int i = 0; i < 16; ++i) { itw_seen[i] = false; itw_lseen[i] = false; }', 'for (int i = 0; i < 16; ++i) itw_lseen[i] = false;'):
    assert t.count(a) == 1, a; t = t.replace(a, a.replace('< 16', '< 24'))
open(TREE + '/src/live/lv_inside_towers.h', 'w').write(t)
print('patched', TREE, 'h1=%g h2=%g' % (h1, h2), '[16] arrays ->24:', n0)
