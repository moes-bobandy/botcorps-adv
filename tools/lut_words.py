#!/usr/bin/env python3
"""Dump the shared code->RGB565 lookup and diff it against c53579e1.

The lookup is native_at(): reserved slots use k_exact_nat, every other
code uses the RGB332 pack. The sheet colours are not part of this table.
"""
import re
import subprocess
import sys

REV = "c53579e1b8ea30f7743dfb0a0505c47d4bc400d7"
PATH = "src/ui/botcorps_pal_tables.inc"


def pack332(i):
    r3, g3, b2 = (i >> 5) & 7, (i >> 2) & 7, i & 3
    r8 = (((r3 << 3) + r3) << 2) + (r3 >> 1)
    g8 = (((g3 << 3) + g3) << 2) + (g3 >> 1)
    b8 = b2 * 0x55
    return ((r8 >> 3) << 11) | ((g8 >> 2) << 5) | (b8 >> 3)


def tables(src):
    res = [int(x) for x in re.findall(r"\d+", re.search(r"k_is_res\[256\] = \{([^}]+)\}", src, re.S).group(1))]
    nat = [int(x, 16) for x in re.findall(r"0x[0-9A-Fa-f]+", re.search(r"k_exact_nat\[256\] = \{([^}]+)\}", src, re.S).group(1))]
    if len(res) != 256 or len(nat) != 256:
        raise SystemExit("table length")
    return [nat[i] if res[i] else pack332(i) for i in range(256)]


def show(rev):
    return subprocess.check_output(["git", "show", f"{rev}:{PATH}"], text=True)


def main():
    old = tables(show(REV))
    new = tables(open(PATH).read())
    n = sum(a != b for a, b in zip(old, new))
    print(f"shared lookup 256 words, diff vs {REV[:12]} = {n}")
    for i, (a, b) in enumerate(zip(old, new)):
        if a != b:
            print(f"  {i}: {a:04X} -> {b:04X}")
    return 1 if n else 0


if __name__ == "__main__":
    sys.exit(main())
