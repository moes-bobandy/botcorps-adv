#!/usr/bin/env python3
"""morpheus-mix: towers3_mo_looks.inc (compile-time MO_LOOK) -> mo_mix.inc (look chosen per device at run time by threat level, 20 devices,
vector emblems). Only switches / tables / emblem calls change; every look's drawing code is towers-3's."""
import os
D=os.path.dirname(os.path.abspath(__file__)); s=open(D+'/towers3_mo_looks.inc').read()
def rep(a,b,cnt=1):
    global s
    n=s.count(a); assert n==cnt,(a[:60],n); s=s.replace(a,b)
rep('''/* ==== MOCK 2026-11-13 morpheus-towers-3: three NEW pillar looks + matching folders (MO_LOOK 1 code glass, 2 construct light, 3 sentinel post).''',
'''/* ==== MOCK 2026-11-13 morpheus-mix (from morpheus-towers-3 mo_looks.inc): the three pillar looks MIXED per device by threat level
 * (MO_LOOKOF[i]: HIGH -> 1 T1 code glass, MEDIUM -> 3 T3 sentinel lantern, LOW -> 2 T2 construct white light), 20 devices, and the device
 * icon art replaced by crisp anti-aliased vector emblems (mo_emblem.h). The look code below is towers-3's, only switched at run time.
 * towers-3 header: three NEW pillar looks + matching folders (MO_LOOK 1 code glass, 2 construct light, 3 sentinel post).''')
rep('''#ifndef MO_LOOK
#define MO_LOOK 1
#endif
static const char *const MO3_FN[12][8] = {''','''static const char *const MO3_FN[MO_N][8] = {''')
rep('''    {"STAG.TRK", "FD5A.UUID", "FD59.UUID", "OWNER.ID", "UWB.PING", "SMART.THG", "SCAN.DEF", "RING.CMD"}};''',
'''    {"STAG.TRK", "FD5A.UUID", "FD59.UUID", "OWNER.ID", "UWB.PING", "SMART.THG", "SCAN.DEF", "RING.CMD"},
    {"AXON.CAM", "AB3.SSID", "TASER.ID", "OUI.LST", "BODYCAM.V", "LE.GEAR", "RSSI.DAT", "ASK.DEF"},
    {"RAVEN.SNS", "UUID.3100", "SHOT.LOG", "POLE.LOC", "MIC.ARY", "ALERT.TX", "UUID.3500", "ASK.DEF"},
    {"ALPR.CAM", "MOTO.OUI", "GENTC.OUI", "PLATE.LOG", "HOTLST.DB", "GPS.MAP", "KEEP.DAYS", "CITY.DEF"},
    {"IPCAM.NET", "WYZE.OUI", "VRKDA.OUI", "TUYA.OUI", "RTSP.STM", "AXIS.OUI", "ROOM.CHK", "COVER.DEF"},
    {"RING.CAM", "RING.OUI", "AMZN.OUI", "PORCH.MP4", "MOTION.LG", "NEIGH.APP", "SHARE.REQ", "ASK.DEF"},
    {"FMDN.TAG", "FEAA.UUID", "CHIPL.ID", "PBBEE.ID", "MOTO.TAG", "OWNER.KEY", "SCAN.DEF", "RING.CMD"},
    {"TWIN.SSID", "BSSID.A", "BSSID.B", "ENC.DIFF", "BEACON.LG", "CAPTV.PRT", "KARMA.SIG", "LOCK.DEF"},
    {"IBCN.ADV", "4C000215", "UUID.MAJ", "MINOR.ID", "STORE.LOC", "SHOP.TRK", "PUSH.ADS", "BLE.OFF"}};
/* vector emblem blit: 1 px halo (contrast ink), body, detail; coverage alpha into RGB565 (mo13_px blends, field-clipped) */
static int mo_ex[MO_N], mo_ey[MO_N], mo_eS[MO_N];   /* this frame's drawn emblem rect per pillar (dump / gate) */
static void mo_emb_draw(LvBand &B, int i, int x0, int y0, int S, uint32_t bc, uint32_t dc, uint32_t hc, int a, int ha, int yhi = 9999)
{
    const MoEmb *E = mo_emb_get(i, S); if (!E) return; S = E->S; const int H = S + 2;
    if (ha > 0) { mo_ex[i] = x0; mo_ey[i] = y0; mo_eS[i] = S; }   /* only the pillar emblems (with halo) are recorded; chip / folder glyphs are not */
    const uint16_t BC = lv_hex(bc), DC = lv_hex(dc), HC = lv_hex(hc);
    if (ha > 0) for (int y = 0; y < H; ++y) { if (y0 - 1 + y > yhi) break; for (int x = 0; x < H; ++x) { const int k = E->halo[y * H + x]; if (k) mo13_px(B, x0 - 1 + x, y0 - 1 + y, HC, ha * k / 255); } }
    for (int y = 0; y < S; ++y) { if (y0 + y > yhi) break; for (int x = 0; x < S; ++x) { const int kb = E->body[y * S + x], kd = E->det[y * S + x];
        if (kb) mo13_px(B, x0 + x, y0 + y, BC, a * kb / 255); if (kd) mo13_px(B, x0 + x, y0 + y, DC, a * kd / 255); } }
}
static inline int mo_embS(int fw) { return fw >= 50 ? 40 : fw >= 44 ? 32 : fw >= 20 ? 16 : fw >= 12 ? 10 : fw >= 8 ? 8 : 0; }   /* idle steps (stable, no size jitter); close-up 40 px */''')
rep('''    const int i = T.i; if (i < 0 || i >= 12) return;
    const bool open''','''    const int i = T.i; if (i < 0 || i >= MO_N) return;
    const bool open''')
rep('''    const int S = fw >= 44 ? 32 : fw >= 20 ? 16 : fw >= 10 ? 8 : 0;
#if MO_LOOK == 1''','''    const int S = mo_embS(fw);
    const int L = MO_LOOKOF[i];
    if (L == 1) {''')
rep('''#elif MO_LOOK == 2
    /* ===== T2''','''    } else if (L == 2) {
    /* ===== T2''')
rep('''#else
    /* ===== T3''','''    } else {
    /* ===== T3''')
rep('''    if (fw >= 10) { mo13_line(B, x0 + 1, bt0 + bh + 1, x0 + 1, bb0 - 2, lv_hex(MO11_G), a * 160 / 256); }
#endif
}''','''    if (fw >= 10) { mo13_line(B, x0 + 1, bt0 + bh + 1, x0 + 1, bb0 - 2, lv_hex(MO11_G), a * 160 / 256); }
    }
}''')
rep('''    if (S) { const int ix = (x0 + x1) / 2 - S / 2; mo3_glow_icon(B, i, ix, iy, S, MO11_GH, a * 110 / 256); mo3_icon(B, i, ix, iy, S, 0xc8ffdc, MO11_G, 0xffffff, a, a * 200 / 256); }''',
'''    if (S) { const int ix = (x0 + x1) / 2 - S / 2; mo_emb_draw(B, i, ix, iy, S, 0xd8ffe6, MO11_GH, 0x010805, a, a * 210 / 256); }''')
rep('''        mo3_glow_icon(B, i, ix, iy, S, 0x9fe8b8, a * 90 / 256); mo3_icon(B, i, ix, iy, S, 0x007a2c, 0x7fd89c, 0xffffff, a, a * 170 / 256); }''',
'''        mo_emb_draw(B, i, ix, iy, S, 0x006a26, 0x00b843, 0xffffff, a, a * 220 / 256); }''')
rep('''        const bool big = fw >= 40; const int Sl = big ? 32 : (px1 - px0 + 1) >= 16 ? 16 : (px1 - px0 + 1) >= 8 ? 8 : 0;''',
'''        const bool big = fw >= 40; const int pw = px1 - px0 + 1, Sl = big ? (pw >= 44 ? 40 : 32) : pw >= 16 ? 16 : pw >= 10 ? 10 : pw >= 8 ? 8 : 0;''')
rep('''        if (Sl) { const int ix = (px0 + px1 + 1) / 2 - Sl / 2; mo3_icon(B, i, ix, iy, Sl, 0x2a1608, 0x6a3e18, 0xfff4d6, a * 220 / 256, a * 150 / 256, py1); }''',
'''        if (Sl) { const int ix = (px0 + px1 + 1) / 2 - Sl / 2; mo_emb_draw(B, i, ix, iy, Sl, 0x2a1608, 0x00863a, 0xfff4d6, a, a * 170 / 256, py1); }''')
a=s.index('''#if MO_LOOK == 1
    const uint32_t BODY'''); b=s.index('#endif',a)+len('#endif')
s=s[:a]+'''const int MO_LOOK = MO_LOOKOF[i];   /* the opened pillar's own look */
    uint32_t BODY, SCAN, HEAD, OUT, IN, TAB, TABT, NAME, SIZE, ACT, BARF, BARB, GLOW, SHAD, IK, IB, IC;
    if (MO_LOOK == 1) { BODY = 0x03140b; SCAN = 0x072a17; HEAD = 0x0b3a22; OUT = 0x10d858; IN = 0x0a6a30; TAB = 0x10d858; TABT = 0x03140b; NAME = 0xd8ffe6; SIZE = 0x10d858; ACT = 0x5ac080; BARF = 0x10d858; BARB = 0x0a6a30; GLOW = 0x10d858; SHAD = 0x0a1a10;
        IK = 0xc8ffdc; IB = 0x10d858; IC = 0xffffff; }
    else if (MO_LOOK == 2) { BODY = 0xfafdfb; SCAN = 0xeef5f1; HEAD = 0xdcf5e6; OUT = 0x00a83c; IN = 0x9fe0b4; TAB = 0x00a83c; TABT = 0xffffff; NAME = 0x1e2a24; SIZE = 0x00a83c; ACT = 0x6a7a72; BARF = 0x00a83c; BARB = 0xcfe8d8; GLOW = 0x10d858; SHAD = 0x5a5448;
        IK = 0x006a26; IB = 0x00b843; IC = 0xffffff; }
    else { BODY = 0x1c120c; SCAN = 0x24170e; HEAD = 0x3a2414; OUT = 0xd8aa52; IN = 0x00a83c; TAB = 0xd8aa52; TABT = 0x1c120c; NAME = 0xf6f0e2; SIZE = 0x10d858; ACT = 0xc8b088; BARF = 0xd8aa52; BARB = 0x3a2414; GLOW = 0xffd878; SHAD = 0x1e1a16;
        IK = 0xfff0c8; IB = 0xd8aa52; IC = 0xffffff; }
    (void)IC;'''+s[b:]
rep('''#if MO_LOOK == 3
    { const uint16_t cap''','''    if (MO_LOOK == 3) { const uint16_t cap''')
rep('''mo12_rect(B, x + w - 2, y + h - L + 1, 3, L, cap, 256); }
#endif''','''mo12_rect(B, x + w - 2, y + h - L + 1, 3, L, cap, 256); }''')
rep('''    static const char *const MO13_FN[12][3] = {''','''    static const char *const MO13_FN[MO_N][3] = {''')
rep('''{"SMARTTAG.TRK", "FD5A.SIG", "TAGSCAN.DEF"}};
    static const char *const MO13_ACT[3]''','''{"SMARTTAG.TRK", "FD5A.SIG", "TAGSCAN.DEF"},
        {"AXON.CAM", "AB3SSID.SIG", "CAMCHK.DEF"},  {"RAVEN.SNS", "UUID31.SIG", "ASK.DEF"},   {"ALPR.CAM", "MOTOOUI.SIG", "PLATE.DEF"},
        {"IPCAM.NET", "CAMOUI.SIG", "ROOMCHK.DEF"}, {"RING.CAM", "RINGOUI.SIG", "PORCH.DEF"}, {"FMDN.TRK", "FEAA.SIG", "TAGSCAN.DEF"},
        {"TWIN.AP", "2BSSID.SIG", "LOCKCHK.DEF"},   {"IBEACON.ADV", "4C0215.SIG", "BTOFF.DEF"}};
    static const char *const MO13_ACT[3]''')
rep('''    mo3_icon(B, i, x + 6, y + 6, 8, IK, IB, IC, 256, 256);   /* device glyph in the header strip */''',
'''    mo_emb_draw(B, i, x + 5, y + 5, 10, IK, IB, 0, 256, 0);   /* device emblem (vector, 10 px) in the header strip */''')
rep('''#if MO_LOOK == 2
        const int sx0''','''        if (MO_LOOK == 2) {
        const int sx0''')
rep('''if (xx > xl(yy) + 1 && xx < xr(yy) - 1) mo_bl(B, xx, yy, lv_hex(0xffffff), 150 - abs(d) * 30); }
#else''','''if (xx > xl(yy) + 1 && xx < xr(yy) - 1) mo_bl(B, xx, yy, lv_hex(0xffffff), 150 - abs(d) * 30); }
        } else {''')
rep('''mo_bl(B, px, py + 1, lv_hex(MO_LOOK == 1 ? 0xeafff1 : 0xfff8d8), al / 2); }
#endif''','''mo_bl(B, px, py + 1, lv_hex(MO_LOOK == 1 ? 0xeafff1 : 0xfff8d8), al / 2); }
        }''')
assert not any(l.lstrip().startswith('#') for l in s.splitlines()), [l for l in s.splitlines() if l.lstrip().startswith('#')]
open(D+'/mo_mix.inc','w').write(s); print('mo_mix.inc written')
