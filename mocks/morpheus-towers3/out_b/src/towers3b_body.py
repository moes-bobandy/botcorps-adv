#!/usr/bin/env python3
"""towers3b (Mobandy 7:12 / 7:13 PM ET): 3 new bodies for the 13 non-Matrix towers, built on a copy of the v5f sources in /tmp/t3b
(v5f untouched). Only the T3 body pixels change, under #if MO_BODY: 4 = D UNIFORM polished gold, 5 = E VARIED own-colour enamel,
6 = F lantern (porcelain + red lacquer, inner glow = signal). Matrix five, emblems, file rows, folders, open sequence, depth, drift,
camera = v5f code."""
import os, shutil, subprocess
R = '/workspace/out/morpheus-mix'; T = '/tmp/t3b'
shutil.rmtree(T, ignore_errors=True); shutil.copytree(R + '/tree', T + '/tree'); shutil.copytree(R + '/src', T + '/src', ignore=shutil.ignore_patterns('__pycache__', 'opt18', '_withdrawn_v1_only'))
p = T + '/src/mo_mix.inc'; s = open(p).read()
def rep(a, b):
    global s; assert s.count(a) == 1, a[:70]; s = s.replace(a, b)
rep('static void mo11_tower_fx(LvBand &B, const LvTower &T)\n{', r'''
#ifndef MO_BODY
#define MO_BODY 0
#endif
static inline float mo_sig(int i) { int l = MO11_RSSI[i] + 100; if (l < 4) l = 4; if (l > 60) l = 60; return (l - 4) / 56.0f; }   /* example signal 0..1 (lv_rd_level) */
static inline uint32_t mo_mixc(uint32_t a, uint32_t b, float t) { if (t < 0) t = 0; if (t > 1) t = 1; uint32_t o = 0;
    for (int sh = 0; sh <= 16; sh += 8) { const int ca = (a >> sh) & 255, cb = (b >> sh) & 255; o |= (uint32_t)(ca + (cb - ca) * t + 0.5f) << sh; } return o; }
/* three-stop horizontal shading: lit (left) -> mid -> shadow (right), plus a gentle darkening toward the base */
static inline uint32_t mo_shade3(uint32_t lit, uint32_t mid, uint32_t dk, float u, float v) {
    const uint32_t c = u < 0.35f ? mo_mixc(lit, mid, u / 0.35f) : mo_mixc(mid, dk, (u - 0.35f) / 0.65f); return mo_mixc(c, dk, v * 0.22f); }
static inline uint32_t mo_bcol(int i) {   /* the body base colour per look */
    return MO_BODY == 4 ? 0xe0b040 : MO_BODY == 5 ? MO_EMB_COL[i] : 0xf2e6d0; }
static void mo11_tower_fx(LvBand &B, const LvTower &T)
{''')
# sides: shadowed body colour (D gold, E own colour, F red lacquer)
rep('for (int y = top; y <= bot; ++y) mo13_px(B, x, y, mo3_mx(0x6a4020, 0x2a160a, u), a); }   /* v5e: solid side */',
    'for (int y = top; y <= bot; ++y) mo13_px(B, x, y, MO_BODY == 4 ? lv_hex(mo_mixc(0xa8741c, 0x5a3a0c, u)) : MO_BODY == 5 ? lv_hex(mo_mixc(mo_mixc(MO_EMB_COL[i], 0x000000, 0.45f), mo_mixc(MO_EMB_COL[i], 0x000000, 0.7f), u)) : MO_BODY == 6 ? lv_hex(mo_mixc(0xb01c26, 0x5a0a10, u)) : mo3_mx(0x6a4020, 0x2a160a, u), a); }   /* v5e: solid side; towers3b: shaded side */')
rep('mo13_line(B, xbk, byt, xbk, byb, lv_hex(0x8a5a2a), a * 200 / 256); mo13_line(B, xf, yt, xbk, byt, lv_hex(0xd8aa52), a); };',
    'mo13_line(B, xbk, byt, xbk, byb, lv_hex(MO_BODY == 0 ? 0x8a5a2a : MO_BODY == 4 ? 0x3a2406 : MO_BODY == 5 ? mo_mixc(MO_EMB_COL[i], 0x000000, 0.8f) : 0x3a0408), MO_BODY ? a : a * 200 / 256); mo13_line(B, xf, yt, xbk, byt, lv_hex(MO_BODY == 0 ? 0xd8aa52 : MO_BODY == 4 ? 0xfff4c0 : MO_BODY == 5 ? mo_mixc(MO_EMB_COL[i], 0xffffff, 0.5f) : 0xff6a6a), a); };')
rep('mo_quad(B, X, Y, lv_hex(0xb08a3a), a);   /* v5e: solid cap */ mo13_line(B, bx0, byt, bx1, byt, lv_hex(0xfff0b0), a); }',
    'mo_quad(B, X, Y, lv_hex(MO_BODY == 4 ? 0xfff2b0 : MO_BODY == 5 ? mo_mixc(MO_EMB_COL[i], 0xffffff, 0.45f) : MO_BODY == 6 ? 0xe8404a : 0xb08a3a), a);   /* v5e: solid cap; towers3b: cap highlight */ mo13_line(B, bx0, byt, bx1, byt, lv_hex(MO_BODY == 6 ? 0xff9a9a : 0xfff0b0), a); }')
rep('            mo13_px(B, x, y, grain(x, y, u), a);   /* v5e: SOLID wood (Mobandy 3:25 PM ET) */',
'''#if MO_BODY == 4   /* D UNIFORM: polished SCANbot-style gold, lit left -> shadow right */
            mo13_px(B, x, y, lv_hex(mo_shade3(0xfff4c0, 0xe0b040, 0x8a5a14, u, v)), a);
#elif MO_BODY == 5   /* E VARIED: each body enamelled in its own device colour (the emblem colour), lit left -> shadow right */
            mo13_px(B, x, y, lv_hex(mo_shade3(mo_mixc(MO_EMB_COL[i], 0xffffff, 0.30f), mo_mixc(MO_EMB_COL[i], 0x000000, 0.12f), mo_mixc(MO_EMB_COL[i], 0x000000, 0.50f), u, v)), a);
#elif MO_BODY == 6   /* F LANTERN: white porcelain lit from inside; the warm core glow strength = the device's example signal */
            { const float g = mo_sig(i) * 0.75f * expf(-((u - 0.5f) * (u - 0.5f)) / 0.06f);
              mo13_px(B, x, y, lv_hex(mo_mixc(mo_shade3(0xfffaf0, 0xf2e6d0, 0xc0a888, u, v), 0xffb648, g)), a); }
#else
            mo13_px(B, x, y, grain(x, y, u), a);   /* v5e: SOLID wood (Mobandy 3:25 PM ET) */
#endif''')
rep('    for (int k = 0; k < bh; ++k) { mo13_span(B, bt0 + k, x0, x1, brass(k, bh), a); mo13_span(B, bb0 + k, x0, x1, brass(k, bh), a); }\n    if (fw >= 8) { mo13_span(B, bt0 + bh, x0 + 1, x1 - 1, lv_hex(MO11_G), a); mo13_span(B, bb0 - 1, x0 + 1, x1 - 1, lv_hex(MO11_G), a); }',
'''#if MO_BODY == 0
    for (int k = 0; k < bh; ++k) { mo13_span(B, bt0 + k, x0, x1, brass(k, bh), a); mo13_span(B, bb0 + k, x0, x1, brass(k, bh), a); }
    if (fw >= 8) { mo13_span(B, bt0 + bh, x0 + 1, x1 - 1, lv_hex(MO11_G), a); mo13_span(B, bb0 - 1, x0 + 1, x1 - 1, lv_hex(MO11_G), a); }
#elif MO_BODY == 6
    /* F: red lacquer cap + base (the lantern's frame), shaded lit -> shadow, a bright lip on the cap */
    for (int y = yt; y <= bt0 + bh && y <= yv; ++y) for (int x = x0; x <= x1; ++x) mo13_px(B, x, y, lv_hex(y == yt ? 0xff8080 : mo_shade3(0xff4a50, 0xc81e28, 0x6a0a10, (x - x0) / (float)fw, 0)), a);
    for (int y = bb0; y <= yv; ++y) for (int x = x0; x <= x1; ++x) mo13_px(B, x, y, lv_hex(mo_shade3(0xff4a50, 0xc81e28, 0x6a0a10, (x - x0) / (float)fw, 0)), a);
#endif''')
rep('    mo13_line(B, x0, yt, x0, yv, lv_hex(0xd8aa52), a); mo13_line(B, x1, yt, x1, yv, lv_hex(0x3a2010), a); mo13_line(B, x0, yt, x1, yt, lv_hex(0xfff0b0), a); mo13_line(B, x0, yb, x1, yb, lv_hex(0x3a2010), a);\n    if (fw >= 10) { mo13_line(B, x0 + 1, bt0 + bh + 1, x0 + 1, bb0 - 2, lv_hex(MO11_G), a); }',
'''#if MO_BODY == 0
    mo13_line(B, x0, yt, x0, yv, lv_hex(0xd8aa52), a); mo13_line(B, x1, yt, x1, yv, lv_hex(0x3a2010), a); mo13_line(B, x0, yt, x1, yt, lv_hex(0xfff0b0), a); mo13_line(B, x0, yb, x1, yb, lv_hex(0x3a2010), a);
    if (fw >= 10) { mo13_line(B, x0 + 1, bt0 + bh + 1, x0 + 1, bb0 - 2, lv_hex(MO11_G), a); }
#elif MO_BODY == 4
    mo13_line(B, x0, yt, x0, yv, lv_hex(0xfffbe8), a); mo13_line(B, x1, yt, x1, yv, lv_hex(0x4a2e08), a); mo13_line(B, x0, yt, x1, yt, lv_hex(0xffffff), a); mo13_line(B, x0, yb, x1, yb, lv_hex(0x3a2406), a);
#elif MO_BODY == 5   /* E: crisp light rim (lit edge) whose brightness = the device's example signal, dark rim on the shadow side */
    { const uint32_t rim = mo_mixc(MO_EMB_COL[i], 0xffffff, 0.25f + 0.65f * mo_sig(i));
      mo13_line(B, x0, yt, x0, yv, lv_hex(rim), a); mo13_line(B, x0, yt, x1, yt, lv_hex(rim), a); if (fw >= 16) { mo13_line(B, x0 + 1, yt, x0 + 1, yv, lv_hex(rim), a); mo13_line(B, x0, yt + 1, x1, yt + 1, lv_hex(rim), a); }
      mo13_line(B, x1, yt, x1, yv, lv_hex(mo_mixc(MO_EMB_COL[i], 0x000000, 0.75f)), a); mo13_line(B, x0, yb, x1, yb, lv_hex(mo_mixc(MO_EMB_COL[i], 0x000000, 0.75f)), a); }
#else
    mo13_line(B, x0, yt, x0, yv, lv_hex(0x8a1018), a); mo13_line(B, x1, yt, x1, yv, lv_hex(0x4a0408), a); mo13_line(B, x0, yb, x1, yb, lv_hex(0x4a0408), a);
#endif''')
# file rows ink per look (dark ink on gold / porcelain, light ink with dark outline on enamel)
rep('mo_txt_oc = 0x1c120c; mo3_files(B, i, px0 + 1, px1 - 1, ly0, ly1, lv_hex(MO11_GH), lv_hex(0xfff0c8), lv_hex(0xffd878), a, big ? 9.0f : 6.0f); mo_txt_oc = -1;',
    'const bool lt5 = ((MO_EMB_COL[i] >> 16 & 255) * 3 + (MO_EMB_COL[i] >> 8 & 255) * 6 + (MO_EMB_COL[i] & 255)) / 10 > 120;   /* E: light enamel -> dark ink, dark enamel -> white ink */\n        mo_txt_oc = MO_BODY >= 4 ? -1 : 0x1c120c; mo3_files(B, i, px0 + 1, px1 - 1, ly0, ly1, lv_hex(MO_BODY == 4 ? 0x5a3a08 : MO_BODY == 6 ? 0xa8141e : MO_BODY == 5 ? (lt5 ? 0x101010 : 0xffffff) : MO11_GH), lv_hex(MO_BODY == 4 ? 0x2a1604 : MO_BODY == 6 ? 0x2a1a10 : MO_BODY == 5 ? (lt5 ? 0x101010 : 0xffffff) : 0xfff0c8), lv_hex(MO_BODY == 4 ? 0x4a2a04 : MO_BODY == 6 ? 0x5a3a20 : MO_BODY == 5 ? (lt5 ? 0x202020 : 0xf0f0f0) : 0xffd878), a, big ? 9.0f : 6.0f); mo_txt_oc = -1;')
# emblem plate (D engraved dark plate; E dark plate so the same-colour emblem reads); F: emblem straight on porcelain
rep('    if (S && px1 - px0 >= 2 && py1 - py0 >= 4) mo_emb_draw(B, i, x0 + 1 + (fw - 1 - S) / 2, py0 + (fw >= 40 ? 4 : 2), S, MO_EMB_COL[i], MO_EMB_DET[i], 0x1c120c, a, a, py1);',
'''#if MO_BODY == 4 || MO_BODY == 5
    if (S && px1 - px0 >= 2 && py1 - py0 >= 4) { const int ex = x0 + 1 + (fw - 1 - S) / 2, ey = py0 + (fw >= 40 ? 4 : 2);   /* the emblem's plate: 1 px around the emblem */
        for (int y = ey - 1; y <= ey + S && y <= py1; ++y) mo13_span(B, y, ex - 1, ex + S, lv_hex(y == ey + S ? (MO_BODY == 4 ? 0xfff4c0 : 0x5a5a64) : 0x14100c), a); }
#endif
    if (S && px1 - px0 >= 2 && py1 - py0 >= 4) mo_emb_draw(B, i, x0 + 1 + (fw - 1 - S) / 2, py0 + (fw >= 40 ? 4 : 2), S, MO_EMB_COL[i], MO_EMB_DET[i], 0x1c120c, a, a, py1);''')
open(p, 'w').write(s)
ZIG = '/tmp/rci/zig/ziglang/zig'
for k, n in ((4, 'D'), (5, 'E'), (6, 'F')):
    r = subprocess.run([ZIG, 'c++', '-target', 'x86_64-linux-gnu.2.36', '-I/tmp/zinc', '-O2', '-std=gnu++17', '-w', '-DMO_BODY=%d' % k, '-o', T + '/sim_' + n, 'tools/livescan/sim_live.cpp', '/usr/lib/x86_64-linux-gnu/libz.so.1'], cwd=T + '/tree', capture_output=True, text=True)
    print(n, r.returncode, r.stderr[:1500])
