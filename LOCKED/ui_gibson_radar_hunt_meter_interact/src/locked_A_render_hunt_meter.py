#!/usr/bin/env python3
"""MOCK ONLY — redesign the hunting signal meter so bars climb the hunted tower
green (base) → yellow (mid) → red (top of fill). Replaces the top-banner 11-bar meter.
Three design variants A/B/C. Does not touch firmware or locked charge D4 current.
Base: fw-port-1104 hunting still (has top hunt bar + D4 current).
"""
from __future__ import annotations
import os, math, hashlib, subprocess
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path("/workspace/out/radar-hunt-meter")
BASE = ROOT / "src" / "base_hunting@3x.png"

# --- geometry at 3x (from fw_D4_hunting@3x.png) ---
# Hunted tower front face (TELLO, orange WIFI with HUNT brackets)
TX0, TX1 = 231, 258          # inner face (inside orange edges)
TY_TOP, TY_BASE = 387, 504   # glass top under HUNT label → base line
# Top hunt-bar meter region to clear (the 11 rising bars)
MX0, MY0, MX1, MY1 = 250, 18, 445, 66
BANNER_BG = (0x1A, 0x00, 0x12)

GREEN  = (0x30, 0xFF, 0x60)
YELLOW = (0xFF, 0xB0, 0x00)
RED    = (0xFF, 0x30, 0x30)
DIM    = (0x3A, 0x2A, 0x34)
GLOW_G = (0x18, 0x80, 0x30)
GLOW_Y = (0x80, 0x58, 0x00)
GLOW_R = (0x80, 0x18, 0x18)

FILLS = {
    "idle": 0.22,   # weak signal — mostly green
    "mid":  0.55,   # ~-66 dBm-ish — green + yellow
    "full": 1.00,   # strong — green + yellow + red tip
}

def lerp(a, b, t):
    return int(a + (b - a) * t)

def blend(c1, c2, t):
    t = max(0.0, min(1.0, t))
    return tuple(lerp(c1[i], c2[i], t) for i in range(3))

def color_at_height(t: float):
    """t=0 at base (green), t=1 at top (red). Continuous green→yellow→red."""
    if t <= 0.45:
        return blend(GREEN, YELLOW, t / 0.45)
    return blend(YELLOW, RED, (t - 0.45) / 0.55)

def glow_at_height(t: float):
    if t <= 0.45:
        return blend(GLOW_G, GLOW_Y, t / 0.45)
    return blend(GLOW_Y, GLOW_R, (t - 0.45) / 0.55)

def clear_top_meter(im: Image.Image):
    """Replace the top 11-bar signal meter with banner bg; keep HUNT/name/dBm/STEADY."""
    d = ImageDraw.Draw(im)
    d.rectangle([MX0, MY0, MX1, MY1], fill=BANNER_BG)
    # subtle hairline so the banner doesn't feel empty — a thin "ON TOWER" cue as 2 px ticks
    # draw a tiny tower-glyph silhouette in the cleared slot (not a meter)
    cx = (MX0 + MX1) // 2
    # small schematic: base bar + rising stem suggesting "meter lives on tower now"
    d.rectangle([cx - 18, 54, cx + 18, 58], fill=(0x60, 0x40, 0x50))
    d.rectangle([cx - 4, 28, cx + 4, 54], fill=(0x60, 0x40, 0x50))
    # three soft level marks on the stem (decorative, not the live meter)
    for i, col in enumerate([(0x30, 0x80, 0x40), (0x80, 0x60, 0x20), (0x80, 0x30, 0x30)]):
        y = 50 - i * 8
        d.rectangle([cx - 10, y, cx - 6, y + 3], fill=col)
        d.rectangle([cx + 6, y, cx + 10, y + 3], fill=col)

def tower_y_for_fill(fill: float):
    """Pixel y of the top of the fill (fill=0 → base, fill=1 → top)."""
    h = TY_BASE - TY_TOP
    return int(TY_BASE - fill * h)

def draw_A(im: Image.Image, fill: float):
    """A — Thermometer bands: horizontal bar segments stacked up the tower glass."""
    d = ImageDraw.Draw(im)
    inset_l, inset_r = TX0 + 2, TX1 - 2
    n_slots = 14
    slot_h = (TY_BASE - TY_TOP) / n_slots
    lit = max(1, int(round(fill * n_slots))) if fill > 0 else 0
    # dim empty slots
    for i in range(n_slots):
        y1 = int(TY_BASE - (i + 1) * slot_h) + 1
        y0 = int(TY_BASE - i * slot_h) - 1
        if y0 <= y1: continue
        t = (i + 0.5) / n_slots
        if i < lit:
            col = color_at_height(t)
            glow = glow_at_height(t)
            # soft glow behind band
            d.rectangle([inset_l - 1, y1, inset_r + 1, y0], fill=glow)
            d.rectangle([inset_l, y1 + 1, inset_r, y0 - 1], fill=col)
            # 1 px highlight on top of each lit band
            hi = blend(col, (255, 255, 255), 0.35)
            d.rectangle([inset_l, y1 + 1, inset_r, y1 + 2], fill=hi)
        else:
            d.rectangle([inset_l, y1 + 2, inset_r, y0 - 2], fill=DIM)
    # keep orange tower edges visible: redraw thin edge lines
    d.rectangle([TX0 - 3, TY_TOP, TX0 - 1, TY_BASE], fill=(255, 121, 0))
    d.rectangle([TX1 + 1, TY_TOP, TX1 + 3, TY_BASE], fill=(255, 121, 0))
    d.rectangle([TX0 - 3, TY_BASE, TX1 + 3, TY_BASE + 2], fill=(255, 121, 0))

def draw_B(im: Image.Image, fill: float):
    """B — Equalizer columns on the tower face: vertical rising bars, stair heights,
    colored by absolute height green→yellow→red. Echo of the old top meter, relocated."""
    d = ImageDraw.Draw(im)
    n_bars = 6
    gap = 1
    total_w = TX1 - TX0
    bar_w = max(2, (total_w - gap * (n_bars - 1)) // n_bars)
    # classic rising profile, scaled by fill
    profile = [0.35, 0.50, 0.65, 0.80, 0.90, 1.00]
    for i in range(n_bars):
        x0 = TX0 + i * (bar_w + gap)
        x1 = x0 + bar_w - 1
        h_frac = profile[i] * fill
        if h_frac <= 0.02:
            # dim stub
            d.rectangle([x0, TY_BASE - 4, x1, TY_BASE - 1], fill=DIM)
            continue
        y_top = tower_y_for_fill(h_frac)
        # unlit shaft above
        if y_top > TY_TOP + 2:
            d.rectangle([x0, TY_TOP + 2, x1, y_top - 1], fill=DIM)
        # lit pixels colored by height
        for y in range(y_top, TY_BASE):
            t = (TY_BASE - y) / max(1, TY_BASE - TY_TOP)
            col = color_at_height(t)
            d.point([(x, y) for x in range(x0, x1 + 1)], fill=col)
        # bright tip cap
        tip = color_at_height(h_frac)
        d.rectangle([x0, y_top, x1, y_top + 2], fill=blend(tip, (255, 255, 255), 0.4))
    d.rectangle([TX0 - 3, TY_TOP, TX0 - 1, TY_BASE], fill=(255, 121, 0))
    d.rectangle([TX1 + 1, TY_TOP, TX1 + 3, TY_BASE], fill=(255, 121, 0))

def draw_C(im: Image.Image, fill: float):
    """C — Edge rails + center mercury: ticks climb L/R edges; 5 px center fill.
    Preserves most of the tower glass so the charge current behind stays readable."""
    d = ImageDraw.Draw(im)
    n_ticks = 12
    lit = max(1, int(round(fill * n_ticks))) if fill > 0 else 0
    span = TY_BASE - TY_TOP
    # center mercury (5 px)
    cx0, cx1 = (TX0 + TX1) // 2 - 2, (TX0 + TX1) // 2 + 2
    y_fill = tower_y_for_fill(fill)
    d.rectangle([cx0, TY_TOP + 2, cx1, TY_BASE - 1], fill=DIM)
    for y in range(y_fill, TY_BASE):
        t = (TY_BASE - y) / max(1, span)
        col = color_at_height(t)
        for x in range(cx0, cx1 + 1):
            d.point((x, y), fill=col)
    if fill > 0.05:
        tip = color_at_height(fill)
        d.rectangle([cx0 - 2, y_fill - 1, cx1 + 2, y_fill + 3], fill=blend(tip, (255, 255, 255), 0.4))
    for i in range(n_ticks):
        t = (i + 0.5) / n_ticks
        y = int(TY_BASE - (i + 0.5) * span / n_ticks)
        on = i < lit
        col = color_at_height(t) if on else DIM
        d.rectangle([TX0 - 7, y - 1, TX0 - 1, y + 1], fill=col)
        d.rectangle([TX1 + 1, y - 1, TX1 + 7, y + 1], fill=col)
        if on:
            d.rectangle([TX0, y - 1, TX0 + 3, y + 1], fill=col)
            d.rectangle([TX1 - 3, y - 1, TX1, y + 1], fill=col)
            d.rectangle([TX0 - 9, y, TX0 - 7, y], fill=col)
            d.rectangle([TX1 + 7, y, TX1 + 9, y], fill=col)

DRAW = {"A": draw_A, "B": draw_B, "C": draw_C}

def render_still(variant: str, fill_name: str) -> Image.Image:
    im = Image.open(BASE).convert("RGB")
    clear_top_meter(im)
    DRAW[variant](im, FILLS[fill_name])
    return im

def to_1x(im3: Image.Image) -> Image.Image:
    return im3.resize((240, 320), Image.NEAREST)

def save_pair(im3: Image.Image, path3: Path):
    path3.parent.mkdir(parents=True, exist_ok=True)
    im3.save(path3)
    path1 = Path(str(path3).replace("@3x.png", "@1x.png"))
    to_1x(im3).save(path1)
    return path3, path1

def side_by_side(left: Image.Image, right: Image.Image, label_l: str, label_r: str) -> Image.Image:
    w, h = left.size
    out = Image.new("RGB", (w * 2 + 16, h + 36), (10, 0, 12))
    out.paste(left, (0, 36))
    out.paste(right, (w + 16, 36))
    d = ImageDraw.Draw(out)
    d.text((8, 8), label_l, fill=(255, 176, 0))
    d.text((w + 24, 8), label_r, fill=(48, 255, 96))
    # divider
    d.rectangle([w + 6, 36, w + 10, h + 36], fill=(255, 42, 212))
    return out

def sheet_abc(mids: dict) -> Image.Image:
    w, h = mids["A"].size
    out = Image.new("RGB", (w * 3 + 32, h + 36), (10, 0, 12))
    d = ImageDraw.Draw(out)
    for i, k in enumerate("ABC"):
        out.paste(mids[k], (i * (w + 16), 36))
        d.text((i * (w + 16) + 8, 8), f"{k} mid (tower meter)", fill=(48, 255, 96))
    return out

def encode_mp4(frames_dir: Path, out_mp4: Path, fps: int = 30):
    """H.264 yuv420p +faststart 720x960 from frame_0001.png ..."""
    out_mp4.parent.mkdir(parents=True, exist_ok=True)
    cmd = [
        "ffmpeg", "-y", "-hide_banner", "-loglevel", "error",
        "-framerate", str(fps),
        "-i", str(frames_dir / "frame_%04d.png"),
        "-c:v", "libx264", "-pix_fmt", "yuv420p",
        "-movflags", "+faststart",
        "-vf", "scale=720:960:flags=neighbor",
        str(out_mp4),
    ]
    subprocess.check_call(cmd)

def animate_variant(variant: str, n_frames: int = 60):
    """Climb idle→full over ~2 s at 30 fps."""
    frames_dir = ROOT / variant / "frames"
    frames_dir.mkdir(parents=True, exist_ok=True)
    for i in range(n_frames):
        # ease-out fill 0.05 → 1.0
        t = i / (n_frames - 1)
        fill = 0.05 + 0.95 * (1 - (1 - t) ** 2)
        im = render_still(variant, "mid")  # placeholder
        # re-render with custom fill
        im = Image.open(BASE).convert("RGB")
        clear_top_meter(im)
        DRAW[variant](im, fill)
        im.save(frames_dir / f"frame_{i+1:04d}.png")
    mp4 = ROOT / variant / f"hunt_meter_{variant}_3x.mp4"
    encode_mp4(frames_dir, mp4)
    return mp4

def main():
    current = Image.open(BASE).convert("RGB")
    mids = {}
    for v in "ABC":
        for fill_name in ("idle", "mid", "full"):
            im = render_still(v, fill_name)
            p3, p1 = save_pair(im, ROOT / v / f"{v}_{fill_name}@3x.png")
            print("wrote", p3.name, p1.name)
            if fill_name == "mid":
                mids[v] = im
        # compare current vs mid
        cmp = side_by_side(current, mids[v], "CURRENT top meter", f"{v} tower meter")
        cmp.save(ROOT / "compare" / f"current_vs_{v}_mid@3x.png")
        print("compare", v)
        # short climb clip
        mp4 = animate_variant(v, n_frames=60)
        print("mp4", mp4)

    sheet = sheet_abc(mids)
    sheet.save(ROOT / "sheets" / "sheet_ABC_mid@3x.png")
    # also a fill ladder per variant: idle|mid|full
    for v in "ABC":
        w, h = mids[v].size
        ladder = Image.new("RGB", (w * 3 + 32, h + 36), (10, 0, 12))
        d = ImageDraw.Draw(ladder)
        for i, fn in enumerate(("idle", "mid", "full")):
            im = Image.open(ROOT / v / f"{v}_{fn}@3x.png")
            ladder.paste(im, (i * (w + 16), 36))
            d.text((i * (w + 16) + 8, 8), f"{v} {fn} fill={FILLS[fn]:.0%}", fill=(255, 176, 0))
        ladder.save(ROOT / "sheets" / f"ladder_{v}@3x.png")

    print("done")

if __name__ == "__main__":
    main()
