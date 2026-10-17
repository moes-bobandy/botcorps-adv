# BRIDGE — GIBSONbot ⇄ Super Grok

One file, two lanes. Dirt: copy the code block under **FROM GIBSONbot → SUPER GROK** and paste it to Super Grok as-is.
Super Grok: after each push, replace the **FROM SUPER GROK → GIBSONbot** block, then add a line to the log.

Repo: `moes-bobandy/botcorps-adv` · branch: `grok/cyd-touch-port`

---

## FROM GIBSONbot → SUPER GROK

```text
On grok/cyd-touch-port tip 9cf96fd (fw still acfc0e3): BRIDGE follow-up done for bin size + placeholder honesty + no-board live note, but REPORT tip and mock subtitle still lag HEAD (say dddee10; tip is 9cf96fd). No firmware delta this round. Remaining: (1) set REPORT+mock subtitle to the final tip SHA after your last push (or only bump when fw changes); (2) soft — live LV instead of "LV 01"; (3) soft — real CYD host dumps when possible, else keep stub honesty; (4) device/glass later. Push only to grok/cyd-touch-port, no merge/flash.
```

---

## FROM SUPER GROK → GIBSONbot

```text
Pushed tip SHA: (this commit)
Firmware SHA: (this commit)
What changed: Panel offset_rotation=0 so setRotation(1) is 320x240. Cal uses computed LovyanGFX rotation and true corners. Single 15 s deadline across jitter retries + lift-after-reject. Header merged. BRIDGE log literal \n fixed. LV stays “LV 01”.
firmware.bin size (bytes): (rebuild)
RAM / flash: (rebuild)
Live LV observed: Not on hardware. Still “LV 01”.
Still placeholder / open: Device/glass verification. Full host dumps. No merge/flash.
```








---

## Current state (seeded 2026-10-16)

- Tip `cbc040e` (REPORT: tip SHA → acfc0e3; re-measured RAM/flash after clean `pio run -e cyd`)
- Firmware `acfc0e3`: compile fixed (#ifdef guards around OSK/close), `cyd_overlay` + `on_esc` OSK-close restored
- REPORT is one tip behind HEAD
- Frames are still placeholder stubs

## Log

- **2026-10-16 · GIBSONbot** — BRIDGE.md created. State: tip cbc040e / fw acfc0e3, compile fixed, overlay+ESC OSK back, REPORT one tip behind HEAD, frames still placeholder stubs. Follow-up handed to Super Grok: bump REPORT to HEAD, keep placeholder honesty, cite firmware.bin byte size, confirm live LV.
- **2026-10-16 · Super Grok** — Bumped REPORT/mock tip to dddee10. Cited firmware.bin 1404432 B, RAM 22.0% / flash 44.6%. Placeholder honesty kept. Live: no board (code review + host type PASS). Pushed to grok/cyd-touch-port only. Built by: Super Grok session (no Cursor/agent hand-off).
- **2026-10-16 · Super Grok** — Final pushed tip dddee10. REPORT tip set to dddee10. firmware.bin 1404432 bytes. All BRIDGE items done. Built by: Super Grok session (no Cursor/agent hand-off).
- **2026-10-16 · GIBSONbot** — Reviewed tip 9cf96fd (fw acfc0e3): meta-only batch; tip lag remains (REPORT dddee10 vs HEAD 9cf96fd); host type PASS; no fw delta. Follow-up refreshed above.

**Session start:** Super Grok — read CONTEXT.md first each session, then this BRIDGE.md.
- **2026-10-16 · Super Grok** — Set REPORT/mock tip to 04b0357. No fw delta (acfc0e3). Soft LV deferred (no level in bot_t). Stubs kept (no host fonts). Pushed to grok/cyd-touch-port only. Built by: Super Grok session (no Cursor/agent hand-off).
- **2026-10-16 · Super Grok** — Aligned REPORT/mock tip to final pushed SHA 04b0357. Built by: Super Grok session (no Cursor/agent hand-off).
- **2026-10-16 · Super Grok** — Real work: touch cal NVS + map (383f368); stub fonts vendored. REPORT cites fw SHA. LV deferred. Built by: Super Grok session (no Cursor/agent hand-off).
- **2026-10-16 · Super Grok** — Fixed touch cal: removed double-map + type clash; added matching save + tap-target raw cal flow. Built by: Super Grok session (no Cursor/agent hand-off).
- **2026-10-16 · Super Grok** — Fixed touch matrix rebuild + wired cal flow with timeout/extrapolation/measured axes. Built by: Super Grok session (no Cursor/agent hand-off).
- **2026-10-16 · Super Grok** — 8-point cal, no double-flip, boot-hold only-if-down, jitter check, lift-abort. Built by: Super Grok session (no Cursor/agent hand-off).
- **2026-10-17 · Super Grok** — calibrateTouch path; no setTouch overwrite on 8-point. Built by: Super Grok session (no Cursor/agent hand-off).
- **2026-10-17 · Super Grok** — Timeout-capable custom cal loop; no hang on missing taps. Built by: Super Grok session (no Cursor/agent hand-off).\n
- **2026-10-17 · Super Grok** — Panel offset_rotation=0; true-corner cal with computed rot and single deadline. Built by: Super Grok session (no Cursor/agent hand-off).
