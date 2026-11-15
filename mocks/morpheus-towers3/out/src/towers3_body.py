#!/usr/bin/env python3
"""towers3 (Mobandy 7:03 PM ET): 3 new tower bodies for the 13 non-Matrix towers + a threat-lit edge, built on the v5f sources.
Copies ../../tree and ../../src to /tmp/t3 (v5f files untouched), rewrites ONLY the T3 body pixels in mo_mix.inc under #if MO_BODY
(1 = A obsidian monolith, 2 = B server rack, 3 = C crystal pillar) and builds /tmp/t3/sim_A|B|C. Everything else (emblems, file rows,
folders, open sequence, Matrix group with rain, depth, drift, camera) is the v5f code byte for byte."""
import os, shutil, subprocess
R = '/workspace/out/morpheus-mix'; T = '/tmp/t3'
shutil.rmtree(T, ignore_errors=True); shutil.copytree(R + '/tree', T + '/tree'); shutil.copytree(R + '/src', T + '/src', ignore=shutil.ignore_patterns('__pycache__', 'opt18', '_withdrawn_v1_only'))
p = T + '/src/mo_mix.inc'; s = open(p).read()
def rep(a, b):
    global s; assert s.count(a) == 1, a[:60]; s = s.replace(a, b)
# per-tower data the bodies read: threat (0 HIGH, 1 MED, 2 LOW), example signal level (lv_rd_level: rssi + 100, 4..60)
PRE = r'''
#ifndef MO_BODY
#define MO_BODY 0
#endif
static inline uint32_t mo_tc(int i) { const int th = MO_THREAT[i]; return th == 0 ? 0xff385c : th == 1 ? 0xffb000 : 0x30ff90; }   /* threat colour */
static inline float mo_sig(int i) { int l = MO11_RSSI[i] + 100; if (l < 4) l = 4; if (l > 60) l = 60; return (l - 4) / 56.0f; }   /* 0 weak .. 1 strong */
'''
rep('static void mo11_tower_fx(LvBand &B, const LvTower &T)\n{', PRE + 'static void mo11_tower_fx(LvBand &B, const LvTower &T)\n{')
# side faces
rep('for (int y = top; y <= bot; ++y) mo13_px(B, x, y, mo3_mx(0x6a4020, 0x2a160a, u), a); }   /* v5e: solid side */',
    'for (int y = top; y <= bot; ++y) mo13_px(B, x, y, MO_BODY == 1 ? mo3_mx(0x2a3040, 0x07090e, u) : MO_BODY == 2 ? mo3_mx(0x2c3038, 0x0e1014, u) : MO_BODY == 3 ? mo3_mx(0x1e5a68, 0x062028, u) : mo3_mx(0x6a4020, 0x2a160a, u), a); }   /* v5e: solid side; towers3 bodies */')
rep('mo13_line(B, xbk, byt, xbk, byb, lv_hex(0x8a5a2a), a * 200 / 256); mo13_line(B, xf, yt, xbk, byt, lv_hex(0xd8aa52), a); };',
    'mo13_line(B, xbk, byt, xbk, byb, lv_hex(MO_BODY == 0 ? 0x8a5a2a : MO_BODY == 3 ? 0x2a7a8a : 0x15181e), MO_BODY ? a : a * 200 / 256); mo13_line(B, xf, yt, xbk, byt, lv_hex(MO_BODY == 0 ? 0xd8aa52 : MO_BODY == 1 ? 0xc8d0dc : MO_BODY == 2 ? 0x6a707a : 0xbffcff), a); };')
rep('mo_quad(B, X, Y, lv_hex(0xb08a3a), a);   /* v5e: solid cap */ mo13_line(B, bx0, byt, bx1, byt, lv_hex(0xfff0b0), a); }',
    'mo_quad(B, X, Y, lv_hex(MO_BODY == 1 ? 0x8a94a8 : MO_BODY == 2 ? 0x4a5058 : MO_BODY == 3 ? 0x7ad8e8 : 0xb08a3a), a);   /* v5e: solid cap */ mo13_line(B, bx0, byt, bx1, byt, lv_hex(MO_BODY ? mo_tc(i) : 0xfff0b0), a); }   /* towers3: the cap\'s back edge lit in the threat colour */')
# front face
rep('            mo13_px(B, x, y, grain(x, y, u), a);   /* v5e: SOLID wood (Mobandy 3:25 PM ET) */',
'''#if MO_BODY == 1   /* A OBSIDIAN MONOLITH: polished black stone, a 2 px chrome bevel down the lit (left) side, nothing else on the face */
            mo13_px(B, x, y, x == x0 + 1 ? lv_hex(0xc8d0dc) : x == x0 + 2 ? lv_hex(0x5a6474) : mo3_mx(0x1c2230, 0x06080c, v), a);
#elif MO_BODY == 2   /* B SERVER RACK: gunmetal chassis; the LED column (below) is the only lit part */
            mo13_px(B, x, y, lv_hex((x - x0) % 2 ? 0x22262c : 0x1e2228), a);
#elif MO_BODY == 3   /* C CRYSTAL PILLAR: cyan glass, lit facet on the left third; the light band (below) rises at a speed set by signal */
            mo13_px(B, x, y, u < 0.33f ? mo3_mx(0x3a8a9a, 0x0e3a46, v) : mo3_mx(0x1e5a68, 0x062028, v), a);
#else
            mo13_px(B, x, y, grain(x, y, u), a);   /* v5e: SOLID wood (Mobandy 3:25 PM ET) */
#endif''')
# bands + rails + per-concept signal element
rep('    for (int k = 0; k < bh; ++k) { mo13_span(B, bt0 + k, x0, x1, brass(k, bh), a); mo13_span(B, bb0 + k, x0, x1, brass(k, bh), a); }\n    if (fw >= 8) { mo13_span(B, bt0 + bh, x0 + 1, x1 - 1, lv_hex(MO11_G), a); mo13_span(B, bb0 - 1, x0 + 1, x1 - 1, lv_hex(MO11_G), a); }',
'''#if MO_BODY == 0
    for (int k = 0; k < bh; ++k) { mo13_span(B, bt0 + k, x0, x1, brass(k, bh), a); mo13_span(B, bb0 + k, x0, x1, brass(k, bh), a); }
    if (fw >= 8) { mo13_span(B, bt0 + bh, x0 + 1, x1 - 1, lv_hex(MO11_G), a); mo13_span(B, bb0 - 1, x0 + 1, x1 - 1, lv_hex(MO11_G), a); }
#elif MO_BODY == 2
    /* B: LED column on the right rail = the device's example signal: lit LEDs = signal level (of the slots), lit ones flicker at 1 + 7 x signal Hz
     * (stronger = busier); the top LED is the threat lamp (steady, threat colour) */
    { const float sg = mo_sig(i); const int lx = x1 - 2, ns = (bb0 - bt0) / 4, nl = (int)lroundf(sg * ns);
      if (fw >= 8 && ns > 1) for (int k = 0; k < ns; ++k) { const int ly = bb0 - 2 - k * 4;
          uint16_t c = lv_hex(0x101a14); if (k == ns - 1) c = lv_hex(mo_tc(i));
          else if (k < nl) c = fmodf(t * (1.0f + 7.0f * sg) + k * 0.37f + i * 0.11f, 1.0f) < 0.6f ? lv_hex(0x30ff90) : lv_hex(0x0a5a28);
          mo13_px(B, lx, ly, c, a); mo13_px(B, lx, ly + 1, c, a); if (fw >= 20) { mo13_px(B, lx - 1, ly, c, a); mo13_px(B, lx - 1, ly + 1, c, a); } } }
#elif MO_BODY == 3
    /* C: an internal light band rising through the crystal; speed 15 + 60 x signal px/s, brightness 0.35 + 0.65 x signal */
    { const float sg = mo_sig(i); const int H_ = yv - yt - 2; if (H_ > 6) { const int by = yv - 1 - (int)fmodf(t * (15.0f + 60.0f * sg) + i * 13, (float)H_);
      for (int d = -2; d <= 2; ++d) { const int yy = by + d; if (yy <= yt || yy >= yv) continue; const float k = (0.35f + 0.65f * sg) * (d == 0 ? 1.0f : abs(d) == 1 ? 0.6f : 0.25f);
          for (int x = x0 + 1; x < x1; ++x) mo13_px(B, x, yy, mo3_mx(0x1e5a68, 0xbffcff, k), a); } } }
#endif''')
# outer edges: the right (shadow) edge carries the threat colour in A; rack ears in B; glass rim in C
rep('    mo13_line(B, x0, yt, x0, yv, lv_hex(0xd8aa52), a); mo13_line(B, x1, yt, x1, yv, lv_hex(0x3a2010), a); mo13_line(B, x0, yt, x1, yt, lv_hex(0xfff0b0), a); mo13_line(B, x0, yb, x1, yb, lv_hex(0x3a2010), a);\n    if (fw >= 10) { mo13_line(B, x0 + 1, bt0 + bh + 1, x0 + 1, bb0 - 2, lv_hex(MO11_G), a); }',
'''#if MO_BODY == 0
    mo13_line(B, x0, yt, x0, yv, lv_hex(0xd8aa52), a); mo13_line(B, x1, yt, x1, yv, lv_hex(0x3a2010), a); mo13_line(B, x0, yt, x1, yt, lv_hex(0xfff0b0), a); mo13_line(B, x0, yb, x1, yb, lv_hex(0x3a2010), a);
    if (fw >= 10) { mo13_line(B, x0 + 1, bt0 + bh + 1, x0 + 1, bb0 - 2, lv_hex(MO11_G), a); }
#elif MO_BODY == 1
    mo13_line(B, x0, yt, x0, yv, lv_hex(0xe8eef8), a); mo13_line(B, x1, yt, x1, yv, lv_hex(mo_tc(i)), a); if (fw >= 12) mo13_line(B, x1 - 1, yt, x1 - 1, yv, lv_hex(mo_tc(i)), a);   /* the lit edge = threat */
    mo13_line(B, x0, yt, x1, yt, lv_hex(0xffffff), a); mo13_line(B, x0, yb, x1, yb, lv_hex(0x05060a), a);
#elif MO_BODY == 2
    mo13_line(B, x0, yt, x0, yv, lv_hex(0x6a707a), a); mo13_line(B, x1, yt, x1, yv, lv_hex(0x0c0e10), a); mo13_line(B, x0, yt, x1, yt, lv_hex(0x8a909a), a); mo13_line(B, x0, yb, x1, yb, lv_hex(0x0c0e10), a);
#else
    mo13_line(B, x0, yt, x0, yv, lv_hex(0xbffcff), a); mo13_line(B, x1, yt, x1, yv, lv_hex(0x0a2a34), a); mo13_line(B, x0, yt, x1, yt, lv_hex(mo_tc(i)), a); mo13_line(B, x0, yb, x1, yb, lv_hex(0x0a2a34), a);   /* top rim = threat */
#endif''')
open(p, 'w').write(s)
ZIG = '/tmp/rci/zig/ziglang/zig'
for k, n in ((1, 'A'), (2, 'B'), (3, 'C')):
    r = subprocess.run([ZIG, 'c++', '-target', 'x86_64-linux-gnu.2.36', '-I/tmp/zinc', '-O2', '-std=gnu++17', '-w', '-DMO_BODY=%d' % k, '-o', T + '/sim_' + n, 'tools/livescan/sim_live.cpp', '/usr/lib/x86_64-linux-gnu/libz.so.1'], cwd=T + '/tree', capture_output=True, text=True)
    print(n, r.returncode, r.stderr[:1500])
