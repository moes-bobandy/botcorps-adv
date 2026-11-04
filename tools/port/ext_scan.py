# Extract the LOCKED ui_scan_glow masks at 1x: WIFI/BLE face masks (face-only tint), halo ring + alpha + colour for WIFI/BLE/GIB.
# Same code path as LOCKED/ui_scan_glow/glow_scan_current.py (glow_screens.masks + halo_masks), with prep_locked's /tmp/lk
# median/range recomputed into a private dir from LOCKED/ui_scanbot/scanbot_bg_themes_v2@3x.mp4.
# Usage: python ext_scan.py LOCKDIR REPO OUT.npz
import sys, os, glob, subprocess, types
import numpy as np
from PIL import Image
LOCK, REPO, OUT = [os.path.abspath(a) for a in sys.argv[1:4]]; LKD = '/tmp/fwp/lk'
d = LKD + '/S'; os.makedirs(d, exist_ok=True)
if not glob.glob(d + '/f*.png'):
    subprocess.run(['ffmpeg', '-v', 'error', '-i', LOCK + '/ui_scanbot/scanbot_bg_themes_v2@3x.mp4', '-start_number', '1', d + '/f%03d.png'], check=True)
if not os.path.exists(LKD + '/med_S.npy'):
    st = np.stack([np.array(Image.open(f).convert('RGB'))[1::3, 1::3] for f in sorted(glob.glob(d + '/f*.png'))])
    np.save(LKD + '/med_S.npy', np.median(st, 0).astype(np.uint8)); np.save(LKD + '/rng_S.npy', (st.max(0).astype(int) - st.min(0).astype(int)).max(-1))
SG = REPO + '/docs/mocks/screen-glow'; sys.path.insert(0, SG)
G = types.ModuleType('glow_screens'); G.__file__ = SG + '/glow_screens.py'
exec(compile(open(G.__file__).read().replace("'/tmp/lk", "'" + LKD), G.__file__, 'exec'), G.__dict__); sys.modules['glow_screens'] = G
src = open(LOCK + '/ui_scan_glow/glow_scan_current.py').read().replace("'/tmp/lk", "'" + LKD).split('def main():')[0]
M = {'__file__': LOCK + '/ui_scan_glow/glow_scan_current.py'}; exec(compile(src, M['__file__'], 'exec'), M)
cfg = G.SCREENS['scan']; chips, _ = G.masks(cfg); M['halo_masks'](chips)
med = np.load(LKD + '/med_S.npy'); tmed = med.copy()
for c in chips:
    if c['name'] in M['COLS']:
        fm = c['face_m']; tmed[fm] = M['face_tint'](med[fm], M['COLS'][c['name']]); c['colour'] = G.dominant(tmed[c['lift']])
    print(c['name'], c['face'], c['colour'], 'ring', int(c['ring'].sum()), 'face', int(c['face_m'].sum()))
# validate: 1x masks applied to the locked 3x base frame at the peak reproduce scan_glow_peak_3x.png
base = np.array(Image.open(LOCK + '/ui_scan_glow/scan_current_base_3x.png').convert('RGB'))
for c in chips: c['ring3'] = G.up3(c['ring']); c['alpha3'] = G.up3(c['alpha'])
pk = M['apply_halo'](base, chips, 1.0); ref = np.array(Image.open(LOCK + '/ui_scan_glow/scan_glow_peak_3x.png').convert('RGB'))
print('peak reproduced from base + 1x masks:', (pk == ref).all(), int((pk != ref).any(-1).sum()))
np.savez_compressed(OUT, names=np.array([c['name'] for c in chips]), face=np.array([c['face_m'] for c in chips]), ring=np.array([c['ring'] for c in chips]),
                    alpha=np.array([c['alpha'] for c in chips]), colour=np.array([c['colour'] for c in chips]), med=med,
                    cols=np.array([M['COLS'].get(c['name'], (0, 0, 0)) for c in chips]), lmid=M['L_MID'])
