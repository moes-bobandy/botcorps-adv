#!/usr/bin/env python3
"""Steering 2 on the REAL sim: 3x3 grid of slight style heights (T1 h1, T2 h2; T3 = 1), 320 idle samples each, pick max idle emblem visibility
(score = mean visible fraction + 0.5 * worst pillar's mean fraction), tie-break on tap p10. Writes htune_sim.json."""
import json, os, re, shutil, subprocess, numpy as np, touch20 as T
ZIG = '/tmp/rci/zig/ziglang/zig'; SRC = os.path.dirname(os.path.abspath(__file__)); res = []
for h1 in (1.05, 1.10, 1.15):
    for h2 in (0.85, 0.90, 0.95):
        d = '/tmp/mmix/ht'; shutil.rmtree(d, ignore_errors=True); shutil.copytree('/workspace/out/morpheus-mix/tree', d, ignore=shutil.ignore_patterns('.git'))
        p = d + '/src/live/lv_morpheus_v13.h'; s = open(p).read().replace('"../../../src/', '"%s/' % SRC)
        s = re.sub(r'static const float MO_HSC_LOOK\[4\] = \{1.0f, [\d.]+f, [\d.]+f, 1.0f\};', 'static const float MO_HSC_LOOK[4] = {1.0f, %.3ff, %.3ff, 1.0f};' % (h1, h2), s); open(p, 'w').write(s)
        subprocess.run([ZIG, 'c++', '-target', 'x86_64-linux-gnu.2.36', '-I/tmp/zinc', '-O2', '-std=gnu++17', '-w', '-o', '/tmp/mmix/sim_ht', 'tools/livescan/sim_live.cpp', '/usr/lib/x86_64-linux-gnu/libz.so.1'], cwd=d, check=True)
        S = T.sample('/tmp/mmix/sim_ht', d, '/tmp/mmix/ht_out'); sq, vp, vf = T.analyse(S); m = vf.mean(0)
        r = dict(h1=h1, h2=h2, vis_frac_mean=round(float(m.mean()), 4), vis_frac_worst=round(float(m.min()), 4), vis_px_min=float(vp.min()), vis_px_mean=round(float(vp.mean()), 1), tap_p10_mean=round(float(np.percentile(sq, 10, axis=0).mean()), 2), score=round(float(m.mean() + 0.5 * m.min()), 4))
        print(r, flush=True); res.append(r)
best = max(res, key=lambda r: (r['score'], r['tap_p10_mean'])); json.dump(dict(grid=res, best=best), open(SRC + '/htune_sim.json', 'w'), indent=1); print('best', best)
