"""Hit-list proof sheets: 3x nearest renders from the sim dumps, labelled, one PNG per topic in docs/livescan_verify/hitlist/.
usage: hitlist_sheets.py <hitlist dump dir> <main dump dir (after)> <before dump dir>"""
import sys, os, numpy as np
from PIL import Image, ImageDraw, ImageFont
H, A, B = sys.argv[1:4]; OUT = 'docs/livescan_verify/hitlist'; os.makedirs(OUT, exist_ok=True)
try: F = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf', 34)
except Exception: F = ImageFont.load_default()
def ld(d, n, crop=None):
    a = np.fromfile('%s/%s.rgb' % (d, n), np.uint8).reshape(320, 240, 3)
    if crop: x0, y0, x1, y1 = crop; a = a[y0:y1, x0:x1]
    return Image.fromarray(a).resize((a.shape[1] * 3, a.shape[0] * 3), Image.NEAREST)
def sheet(name, items, cols=None):
    cols = cols or len(items); rows = (len(items) + cols - 1) // cols
    w = max(i.width for _, i in items); h = max(i.height for _, i in items)
    S = Image.new('RGB', (cols * (w + 20) + 20, rows * (h + 70) + 20), (24, 24, 28)); D = ImageDraw.Draw(S)
    for k, (lab, im) in enumerate(items):
        x = 20 + (k % cols) * (w + 20); y = 20 + (k // cols) * (h + 70)
        D.text((x, y), lab, fill=(255, 255, 255), font=F); S.paste(im, (x, y + 50))
    S.save('%s/%s.png' % (OUT, name)); print(name, S.size)
pairs = [('home', 'home'), ('scan_all', 'scan_all'), ('term', 'term'), ('ctl', 'ctl')]
sheet('before_after', [(t + (' BEFORE' if k == 0 else ' AFTER'), ld(B if k == 0 else A, n)) for n, _ in pairs for k, t in ((0, n), (1, n))], 4)
sheet('chips_lit', [('idle (dark)', ld(H, 'h_scan_idle')), ('WiFi scanning', ld(H, 'h_scan_wifi_lit')), ('BLE scanning', ld(H, 'h_scan_ble_lit')), ('both have results', ld(H, 'h_scan_both_have'))])
sheet('ctl_toggles', [(l, ld(H, n)) for l, n in (('start', 'h_ctl_0'), ('THEME', 'h_ctl_theme'), ('MIX', 'h_ctl_mix'), ('RAIN COL', 'h_ctl_raincol'), ('MASHUP', 'h_ctl_mashup'),
                      ('RAIN', 'h_ctl_rain'), ('RAIN MODE', 'h_ctl_rainmode'), ('MULTI', 'h_ctl_multi'), ('ORIENT (flip)', 'h_ctl_orient'), ('settled', 'h_ctl_all'))], 5)
sheet('press_flash', [('SCAN button', ld(H, 'h_flash_scanbtn')), ('BACKbot', ld(H, 'h_flash_backbot')), ('home GIBSON', ld(H, 'h_flash_gibson')), ('CTL MIX', ld(H, 'h_ctl_mix'))])
BR = (150, 240, 240, 320)
sheet('backbot_per_screen', [('SCAN', ld(H, 'h_scan_idle', BR)), ('TERM', ld(H, 'h_term', BR)), ('CTL', ld(H, 'h_ctl_0', BR)), ('RADAR', ld(H, 'h_radar', BR)),
                             ('TTY (top-right)', ld(H, 'h_hunt_found', (150, 0, 240, 80)))])
sheet('hunt_banner_ticker', [('hunt: SCANNING', ld(H, 'h_hunt_scanning')), ('hunt: N FOUND / OF M SEEN', ld(H, 'h_hunt_found')), ('ticker find info', ld(H, 'h_hunt_ticker')), ('home ticker', ld(H, 'h_home_ticker'))])
sheet('radar', [('JACK IN (radar)', ld(H, 'h_radar')), ('tower tapped', ld(H, 'h_radar_tap')), ('radar + ticker', ld(H, 'h_radar_ticker'))])
sheet('grid_sublists', [('emoji grid', ld(H, 'h_scan_both_have')), ('WIFI sublist', ld(H, 'h_sub_wifi')), ('tile 2 sublist', ld(H, 'h_sub_ble_cat')),
                        ('tile 3 sublist', ld(H, 'h_sub_cat3')), ('grid page 2', ld(H, 'h_grid_page2'))])
