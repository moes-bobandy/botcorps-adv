import sys, os, json; sys.path.insert(0, os.path.dirname(__file__)); from mlib import *
from PIL import Image
FR = '/workspace/out/morpheus-rename/frames'; OUT = '/workspace/out/morpheus-rename/home_mock_frames'
os.makedirs(OUT, exist_ok=True)
def bm(rows): return [(c, r) for r, row in enumerate(rows) for c, ch in enumerate(row) if ch == '#']
LOWER = {'b': bm(['#..', '#..', '###', '#.#', '###']), 'o': bm(['...', '...', '###', '#.#', '###']),
         't': bm(['.#.', '###', '.#.', '.#.', '.##'])}   # the clip's own tiny lowercase (BOOTbot chip)
def blk(a, x, y): return a[3*y:3*y+3, 3*x:3*x+3].copy()
log = {}
N = int(sys.argv[1]) if len(sys.argv) > 1 else 540
for n in range(N):
    p = '%s/g%03d.png' % (FR, n)
    if not os.path.exists(p): break
    a = np.asarray(Image.open(p).convert('RGB')).copy(); o = a.copy(); s = s1(o).astype(int)
    ed = []
    # glasses: brow row = the 17-px bright row inside the BOOT chip face
    brow = [y for y in range(60, 92) if sum(s[y, x].sum() > 600 for x in range(43, 64)) == 17
            and s[y, 45].sum() > 600 and s[y, 61].sum() > 600 and s[y, 44].sum() < 100]
    if brow:
        t = brow[0]; faceb = blk(o, 43, t + 2); white = blk(o, 50, t)
        shy = [y for y in range(t + 6, t + 22) if 70 < s[y, 53].sum() < 400]
        shade = blk(o, 53, shy[0]) if shy else faceb
        for y in range(t - 1, t + 6):
            for x in range(44, 63): put(a, x, y, faceb)
        for (x, y), ch in glasses_pts(48, 58, t, LENS5, BRIDGE_SMALL).items():
            put(a, x, y, shade if ch == 'L' else white)
        ed.append('glasses@%d' % t)
    # label: old BOOT / BOOTbot -> tiny two lines MORPHEUS / bot (full name, hub12 TERMINAL/BOT rule)
    lab = [(x, y) for y in range(94, 108) for x in range(37, 70) if s[y, x].sum() > 600]
    if 40 <= len(lab) <= 120 and brow:
        xs = [x for x, y in lab]; ys = [y for x, y in lab]
        cx2 = min(xs) + max(xs); lb = max(ys)                 # 2*centre, bottom row
        ink = blk(o, *lab[0]); bgb = blk(o, min(xs) - 2, lb)
        assert s[lb, min(xs) - 2].sum() < 600
        for y in range(min(ys) - 1, lb + 2):
            for x in range(min(xs) - 1, max(xs) + 2): put(a, x, y, bgb)
        for word, y0 in (('MORPHEUS', lb - 10), ('bot', lb - 4)):
            w = len(word) * 4 - 1
            for (x, y) in tiny_pts(word, (cx2 + 1 - w) // 2, y0, LOWER): put(a, x, y, ink)
        ed.append('label %d..%d bot%d n%d' % (min(xs), max(xs), lb, len(lab)))
    log[n] = ed
    Image.fromarray(a).save('%s/m%03d.png' % (OUT, n))
json.dump(log, open(OUT + '/edits.json', 'w'))
from collections import Counter
print(Counter(' '.join(v) for v in log.values()).most_common(20))
