# LOCKED/harness — CYD sim harness (rebuilt 2026-10-29)

- `poke/`: cyd-sim-v3 sim (sim_main, sim_rt, sim_ui_poke, sim_parts.inc, shim/) for target `cydsim_poke`.
  It compiles the firmware sources without modification (BOTCORPS_CYD=1, SIM_POKE=1) against LovyanGFX 1.2.34 Panel_sdl.
- `gp/`: the same harness plus the sim-only GIBSON v8 mock painter (`sim/gib_v8.inc`, SIM_GP=1), target `cydsim_gp`.
- Build: `ln -s <LovyanGFX 1.2.34> LovyanGFX; ln -s <repo root> src; cmake -S . -B build && make -C build cydsim_poke`.
- Run headless: `./build/cydsim_poke --headless --script ../../scripts/home.script --out OUT`.
  The virtual clock makes every run byte-repeatable.
- Python: /workspace/cydpaint-venv (Pillow 13.0.0, numpy 2.5.4).
- `tools/lockcheck.sh` (repo root) builds both harnesses from a worktree and pixel-diffs every row of `LOCKED/locks.tsv`.
  It uses `LOCKED/pxdiff.py`, and any non-zero diff is a FAIL.

Toolchain on the box (2026-10-29): g++ 14, cmake 3.31.6, libsdl2-dev 2.32.4, zlib1g-dev. These were installed from locally downloaded Debian trixie .debs.
