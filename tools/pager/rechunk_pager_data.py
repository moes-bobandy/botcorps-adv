#!/usr/bin/env python3
"""v1.2.1: re-pack the pager face/panel overlays as independent 16-row raw-deflate chunks.
Same RGB565 pixels (decoded from the v1.2 arrays and re-split), so the pager stays pixel-identical.
Why: v1.2 inflated each whole overlay into its own heap block (face 24,200 B + panel 21,584 B, kept
forever). On the CYD that ate the radio / sub-screen heap. Chunks let the pager decode 16 rows at a
time into the static lock inflate buffer tail (no heap on a still page).
Usage: python3 tools/pager/rechunk_pager_data.py src/live/pager/lv_pager_data.h"""
import re, sys, zlib
path = sys.argv[1]
s = open(path).read()
def grab(name):
    m = re.search(r'static const (\w+) %s\[\d+\]\s*=\s*\{([^}]*)\};\n' % name, s)
    return m, [int(x, 0) for x in m.group(2).replace('\n', ' ').split(',') if x.strip()]
R = 16
out_blocks = []
for nm, w, h in (('FACE', 110, 110), ('PAN', 152, 71)):
    mo, O = grab('LV_PG_%s_OFF' % nm); mz, Z = grab('LV_PG_%s_Z' % nm); Z = bytes(Z)
    nch = (h + R - 1) // R
    offs, blob = [0], b''
    for i in range(len(O) - 1):
        im = zlib.decompress(Z[O[i]:O[i + 1]], -15); assert len(im) == w * h * 2
        for c in range(nch):
            raw = im[c * R * w * 2:min(h, (c + 1) * R) * w * 2]
            co = zlib.compressobj(9, zlib.DEFLATED, -15, 9); z = co.compress(raw) + co.flush()
            assert zlib.decompress(z, -15) == raw
            blob += z; offs.append(len(blob))
    txt = '#define LV_PG_%s_NCH %d   /* 16-row chunks per image (v1.2.1) */\n' % (nm, nch)
    txt += 'static const uint32_t LV_PG_%s_OFF[%d] = { %s };\n' % (nm, len(offs), ', '.join(map(str, offs)))
    lines = [', '.join('0x%02x' % b for b in blob[k:k + 16]) for k in range(0, len(blob), 16)]
    txt += 'static const uint8_t LV_PG_%s_Z[%d] = {\n%s\n};\n' % (nm, len(blob), ',\n'.join(lines))
    s = s[:mo.start()] + txt + s[mz.end():] if mo.start() < mz.start() else None
    print(nm, 'v1.2', len(Z), '-> chunked', len(blob), 'chunks', len(offs) - 1)
s = s.replace('#define LV_PG_NSLOT   24\n', '#define LV_PG_NSLOT   24\n#define LV_PG_CH      16   /* v1.2.1: overlay rows per deflate chunk */\n', 1)
open(path, 'w').write(s)
