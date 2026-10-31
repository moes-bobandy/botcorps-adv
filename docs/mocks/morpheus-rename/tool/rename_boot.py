import sys, os, json; sys.path.insert(0, os.path.dirname(__file__)); from mlib import *
from PIL import Image
FR = '/workspace/out/morpheus-rename/frames'; OUT = '/workspace/out/morpheus-rename/boot_mock_frames'
os.makedirs(OUT, exist_ok=True)
log = {}
def blk(a, x, y): return a[3*y:3*y+3, 3*x:3*x+3].copy()
for n in range(85):
    a = np.asarray(Image.open('%s/b%03d.png' % (FR, n)).convert('RGB')).copy(); o = a.copy(); s = s1(o).astype(int)
    ed = []
    # 1. big chip label BOOTbot (GLCD 5x7, 6 px pitch) -> MORPHEUSbot, same font, centred on the same column
    old = [(x, y) for y in range(155, 162) for x in range(99, 140) if bright(s[y, x])]
    if len(old) == 105:
        ink = blk(o, *old[0])
        bgb = blk(o, 92, 158)                                  # silk-window bg block left of the label
        assert not bright(s[158, 92])
        new = glcd_pts('MORPHEUSbot', 119 - 32, 155)          # 11*6-1 = 65 px, centre 119 like BOOTbot (99..139)
        for y in range(154, 163):
            for x in range(98, 141): put(a, x, y, bgb)   # whole old label box (+1): no H.264 ringing ghosts
        for (x, y) in new: put(a, x, y, ink)
        ed.append('label')
    # 2. bottom silk BOOTBOT REV C (tiny 3x5, right aligned at W-17) -> MORPHEUSBOT REV C, e_silk black plate
    if sum(s[y, x].sum() > 300 for y in range(309, 316) for x in range(150, 224)) == 94:
        silk = [(x, y) for y in range(310, 315) for x in range(172, 223) if s[y, x].sum() > 300]
        ink = blk(o, *silk[0]); blackb = blk(o, 171, 309)
        t = 'MORPHEUSBOT REV C'; w = len(t) * 4 - 1; x0 = 240 - 17 - w
        for y in range(309, 316):
            for x in range(x0 - 1, x0 + w + 1): put(a, x, y, blackb)
        for (x, y) in tiny_pts(t, x0, 310): put(a, x, y, ink)
        ed.append('silk')
    # 3. glasses: BOOTbot shades -> MORPHEUS pince-nez, same centres, top on the old brow row (keeps the bob)
    gl = [(x, y) for y in range(87, 112) for x in range(97, 143) if s[y, x].sum() > 300]
    if len(gl) == 224:
        top = min(y for x, y in gl)
        white = blk(o, 110, top + 1); faceb = blk(o, 100, top + 1); shade = blk(o, 110, 132)
        assert s[top+1, 110].sum() > 700 and s[top+1, 100].sum() < 60
        gl2 = [(x, y) for y in range(87, 112) for x in range(97, 143) if s[y, x].sum() > 70]
        xs = [x for x, y in gl2]; ys = [y for x, y in gl2]
        for y in range(min(ys) - 1, max(ys) + 2):
            for x in range(min(xs) - 1, max(xs) + 2): put(a, x, y, faceb)
        for (x, y), ch in glasses_pts(111, 128, top, LENS11, BRIDGE_BIG).items():
            put(a, x, y, shade if ch == 'L' else white)
        ed.append('glasses@%d' % top)
    log[n] = ed
    Image.fromarray(a).save('%s/m%03d.png' % (OUT, n))
json.dump(log, open(OUT + '/edits.json', 'w'))
print({k: v for k, v in log.items() if k % 10 == 0 or k > 72})
