#!/usr/bin/env python3
"""Report whether k_fit4 and k_hub_fade match commit c53579e1 byte for byte."""
import re
import subprocess
import sys

REV = "c53579e1b8ea30f7743dfb0a0505c47d4bc400d7"
PATHS = {
    "k_fit4": "src/ui/botcorps_pal_fit.inc",
    "k_hub_fade": "src/ui/botcorps_pal_hub.inc",
}

def grab(src, name):
    m = re.search(name + r"\[[0-9]+\] = \{(.*?)\};", src, re.S)
    if not m:
        raise SystemExit("missing " + name)
    return re.findall(r"0x[0-9A-Fa-f]+|\d+", m.group(1))

def show(rev, path):
    return subprocess.check_output(["git", "show", f"{rev}:{path}"], text=True)

def main():
    bad = 0
    for name, path in PATHS.items():
        old = grab(show(REV, path), name)
        new = grab(open(path).read(), name)
        n = sum(a != b for a, b in zip(old, new)) + abs(len(old) - len(new))
        print(f"{name}: {len(new)} entries, diff vs {REV[:12]} = {n}")
        if n:
            bad += 1
    # whole-file check for the frozen tables
    for path in PATHS.values():
        old = show(REV, path)
        new = open(path).read()
        print(f"{path}: file bytes {'identical' if old == new else 'DIFFER'}")
        if old != new:
            bad += 1
    return bad

if __name__ == "__main__":
    sys.exit(main())
