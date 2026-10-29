#!/usr/bin/env python3
"""Pixel diff: sim shots vs host frames. Writes OUT/diffs/*.png (sim | ref | red diff) and OUT/diffs/report.txt.
Usage: diff_refs.py SHOTS_DIR FRAMES_DIR HOST_FRESH_DIR OUT_DIR"""
import sys, os, numpy as np
from PIL import Image
S, F, H, O = sys.argv[1:5]
os.makedirs(O, exist_ok=True)
def load(p, shape=(320, 240)):
    if p.endswith('.rgb'):
        r = np.fromfile(p, np.uint8); h = r.size // (shape[1] * 3) if r.size == 230400 and shape == (320, 240) else None
        return r.reshape(320, 240, 3) if 'land' not in p else r.reshape(240, 320, 3)
    return np.array(Image.open(p).convert('RGB'))
def side(a, b, d, out):
    dm = (a // 3).copy(); dm[d] = [255, 0, 0]
    s = np.full((a.shape[0], a.shape[1] * 3 + 20, 3), 60, np.uint8)
    w = a.shape[1]; s[:, :w] = a; s[:, w + 10:2 * w + 10] = b; s[:, 2 * w + 20:] = dm
    Image.fromarray(s).resize((s.shape[1] * 2, s.shape[0] * 2), Image.NEAREST).save(out)
pairs = [
 ('03-hub', F + '/port-hub.rgb', 'host port-hub (port_compose_hub, host-only crop of landscape)'),
 ('03-hub', F + '/pterm/hub.png', 'older pterm mock frame'),
 ('07-terminal', F + '/port-term.png', 'host port-term (port_compose_term, host-only)'),
 ('08-terminal-glossary', F + '/pterm/term-list.png', 'older pterm mock frame'),
 ('09-terminal-keyboard', F + '/pterm/term-keys.png', 'older pterm mock frame'),
 ('09-terminal-keyboard', F + '/port-TermplusOSK.png', 'host term + landscape OSK'),
 ('04-chip-bot0-scan', F + '/port-wifi.png', 'host paint_ext_portrait wifi'),
 ('06-chip-gibson-scan', F + '/port-scan.png', 'host paint_ext_portrait scan (Gibson)'),
 ('19-control', F + '/controlbot/control-ctlbot-GRAPE.png', 'host ctlbot GRAPE (RAIN_WARM=200)'),
 ('21-control-color-tap2', F + '/controlbot/control-ctlbot-TOXIC.png', 'host ctlbot TOXIC'),
 ('23-control-color-tap4', F + '/controlbot/control-ctlbot-ICE.png', 'host ctlbot ICE'),
 ('28-control-color-tap9', F + '/controlbot/control-ctlbot-BLAZE.png', 'host ctlbot BLAZE'),
 ('29-control-color-tap10', F + '/controlbot/control-ctlbot-ROSE.png', 'host ctlbot ROSE'),
]
ctl = ['19-control'] + [f'{20+i}-control-color-tap{i+1}' for i in range(10)]
for i, s in enumerate(ctl):
    pairs.append((s, f'{H}/ctl{i}.rgb', f'fresh e9b388a host render RC={i} ctlbot'))
lines = []
for s, r, note in pairs:
    if not os.path.exists(r): lines.append(f'{s} vs {r}: MISSING'); continue
    a = load(f'{S}/{s}.png'); b = load(r)
    d = (a != b).any(2)
    tag = os.path.basename(os.path.dirname(r)) + '_' + os.path.splitext(os.path.basename(r))[0]
    side(a, b, d, f'{O}/{s}__vs__{tag}.png')
    lines.append(f'{s:24s} vs {os.path.relpath(r, os.path.dirname(F)) if r.startswith(F) else r:48s} {d.sum():6d} px {100*d.mean():6.2f}%  [{note}]')
# scan overdraw proof: sim portrait gibson scan rows 0-239 vs landscape host gibson scan (rend chip 12)
L = f'{H}/land-chip12.rgb'
if os.path.exists(L):
    a = load(f'{S}/06-chip-gibson-scan.png')[:240, :240]; b = np.fromfile(L, np.uint8).reshape(240, 320, 3)[:, :240]
    d = (a != b).any(2); side(a, b, d, f'{O}/06-chip-gibson-scan_top240__vs__LANDSCAPE_chip12.png')
    lines.append(f'06-chip-gibson-scan rows0-239 x0-239 vs LANDSCAPE host scan (rend chip 12) crop: {d.sum()} px {100*d.mean():.2f}%  [proves landscape paint_list overdraw]')
open(f'{O}/report.txt', 'w').write('\n'.join(lines) + '\n'); print('\n'.join(lines))
