#!/usr/bin/env python3
"""Static-RAM gate. Measures .dram0.data + .dram0.bss on each firmware.elf and
compares it to release/gate/ram-caps.txt. PASS at or under the cap, FAIL above.
Exit code = number of failing rows (0 = every measured image is inside its cap).

An elf older than src/ or platformio.ini is FAIL: a stale image must not pass.
Rebuild with `pio run -e <id>` (the livescan id is the dual image plus
-DBOTCORPS_LIVE_SCAN=1; it is a measurement env, not a flashed product).
"""
import os
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
CAPS = os.path.join(ROOT, "release", "gate", "ram-caps.txt")
SRC_EXT = {".c", ".cc", ".cpp", ".h", ".hpp", ".inc", ".S", ".s"}


def size_tool():
    cand = os.path.expanduser(
        "~/.platformio/packages/toolchain-xtensa-esp-elf/bin/xtensa-esp32s3-elf-size")
    if os.path.isfile(cand) and os.access(cand, os.X_OK):
        return cand
    return "xtensa-esp32s3-elf-size"


def load_caps():
    rows = []
    for ln in open(CAPS):
        ln = ln.strip()
        if not ln or ln.startswith("#"):
            continue
        ident, cap, label = ln.split(None, 2)
        rows.append((ident, int(cap), label))
    return rows


def src_mtime():
    newest = os.path.getmtime(os.path.join(ROOT, "platformio.ini"))
    src = os.path.join(ROOT, "src")
    for dirpath, dirnames, filenames in os.walk(src):
        dirnames[:] = [d for d in dirnames if d != ".pio"]
        for fn in filenames:
            if os.path.splitext(fn)[1] in SRC_EXT:
                newest = max(newest, os.path.getmtime(os.path.join(dirpath, fn)))
    return newest


def dram_bytes(elf):
    out = subprocess.check_output([size_tool(), "-A", elf], text=True)
    data = bss = None
    for ln in out.splitlines():
        parts = ln.split()
        if len(parts) < 2:
            continue
        if parts[0] == ".dram0.data":
            data = int(parts[1])
        elif parts[0] == ".dram0.bss":
            bss = int(parts[1])
    if data is None or bss is None:
        raise RuntimeError("missing .dram0.data or .dram0.bss")
    return data + bss


def main():
    os.chdir(ROOT)
    fails = 0
    newest = src_mtime()
    for ident, cap, label in load_caps():
        elf = os.path.join(ROOT, ".pio", "build", ident, "firmware.elf")
        if not os.path.isfile(elf):
            print(f"RAM {label}: no firmware.elf / cap {cap} B FAIL")
            fails += 1
            continue
        try:
            used = dram_bytes(elf)
        except (OSError, subprocess.CalledProcessError, RuntimeError) as exc:
            print(f"RAM {label}: measure failed ({exc}) / cap {cap} B FAIL")
            fails += 1
            continue
        stale = os.path.getmtime(elf) + 1 < newest
        over = used > cap
        if over:
            rel = f"{used} B > cap {cap} B"
        else:
            rel = f"{used} B <= cap {cap} B"
        if stale:
            print(f"RAM {label}: {rel} elf older than sources FAIL")
            fails += 1
        elif over:
            print(f"RAM {label}: {rel} FAIL")
            fails += 1
        else:
            print(f"RAM {label}: {rel} PASS")
    return min(fails, 255)


if __name__ == "__main__":
    sys.exit(main())
