#!/usr/bin/env python3
import sys
"""morpheus-mix: towers3_mo_looks.inc (compile-time MO_LOOK) -> mo_mix.inc (look chosen per device at run time by threat level, 18 devices (WIFI + BLUETOOTH removed 2026-11-14),
vector emblems). Only switches / tables / emblem calls change; every look's drawing code is towers-3's."""
import os
D=os.path.dirname(os.path.abspath(__file__)); s=open(D+'/towers3_mo_looks.inc').read()
def rep(a,b,cnt=1):
    global s
    n=s.count(a); assert n==cnt,(a[:60],n); s=s.replace(a,b)
rep('''/* ==== MOCK 2026-11-13 morpheus-towers-3: three NEW pillar looks + matching folders (MO_LOOK 1 code glass, 2 construct light, 3 sentinel post).''',
'''/* ==== MOCK 2026-11-13 morpheus-mix (from morpheus-towers-3 mo_looks.inc): the three pillar looks MIXED per device by threat level
 * (MO_LOOKOF[i]: HIGH -> 1 T1 code glass, MEDIUM -> 3 T3 sentinel lantern, LOW -> 2 T2 construct white light), 18 devices (WIFI + BLUETOOTH removed 2026-11-14), and the device
 * icon art replaced by crisp anti-aliased vector emblems (mo_emblem.h). The look code below is towers-3's, only switched at run time.
 * towers-3 header: three NEW pillar looks + matching folders (MO_LOOK 1 code glass, 2 construct light, 3 sentinel post).''')
rep('''#ifndef MO_LOOK
#define MO_LOOK 1
#endif
static const char *const MO3_FN[12][8] = {
    {"SSID.LST", "BEACON.LG", "CHAN.MAP", "WPA3.CFG", "RSSI.DAT", "OPEN.NET", "MAC.OUI", "VPN.DEF"},
    {"ADVERT.LG", "MAKER.ID", "PAIR.REQ", "RSSI.DAT", "NAME.LST", "UUID.MAP", "LE.SCAN", "OFF.DEF"},''','''static const char *const MO3_FN[MO_N][8] = {   /* 2026-11-14 spread: WIFI + BLUETOOTH rows removed (18 devices) */''')
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
rep('''    static const char *const MO13_FN[12][3] = {
        {"ROUTER.NET", "BEACON.SIG", "WPA3.DEF"},   {"BT.RADIO", "ADVERT.SIG", "PAIR.DEF"},  {"DEAUTH.PKT", "FLOOD.SIG", "PMF.DEF"},''','''    static const char *const MO13_FN[MO_N][3] = {   /* 2026-11-14 spread: WIFI + BLUETOOTH removed */
        {"DEAUTH.PKT", "FLOOD.SIG", "PMF.DEF"},''')
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
# ---- 2026-11-14 (Mobandy 8:08 AM ET): ONLY the T3 (wood) pillars become SEE-THROUGH WOOD: the translucent see-through treatment of the
# glass towers, in wood (grain + tone), so the emblem and the scrolling file line-items show through. T1 + T2 code untouched.
a = s.index('    } else {\n    /* ===== T3 DOJO SENTINEL POST'); b = s.index('    }\n}\n/* ---- the 3 right-hand folders')
s = s[:a] + """    } else {
    /* ===== T3 SEE-THROUGH WOOD (2026-11-14): translucent wood glass (grain + warm tone, the room shows through), thin brass bands,
     * the device emblem and the scrolling file line-items inside the pillar, green trim. Geometry / emblem sizes / file list as before. ===== */
    if (cp < 0.3f) mo3_shadow(B, x0, x1, yb, 0x2a2118, a * 80 / 256);
    const int bh = fw >= 40 ? 4 : fw >= 14 ? 2 : 1;   /* thin brass band height */
    const int bt0 = yt + (fw >= 40 ? 4 : 2), bb0 = yb - (fw >= 40 ? 10 : 3) - bh;
    auto brass = [&](int k, int n) -> uint16_t { const float v = k / (float)(n > 1 ? n - 1 : 1); return v < 0.35f ? mo3_mx(0xfff0b0, 0xd8aa52, v / 0.35f) : mo3_mx(0xd8aa52, 0x6a4a18, (v - 0.35f) / 0.65f); };
    auto grain = [&](int x, int y, float u) -> uint16_t {   /* wood grain: wavy vertical figure, light / dark streaks */
        const float g = sinf((x - x0) * (fw >= 40 ? 0.55f : 1.3f) + 1.6f * sinf(y * 0.11f + i * 1.3f) + i);
        const float k = 0.5f + 0.5f * g; return mo3_mx(0x9a6232, 0x3a2010, 0.25f + 0.6f * (1 - k) * (0.6f + 0.4f * u)); };
    auto side = [&](int xf, int xbk) { const int dir = xbk > xf ? 1 : -1, n = abs(xbk - xf); if (n < 1) return;
        for (int s = 0; s <= n; ++s) { const int x = xf + dir * s; const float u = s / (float)n; const int top = (int)lroundf(yt + (byt - yt) * u), bot = (int)lroundf(yb + (byb - yb) * u);
            for (int y = top; y <= bot; ++y) mo13_px(B, x, y, mo3_mx(0x6a4020, 0x2a160a, u), a * 105 / 256); }
        mo13_line(B, xbk, byt, xbk, byb, lv_hex(0x8a5a2a), a * 200 / 256); mo13_line(B, xf, yt, xbk, byt, lv_hex(0xd8aa52), a); };
    if (bx1 > x1) side(x1, bx1);
    if (bx0 < x0) side(x0, bx0);
    if (byt < yt) { const float X[4] = {(float)x0, (float)bx0, (float)bx1, (float)x1}, Y[4] = {(float)yt, (float)byt, (float)byt, (float)yt}; mo_quad(B, X, Y, lv_hex(0xb08a3a), a * 150 / 256); mo13_line(B, bx0, byt, bx1, byt, lv_hex(0xfff0b0), a); }
    /* translucent wood front: grain at ~38% (v3; v2 was 55%) so the room / rain behind shows through, a soft sheen near the lit edge, a faint warm inner light */
    for (int y = yt; y <= yv; ++y) { const float v = (y - yt) / (float)(yb - yt + 1);
        for (int x = x0; x <= x1; ++x) { const float u = (x - x0) / (float)fw, sh = expf(-((u - 0.2f) * (u - 0.2f)) / 0.012f);
            mo13_px(B, x, y, grain(x, y, u), a * 96 / 256);   /* v3: ~38% wood so the room shows through at idle too */
            if (sh > 0.05f) mo13_px(B, x, y, lv_hex(0xffe0a8), (int)(a * 46 * sh / 256));
            const float w = 1 - fabsf(u - 0.5f) * 2; if (w > 0) mo13_px(B, x, y, lv_hex(0xffc860), (int)(a * 26 * w * (1 - 0.5f * v) / 256)); } }
    /* thin brass bands with a green trim line (the pillar's frame) */
    for (int k = 0; k < bh; ++k) { mo13_span(B, bt0 + k, x0, x1, brass(k, bh), a); mo13_span(B, bb0 + k, x0, x1, brass(k, bh), a); }
    if (fw >= 8) { mo13_span(B, bt0 + bh, x0 + 1, x1 - 1, lv_hex(MO11_G), a * 200 / 256); mo13_span(B, bb0 - 1, x0 + 1, x1 - 1, lv_hex(MO11_G), a * 200 / 256); }
    /* inside the wood: the device emblem + the scrolling file line-items, seen through */
    const int m = fw >= 40 ? 5 : fw >= 12 ? 2 : 1, px0 = x0 + m, px1 = x1 - m, py0 = bt0 + bh + (fw >= 40 ? 4 : 2), py1 = (bb0 - (fw >= 40 ? 4 : 2) < yv ? bb0 - (fw >= 40 ? 4 : 2) : yv);
    if (px1 - px0 >= 2 && py1 - py0 >= 4) {
        const bool big = fw >= 40; const int pw = px1 - px0 + 1, Sl = big ? (pw >= 44 ? 40 : 32) : pw >= 16 ? 16 : pw >= 10 ? 10 : pw >= 8 ? 8 : 0;
        const int iy = py0 + (big ? 4 : 2);
        if (Sl) { const int ix = (px0 + px1 + 1) / 2 - Sl / 2; mo_emb_draw(B, i, ix, iy, Sl, 0xfff0c8, MO11_GH, 0x1c120c, a, a * 190 / 256, py1); }
        const int ly0 = Sl ? iy + Sl + (big ? 8 : 2) : py0 + 1, ly1 = py1 - 1;
        if (big) mo13_line(B, px0 + 2, ly0 - 4, px1 - 2, ly0 - 4, lv_hex(0xd8aa52), a * 200 / 256);
        mo3_files(B, i, px0 + 1, px1 - 1, ly0, ly1, lv_hex(MO11_GH), lv_hex(big ? 0xfff0c8 : 0xe8c890), lv_hex(0xd8aa52), a, big ? 9.0f : 6.0f);
    }
    /* edges: lit brass left / top, dark right, green inner rail */
    mo13_line(B, x0, yt, x0, yv, lv_hex(0xd8aa52), a); mo13_line(B, x1, yt, x1, yv, lv_hex(0x3a2010), a); mo13_line(B, x0, yt, x1, yt, lv_hex(0xfff0b0), a); mo13_line(B, x0, yb, x1, yb, lv_hex(0x3a2010), a);
    if (fw >= 10) { mo13_line(B, x0 + 1, bt0 + bh + 1, x0 + 1, bb0 - 2, lv_hex(MO11_G), a * 140 / 256); }
""" + s[b:]

# ---- 2026-11-14 (Mobandy 9:06 AM ET): REAL folder text from folders18.py; text only, same folder / line-item layout ----
import subprocess as _sp, sys as _sys
_sp.run([_sys.executable, D + '/folders18.py'], check=True)
_FT = open(D + '/folders18.inc').read()
def rep2(a, b):
    global s
    assert s.count(a) == 1, a[:80]
    s = s.replace(a, b)
rep2('static void mo3_size(const char *nm, char *out)', _FT + 'static void mo3_size(const char *nm, char *out)')
rep2('const int e = (base + k) % 8; const char *nm = MO3_FN[i][e];', 'const int e = (base + k) % 8; const char *nm = MO_ITEM_K[i][e];   /* real KEY (was a made-up file name) */')
rep2('char sz[12]; mo3_size(nm, sz); mo3_t35(B, x0 + 9, y + 7, sz, sc, a, ylo, yhi, x1);', 'char sz[12]; snprintf(sz, sizeof sz, "%s", MO_ITEM_V[i][e]); mo3_t35(B, x0 + 9, y + 7, sz, sc, a, ylo, yhi, x1);   /* real VALUE (was a fake KB size) */')
rep2('mo3_t35(B, x0 + 8, y, nm, tc, a, ylo, yhi, x1); }', 'mo3_t35(B, x0 + 8, y, MO_ITEM_V[i][e], tc, a, ylo, yhi, x1); }   /* one-line rows: the VALUE (the fact) */')
rep2('const char *nm = MO13_FN[i][j]; b_text(B, x + 17, y + 7, nm, ink);', 'const char *nm = MO_FOLD_NAME[i][j]; b_text(B, x + 17, y + 7, nm, ink);   /* real folder name: ID / DETECT / ACTION */')
i0 = s.index('    const char *src = j == 0 ? MO_DEV[i].what'); i1 = s.index('b_text(B, sx, y + 20, sz, lv_hex(SIZE));')
s = s[:i0] + '    const char *sz = MO_FOLD_TAG[i][j]; const int sw = (int)strlen(sz) * 6 - 1, sx = x + w - 6 - sw;   /* right tag: threat / radio / DO (was a fake KB size) */\n    ' + s[i1:]
rep2('b_text35(B, x + 6, y + 22, MO13_ACT[j], lv_hex(ACT));', 'b_text35(B, x + 6, y + 22, MO_FOLD_ACT[i][j], lv_hex(ACT));')
rep2('lv_w35(MO13_ACT[j]), 5, x + 2, y + 18, sx - 3, y + 30, "fact");', 'lv_w35(MO_FOLD_ACT[i][j]), 5, x + 2, y + 18, sx - 3, y + 30, "fact");')

# ---- v4 2026-11-14 (Mobandy 9:27-9:29 AM ET): crisp + stable + opaque UI; white T2 see-through shell; bigger idle emblems ----
rep2('static int mo_ex[MO_N], mo_ey[MO_N], mo_eS[MO_N];   /* this frame\'s drawn emblem rect per pillar (dump / gate) */',
     """static int mo_ex[MO_N], mo_ey[MO_N], mo_eS[MO_N];   /* this frame's drawn emblem rect per pillar (dump / gate) */
/* v4: opaque-UI audit. Every emblem / on-tower folder / file-line pixel goes through mo_ui_px; a pixel drawn below the pillar's own
 * alpha (fade-out while another pillar is open) would be a translucent UI pixel. Counts are printed at exit (MOUI) for the gate. */
static long mo_ui_tot, mo_ui_part; static int mo_ui_a = 256, mo_ui_cnt = 1;   /* mo_ui_cnt = 0 while the T1 code rain (not UI) borrows the 3x5 text routine */
static inline void mo_ui_px(LvBand &B, int x, int y, uint16_t c, int al) { if (mo_ui_cnt) { ++mo_ui_tot; if (al < mo_ui_a) ++mo_ui_part; } mo13_px(B, x, y, c, al); }
struct MoUiRep { ~MoUiRep() { printf("MOUI tot=%ld part=%ld\\n", mo_ui_tot, mo_ui_part); fflush(stdout); } };
static MoUiRep mo_ui_rep;
static int mo_txt_oc = -1;   /* v4: 1 px opaque outline colour behind on-tower text (-1 none) */""")
# emblem: binary masks drawn fully opaque (halo outline, body, detail); no alpha ramps
rep2("if (ha > 0) for (int y = 0; y < H; ++y) { if (y0 - 1 + y > yhi) break; for (int x = 0; x < H; ++x) { const int k = E->halo[y * H + x]; if (k) mo13_px(B, x0 - 1 + x, y0 - 1 + y, HC, ha * k / 255); } }",
     "if (ha > 0) for (int y = 0; y < H; ++y) { if (y0 - 1 + y > yhi) break; for (int x = 0; x < H; ++x) { const int k = E->halo[y * H + x]; if (k) mo_ui_px(B, x0 - 1 + x, y0 - 1 + y, HC, a); } }   /* v4: opaque outline */")
rep2("if (kb) mo13_px(B, x0 + x, y0 + y, BC, a * kb / 255); if (kd) mo13_px(B, x0 + x, y0 + y, DC, a * kd / 255); } }",
     "if (kb) mo_ui_px(B, x0 + x, y0 + y, BC, a); if (kd) mo_ui_px(B, x0 + x, y0 + y, DC, a); } }   /* v4: binary masks, opaque */")
rep2("static inline int mo_embS(int fw) { return fw >= 50 ? 40 : fw >= 44 ? 32 : fw >= 20 ? 16 : fw >= 12 ? 10 : fw >= 8 ? 8 : 0; }   /* idle steps (stable, no size jitter); close-up 40 px */",
     """static inline int mo_embS(int fw) { return fw >= 50 ? 40 : fw >= 44 ? 32 : fw >= 20 ? 16 : fw >= 12 ? 10 : fw >= 8 ? 8 : 0; }   /* v3 steps (kept for reference) */
/* v4: idle emblem as large as the face allows (face interior = fw - 1 px), in 2 px steps with hysteresis so a drifting pillar does not
 * flip sizes frame to frame; close-up 40 px. Returns 0 when the face is too narrow (< 7 px). */
static int mo_eS_last[MO_N];
static int mo_embS4(int i, int fw)
{
    if (fw >= 50) return mo_eS_last[i] = 40;
    int fit = fw - 1; if (fit > 36) fit = 36; if (fit < 6) { mo_eS_last[i] = 0; return 0; }
    int &L = mo_eS_last[i]; if (L == 40 || L == 0 || L > fit + 1 || L < fit - 3) L = fit & ~1; return L;   /* v5: shrink only when the face is 2+ px too narrow (a 1 px overhang onto the edge line is drawn over it), so a +-1 px face wobble never flips the size */
}""")
# on-tower text + file pixels: hard clip (no fade ramp), opaque, optional 1 px outline
rep2("""            const int f = mo3_cl((yy - ylo) * 64, 0, 256) < mo3_cl((yhi - yy) * 64, 0, 256) ? mo3_cl((yy - ylo) * 64, 0, 256) : mo3_cl((yhi - yy) * 64, 0, 256);
            mo13_px(B, x + k % 3, yy, c, a * f / 256); } }""",
     """            if (mo_txt_oc >= 0) for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) { const int oy = yy + dy; if (oy < ylo || oy > yhi) continue;
                const int r2 = k / 3 + dy, c2 = k % 3 + dx; if (r2 >= 0 && r2 < 5 && c2 >= 0 && c2 < 3 && (g >> (14 - (r2 * 3 + c2)) & 1)) continue; mo_ui_px(B, x + c2, oy, lv_hex((uint32_t)mo_txt_oc), a); }
            mo_ui_px(B, x + k % 3, yy, c, a); } }   /* v4: hard clip, opaque (was a 4-row alpha fade) */""")
rep2("""static void mo3_pxc(LvBand &B, int x, int y, uint16_t c, int a, int ylo, int yhi) { if (y < ylo || y > yhi) return;
    const int f = mo3_cl((y - ylo) * 64, 0, 256) < mo3_cl((yhi - y) * 64, 0, 256) ? mo3_cl((y - ylo) * 64, 0, 256) : mo3_cl((yhi - y) * 64, 0, 256); mo13_px(B, x, y, c, a * f / 256); }""",
     """static void mo3_pxc(LvBand &B, int x, int y, uint16_t c, int a, int ylo, int yhi) { if (y < ylo || y > yhi) return; mo_ui_px(B, x, y, c, a); }   /* v4: hard clip, opaque */""")
rep2("for (int q = 0; q < bw; ++q) mo3_pxc(B, x0 + 9 + lv_w35(sz) + 3 + q, y + 9, q < fl ? ic : sc, q < fl ? a : a / 3, ylo, yhi); }",
     "for (int q = 0; q < bw; ++q) if (q < fl || (q & 1) == 0) mo3_pxc(B, x0 + 9 + lv_w35(sz) + 3 + q, y + 9, q < fl ? ic : sc, a, ylo, yhi); }   /* v4: opaque (unfilled = dotted, was 1/3 alpha) */")
rep2("for (int q = 0; q < L; ++q) mo3_pxc(B, x0 + 3 + q, y + 1, tc, a * 3 / 4, ylo, yhi); } }",
     "for (int q = 0; q < L; ++q) mo3_pxc(B, x0 + 3 + q, y + 1, tc, a, ylo, yhi); } }")
# tower: this frame's alpha for the audit, v4 emblem size
rep2("    const int S = mo_embS(fw);\n    const int L = MO_LOOKOF[i];", "    const int S = mo_embS4(i, fw); mo_ui_a = a;\n    const int L = MO_LOOKOF[i];")
# ---- T1: sharpen (no soft outer glow, opaque glass, crisp 1 px highlight instead of a blurred band), no emblem bob, opaque files panel
rep2("    for (int r = 1; r <= 2; ++r) for (int y = yt - r; y <= yb; ++y) { mo13_px(B, x0 - r, y, lv_hex(MO11_GH), a * (r == 1 ? 70 : 28) / 256); mo13_px(B, (bx1 > x1 ? bx1 : x1) + r, y, lv_hex(MO11_GH), a * (r == 1 ? 70 : 28) / 256); }\n",
     "    /* v4: no soft outer glow (it muddied the edges) */\n")
rep2("for (int x = x0; x <= x1; ++x) { const float u = (x - x0) / (float)fw, rb = expf(-((u - 0.22f) * (u - 0.22f)) / 0.006f); mo13_px(B, x, y, base, a * 245 / 256); if (rb > 0.05f) mo13_px(B, x, y, lv_hex(0xbfffd8), (int)(a * 34 * rb / 256)); } }",
     "const int hx = x0 + 1 + fw / 5; for (int x = x0; x <= x1; ++x) mo13_px(B, x, y, x == hx ? mo3_mx(0x0b3320, 0x5ac080, 0.45f - 0.3f * v) : base, a); }   /* v4: opaque glass + one crisp highlight column */")
rep2("    const int iy = yt + (S >= 32 ? 6 : 3) + (int)lroundf(sinf(t * 1.6f + i) * (S >= 16 ? 1.0f : 0.0f));\n    if (S) { const int ix = (x0 + x1) / 2 - S / 2; mo_emb_draw(B, i, ix, iy, S, 0xd8ffe6, MO11_GH, 0x010805, a, a * 210 / 256); }",
     "    const int iy = yt + (S >= 32 ? 6 : 3);   /* v4: no bob */\n    if (S) { const int ix = x0 + 1 + (fw - 1 - S) / 2; mo_emb_draw(B, i, ix, iy, S, 0xd8ffe6, MO11_GH, 0x010805, a, a); }")
rep2("    if (big) { for (int y = ly0 - 3; y <= ly1 + 1; ++y) mo13_span(B, y, x0 + 2, x1 - 2, lv_hex(0x010604), a * 170 / 256);",
     "    if (big) { for (int y = ly0 - 3; y <= ly1 + 1; ++y) mo13_span(B, y, x0 + 2, x1 - 2, lv_hex(0x010604), a);")
rep2("    mo3_files(B, i, x0 + 2, x1 - 2, ly0, ly1, lv_hex(MO11_GH), lv_hex(big ? 0xd8ffe6 : 0x7dffb0), lv_hex(0x3a9a60), a, big ? 9.0f : 6.0f);",
     "    mo_txt_oc = 0x010805; mo3_files(B, i, x0 + 2, x1 - 2, ly0, ly1, lv_hex(MO11_GH), lv_hex(big ? 0xd8ffe6 : 0x7dffb0), lv_hex(0x5ac080), a, big ? 9.0f : 6.0f); mo_txt_oc = -1;")
# ---- T2: SEE-THROUGH WHITE LIGHT shell (Mobandy 9:28 AM): translucent flat white front / sides / cap, crisp opaque green edges; no soft glow, no rising band, no bob
rep2("""    if (cp < 0.3f) { mo3_shadow(B, x0, x1, yb, 0x5a5448, a * 120 / 256);
        for (int y = yt - 2; y <= yb; ++y) for (int r = 1; r <= 2; ++r) { mo13_px(B, x0 - r, y, lv_hex(0xffffff), a * (r == 1 ? 140 : 60) / 256); } }""",
     "    if (cp < 0.3f) mo3_shadow(B, x0, x1, yb, 0x5a5448, a * 120 / 256);   /* v4: no soft white outer glow */")
rep2("for (int y = top; y <= bot; ++y) mo13_px(B, x, y, mo3_mx(0xd2dad6, 0x9aa49f, u * 0.75f + 0.25f * (y - top) / (float)(bot - top + 1)), a); }\n        mo13_line(B, xbk, byt, xbk, byb, lv_hex(MO11_G), a);",
     "for (int y = top; y <= bot; ++y) mo13_px(B, x, y, lv_hex(0xc4ccc8), a * 150 / 256); }   /* v4: see-through side */\n        mo13_line(B, xbk, byt, xbk, byb, lv_hex(MO11_G), a);")
rep2("mo_quad(B, X, Y, lv_hex(0xffffff), a); mo13_line(B, bx0, byt, bx1, byt, lv_hex(MO11_G), a); }\n    for (int y = yt; y <= yv; ++y) { const float v = (y - yt) / (float)(yb - yt + 1);\n        for (int x = x0; x <= x1; ++x) { const float u = (x - x0) / (float)fw; mo13_px(B, x, y, mo3_mx(0xffffff, 0xd9e2dd, 0.55f * u * u + 0.35f * v), a); } }",
     "mo_quad(B, X, Y, lv_hex(0xffffff), a * 170 / 256); mo13_line(B, bx0, byt, bx1, byt, lv_hex(MO11_G), a); }\n    for (int y = yt; y <= yv; ++y) mo13_span(B, y, x0, x1, lv_hex(0xf4f8f6), a * 120 / 256);   /* v4: SEE-THROUGH white light front (~47%), flat = crisp; the room / pillars behind show through */")
i0 = s.index("    /* a soft light band rising slowly through the slab (the Construct loading light) */"); i1 = s.index("    const bool big = fw >= 40;\n    const int iy = yt + (S >= 32 ? 7 : 3)")
s = s[:i0] + "    /* v4: the soft rising light band removed (alpha shimmer) */\n" + s[i1:]
rep2("    const int iy = yt + (S >= 32 ? 7 : 3) + (int)lroundf(sinf(t * 1.4f + i) * (S >= 16 ? 1.0f : 0.0f));\n    if (S) { const int ix = (x0 + x1) / 2 - S / 2;",
     "    const int iy = yt + (S >= 32 ? 7 : 3);   /* v4: no bob */\n    if (S) { const int ix = x0 + 1 + (fw - 1 - S) / 2;")
rep2("        mo_emb_draw(B, i, ix, iy, S, 0x006a26, 0x00b843, 0xffffff, a, a * 220 / 256); }", "        mo_emb_draw(B, i, ix, iy, S, 0x005a20, 0x00b843, 0xffffff, a, a); }")
rep2("    mo3_files(B, i, x0 + 2, x1 - 2, ly0, ly1, lv_hex(MO11_G), lv_hex(big ? 0x1e2a24 : 0x6a7a70), lv_hex(0x7a8a82), a, big ? 9.0f : 6.0f);",
     "    mo_txt_oc = 0xffffff; mo3_files(B, i, x0 + 2, x1 - 2, ly0, ly1, lv_hex(0x008a32), lv_hex(0x1e2a24), lv_hex(0x2e4a3a), a, big ? 9.0f : 6.0f); mo_txt_oc = -1;   /* v4: dark text, white outline, opaque */")
# ---- T3: sharpen the see-through wood (two-tone grain, no blurred sheen / glow), opaque trim; emblem bigger + opaque; outlined text
rep2("        const float k = 0.5f + 0.5f * g; return mo3_mx(0x9a6232, 0x3a2010, 0.25f + 0.6f * (1 - k) * (0.6f + 0.4f * u)); };",
     "        (void)u; return lv_hex(g > 0.15f ? 0x8a5428 : 0x4a2a12); };   /* v4: two-tone grain (crisp streaks, no ramp) */")
rep2("""            if (sh > 0.05f) mo13_px(B, x, y, lv_hex(0xffe0a8), (int)(a * 46 * sh / 256));
            const float w = 1 - fabsf(u - 0.5f) * 2; if (w > 0) mo13_px(B, x, y, lv_hex(0xffc860), (int)(a * 26 * w * (1 - 0.5f * v) / 256)); } }""",
     "            (void)sh; (void)v; } }   /* v4: blurred sheen + warm glow removed (muddy) */")
rep2("if (fw >= 8) { mo13_span(B, bt0 + bh, x0 + 1, x1 - 1, lv_hex(MO11_G), a * 200 / 256); mo13_span(B, bb0 - 1, x0 + 1, x1 - 1, lv_hex(MO11_G), a * 200 / 256); }",
     "if (fw >= 8) { mo13_span(B, bt0 + bh, x0 + 1, x1 - 1, lv_hex(MO11_G), a); mo13_span(B, bb0 - 1, x0 + 1, x1 - 1, lv_hex(MO11_G), a); }")
rep2("const bool big = fw >= 40; const int pw = px1 - px0 + 1, Sl = big ? (pw >= 44 ? 40 : 32) : pw >= 16 ? 16 : pw >= 10 ? 10 : pw >= 8 ? 8 : 0;",
     "const bool big = fw >= 40; const int Sl = S;   /* v4: same big idle emblem rule as T1 / T2 */")
rep2("if (Sl) { const int ix = (px0 + px1 + 1) / 2 - Sl / 2; mo_emb_draw(B, i, ix, iy, Sl, 0xfff0c8, MO11_GH, 0x1c120c, a, a * 190 / 256, py1); }",
     "if (Sl) { const int ix = x0 + 1 + (fw - 1 - Sl) / 2; mo_emb_draw(B, i, ix, iy, Sl, 0xfff0c8, MO11_GH, 0x1c120c, a, a, py1); }")
rep2("if (big) mo13_line(B, px0 + 2, ly0 - 4, px1 - 2, ly0 - 4, lv_hex(0xd8aa52), a * 200 / 256);", "if (big) mo13_line(B, px0 + 2, ly0 - 4, px1 - 2, ly0 - 4, lv_hex(0xd8aa52), a);")
rep2("mo3_files(B, i, px0 + 1, px1 - 1, ly0, ly1, lv_hex(MO11_GH), lv_hex(big ? 0xfff0c8 : 0xe8c890), lv_hex(0xd8aa52), a, big ? 9.0f : 6.0f);",
     "mo_txt_oc = 0x1c120c; mo3_files(B, i, px0 + 1, px1 - 1, ly0, ly1, lv_hex(MO11_GH), lv_hex(0xfff0c8), lv_hex(0xffd878), a, big ? 9.0f : 6.0f); mo_txt_oc = -1;   /* v4: cream text, dark outline, opaque */")
rep2("if (fw >= 10) { mo13_line(B, x0 + 1, bt0 + bh + 1, x0 + 1, bb0 - 2, lv_hex(MO11_G), a * 140 / 256); }", "if (fw >= 10) { mo13_line(B, x0 + 1, bt0 + bh + 1, x0 + 1, bb0 - 2, lv_hex(MO11_G), a); }")
rep2("mo3_t35(B, cx - 1, gy, gl, c, al, yt + 1, yv - 1, x1 - 1); }", "mo_ui_cnt = 0; mo3_t35(B, cx - 1, gy, gl, c, al, yt + 1, yv - 1, x1 - 1); mo_ui_cnt = 1; }   /* code rain: decoration, not audited UI */")
# v4 stability: the emblem is drawn again AFTER the pillar's edge lines / rails, so a 1 px change of the face width can never let an edge
# line cut into the emblem's outline (that was the frame-to-frame flicker on the emblem edge columns)
rep2("    mo13_line(B, x0, yt, x0, yv, lv_hex(0x7dffb0), a); mo13_line(B, x1, yt, x1, yv, lv_hex(MO11_GH), a); mo13_line(B, x0, yt, x1, yt, lv_hex(0xd8ffe6), a); mo13_line(B, x0, yb, x1, yb, lv_hex(0x0a6a30), a);\n",
     "    mo13_line(B, x0, yt, x0, yv, lv_hex(0x7dffb0), a); mo13_line(B, x1, yt, x1, yv, lv_hex(MO11_GH), a); mo13_line(B, x0, yt, x1, yt, lv_hex(0xd8ffe6), a); mo13_line(B, x0, yb, x1, yb, lv_hex(0x0a6a30), a);\n    if (S) mo_emb_draw(B, i, x0 + 1 + (fw - 1 - S) / 2, iy, S, 0xd8ffe6, MO11_GH, 0x010805, a, a);   /* v4: emblem on top of the edges */\n")
rep2("    mo13_line(B, x0, yt, x0, yv, lv_hex(MO11_G), a); mo13_line(B, x1, yt, x1, yv, lv_hex(MO11_G), a); mo13_line(B, x0, yt, x1, yt, lv_hex(MO11_GH), a); mo13_line(B, x0, yb, x1, yb, lv_hex(MO11_G), a);\n",
     "    mo13_line(B, x0, yt, x0, yv, lv_hex(MO11_G), a); mo13_line(B, x1, yt, x1, yv, lv_hex(MO11_G), a); mo13_line(B, x0, yt, x1, yt, lv_hex(MO11_GH), a); mo13_line(B, x0, yb, x1, yb, lv_hex(MO11_G), a);\n    if (S) mo_emb_draw(B, i, x0 + 1 + (fw - 1 - S) / 2, iy, S, 0x005a20, 0x00b843, 0xffffff, a, a);   /* v4: emblem on top of the edges */\n")
rep2("    if (fw >= 10) { mo13_line(B, x0 + 1, bt0 + bh + 1, x0 + 1, bb0 - 2, lv_hex(MO11_G), a); }\n",
     "    if (fw >= 10) { mo13_line(B, x0 + 1, bt0 + bh + 1, x0 + 1, bb0 - 2, lv_hex(MO11_G), a); }\n    if (S && px1 - px0 >= 2 && py1 - py0 >= 4) mo_emb_draw(B, i, x0 + 1 + (fw - 1 - S) / 2, py0 + (fw >= 40 ? 4 : 2), S, 0xfff0c8, MO11_GH, 0x1c120c, a, a, py1);   /* v4: emblem on top of the edges */\n")
# ---- right-hand folder pull-down: 100% opaque body / header / drop outline / tab (Mobandy 9:28 AM)
rep2("        mo_hspan(B, yy, xl(yy), xr(yy), c, MO_LOOK == 1 ? 242 : 256); }", "        mo_hspan(B, yy, xl(yy), xr(yy), c, 256); }   /* v4: opaque */")
rep2("mo_hspan(B, yy, xl(yy) + 3, xr(yy) - 3, lv_hex(HEAD), MO_LOOK == 2 ? 256 : 200);", "mo_hspan(B, yy, xl(yy) + 3, xr(yy) - 3, lv_hex(HEAD), 256);")
rep2("if (!solid) { mo_bl(B, xl(yy), yy, lv_hex(OUT), 200); mo_bl(B, xr(yy), yy, lv_hex(OUT), 200); continue; }", "if (!solid) { mo_bl(B, xl(yy), yy, lv_hex(OUT), 256); mo_bl(B, xr(yy), yy, lv_hex(OUT), 256); continue; }")
rep2("lv_hex(TAB), solid ? 256 : 120);", "lv_hex(TAB), 256);")

# ---- v5 2026-11-14 (Mobandy 11:51 AM ET "Go back to all wood with different colored device emblem") ----
sys.path.insert(0, D); import emb_colors as _EC
rep2("static inline int mo3_cl(int v, int lo, int hi)", _EC.c_tables(_EC.colors()) + "static inline int mo3_cl(int v, int lo, int hi)")
rep2("    const int L = MO_LOOKOF[i];\n    if (L == 1) {", "    const int L = 3;   /* v5: EVERY pillar body is see-through wood (T3); MO_LOOKOF[i] still picks the height group (MO_HFIX) */\n    if (L == 1) {")
rep2("mo_emb_draw(B, i, ix, iy, Sl, 0xfff0c8, MO11_GH, 0x1c120c, a, a, py1); }", "mo_emb_draw(B, i, ix, iy, Sl, MO_EMB_COL[i], MO_EMB_DET[i], 0x1c120c, a, a, py1); }   /* v5: per-device colour */")
rep2("S, 0xfff0c8, MO11_GH, 0x1c120c, a, a, py1);   /* v4: emblem on top of the edges */", "S, MO_EMB_COL[i], MO_EMB_DET[i], 0x1c120c, a, a, py1);   /* v4: emblem on top of the edges; v5 per-device colour */")
assert not any(l.lstrip().startswith('#') for l in s.splitlines()), [l for l in s.splitlines() if l.lstrip().startswith('#')]
# ---- v5d (Mobandy 2:35 PM ET 'make a portion of them the black matrix tower'): the 5 HIGH-threat towers (MO_LOOKOF 1: DEAUTH, HACK,
# SKIMMER, FLIPPER, EVIL TWIN) use the existing T1 code-glass branch of mo11_tower_fx (the v4 green-group path: opaque glass + green Matrix
# code rain + glowing edges); only its body base is darkened toward black; the code rain, edges and folder rows are unchanged. Emblems use the
# per-device colours (MO_EMB_COL / MO_EMB_DET) like the 13 wood towers.
rep2("    const int L = 3;   /* v5: EVERY pillar body is see-through wood (T3); MO_LOOKOF[i] still picks the height group (MO_HFIX) */",
     "    const int L = MO_LOOKOF[i] == 1 ? 1 : 3;   /* v5d: the 5 HIGH towers = T1 black Matrix code glass, the other 13 = see-through wood (T3) */")
rep2("mo3_mx(0x0b3320, 0x010805, v)", "mo3_mx(0x03120a, 0x000201, v)")   # v5d: front glass base darkened toward black (was dark green 0x0b3320 -> 0x010805)
rep2("mo3_mx(0x0e3a22, 0x020c07", "mo3_mx(0x05160d, 0x000201")       # v5d: side glass darkened the same way
rep2("mo3_mx(0x0b3320, 0x5ac080", "mo3_mx(0x03120a, 0x3a8a58")       # v5d: highlight column keeps a faint green glint on black
assert s.count("0xd8ffe6, MO11_GH, 0x010805, a, a)") == 2
s = s.replace("0xd8ffe6, MO11_GH, 0x010805, a, a)", "MO_EMB_COL[i], MO_EMB_DET[i], 0x010805, a, a)")   # v5d: T1 emblems in the per-device colour
# ---- v5d open state (Mobandy 2:36 - 2:38 PM ET): mo12_folder = HACKbot DOWNLOAD's folder (lv_dl_drop) with richer real info; mo12_drop0 (search first)
if False:   # v5e: REJECTED 3:23 PM ET, off (v5c folder restored)
    _a = s.index('static void mo12_folder(LvBand &B, int i, int j, int ys, float ph, int tl)\n{'); _b = s.index('\n}\n', _a) + 3
    s = s[:_a] + open(D + '/v5d_open.inc.txt').read() + s[_b:]
# ---- v5e (Mobandy 3:25 PM ET) ----
import json
# 1) solid wood: the 13 wood bodies opaque again (front grain, sides, cap at full alpha); brass, rails, emblem, file rows unchanged
rep2("            mo13_px(B, x, y, grain(x, y, u), a * 96 / 256);   /* v3: ~38% wood so the room shows through at idle too */", "            mo13_px(B, x, y, grain(x, y, u), a);   /* v5e: SOLID wood (Mobandy 3:25 PM ET) */")
rep2("for (int y = top; y <= bot; ++y) mo13_px(B, x, y, mo3_mx(0x6a4020, 0x2a160a, u), a * 105 / 256); }", "for (int y = top; y <= bot; ++y) mo13_px(B, x, y, mo3_mx(0x6a4020, 0x2a160a, u), a); }   /* v5e: solid side */")
rep2("mo_quad(B, X, Y, lv_hex(0xb08a3a), a * 150 / 256);", "mo_quad(B, X, Y, lv_hex(0xb08a3a), a);   /* v5e: solid cap */")
# 2) Matrix rain on the 5 black towers: the CYD code-streak rain of lv_radar.h (the radar field's 'code streaks' / lv_rd_bg case 0 RAIN):
#    x = (s*53+17) % W, len = 10+(s*7)%18, sp = 40+(s*29)%70 px/s, head = top + (t*sp + s*37) % (H+len), every 2nd px, head 0x10d858 /
#    tail 0x00a83c (the MO v11 Matrix colours), fade k==0 ? 8 : 6-k*5/len of 8 - mapped onto the tower face, drawn before the file rows + emblem
_a = s.index("    /* falling code: glyph columns (close-up) or 1 px streaks (idle), dim so the file rows stay readable */")
_b = s.index("else mo13_px(B, cx, gy, c, al); } }", _a) + len("else mo13_px(B, cx, gy, c, al); } }")
s = s[:_a] + """    const bool big = fw >= 40;
    /* v5e: Matrix rain = lv_radar.h's CYD code streaks (field rain law, same constants + colours), mapped into this tower's face */
    { const int W_ = fw - 3, H_ = yv - yt - 1, ns = fw >= 40 ? 18 : (fw / 3 > 2 ? fw / 3 : 2);
      if (W_ > 0 && H_ > 4) for (int s_ = 0; s_ < ns; ++s_) {
        const int x = x0 + 2 + (s_ * 53 + 17 + i * 7) % W_, len = 10 + (s_ * 7) % 18, sp = 40 + (s_ * 29) % 70;
        const int head = yt + 1 + (int)(lv_now / 1000.0f * sp + s_ * 37 + i * 11) % (H_ + len);
        for (int k = 0; k < len; k += 2) { const int y = head - k; if (y > yt && y < yv) mo13_px(B, x, y, lv_hex(k == 0 ? 0x10d858 : 0x00a83c), a * (k == 0 ? 8 : 6 - k * 5 / len) / 8); } } }""" + s[_b:]
# 3) emblems bolted to the face: one fixed size per tower (largest even size that fits its narrowest drifting face with >= 1 px inset each
#    side, from 320 real-sim samples: src/emb_fix_v5e.json), centred on the face; no size steps while idle; 40 px only in the open close-up
rep2("static int mo_embS4(int i, int fw)\n{\n    if (fw >= 50) return mo_eS_last[i] = 40;",
     "static const int MO_EMB_SFIX[MO_N] = {%s};   /* v5e: fixed idle emblem size per tower */\nstatic int mo_embS4(int i, int fw)\n{\n    if (fw >= 50) return mo_eS_last[i] = 40;\n    { int S = MO_EMB_SFIX[i], lim = fw - 3; if (S > lim) S = lim < 4 ? 0 : lim; return mo_eS_last[i] = S; }   /* v5e: fixed, inset >= 1 px */" % ', '.join(map(str, json.load(open(D + '/emb_fix_v5e.json'))['S'])))

# ---- v5f (Mobandy 3:24 / 3:48 PM ET): folder squares = digital instruments (every element a real value); no progress bar, no landing sweep, no idle glow cycle
_a = s.index("    const char *sz = MO_FOLD_TAG[i][j]; const int sw = (int)strlen(sz) * 6 - 1, sx = x + w - 6 - sw;")
_b = s.index('"fst"); }\n}\n', _a) + len('"fst"); }\n}\n')
s = s[:_a] + open(D + '/v5f_folder.inc.txt').read() + s[_b:]
rep2("    const bool act = (tl >= 0 && tl < MO12_LOAD_MS) || (tl >= MO12_LOAD_MS && ((int)(lv_now / 1400) % 3) == j && mo12_nland == 3);", "    const bool act = false;   /* v5f: no decorative glow cycle */")

# v5f: the black towers' rain brightened to read at idle size (same lv_radar.h streak law: x / len / speed / head; denser streams,
# continuous trail instead of every 2nd px, a bright 0xd8ffe6 head pixel pair, trail 0x10d858 -> 0x00a83c at full strength for its first half)
rep2("    { const int W_ = fw - 3, H_ = yv - yt - 1, ns = fw >= 40 ? 18 : (fw / 3 > 2 ? fw / 3 : 2);", "    { const int W_ = fw - 3, H_ = yv - yt - 1, ns = fw >= 40 ? 18 : (fw / 2 > 3 ? fw / 2 : 3);   /* v5f: denser */")
rep2("        for (int k = 0; k < len; k += 2) { const int y = head - k; if (y > yt && y < yv) mo13_px(B, x, y, lv_hex(k == 0 ? 0x10d858 : 0x00a83c), a * (k == 0 ? 8 : 6 - k * 5 / len) / 8); } } }",
     "        for (int k = 0; k < len; ++k) { const int y = head - k; if (y > yt && y < yv) mo13_px(B, x, y, lv_hex(k < 2 ? 0xd8ffe6 : k < len / 2 ? 0x10d858 : 0x00a83c), k < len / 2 ? a : a * (8 - (k - len / 2) * 7 / (len - len / 2)) / 8); } } }   /* v5f: bright head + continuous fading trail */")

open(D+'/mo_mix.inc','w').write(s); print('mo_mix.inc written')
