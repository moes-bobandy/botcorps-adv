# Home board (CYD ESP32-2432S028R, 240x320 portrait): non-rain bot chips breathe to show they are tappable. MOCK ONLY.
# Base = the locked main-bot rain home board: g200 green home frame + GIBSONbot gaps3 rain (LOCKED/ui_main_bot_rain/gaps3.py,
# imported unchanged, 140 ms/row). Non-rain chips on that board: BOOTbot, CONTROLbot, SCANbot, TERMINALbot (none moved).
#  A PULSE: each chip's own pixels x k, k = 1.00 -> 1.40 -> 1.00 (raised cosine, 1.4 s, all four in sync), clamped at 255.
#  B GLOW : soft halo outside each chip edge in the chip's own accent colour, alpha (1-(d-1)/R)^2 * 0.50 * s, R = 7 px,
#           painted only on dark PCB pixels, never inside the GIBSONbot rain-chip box (tapers to 0 over 3 px before it);
#           chips themselves untouched.
import sys, os, math, subprocess; sys.path.insert(0, '/workspace/out/main-bot-rain')
os.chdir('/workspace/out/main-bot-rain')
exec(open('/workspace/out/main-bot-rain/gaps3.py').read().rsplit("if __name__ == '__main__':", 1)[0])   # preps3, chips3, T0, P (locked)
from scipy.ndimage import distance_transform_edt
from PIL import ImageDraw
OUT = '/workspace/out/home-tap-pulse'
FPS, PERIOD_F, NF = 30, 42, 168        # 1.4 s period (42 frames), 4 periods = 5.6 s = 40 rain steps (rain loops every 10) -> seamless
KMAX, R, AMAX = 1.40, 7, 0.50
CHIPS = {  # name: (x0, y0, x1, y1) inclusive 1x rect incl. bezel + pins, accent rgb (sampled from the chip itself)
    'BOOTbot':     ((31, 51, 75, 111),    (147, 254, 0)),     # theme-lime pins/waveform (face is black/white)
    'CONTROLbot':  ((165, 51, 209, 111),  (147, 254, 0)),     # lime face
    'SCANbot':     ((31, 158, 104, 256),  (245, 180, 73)),    # gold face highlight
    'TERMINALbot': ((137, 158, 210, 256), (197, 205, 212)),   # silver face
}
RAIN_BOX = (76, 40, 164, 155)          # GIBSONbot rain chip incl. its pins + margin: never touched
H, W = 320, 240
def rect(b):
    m = np.zeros((H, W), bool); m[b[1]:b[3] + 1, b[0]:b[2] + 1] = True; return m
rain = rect(RAIN_BOX)
chipm = {k: rect(b) for k, (b, _) in CHIPS.items()}
anychip = np.any(list(chipm.values()), 0)
allowed = np.zeros((H, W), bool); allowed[45:262, 26:214] = True; allowed &= ~rain & ~anychip   # board interior only, away from pin labels / terminal
taper = np.minimum(1, distance_transform_edt(~rain) / 3)   # halo fades out before the rain-chip box, no hard cut
GLOW = []                              # precomputed ring per chip: (mask, alpha at peak) -- what firmware would store
for k, (b, acc) in CHIPS.items():
    d = distance_transform_edt(~chipm[k]); ring = (d > 0) & (d <= R) & allowed
    GLOW.append((ring, np.where(ring, AMAX * (1 - (d - 1) / R) ** 2 * taper, 0.0), np.array(acc, float)))
def breath(n): return 0.5 - 0.5 * math.cos(2 * math.pi * n / PERIOD_F)      # 0 at n=0, 1 at n=21
def board(Pp, n): return chips3(Pp, T0 + n * 1000 // FPS // 140)[0]         # locked GIBSON home frame at this 30 fps frame
def pulse(base, s):
    a = base.astype(float); k = 1 + (KMAX - 1) * s
    a[anychip] = np.minimum(a[anychip] * k, 255); return a.round().astype(np.uint8)
def glow(base, s):
    a = base.astype(float); dark = base.astype(int).sum(-1) < 60; al = np.zeros((H, W)); col = np.zeros((H, W, 3))
    for ring, alpha, acc in GLOW:
        m = alpha * s > al; al = np.where(m, alpha * s, al); col[m] = acc               # overlapping halos: strongest wins
    al = al * dark; a = a * (1 - al[..., None]) + col * al[..., None]
    return a.round().astype(np.uint8)
up = lambda a: np.repeat(np.repeat(a.astype(np.uint8), 3, 0), 3, 1)
def enc(fd, out):
    subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-framerate', str(FPS), '-i', fd + '/f%03d.png', '-vf', 'pad=ceil(iw/2)*2:ceil(ih/2)*2',
                    '-c:v', 'libx264', '-profile:v', 'main', '-crf', '12', '-pix_fmt', 'yuv420p', '-movflags', '+faststart', out], check=True)
if __name__ == '__main__':
    Pp = preps3('green'); fa, fb, fs = '/tmp/htp_A', '/tmp/htp_B', '/tmp/htp_SBS'
    for d in (fa, fb, fs): os.makedirs(d, exist_ok=True)
    try: from PIL import ImageFont; font = ImageFont.load_default(size=24)
    except Exception: font = None
    for n in range(NF):
        base = board(Pp, n); s = breath(n); A = up(pulse(base, s)); B = up(glow(base, s))
        Image.fromarray(A).save('%s/f%03d.png' % (fa, n)); Image.fromarray(B).save('%s/f%03d.png' % (fb, n))
        sbs = np.zeros((960 + 36, 720 * 2 + 12, 3), np.uint8); sbs[36:, :720] = A; sbs[36:, 732:] = B
        im = Image.fromarray(sbs); dr = ImageDraw.Draw(im)
        dr.text((8, 6), 'A  PULSE (chip colours 100->140%)', fill=(230, 230, 230), font=font); dr.text((740, 6), 'B  GLOW (soft halo, chip accent)', fill=(230, 230, 230), font=font)
        im.save('%s/f%03d.png' % (fs, n))
        if n == PERIOD_F // 2:
            Image.fromarray(A).save(OUT + '/home_tap_pulse_A_peak_3x.png'); Image.fromarray(B).save(OUT + '/home_tap_glow_B_peak_3x.png')
    enc(fa, OUT + '/home_tap_pulse_A_3x.mp4'); enc(fb, OUT + '/home_tap_glow_B_3x.mp4'); enc(fs, OUT + '/home_tap_sbs_A_B_3x.mp4')
    print('glow ring px per chip', [int(g[0].sum()) for g in GLOW], 'chip px', [int(m.sum()) for m in chipm.values()])
