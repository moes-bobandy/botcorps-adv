# v1.2.22: re-pack LV_CB_Z / LV_PG_CORNER_Z: slots 0/11/15 in place (VAPOR/TEAL/AMBER), append SUNST/TRON/ACID -> 27 slots.
# Stills come from the mock reink pipeline (mock 7c3c717 headers in /tmp/v1221-theme-wt), reink t: VAPOR 32, AMBER 30, TEAL 31, SUNST 33, TRON 34, ACID 35.
import sys, importlib.util, subprocess, re, zlib
sys.dont_write_bytecode = True
from pathlib import Path
import numpy as np
from PIL import Image
import scipy.ndimage as ndi
BW = Path('/workspace/v1222-build-wt')
WT = Path('/tmp/v1221-theme-wt')
V4_SIM = Path('/workspace/out/controlbot-colorui-1115/v4-24themes/sim')
B = Path('/tmp/v1222_build/cb_build'); B.mkdir(parents=True, exist_ok=True)
sp = importlib.util.spec_from_file_location('v2', V4_SIM / 'render_security_color_themes_v2.py')
v2 = importlib.util.module_from_spec(sp); sp.loader.exec_module(v2)
v2.ROOT = Path('/workspace/securitybot'); v2.B = B; v2.ZIG = 'g++'
v2.SPEC = WT / 'src/live/controlbot/themes_spec_v2.h'; v2.OUT = WT / 'src/live/controlbot'; v2.LIVE = str(WT / 'src/live')
v2.TOPBAR = v2.ROOT / 'mocks/topbar-v1-matrix-security'
v2.v9c.V5PNG = v2.ROOT / 'mocks/controlbot-bigchip-v5-reference/01_bigchip@1x.png'
(B/'reink.cpp').write_text((V4_SIM / 'controlbot_colorui_reink.cpp').read_text())
subprocess.check_call(['g++','-O2','-std=gnu++17','-w','-I'+str(WT/'src/live'),'-I'+str(WT/'src/live/controlbot'),'-I'+str(V4_SIM),'-o',str(B/'reink_v2'),str(B/'reink.cpp')])
def run_reink(base, role, samp, t):
    (B/'b.rgb').write_bytes(base.tobytes()); (B/'r.u8').write_bytes(role.tobytes()); (B/'s.i16').write_bytes(samp.tobytes())
    subprocess.check_call([str(B/'reink_v2'), str(t), str(B/'b.rgb'), str(B/'r.u8'), str(B/f'o{t}.rgb'), str(B/'s.i16')])
    return np.frombuffer((B/f'o{t}.rgb').read_bytes(), np.uint8).reshape(320, 240, 3).copy()
def coil_rect():
    m = np.zeros((320, 240), bool); m[28:64, 30:210] = True; return m
def coil_pin_mask(base):
    a = base.astype(int); R, G, Bl = a[..., 0], a[..., 1], a[..., 2]
    gold = (R > G) & (G > Bl) & (R - Bl >= 40) & (R >= 90)
    m = np.zeros((320, 240), bool); m[28:62, 30:210] |= gold[28:62, 30:210]
    side = np.zeros_like(m); side[25:316, 0:31] = True; side[25:316, 209:240] = True
    bottom = np.zeros_like(m); bottom[292:316, 31:209] = True
    m |= gold & (side | bottom)
    lab = (abs(R - G) < 20) & (abs(G - Bl) < 16) & (Bl > R) & (R >= 120) & (R < 250)
    m |= lab & side
    return m
base, role, samp0, _, _ = v2.base_and_roles()
gold_lab = coil_pin_mask(base)
a = base.astype(int); R, G, Bl = a[..., 0], a[..., 1], a[..., 2]
gold = (R > G) & (G > Bl) & (R - Bl >= 40) & (R >= 90)
role8 = gold_lab & gold & (role == 0); role9 = gold_lab & ~gold & (role == 0)
lab8, n8 = ndi.label(role8, structure=np.ones((3, 3)))
for k in range(1, n8 + 1):
    ys, xs = np.nonzero(lab8 == k)
    if xs.mean() < 20 and ys.mean() > 305: role8[lab8 == k] = False
role9 &= ~((np.arange(240)[None, :] < 20) & (np.arange(320)[:, None] > 305))
samp = samp0.copy(); lab8, n8 = ndi.label(role8 & ~coil_rect(), structure=np.ones((3, 3))); cents = []
for k in range(1, n8 + 1):
    ys, xs = np.nonzero(lab8 == k); cx, cy = int(round(xs.mean())), int(round(ys.mean())); samp[ys, xs] = (cx, cy); cents.append((cx, cy, xs.size))
ys, xs = np.nonzero(role8 & coil_rect())
seg = (xs - 30) * 12 // 180; samp[ys, xs] = np.stack([30 + (seg * 15 + 7), np.full_like(xs, 45)], 1)
pads = [c for c in cents if c[2] >= 16]
for y, x in zip(*np.nonzero(role9)):
    side = [c for c in pads if (c[0] < 120) == (x < 120) and c[1] < 292]
    c = min(side, key=lambda c: abs(c[1] - y)); samp[y, x] = (c[0], c[1])
role_cp = role.copy(); role_cp[role8] = 8; role_cp[role9] = 9
def to565(im):
    a = im.astype(np.uint32)
    v = (((a[..., 0] >> 3) << 11) | ((a[..., 1] >> 2) << 5) | (a[..., 2] >> 3)).astype('<u2')
    v[1:24, 1:239] = 0
    return v
def strips(v):
    out = []
    for y in range(0, 320, 16):
        co = zlib.compressobj(9, zlib.DEFLATED, -15); out.append(co.compress(v[y:y + 16].tobytes()) + co.flush())
    return out
def grab_arr(txt, nm):
    m = re.search(rf'static const \w+ {nm}\[\d+\]\s*=\s*\{{([^}}]*)\}};', txt, re.S)
    return m, [int(x, 0) for x in m.group(1).replace('\n', ' ').split(',') if x.strip()]
cbd_path = BW / 'src/live/controlbot/lv_controlbot_data.h'; cbd = cbd_path.read_text()
assert '#define LV_CB_N 24' in cbd
_, offs = grab_arr(cbd, 'LV_CB_OFF'); _, zb = grab_arr(cbd, 'LV_CB_Z'); zb = bytes(zb)
old = [[zb[offs[k*20+s]:offs[k*20+s+1]] for s in range(20)] for k in range(24)]
def unpack(sl): return np.concatenate([np.frombuffer(zlib.decompress(z, -15), '<u2').reshape(16, 240) for z in sl])
# sanity: CIRCUIT (mock t=23) re-packs to the existing slot 0 pixels
chk = to565(run_reink(base, role_cp, samp, 23))
d0 = int((unpack(old[0]) != chk).sum()); print('sanity slot0 CIRCUIT repack vs LV_CB_Z:', d0, 'px'); assert d0 == 0
NEW = {0: 32, 11: 31, 15: 30}; APP = [33, 34, 35]
frames = {}
for k, t in list(NEW.items()) + [(24 + i, t) for i, t in enumerate(APP)]:
    im = run_reink(base, role_cp, samp, t); frames[k] = to565(im); Image.fromarray(im).save(B / f'slot{k}_t{t}.png')
slots = [strips(frames[k]) if k in frames else old[k] for k in range(24)] + [strips(frames[k]) for k in (24, 25, 26)]
blob = bytearray(); no = [0]
for sl in slots:
    for z in sl: blob += z; no.append(len(blob))
lines = []
for ln in cbd.splitlines():
    if ln.startswith('static const uint32_t LV_CB_OFF'): break
    lines.append('#define LV_CB_N 27' if ln.startswith('#define LV_CB_N ') else ln)
lines.append(f'static const uint32_t LV_CB_OFF[{len(no)}] = {{{",".join(map(str, no))}}};')
lines.append(f'static const uint8_t LV_CB_Z[{len(blob)}] = {{')
for i in range(0, len(blob), 32): lines.append(','.join(str(b) for b in blob[i:i + 32]) + ',')
lines.append('};')
cbd_path.write_text('\n'.join(lines) + '\n')
print('LV_CB_Z', len(zb), '->', len(blob), 'B; slots 27')
pd_path = BW / 'src/live/pager/lv_pager_data.h'; pd = pd_path.read_text()
assert re.search(r'#define LV_PG_NSLOT\s+24', pd)
mo, co = grab_arr(pd, 'LV_PG_CORNER_OFF'); mz, cz = grab_arr(pd, 'LV_PG_CORNER_Z'); cz = bytes(cz)
oldc = [cz[co[k]:co[k+1]] for k in range(24)]
tmpl = np.frombuffer(zlib.decompress(oldc[11], -15), '<u2').reshape(19, 19).copy()
def corner(v):
    c = tmpl.copy(); c[0:3, 1] = v[301, 222]; c[0:15, 18] = v[300, 239]
    comp = zlib.compressobj(9, zlib.DEFLATED, -15, 9); return comp.compress(c.tobytes()) + comp.flush()
# sanity: corner template reproduces slot 0 (CIRCUIT) corner pixels
d1 = int((np.frombuffer(zlib.decompress(oldc[0], -15), '<u2') != np.frombuffer(zlib.decompress(corner(chk), -15), '<u2')).sum())
print('sanity corner slot0:', d1); assert d1 == 0
cs = [corner(frames[k]) if k in frames else oldc[k] for k in range(24)] + [corner(frames[k]) for k in (24, 25, 26)]
cb = bytearray(); cn = [0]
for z in cs: cb += z; cn.append(len(cb))
pd = re.sub(r'#define LV_PG_NSLOT(\s+)24', r'#define LV_PG_NSLOT\g<1>27', pd, count=1)
txt = f'static const uint32_t LV_PG_CORNER_OFF[{len(cn)}] = {{ {", ".join(map(str, cn))} }};\n'
txt += f'static const uint8_t LV_PG_CORNER_Z[{len(cb)}] = {{\n' + ',\n'.join(', '.join(str(b) for b in cb[k:k+16]) for k in range(0, len(cb), 16)) + '\n};'
pd = pd[:mo.start()] + txt + pd[mz.end():]
pd_path.write_text(pd); print('corners', len(cz), '->', len(cb))
