"""MORPHEUSbot rename mock helpers. Frames are the locked 720x960 (3x) clip frames; all edits are
whole 3x3 blocks addressed in 1x CYD coordinates (240x320). Nothing outside an edit is touched."""
import re, numpy as np
SRC = open('/workspace/cyd-portrait-wt/src/live/lv_font.h').read()
_g = SRC[SRC.index('LV_GLCD[95 * 5] = {'):]
_g = _g[:_g.index('};')]
GLCD = [int(x, 16) for x in re.findall(r'0x[0-9A-Fa-f]{2}', _g)][:475]
TINY3 = [0x7B6F, 0x2C97, 0x73E7, 0x73CF, 0x5BC9, 0x79CF, 0x79EF, 0x7292, 0x7BEF, 0x7BCF, 0x2BED, 0x6BAE, 0x3923, 0x6B6E, 0x79A7, 0x79A4, 0x396B, 0x5BED, 0x7497, 0x126A, 0x5BAD, 0x4927, 0x5FED, 0x6B6D, 0x2B6A, 0x6BA4, 0x2B73, 0x6BAD, 0x388E, 0x7492, 0x5B6F, 0x5B6A, 0x5BFD, 0x5AAD, 0x5A92, 0x72A7, 0x01C0]

def glcd_pts(s, x0, y0):
    pts = []
    for i, ch in enumerate(s):
        k = ord(ch) - 0x20
        for c in range(5):
            col = GLCD[k * 5 + c]
            for r in range(7):
                if col >> r & 1: pts.append((x0 + i * 6 + c, y0 + r))
    return pts

def tiny_pts(s, x0, y0, lower=None):
    pts = []
    for i, ch in enumerate(s):
        x = x0 + i * 4
        if lower and ch in lower:
            for (dx, dy) in lower[ch]: pts.append((x + dx, y0 + dy))
            continue
        if '0' <= ch <= '9': k = ord(ch) - 48
        elif 'A' <= ch <= 'Z': k = 10 + ord(ch) - 65
        elif ch == '-': k = 36
        else: continue
        m = TINY3[k]
        for b in range(15):
            if m & (1 << (14 - b)): pts.append((x + b % 3, y0 + b // 3))
    return pts

def s1(a):  # 1x view (block centre)
    return a[1::3, 1::3]

def put(a, x, y, c):
    a[3 * y:3 * y + 3, 3 * x:3 * x + 3] = c

def bright(p, thr=600): return int(p[0]) + int(p[1]) + int(p[2]) > thr

# ---- MORPHEUS pince-nez (1x bitmaps). 'R' rim/bridge, 'G' glint, '.' lens interior (face black), ' ' untouched
LENS11 = ["  RRRRR  ",
          " RLLLLLR ",
          "RLGGLLLLR",
          "RLGLLLLLR",
          "RLLLLLLLR",
          "RLLLLLLLR",
          "RLLLLLLLR",
          " RLLLLLR ",
          "  RRRRR  "]
LENS5 = [" RRRRR ",
         "RGLLLLR",
         "RLLLLLR",
         "RLLLLLR",
         " RRRRR "]
def glasses_pts(cxl, cxr, top, lens, bridge):
    """returns dict (x,y)->'R'|'G'|'.' ; lenses centred on cxl/cxr, top row = top; bridge = list of (dx,dy) from midpoint"""
    out = {}
    n = len(lens)
    for cx in (cxl, cxr):
        x0 = cx - n // 2
        for r, row in enumerate(lens):
            for c, ch in enumerate(row):
                if ch != ' ': out[(x0 + c, top + r)] = ch
    mid = (cxl + cxr) // 2
    for dx, dy in bridge: out[(mid + dx, top + dy)] = 'R'
    return out

BRIDGE_BIG = [(d, 1) for d in range(-3, 5)]   # arched bridge, lenses 107-115 / 124-132
BRIDGE_SMALL = [(-1, 1), (0, 1), (1, 1)]
