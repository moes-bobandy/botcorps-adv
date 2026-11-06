#!/usr/bin/env python3
"""MOCK ONLY - radar-current-meter-interact.
The LOCKED D4 grid current (firmware sim, src/live/lv_radar.h at c83a27e, LV_RD_CHARGE=1) connecting into and
interacting with the LOCKED A thermometer hunt meter (draw_A / clear_top_meter imported unchanged from
docs/mocks/radar-hunt-meter/src/render_hunt_meter.py @ c83a27e) while hunting is on.

Inputs (made by src/run.sh):
  SIM/hunt.rgb   200 sim frames (30 ms ticks) of the hunt screen, 1x 240x320, D4 current animating
  SIM/hunting.rgb the locked hunting dump (== hunt-meter base_hunting@3x, 0 px diff)
  SIM/err.txt    CGP route log per frame (LV_RD_CG_LOG)
Variants (all draw ON TOP of A; the D4 layer and A's look are never repainted):
  V1 FEED  - packets arriving at the tower contact run up a feed seam inside the glass and light the next band
  V2 ARC   - each arriving packet arcs from the contact into the lowest green bands; a brightness ripple climbs the stack
  V3 WRAP  - the wire continues up the tower edge beside the bands to the fill top; packets climb it and top up the meter
"""
from __future__ import annotations
import sys, math, json, subprocess, importlib.util
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw, ImageFont
FONT = ImageFont.load_default(size=26)

OUT = Path(sys.argv[1] if len(sys.argv) > 1 else "/workspace/out/radar-current-meter-interact")
SIM = Path(sys.argv[2] if len(sys.argv) > 2 else "/tmp/rci/o2")
HERE = Path(__file__).resolve().parent

spec = importlib.util.spec_from_file_location("lockedA", HERE / "locked_A_render_hunt_meter.py")
A = importlib.util.module_from_spec(spec); spec.loader.exec_module(A)

PK, ICE2, ICE3, ELEC = (0xc4, 0xea, 0xff), (0xa8, 0xdc, 0xff), (0x8c, 0xca, 0xff), (0x3a, 0xa8, 0xff)
CAP_OUT, CAP_IN = 176, 200      # added current px outside the glass: D4 ceiling 176; band hit flashes inside glass <= 200 (< banner 203)
N_SLOTS = 14
T0 = 26640                      # first recorded sim frame (ms), hunting dump is t=26610

# ---------------- inputs ----------------
fr = np.fromfile(SIM / "hunt.rgb", np.uint8).reshape(-1, 320, 240, 3)
hunting = np.frombuffer(open(SIM / "hunting.rgb", "rb").read(), np.uint8).reshape(320, 240, 3)
routes = {}
for l in open(SIM / "err.txt"):
    f = l.split()
    if f[0] == "CGP":
        t = int(f[1][2:])
        if t >= 26610 and t not in routes:
            routes[t] = [tuple(map(int, p.split(","))) for p in f[4:]]
ts = [t for t in sorted(routes) if t > 26610]
assert len(ts) == len(fr) == 200, (len(ts), len(fr))

def anchors(a1x):
    """A's face anchors from the frame: orange edge cols on row 150 (locked still: L=76,R=86 -> TX0=231,TX1=258)."""
    cols = [x for x in range(240) if tuple(a1x[150, x]) == (255, 121, 0)]
    L, R = cols[0], cols[-1]
    return 3 * (L + 1), 3 * R

def set_A(tx0, tx1):
    A.TX0, A.TX1 = tx0, tx1     # TY_TOP/TY_BASE are constant during the hunt (tower y 117..172)

def band_rect(i):
    sh = (A.TY_BASE - A.TY_TOP) / N_SLOTS
    y1 = int(A.TY_BASE - (i + 1) * sh) + 1; y0 = int(A.TY_BASE - i * sh) - 1
    return A.TX0 + 2, y1 + 1, A.TX1 - 2, y0 - 1     # lit band inner rect (inclusive)

def luma(c): return 0.299 * c[..., 0] + 0.587 * c[..., 1] + 0.114 * c[..., 2]

def blend(img, ys, xs, col, k, cap):
    """img float HxWx3; mix toward col by k at (ys,xs) with a luma ceiling (never brighter than max(cap, base))."""
    ys = np.asarray(ys); xs = np.asarray(xs); k = np.broadcast_to(np.asarray(k, float), ys.shape).copy()
    ok = (ys >= 0) & (ys < img.shape[0]) & (xs >= 0) & (xs < img.shape[1]) & (k > 0)
    ys, xs, k = ys[ok], xs[ok], np.clip(k[ok], 0, 1)
    if not len(ys): return
    b = img[ys, xs]; c = np.array(col, float)
    m = b + (c - b) * k[:, None]
    lb, lm = luma(b), luma(m)
    over = (lm > cap) & (lm > lb)
    if over.any():
        lim = np.maximum(cap, lb[over]); kk = np.where(lm[over] - lb[over] > 0, (lim - lb[over]) / (lm[over] - lb[over]), 0)
        m[over] = b[over] + (m[over] - b[over]) * np.clip(kk, 0, 1)[:, None]
    img[ys, xs] = m

def rect(img, x0, y0, x1, y1, col, k, cap):
    x0, x1 = sorted((int(x0), int(x1))); y0, y1 = sorted((int(y0), int(y1)))
    yy, xx = np.mgrid[y0:y1 + 1, x0:x1 + 1]
    blend(img, yy.ravel(), xx.ravel(), col, k, cap)

def disc(img, cx, cy, r, col, kmax, cap):
    yy, xx = np.mgrid[cy - r:cy + r + 1, cx - r:cx + r + 1]
    d = np.hypot(xx - cx, yy - cy); k = kmax * np.clip(1 - d / r, 0, 1) ** 2
    blend(img, yy.ravel(), xx.ravel(), col, k.ravel(), cap)

def block(img, x1, y1, col, k, cap):     # one 1x pixel = 3x3 block (D4 pixel grid)
    rect(img, 3 * x1, 3 * y1, 3 * x1 + 2, 3 * y1 + 2, col, k, cap)

def hsh(a, b):
    h = (a * 2654435761 ^ (b + 0x9e3779b9) * 2246822519) & 0xffffffff
    h ^= h >> 15; h = (h * 2654435761) & 0xffffffff; h ^= h >> 13; return h

def breath(t): return 1 + (2 / 32) * math.sin(2 * math.pi * (t % 2400) / 2400)   # D4 2.4 s breathing +-2/32

def target_slots(trel):
    """signal schedule for the clip: idle hunt 22% -> mid 55% -> full 100% (A's three locked fills)."""
    if trel < 1500: f = 0.22
    elif trel < 3500: f = 0.22 + (0.55 - 0.22) * (trel - 1500) / 2000
    elif trel < 4800: f = 0.55 + 0.45 * (trel - 3500) / 1300
    else: f = 1.0
    return int(round(f * N_SLOTS))

# ---------------- packet arrivals from the locked D4 packet law (lv_radar.h: ph = now/5 % 40, heads every 40 px) ----------------
def hit_dist(t):
    n = len(routes[t]); ph = (t // 5) % 40
    return (n - 1 - ph) % 40          # px from the nearest packet head to the contact (route end)
hits = [t for t in ts if hit_dist(t) < 6]     # head reaches the contact during this 30 ms tick (0.2 px/ms = 6 px/tick)

TRAVEL = {"V1": 120, "V2": 150, "V3": None}   # ms from contact hit to the meter top (V3: wire length / 0.2 px/ms)

def simulate(variant):
    """lit band count over time and the band-light / landing events (deterministic, in hit order)."""
    lit, land_ev = 0, []
    for th in hits:
        if variant == "V3":   # the packet climbs the wrap from the contact (y=163) to the current fill top at 0.2 px/ms
            land = th + int((163 - top_y1x(lit)) / 0.2) + 30
        else:
            land = th + TRAVEL[variant]
        new = lit < target_slots(land - T0)
        if new: lit += 1
        land_ev.append(dict(hit=th, land=land, band=lit - 1, new=new, lit_after=lit))
    return land_ev

def top_y1x(lit):
    sh = (A.TY_BASE - A.TY_TOP) / N_SLOTS
    return int(A.TY_BASE - max(lit, 1) * sh) // 3

def lit_at(ev, t):
    lit = 0
    for e in ev:
        if e["land"] <= t: lit = e["lit_after"]
    return lit

# ---------------- drawing ----------------
def base_A(s, lit):
    a1 = fr[s]
    im = Image.fromarray(a1).resize((720, 960), Image.NEAREST)
    tx0, tx1 = anchors(a1); set_A(tx0, tx1)
    A.clear_top_meter(im)
    A.draw_A(im, lit / N_SLOTS)
    return im

def contact(s):
    x, y = routes[ts[s]][-1]
    return x, y

def draw_flashes(img, ev, t, lit):
    """band flashes from landings: new band lights from ice to its own colour (300 ms); top-ups flash softer."""
    for e in ev:
        dt = t - e["land"]
        if 0 <= dt < 300 and e["band"] < lit:
            k = (0.85 if e["new"] else 0.50) * (1 - dt / 300) ** 1.2
            x0, y0, x1, y1 = band_rect(e["band"]); rect(img, x0, y0, x1, y1, PK, k, CAP_IN)

def contact_glow(img, s, t, bump):
    cx, cy = contact(s)
    k = 0.42 * breath(t) + 0.35 * bump
    disc(img, 3 * cx + 1, 3 * cy + 1, 8, ICE2, k, CAP_IN)
    block(img, cx, cy, PK, 0.55 + 0.3 * bump, CAP_IN)

def bump_at(t):
    b = 0.0
    for th in hits:
        dt = t - th
        if 0 <= dt < 200: b = max(b, 1 - dt / 200)
    return b

def render(variant, s, ev):
    t = ts[s]; lit = lit_at(ev, t) if variant != "A" else lit_at(EV_A, t)
    im = base_A(s, lit)
    if variant == "A": return im
    img = np.asarray(im).astype(float)
    bump = bump_at(t)
    xr = A.TX1 - 2                                  # inner right edge (contact side) of the band stack
    ytop = band_rect(max(lit, 1) - 1)[1]            # top of the highest lit band (3x)
    cy3 = 3 * contact(s)[1] + 1
    if variant == "V1":
        # feed seam: the current is plugged into the meter - faint ice seam from the contact up the stack's contact edge
        if lit:
            rect(img, xr - 2, ytop, xr, cy3, ICE2, 0.42 * breath(t), CAP_IN); rect(img, xr - 4, ytop, xr - 3, cy3, ELEC, 0.30 * breath(t), CAP_IN)
        # beads: packet runs up the seam to the top band, then lights it
        for e in ev:
            dt = t - e["hit"]; dur = e["land"] - e["hit"]
            if 0 <= dt <= dur:
                ytgt = band_rect(max(e["band"], 0))[1]
                yb = cy3 + (ytgt - cy3) * (dt / dur)
                for j in range(24):                 # 9 px core + 15 px fading tail (3x px)
                    y = int(yb + j)
                    k = 0.95 if j < 9 else 0.7 * (1 - (j - 9) / 15)
                    rect(img, xr - 3, y, xr, y, PK if j < 9 else ICE2, k, CAP_IN)
                    if j < 12: rect(img, xr - 6, y, xr - 4, y, ELEC, 0.5, CAP_IN)
        draw_flashes(img, ev, t, lit)
        contact_glow(img, s, t, bump)
    elif variant == "V2":
        draw_flashes(img, ev, t, lit)
        for e in ev:
            dt = t - e["hit"]
            # ripple: band b pulses at hit + 25 ms * b, up to the lit top
            for b in range(min(lit, N_SLOTS)):
                db = dt - 25 * b
                if 0 <= db < 220:
                    base_k = 0.80 if b < 3 else 0.55 - 0.015 * b
                    x0, y0, x1, y1 = band_rect(b); rect(img, x0, y0, x1, y1, PK, base_k * (1 - db / 220) ** 1.6, CAP_IN)
            # stepped arc from the contact into the lowest green bands (D4 crackle grammar: stepped, 1 px kinks, soft ice)
            if 0 <= dt < 120 and lit:
                cx, cy = contact(s); h = hsh(e["hit"] // 120, 7)
                tgt_b = h % min(3, lit)
                bx0, by0, bx1, by1 = band_rect(tgt_b)
                ex, ey = (bx0 + 3) // 3, (by0 + by1) // 6
                pts, x, y = [], cx, cy
                step = 0
                while x > ex:
                    pts.append((x, y)); x -= 1; step += 1
                    if step in (2, 4) and y != ey: y += 1 if ey > y else -1; pts.append((x, y))
                pts.append((ex, ey))
                kf = 1 - dt / 150
                for (px, py) in pts:
                    block(img, px, py, PK, 0.95 * kf, CAP_IN)
                    rect(img, 3 * px, 3 * py - 3, 3 * px + 2, 3 * py - 1, ICE3, 0.6 * kf, CAP_IN)
                    rect(img, 3 * px, 3 * py + 3, 3 * px + 2, 3 * py + 5, ICE3, 0.6 * kf, CAP_IN)
        contact_glow(img, s, t, bump)
    elif variant == "V3":
        cx, cy = contact(s)
        wx = A.TX1 + 4                              # just outside A's redrawn orange edge (TX1+1..TX1+3 stays on top)
        ty1 = top_y1x(lit) if lit else cy - 1
        br = breath(t)
        y0w = 3 * ty1                               # wire top (3x)
        yy = cy3 + 2
        # conduit: 2 px (1x) ice core + 3-step blue glow on the open side (D4 band; edge line stays on top)
        for j, kk in enumerate((0.60, 0.38, 0.18)):
            rect(img, wx + 6 + 3 * j, y0w, wx + 8 + 3 * j, yy, ELEC, kk * br, CAP_OUT)
        rect(img, wx, y0w, wx + 5, yy, ELEC, 0.55, CAP_OUT)
        rect(img, wx, y0w, wx + 5, yy, ICE2, 0.70 * br, CAP_OUT)
        # bridge from the route end to the wrap foot (the corner where the current turns up the tower)
        rect(img, 3 * cx, cy3 - 2, wx + 5, cy3 + 3, ICE2, 0.70 * br, CAP_OUT)
        # packets climbing the wrap (continuation of the D4 packets, same 0.2 px/ms, 6 px core + 10 px tail at 1x)
        for e in ev:
            dt = t - e["hit"]; dur = e["land"] - e["hit"]
            if 0 <= dt <= dur:
                yh = cy3 - (cy3 - 3 * top_y1x(max(e["band"], 0))) * dt / max(dur, 1)
                for j in range(16):
                    y = int(yh) + 3 * j
                    if y > yy: break
                    if j < 6:
                        rect(img, wx, y, wx + 5, y + 2, PK, 0.75, CAP_OUT); rect(img, wx + 6, y, wx + 11, y + 2, ICE2, 0.40, CAP_OUT)
                    else:
                        rect(img, wx, y, wx + 5, y + 2, ICE2, 0.55 * (16 - j) / 10, CAP_OUT)
        # charge tip at the fill top: steady 2x2 + one small spark re-rolled every 4 frames; touches the top band
        rect(img, wx, y0w - 3, wx + 5, y0w + 2, PK, 0.8, CAP_OUT)
        sh = hsh(t // 120, 11)
        sx, sy = wx + 6 + (sh % 2) * 3, y0w - 6 - ((sh >> 3) % 2) * 3
        rect(img, sx, sy, sx + 2, sy + 2, ICE2, 0.5, CAP_OUT)
        # soft contact glow at the join: where the wire touches the lit top band
        if lit:
            x0, y0, x1, y1 = band_rect(lit - 1)
            rect(img, x1 - 5, y0, x1, y1, ICE2, 0.45 * br + 0.25 * bump, CAP_IN)
        draw_flashes(img, ev, t, lit)
        contact_glow(img, s, t, bump)
    return Image.fromarray(np.clip(img + 0.5, 0, 255).astype(np.uint8))

# ---------------- run ----------------
EV = {}
set_A(*anchors(fr[0]))
for v in ("V1", "V2", "V3"): EV[v] = simulate(v)
EV_A = EV["V1"]          # A alone shows the same fill steps (no current interaction drawn)

def mask_for(s):
    """where the interaction may draw: tower glass + 30 px (3x) around it + the contact disc."""
    set_A(*anchors(fr[s])); m = np.zeros((960, 720), bool)
    m[A.TY_TOP - 30:A.TY_BASE + 30, A.TX0 - 30:A.TX1 + 34] = True
    cx, cy = contact(s); m[3 * cy - 30:3 * cy + 33, 3 * cx - 30:3 * cx + 33] = True
    return m

def main():
    OUT.mkdir(parents=True, exist_ok=True)
    report = {"hits_ms": [t - T0 for t in hits], "variants": {}}
    # locked-A identity check: A alone on the hunting dump at mid fill == the locked A_mid@3x
    im = Image.fromarray(hunting).resize((720, 960), Image.NEAREST); set_A(*anchors(hunting))
    A.clear_top_meter(im); A.draw_A(im, 0.55)
    locked = np.array(Image.open(HERE / "locked_A_mid@3x.png").convert("RGB"))
    report["A_alone_vs_locked_A_mid_px"] = int((np.asarray(im) != locked).any(2).sum())
    report["anchors_hunting"] = [A.TX0, A.TX1, A.TY_TOP, A.TY_BASE]
    nv = int(len(fr) * 30 * 30 / 1000)              # 180 video frames = 6.0 s
    vids = {}
    for v in ("A", "V1", "V2", "V3"):
        name = "A_alone" if v == "A" else v
        frames = [render(v, s, EV.get(v)) for s in range(len(fr))]
        out_mp4 = OUT / ("A_alone" if v == "A" else v) / f"{name}_3x.mp4"
        out_mp4.parent.mkdir(parents=True, exist_ok=True)
        p = subprocess.Popen(["ffmpeg", "-y", "-loglevel", "error", "-f", "rawvideo", "-pix_fmt", "rgb24", "-s", "720x960", "-r", "30",
                              "-i", "-", "-c:v", "libx264", "-preset", "slow", "-crf", "14", "-pix_fmt", "yuv420p", "-movflags", "+faststart", str(out_mp4)],
                             stdin=subprocess.PIPE)
        for j in range(nv): p.stdin.write(np.asarray(frames[min(len(fr) - 1, int(j * 1000 / 30 / 30))]).tobytes())
        p.stdin.close(); p.wait()
        vids[v] = frames
    # outside-mask check vs A alone (radar otherwise unchanged)
    for v in ("V1", "V2", "V3"):
        outside, changed = 0, 0
        for s in range(len(fr)):
            d = (np.asarray(vids[v][s]) != np.asarray(vids["A"][s])).any(2)
            changed += int(d.sum()); outside += int((d & ~mask_for(s)).sum())
        report["variants"][v] = dict(changed_px_total=changed, outside_mask_px=outside,
                                     landings=[dict(hit=e["hit"] - T0, land=e["land"] - T0, band=e["band"], new=e["new"]) for e in EV[v]])
    # stills: pick sim frames per moment (same frame for A and all variants)
    def quiet(v, s):
        t = ts[s]
        return bump_at(t) == 0 and all(not (0 <= t - e["hit"] <= e["land"] - e["hit"] + 300) for e in EV[v])
    picks = {}
    for nm, want in (("idle", 3), ("mid", 8), ("full", 14)):
        cand = [s for s in range(len(fr)) if all(lit_at(EV[v], ts[s]) == want for v in ("V1", "V2", "V3"))]
        q = [s for s in cand if all(quiet(v, s) for v in ("V1",))]
        picks[nm] = (q or cand)[len(q or cand) // 2]
    # pulse hit: a hit near mid fill, 1 sim frame after the packet reaches the contact
    mids = [th for th in hits if 7 <= lit_at(EV["V1"], th) <= 9]
    picks["pulse"] = ts.index(mids[len(mids) // 2]) + 1
    report["still_frames"] = {k: dict(sim_frame=s, t_ms=ts[s] - T0, lit={v: lit_at(EV[v] if v != "A" else EV_A, ts[s]) for v in ("A", "V1", "V2", "V3")}) for k, s in picks.items()}
    stills = {}
    for v in ("A", "V1", "V2", "V3"):
        d = OUT / ("A_alone" if v == "A" else v); d.mkdir(parents=True, exist_ok=True)
        for k, s in picks.items():
            im = vids[v][s]; stills[(v, k)] = im
            pre = "A" if v == "A" else v
            im.save(d / f"{pre}_{k}@3x.png"); im.resize((240, 320), Image.NEAREST).save(d / f"{pre}_{k}@1x.png")
    # V2 pulse: also save the frame where the arc is visible (hit frame) - pulse already is hit+1 tick
    # compare sheets vs locked A alone
    cmpd = OUT / "compare"; cmpd.mkdir(exist_ok=True)
    lab = {"A": "LOCKED A alone", "V1": "V1 FEED", "V2": "V2 ARC", "V3": "V3 WRAP"}
    for k in picks:
        sheet = Image.new("RGB", (720 * 4 + 48, 960 + 40), (10, 0, 12)); dr = ImageDraw.Draw(sheet)
        for i, v in enumerate(("A", "V1", "V2", "V3")):
            sheet.paste(stills[(v, k)], (i * 736, 40)); dr.text((i * 736 + 8, 12), f"{lab[v]} - {k}", fill=(255, 176, 0) if v == "A" else (120, 210, 255), font=FONT)
        sheet.save(cmpd / f"compare_{k}@3x.png")
    # zoom sheet: tower + contact crop x3 for every moment and variant
    zs = []
    for k in ("idle", "mid", "full", "pulse"):
        s = picks[k]; set_A(*anchors(fr[s])); cx, cy = contact(s)
        x0 = A.TX0 - 40; y0 = A.TY_TOP - 40
        row = [stills[(v, k)].crop((x0, y0, x0 + 150, y0 + 200)).resize((300, 400), Image.NEAREST) for v in ("A", "V1", "V2", "V3")]
        zs.append(row)
    Z = Image.new("RGB", (4 * 308 + 60, 4 * 408 + 34), (10, 0, 12)); dr = ImageDraw.Draw(Z)
    for i, v in enumerate(("A", "V1", "V2", "V3")): dr.text((60 + i * 308 + 6, 2), lab[v], fill=(255, 176, 0) if v == "A" else (120, 210, 255), font=ImageFont.load_default(size=20))
    for r, k in enumerate(("idle", "mid", "full", "pulse")):
        dr.text((4, 30 + r * 408 + 190), k, fill=(255, 255, 255), font=ImageFont.load_default(size=16))
        for i, im in enumerate(zs[r]): Z.paste(im, (60 + i * 308, 30 + r * 408))
    Z.save(cmpd / "zoom_tower_all@3x.png")
    # V2 arc frame (packet just hit: arc drawn) zoom - the pulse moment for ARC at the hit tick itself
    json.dump(report, open(OUT / "report.json", "w"), indent=1)
    print(json.dumps({k: v for k, v in report.items() if k != "variants"}, indent=1))
    for v in ("V1", "V2", "V3"): print(v, report["variants"][v]["changed_px_total"], "outside", report["variants"][v]["outside_mask_px"])

if __name__ == "__main__":
    main()
