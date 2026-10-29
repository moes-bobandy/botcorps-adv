#!/usr/bin/env python3
"""Gibson Hub reachability audit: one fresh boot per target; open it, then crown back (up to 3 taps).
usage: audit.py SIMBIN OUTDIR"""
import os, re, subprocess, sys, json
SIM, OUT = sys.argv[1], sys.argv[2]
os.makedirs(OUT, exist_ok=True)
CROWN = (146, 12)
KEYS = {}
rows = ["1234567890", "qwertyuiop", "asdfghjkl"]
for r, s in enumerate(rows):
    for c, ch in enumerate(s): KEYS[ch] = (24 + 21 * c, 267 + 6 * r)
for c, ch in enumerate("zxcvbnm", start=1): KEYS[ch] = (24 + 21 * c, 267 + 18)
KEYS[' '] = (108, 291)
OK, ABC, GL = (202, 291), (208, 287), (171, 287)
def type_cmd(s):
    t = ["tap %d %d" % ABC, "wait 200"]
    for ch in s: t.append("tap %d %d" % KEYS[ch])
    t += ["wait 200", "tap %d %d" % OK]
    return t
def taps(*pts): return ["tap %d %d" % p for p in pts]
cases = []   # (name, open-steps, crown point)
for i in range(12):
    sx = 10 if i < 6 else 192; sy = 44 + (i % 6) * 23
    cases.append(("hub-bot%02d" % i, taps((sx + 14, sy + 10)), CROWN))
cases.append(("hub-core", taps((120, 114)), CROWN))
cases.append(("hub-topleft-chip", taps((15, 12)), CROWN))
cases.append(("hub-terminal-plate", taps((102, 221)) + ["wait 300"] + taps((102, 221)), CROWN))
cases.append(("hub-control-plate", taps((138, 221)) + ["wait 300"] + taps((138, 221)), (120, 8)))
cases.append(("hub-top-term", taps((177, 12)), CROWN))
GP = ["hunt", "defend", "scan", "scan wifi", "scan ble", "wifi", "ble", "bots", "chat", "ping", "info",
      "track", "sniff", "filter", "rain", "terminal", "control", "go", "load", "help", "clear"]
for c in GP:
    stay = c in ("ping", "info", "track", "sniff", "filter", "rain", "go", "load", "help", "clear")
    cases.append(("hub-cmd-" + c.replace(" ", "_"), type_cmd(c) + (["wait 300", "tap 34 291"] if stay else []),
                  (120, 8) if c == "control" else CROWN))
cases.append(("hub-list-row0", taps(GL) + ["wait 300"] + taps((60, 268)) + ["wait 200"] + taps((60, 268)), CROWN))
# terminal tiles: open terminal via >_, double tap tile i
TX = lambda i: (6 + (i % 3) * 78 + 38, 19 + (i // 3) * 50 + 25)
for i in range(12):
    cases.append(("term-tile%02d" % i, taps((177, 12)) + ["wait 400"] + taps(TX(i)) + ["wait 200"] + taps(TX(i)), CROWN))
cases.append(("term-cmd-defend", taps((177, 12)) + ["wait 400"] + type_cmd("defend"), CROWN))
for b, nm in ((12, "gibson"), (2, "bot02"), (6, "bot06")):
    open_ = taps((120, 114)) if b == 12 else taps(((10 if b < 6 else 192) + 14, 44 + (b % 6) * 23 + 10))
    cases.append(("scan-%s-mcu" % nm, open_ + ["wait 400"] + taps((120, 208)), CROWN))
    cases.append(("scan-%s-peri" % nm, open_ + ["wait 400"] + taps((171, 206)), CROWN))
    cases.append(("scan-%s-typed" % nm, open_ + ["wait 400"] + taps(ABC, KEYS['h'], KEYS['e'], (34, 291)), CROWN))
cases.append(("hub-crown-rain-tap", taps(CROWN) + ["wait 700"] + taps((120, 160)), CROWN))
res = []
for name, steps, crown in cases:
    sc = ["until 9000"] + steps + ["wait 700", "shot %s-0open" % name]
    for k in range(1, 4):
        sc += ["tap %d %d" % crown, "wait 700", "shot %s-%dback" % (name, k)]
    p = os.path.join(OUT, name + ".txt"); open(p, "w").write("\n".join(sc) + "\n")
    log = subprocess.run([SIM, "--headless", "--serial", "--script", p, "--out", OUT],
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True).stdout
    scr, at = "gibson", {}
    for ln in log.splitlines():
        m = re.match(r"\[int\] enter_screen (\S+)", ln)
        if m: scr = m.group(1)
        m = re.search(r"shot \S+/(%s-\w+)\.png" % re.escape(name), ln)
        if m: at[m.group(1)] = scr
    o = at.get(name + "-0open")
    back = next((k for k in range(1, 4) if at.get("%s-%dback" % (name, k)) == "gibson"), 0)
    res.append(dict(target=name, screen=o, crowns_to_hub=back, trail=[at.get("%s-%dback" % (name, k)) for k in range(1, 4)],
                    shot_open=os.path.join(OUT, name + "-0open.png"),
                    shot_back=os.path.join(OUT, "%s-%dback.png" % (name, back or 1))))
    print("%-24s -> %-12s back:%s trail=%s" % (name, o, back, res[-1]["trail"]), flush=True)
json.dump(res, open(os.path.join(OUT, "audit.json"), "w"), indent=1)
