#!/usr/bin/env python3
"""MOCK ONLY. Pager home built from locked v3.1 stills plus a 5-pip indicator
painted by pager_pips (C++). Default page is CONTROLbot. No wrap; edge bounce.
"""
import hashlib, json, subprocess, base64, struct, sys
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw, ImageFont

HERE = Path('/tmp/bigbot-1116-build')
OUT = Path('/workspace/out/bigbot-1116/pager')
STILL = OUT / 'stills'
SIM = OUT / 'sim'
for d in (STILL, SIM, OUT):
    d.mkdir(parents=True, exist_ok=True)

PIPS = HERE / 'pager_pips'
COMPOSE = HERE / 'bigbot_v3_compose'
V31 = Path('/workspace/out/bigbot-1116/v3.1/stills')
V6 = Path('/workspace/out/controlbot-colorui-1115/v6-cyd-ticker/stills')
FONT = Path('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf')
FONTB = Path('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf')

W, H = 240, 320
FPS = 30
SLIDE_N = 7            # 7 frames = 233 ms at 30 fps
BOUNCE_N = 7
DISSOLVE_N = 6         # 200 ms whole-screen recolor
FACE = (65, 88, 174, 197)
PANEL = (45, 220, 196, 290)
PIP_BAND = (102, 25, 137, 29)  # expected, measured after paint
BOT = (54, 79, 185, 210)
# page order
PAGES = ['botpheus', 'gibson', 'control', 'hackbot', 'terminal']
LABELS = ['BOTpheus', 'GIBSONbot', 'CONTROLbot', 'HACKbot', 'TERMINALbot']
TAPS = [
    'Center tap opens the Dojo / BOTpheus EB3 (LV_SC_MO)',
    'Center tap opens the Gibson scan tower (LV_SC_RADAR)',
    'Center tap cycles the theme (whole pager recolours)',
    'Center tap opens the HACKbot game (LV_SC_DL)',
    'Center tap opens the terminal screen (LV_SC_TERM)',
]
DEFAULT = 2  # CONTROLbot
HOLO_STILL = V6 / '13_holo_foil@1x.png'
HOLO_SLOT = 12
# v3 compose panel keys
PANEL_KEY = {'botpheus': 'botpheus', 'gibson': 'gibson', 'control': 'none',
             'hackbot': 'hackbot', 'terminal': 'terminal'}

def sha256(p: Path) -> str:
    return hashlib.sha256(p.read_bytes()).hexdigest()

def nn3(a):
    return np.repeat(np.repeat(a, 3, 0), 3, 1)

def load_png(p):
    return np.array(Image.open(p).convert('RGB'))

def to_rgb(im, path):
    Path(path).write_bytes(np.ascontiguousarray(im, np.uint8).tobytes())

def from_rgb(path):
    return np.fromfile(path, np.uint8).reshape(H, W, 3)

def save_pngs(a, stem: Path):
    Image.fromarray(a).save(stem.with_name(stem.name + '@1x.png'))
    Image.fromarray(nn3(a)).save(stem.with_name(stem.name + '@3x.png'))

def add_pips(im, idx):
    inp, outp = HERE / 'pg_in.rgb', HERE / 'pg_out.rgb'
    to_rgb(im, inp)
    subprocess.check_call([str(PIPS), str(inp), str(outp), str(idx)])
    return from_rgb(outp)

def compose_panel(base, panelk, slot):
    inp, outp = HERE / 'pg_c_in.rgb', HERE / 'pg_c_out.rgb'
    to_rgb(base, inp)
    subprocess.check_call([str(COMPOSE), str(inp), str(outp), 'none', panelk, str(slot), ''])
    return from_rgb(outp)

def ease_inout_cubic(t):
    t = float(np.clip(t, 0.0, 1.0))
    if t < 0.5:
        return 4 * t * t * t
    u = -2 * t + 2
    return 1 - u * u * u / 2

def slide(a, b, t, incoming_from_right):
    """t=0 -> a, t=1 -> b. If incoming_from_right, b enters from the right (swipe left / next)."""
    e = ease_inout_cubic(t)
    off = int(round(W * e))
    off = max(0, min(W, off))
    out = np.empty_like(a)
    if incoming_from_right:
        if off < W:
            out[:, :W - off] = a[:, off:]
        if off > 0:
            out[:, W - off:] = b[:, :off]
        if off == 0:
            out[:] = a
        if off == W:
            out[:] = b
    else:
        if off < W:
            out[:, off:] = a[:, :W - off]
        if off > 0:
            out[:, :off] = b[:, W - off:]
        if off == 0:
            out[:] = a
        if off == W:
            out[:] = b
    return out

def bounce(frame, toward_right_edge, t):
    """Nudge the page and spring back. toward_right_edge=True means we tried to go next (page shifts left)."""
    # peak at t=0.4
    if t < 0.4:
        e = ease_inout_cubic(t / 0.4)
        amt = 18 * e
    else:
        e = ease_inout_cubic((t - 0.4) / 0.6)
        amt = 18 * (1 - e)
    off = int(round(amt))
    out = np.zeros_like(frame)
    if toward_right_edge:
        out[:, :W - off] = frame[:, off:]
    else:
        out[:, off:] = frame[:, :W - off]
    return out

def flash_bot(frame, amount):
    out = frame.copy()
    x0, y0, x1, y1 = BOT
    roi = out[y0:y1 + 1, x0:x1 + 1].astype(np.float32)
    out[y0:y1 + 1, x0:x1 + 1] = np.clip(roi * (1 - amount) + 255.0 * amount, 0, 255).astype(np.uint8)
    return out

def dissolve(a, b, t):
    e = float(np.clip(t, 0.0, 1.0))
    return np.clip(a.astype(np.float32) * (1 - e) + b.astype(np.float32) * e, 0, 255).astype(np.uint8)

def diff_report(a, b):
    d = np.any(a != b, axis=2)
    if not d.any():
        return dict(changed_px=0, bbox=None, outside_pip_band=0)
    ys, xs = np.where(d)
    bbox = (int(xs.min()), int(ys.min()), int(xs.max()), int(ys.max()))
    mask = np.zeros_like(d)
    mask[25:30, 102:138] = True  # generous; refined after
    # compute exact outside y=25-29 or outside x of actual bbox later
    outside = int((d & ~np.zeros_like(d)).sum())  # placeholder
    band = np.zeros_like(d)
    band[25:30, :] = True
    outside = int((d & ~band).sum())
    # also face / panel
    face = np.zeros_like(d)
    face[88:198, 65:175] = True
    pan = np.zeros_like(d)
    pan[220:291, 45:197] = True
    return dict(
        changed_px=int(d.sum()),
        bbox=bbox,
        outside_y25_29=outside,
        in_face=int((d & face).sum()),
        in_panel=int((d & pan).sum()),
    )

def build_grape():
    pages = []
    proof = {'grape_vs_v31': []}
    for i, name in enumerate(PAGES):
        src = load_png(V31 / f'{name}@1x.png')
        out = add_pips(src, i)
        rec = diff_report(out, src)
        rec['id'] = name
        rec['index'] = i
        rec['lit_pip'] = i
        proof['grape_vs_v31'].append(rec)
        print('grape', name, rec)
        assert rec['outside_y25_29'] == 0 and rec['in_face'] == 0 and rec['in_panel'] == 0
        assert rec['bbox'][1] >= 25 and rec['bbox'][3] <= 29
        pages.append(out)
        save_pngs(out, STILL / f'grape_{name}')
    return pages, proof

def build_holo():
    body = load_png(HOLO_STILL)
    # v3.1 identity faces (theme-independent)
    faces = {n: load_png(V31 / f'{n}@1x.png')[88:198, 65:175].copy()
             for n in PAGES if n != 'control'}
    pages = []
    for i, name in enumerate(PAGES):
        if name == 'control':
            im = body.copy()
        else:
            im = compose_panel(body, PANEL_KEY[name], HOLO_SLOT)
            im[88:198, 65:175] = faces[name]
            # name pad stays as on the locked body (already empty/black)
        im = add_pips(im, i)
        pages.append(im)
        save_pngs(im, STILL / f'holo_{name}')
        print('holo', name, im.shape)
    return pages

def extend(frames, fr, n):
    for _ in range(n):
        frames.append(fr.copy())

def add_slide(frames, a, b, from_right):
    for k in range(1, SLIDE_N + 1):
        frames.append(slide(a, b, k / SLIDE_N, from_right))

def add_bounce(frames, fr, toward_right_edge):
    for k in range(1, BOUNCE_N + 1):
        frames.append(bounce(fr, toward_right_edge, k / BOUNCE_N))

def build_mp4(grape, holo):
    f = []
    # start on default CONTROLbot, hold
    extend(f, grape[2], 18)
    # through to the right end and bounce
    add_slide(f, grape[2], grape[3], True);  extend(f, grape[3], 10)
    add_slide(f, grape[3], grape[4], True);  extend(f, grape[4], 10)
    add_bounce(f, grape[4], True);           extend(f, grape[4], 8)
    # all the way back to the left end and bounce
    add_slide(f, grape[4], grape[3], False); extend(f, grape[3], 8)
    add_slide(f, grape[3], grape[2], False); extend(f, grape[2], 8)
    add_slide(f, grape[2], grape[1], False); extend(f, grape[1], 8)
    add_slide(f, grape[1], grape[0], False); extend(f, grape[0], 10)
    add_bounce(f, grape[0], False);          extend(f, grape[0], 8)
    # back to default CONTROL
    add_slide(f, grape[0], grape[1], True);  extend(f, grape[1], 8)
    add_slide(f, grape[1], grape[2], True);  extend(f, grape[2], 14)
    # center-tap theme cycle: brief face flash, then whole-screen dissolve to HOLO FOIL
    flash_amt = [0.15, 0.45, 0.55, 0.30, 0.10]
    for a in flash_amt:
        f.append(flash_bot(grape[2], a))
    for k in range(1, DISSOLVE_N + 1):
        f.append(dissolve(grape[2], holo[2], k / DISSOLVE_N))
    extend(f, holo[2], 12)
    # peek that the whole pager recoloured: slide to HACKbot and BOTpheus in HOLO, back to CONTROL
    add_slide(f, holo[2], holo[3], True);  extend(f, holo[3], 10)
    add_slide(f, holo[3], holo[2], False); extend(f, holo[2], 6)
    add_slide(f, holo[2], holo[1], False); extend(f, holo[1], 10)
    add_slide(f, holo[1], holo[2], True);  extend(f, holo[2], 16)

    raw = HERE / 'pager_frames.rgb'
    with raw.open('wb') as fh:
        for fr in f:
            fh.write(np.ascontiguousarray(fr, np.uint8).tobytes())
    mp4 = OUT / 'swipe_tour@3x.mp4'
    subprocess.check_call([
        'ffmpeg', '-y', '-framerate', str(FPS),
        '-f', 'rawvideo', '-pix_fmt', 'rgb24', '-s', f'{W}x{H}',
        '-i', str(raw),
        '-vf', 'scale=720:960:flags=neighbor',
        '-c:v', 'libx264', '-pix_fmt', 'yuv420p', '-crf', '17',
        '-movflags', '+faststart', str(mp4)
    ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    dur = len(f) / FPS
    print(f'mp4 frames={len(f)} duration={dur:.3f}s path={mp4}')
    # mid-slide and bounce stills for the storyboard
    mid = slide(grape[2], grape[3], 0.5, True)
    bnc = bounce(grape[4], True, 0.4)
    return f, mid, bnc, len(f), dur, mp4

def font(size, bold=False):
    p = FONTB if bold and FONTB.exists() else FONT
    return ImageFont.truetype(str(p), size) if p.exists() else ImageFont.load_default()

def storyboard(grape, holo, mid, bnc):
    """@3x frames with off-screen callouts. Not painted onto the CYD pixels."""
    tw, th = 720, 960
    pad = 28
    gutter = 24
    caption_h = 96
    title_h = 150
    note_h = 80
    # layout: title, row of 5 pages, callout row already in caption_h,
    # then a second row: mid-slide, bounce, theme cycle (grape control, holo control, holo hack)
    cols1 = 5
    width = pad * 2 + cols1 * tw + (cols1 - 1) * gutter
    # rows: title, 5 pages + captions, section, 5 supporting frames + captions, notes
    sec_h = 56
    height = (title_h + pad
              + th + caption_h + 36
              + sec_h
              + th + caption_h
              + note_h + pad)
    canvas = Image.new('RGB', (width, height), (10, 10, 14))
    draw = ImageDraw.Draw(canvas)
    ft = font(36, True)
    fs = font(20)
    fsb = font(22, True)
    fl = font(16)

    def paste(im, x, y, label, sub, tag=None):
        canvas.paste(Image.fromarray(nn3(im) if im.shape[0] == H else im), (x, y))
        # frame
        draw.rectangle([x - 1, y - 1, x + tw, y + th], outline=(60, 60, 80))
        draw.text((x, y + th + 8), label, font=fsb, fill=(230, 230, 240))
        if tag:
            # pill in the caption row under the still — never on CYD pixels
            twid = draw.textlength(tag, font=fl)
            lw = draw.textlength(label, font=fsb)
            bx0, by0 = x + lw + 16, y + th + 8
            draw.rounded_rectangle([bx0, by0, bx0 + twid + 16, by0 + 22], 5, fill=(80, 50, 140))
            draw.text((bx0 + 8, by0 + 3), tag, font=fl, fill=(240, 230, 255))
        if sub:
            draw.text((x, y + th + 36), sub, font=fl, fill=(180, 200, 140))

    draw.text((pad, 28), 'Big-bot pager  —  the new home screen  (MOCK)', font=ft, fill=(240, 240, 250))
    draw.text((pad, 76), 'Swipe left / right on the top ticker to change pages.  Default page is CONTROLbot (changeable).  No wrap — the ends bounce.',
              font=fs, fill=(160, 160, 175))

    y = title_h + pad
    for i, (im, lab, tap) in enumerate(zip(grape, LABELS, TAPS)):
        x = pad + i * (tw + gutter)
        tag = 'DEFAULT  (changeable)' if i == DEFAULT else None
        paste(im, x, y, f'{i + 1} / 5   {lab}', 'Callout (not on screen):  ' + tap, tag)

    y2 = y + th + caption_h + 36
    draw.text((pad, y2 + 10), 'Swipe, bounce, and a CONTROLbot center-tap theme cycle  (whole pager recolours to HOLO FOIL, the next theme in the locked v6 order)',
              font=fs, fill=(160, 160, 175))
    y3 = y2 + sec_h
    extras = [
        (mid, 'Mid-swipe  (CONTROL → HACKbot)', 'Whole 240×320 frame slides 233 ms, ease-in-out cubic.'),
        (bnc, 'Edge bounce  (at TERMINALbot)', 'No wrap. The page nudges and springs back.'),
        (grape[2], 'CONTROLbot  ·  NEON GRAPE', 'Center tap cycles the theme.'),
        (holo[2], 'CONTROLbot  ·  HOLO FOIL', 'After the tap, the current page is the new theme.'),
        (holo[3], 'HACKbot  ·  HOLO FOIL', 'Swipe after the cycle: every page has recoloured.'),
    ]
    for i, (im, lab, sub) in enumerate(extras):
        x = pad + i * (tw + gutter)
        paste(im, x, y3, lab, sub)

    note = (f'Pips live in the 5-px black gap under the ticker (y=25–29), theme ink, 5 dots.  '
            f'Face box (65,88)–(174,197) and bottom panels are untouched.  '
            f'Transition is a full-frame horizontal slide (not just the well between the pin rails) so the themed ticker, pins, and chrome stay attached to each page.')
    draw.text((pad, y3 + th + caption_h + 16), note, font=fl, fill=(140, 140, 155))
    path = OUT / 'storyboard.png'
    canvas.save(path)
    print('storyboard', canvas.size, path)
    return path

def b64_png(im):
    import io
    buf = io.BytesIO()
    Image.fromarray(nn3(im)).save(buf, format='PNG', optimize=True)
    return base64.b64encode(buf.getvalue()).decode('ascii')

def write_html(grape, holo):
    b64_g = [b64_png(im) for im in grape]
    b64_h = [b64_png(im) for im in holo]
    # compact json
    labels = LABELS
    taps = TAPS
    html = f'''<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
<title>BOTCORPS pager (mock)</title>
<style>
  :root {{ color-scheme: dark; }}
  html, body {{ margin: 0; padding: 0; background: #0b0b10; color: #e8e8f0;
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
    min-height: 100%; -webkit-user-select: none; user-select: none; }}
  body {{ padding: 16px 16px 32px; }}
  h1 {{ font-size: 18px; font-weight: 650; margin: 0 0 4px; }}
  .sub {{ font-size: 13px; color: #9aa; margin: 0 0 16px; line-height: 1.4; }}
  .stage {{ width: min(100%, 390px); margin: 0 auto; }}
  .device {{ position: relative; width: 100%; aspect-ratio: 240 / 320; background: #000;
    overflow: hidden; border-radius: 6px; box-shadow: 0 8px 28px rgba(0,0,0,.45);
    image-rendering: pixelated; image-rendering: crisp-edges; touch-action: none; }}
  .strip {{ position: absolute; inset: 0 auto 0 0; height: 100%; display: flex;
    will-change: transform; }}
  .strip img {{ width: calc(100vw); max-width: 390px; height: 100%; object-fit: fill;
    pointer-events: none; image-rendering: pixelated; flex: 0 0 auto; display: block; }}
  /* ticker swipe affordance — top of the CYD, not drawn into the frames */
  .ticker-hit {{ position: absolute; left: 0; right: 0; top: 0; height: 9.5%; z-index: 3; }}
  .face-hit {{ position: absolute; left: 22.1%; top: 24.4%; width: 55.8%; height: 41.9%; z-index: 2; }}
  .hint {{ margin-top: 14px; font-size: 13px; color: #9aa; line-height: 1.45; }}
  .toast {{ margin-top: 10px; min-height: 2.2em; font-size: 13px; color: #cde8b0; }}
  kbd {{ display: inline-block; border: 1px solid #556; border-bottom-width: 2px;
    border-radius: 3px; padding: 0 5px; font-size: 11px; background: #1a1a22; }}
</style>
</head>
<body>
  <div class="stage">
    <h1>BOTCORPS pager  ·  mock home</h1>
    <p class="sub">The five big-bot screens, in place of the home chip board.
      Swipe on the top ticker. Arrow keys work too. Default page is CONTROLbot (changeable).
      No wrap — the ends bounce.</p>
    <div class="device" id="device" aria-label="CYD pager">
      <div class="strip" id="strip"></div>
      <div class="ticker-hit" id="ticker" title="Swipe here"></div>
      <div class="face-hit" id="face" title="Center tap"></div>
    </div>
    <div class="toast" id="toast"></div>
    <p class="hint">
      Swipe left or right on the <strong>top ticker / announcement bar</strong> to change pages.
      <kbd>←</kbd> <kbd>→</kbd> do the same. At either end the page bounces; it does not wrap.
      Tap the big face on CONTROLbot to cycle the theme (NEON GRAPE ↔ HOLO FOIL) — the whole pager recolours.
      A center tap on the other bots is noted below; those screens are not in this mock.
    </p>
  </div>
<script>
const LABELS = {json.dumps(labels)};
const TAPS = {json.dumps(taps)};
const GRAPE = {json.dumps(b64_g)};
const HOLO  = {json.dumps(b64_h)};
const N = 5, DEFAULT = 2, SLIDE_MS = 233, BOUNCE_MS = 233;
const device = document.getElementById('device');
const strip  = document.getElementById('strip');
const ticker = document.getElementById('ticker');
const face   = document.getElementById('face');
const toast  = document.getElementById('toast');
let theme = 0; // 0 grape, 1 holo
let page = DEFAULT;
let width = 0;
let dragging = false, startX = 0, startT = 0, dragX = 0;
let anim = null;

function set() {{ return theme ? HOLO : GRAPE; }}
function pageWidth() {{ return device.getBoundingClientRect().width; }}
function applyImgs() {{
  const s = set();
  strip.innerHTML = s.map((b,i) => '<img alt="'+LABELS[i]+'" src="data:image/png;base64,'+b+'">').join('');
  sizeImgs();
}}
function sizeImgs() {{
  width = pageWidth();
  strip.querySelectorAll('img').forEach(img => {{ img.style.width = width+'px'; img.style.flexBasis = width+'px'; }});
  strip.style.width = (width * N) + 'px';
}}
function tx(x) {{ strip.style.transform = 'translateX(' + x + 'px)'; }}
function at(p, extra) {{ tx(-p * width + (extra||0)); }}
function ease(t) {{ return t<0.5 ? 4*t*t*t : 1-Math.pow(-2*t+2,3)/2; }}
function animate(from, to, ms, done) {{
  if (anim) cancelAnimationFrame(anim);
  const t0 = performance.now();
  function step(now) {{
    const u = Math.min(1, (now - t0) / ms);
    const x = from + (to - from) * ease(u);
    tx(x);
    if (u < 1) anim = requestAnimationFrame(step);
    else {{ anim = null; if (done) done(); }}
  }}
  anim = requestAnimationFrame(step);
}}
function go(next) {{
  next = Math.max(0, Math.min(N-1, next));
  const from = -page * width + dragX;
  page = next; dragX = 0;
  animate(from, -page * width, SLIDE_MS);
}}
function bounce(dir) {{
  // dir +1: tried to go next (nudge left)
  const origin = -page * width;
  const peak = origin - dir * Math.min(28, width * 0.08);
  animate(origin + dragX, peak, BOUNCE_MS * 0.4, () => {{
    animate(peak, origin, BOUNCE_MS * 0.6);
  }});
  dragX = 0;
}}
function flip(dir) {{
  const next = page + dir;
  if (next < 0 || next >= N) bounce(dir);
  else go(next);
}}
function showTap() {{
  toast.textContent = LABELS[page] + ' — ' + TAPS[page];
}}

ticker.addEventListener('pointerdown', ev => {{
  ev.preventDefault();
  dragging = true; startX = ev.clientX; startT = performance.now(); dragX = 0;
  ticker.setPointerCapture(ev.pointerId);
  if (anim) {{ cancelAnimationFrame(anim); anim = null; }}
}});
ticker.addEventListener('pointermove', ev => {{
  if (!dragging) return;
  ev.preventDefault();
  dragX = ev.clientX - startX;
  // rubber-band past the ends
  if ((page === 0 && dragX > 0) || (page === N-1 && dragX < 0)) dragX *= 0.35;
  at(page, dragX);
}});
function endDrag() {{
  if (!dragging) return;
  dragging = false;
  const dx = dragX;
  const dt = Math.max(1, performance.now() - startT);
  const flick = Math.abs(dx) / dt; // px/ms
  const thresh = width * 0.18;
  if (dx < -thresh || (dx < -20 && flick > 0.5)) flip(+1);
  else if (dx > thresh || (dx > 20 && flick > 0.5)) flip(-1);
  else if ((page === 0 && dx > 8) || (page === N-1 && dx < -8)) bounce(dx < 0 ? +1 : -1);
  else {{ const from = -page * width + dx; dragX = 0; animate(from, -page * width, SLIDE_MS); }}
}}
ticker.addEventListener('pointerup', endDrag);
ticker.addEventListener('pointercancel', endDrag);

face.addEventListener('click', ev => {{
  ev.preventDefault();
  showTap();
  if (page !== 2) return;
  theme = 1 - theme;
  const keep = page;
  applyImgs();
  page = keep;
  at(page, 0);
}});

window.addEventListener('keydown', ev => {{
  if (ev.key === 'ArrowLeft')  {{ ev.preventDefault(); flip(-1); }}
  if (ev.key === 'ArrowRight') {{ ev.preventDefault(); flip(+1); }}
}});
window.addEventListener('resize', () => {{ sizeImgs(); at(page, 0); }});

applyImgs();
at(DEFAULT, 0);
</script>
</body>
</html>
'''
    path = OUT / 'pager.html'
    path.write_text(html)
    print('html', path, 'bytes', path.stat().st_size)
    return path

def main():
    assert PIPS.exists() and COMPOSE.exists()
    grape, proof = build_grape()
    holo = build_holo()
    frames, mid, bnc, nfr, dur, mp4 = build_mp4(grape, holo)
    sb = storyboard(grape, holo, mid, bnc)
    html = write_html(grape, holo)
    proof['default_page'] = {'index': DEFAULT, 'name': 'CONTROLbot', 'changeable': True}
    proof['wrap'] = 'no wrap; edge bounce (~18 px, 233 ms, ease-in-out cubic)'
    proof['transition'] = {
        'type': 'full-frame horizontal slide',
        'duration_ms': round(1000 * SLIDE_N / FPS),
        'frames': SLIDE_N,
        'fps': FPS,
        'easing': 'ease-in-out cubic',
        'why': 'Each v3.1 / themed still is a complete locked board (ticker, pins, chrome, face, panel). Sliding the whole 240x320 frame keeps that board attached to the page, avoids a seam at the pin rails when a theme cycle recolours the ticker and pins, and at 7 frames / 233 ms on an ILI9341 (native 240x320, @3x shown with nearest-neighbour) it stays crisp without interpolated blur.',
    }
    proof['pip_band'] = {'y_inclusive': [25, 29], 'placement': '5-px black gap immediately under the ticker hatch, before the coil well; theme-ink AA discs; current pip brighter'}
    proof['mp4'] = {'path': str(mp4), 'frames': nfr, 'fps': FPS, 'duration_s': dur, 'codec': 'h264 yuv420p', 'size': '720x960 nearest-neighbour'}
    proof['theme_cycle'] = {'from': 'NEON GRAPE (lv_theme=0, slot 11)', 'to': 'HOLO FOIL (next in LV_CB_FW / locked v6 order, slot 12)', 'applies_to': 'all five pager pages'}
    (OUT / 'diff_proof.json').write_text(json.dumps(proof, indent=2) + '\n')
    print('done')

if __name__ == '__main__':
    main()
