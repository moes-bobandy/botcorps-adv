# MOCK 2026-11-27 (CYD-23): G2 full-screen typist variations E and F

Mock only, sim-only, not in any bin. No build, no flash, no SD, no merge.
Branch `mock/cyd23-g2-botcorps-motd-2026-11-27` from `016c25b` (`mock/cyd22-g2-longpress-2026-11-26`).
Flag `LV_CYD22_G2` stays default 0. Long-press exit, CH=0, LIVE empty-state, and the tightened BOT letter gap are unchanged.
Var A (spreadsheet scan chart) from 016c25b is still available via `SIM_C22_VAR=A`.

These two stills are whole-screen typist designs, not just a different scan block. Pure terminal text, live theme accent, on-image `VAR E` / `VAR F` labels.

- **Var E** — dmesg / boot log: early `0.00` lines, BOTcorps wordmark banner, radio counts (including deauth 0), then a timestamped find feed that settles at `Gibson>`.
- **Var F** — tmux-style workspace: window bar (`0:motd  1:scan  2:devs*  LIVE`), logo in the motd pane, a `-- scan --` pane, a `-- devices --` pane with the name/class/rssi/ch chart, then `Gibson>`.

Reproduce with `SIM_C22_VAR=E|F` on the LIVE host sim (Vapor Grid, theme 22).
Renders: `/workspace/out/fw-bigbot-1117/cyd23-g2-botcorps-motd/`.
