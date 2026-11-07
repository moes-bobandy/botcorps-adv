#!/usr/bin/env python3
"""Adv landscape gate. Compares each copied slot to the portrait shadow 1:1
and checks the CYD chip sizes that the landscape plate is required to keep.
Exit code = fail count."""
import os, sys

D = sys.argv[1]
LW, LH = 320, 240
PW, PH = 240, 320

def load(path, w, h):
    raw = open(path, "rb").read()
    need = w * h * 3
    if len(raw) != need:
        raise SystemExit(f"bad size {path} {len(raw)} want {need}")
    return raw

def pix(buf, w, x, y):
    i = (y * w + x) * 3
    return buf[i], buf[i + 1], buf[i + 2]

fails = 0
def rep(ok, msg):
    global fails
    print(("ADV_PX PASS " if ok else "ADV_PX FAIL ") + msg)
    if not ok:
        fails += 1

# name -> required (w, h) from the CYD portrait canon
CANON = {
    "home": {"boot": (44, 61), "gibson": (83, 121), "control": (44, 60), "scan": (69, 100), "terminal": (69, 100), "tbox": (171, 34)},
    "scan": {"wifi": (43, 63), "ble": (43, 60), "scanbot": (88, 121), "sbox": (174, 104)},
    "term": {"terminalbot": (91, 121), "deauth": (36, 51), "samsung": (36, 51), "tbox": (171, 34)},
    "ctl": {"face": (90, 124), "mix": (36, 56), "rain": (41, 51)},
}

screens = ["boot", "intro", "home", "scan", "term", "ctl", "tty", "radar"]
for name in screens:
    land = load(os.path.join(D, name + ".rgb"), LW, LH)
    port = load(os.path.join(D, name + ".port.rgb"), PW, PH)
    slots = []
    for line in open(os.path.join(D, name + ".slots")):
        f = line.split()
        slots.append((f[0], int(f[1]), int(f[2]), int(f[3]), int(f[4]), int(f[5]), int(f[6])))
    bad = 0
    checked = 0
    for nm, sx, sy, sw, sh, dx, dy in slots:
        for y in range(sh):
            for x in range(sw):
                if pix(land, LW, dx + x, dy + y) != pix(port, PW, sx + x, sy + y):
                    bad += 1
                    if bad > 8:
                        break
            if bad > 8:
                break
        checked += sw * sh
    rep(bad == 0, f"{name} 1:1 slots px checked={checked} mismatches={bad}")
    want = CANON.get(name, {})
    have = {nm: (sw, sh) for nm, sx, sy, sw, sh, dx, dy in slots}
    for nm, wh in want.items():
        rep(have.get(nm) == wh, f"{name} {nm} size {have.get(nm)} canon {wh}")

# Wi-Fi orange / BLE blue still live inside the scan chips (tint, not a flat grey).
scan = load(os.path.join(D, "scan.rgb"), LW, LH)
def count_hue(name, pred):
    slots = {}
    for line in open(os.path.join(D, "scan.slots")):
        f = line.split()
        slots[f[0]] = tuple(int(x) for x in f[1:])
    sx, sy, sw, sh, dx, dy = slots[name]
    n = 0
    for y in range(sh):
        for x in range(sw):
            r, g, b = pix(scan, LW, dx + x, dy + y)
            if pred(r, g, b):
                n += 1
    return n

orange = count_hue("wifi", lambda r, g, b: r > 140 and g > 40 and b < 80 and r > b + 60)
blue = count_hue("ble", lambda r, g, b: b > 80 and b > r + 20 and r < 140)
rep(orange >= 20, f"scan wifi orange px={orange}")
rep(blue >= 20, f"scan ble blue px={blue}")

# Landscape glass is 320 wide: the home plate is not a portrait frame rotated into a 240-wide strip.
home = load(os.path.join(D, "home.rgb"), LW, LH)
ink = 0
for x in range(LW):
    r, g, b = pix(home, LW, x, 80)
    if r + g + b > 30:
        ink += 1
rep(ink > 80, f"home row y=80 spans {ink}px (landscape content, not a 240-wide column)")

print(f"ADV_PX done fails={fails}")
sys.exit(1 if fails else 0)
