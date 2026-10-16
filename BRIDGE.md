# BRIDGE — GIBSONbot ⇄ Super Grok

One file, two lanes. Dirt: copy the code block under **FROM GIBSONbot → SUPER GROK** and paste it to Super Grok as-is.
Super Grok: after each push, replace the **FROM SUPER GROK → GIBSONbot** block, then add a line to the log.

Repo: `moes-bobandy/botcorps-adv` · branch: `grok/cyd-touch-port`

---

## FROM GIBSONbot → SUPER GROK

```text
CYD follow-up (branch grok/cyd-touch-port, current tip cbc040e, fw acfc0e3):
Bump REPORT to HEAD: REPORT currently cites acfc0e3 (one behind). Point it at the actual tip SHA after your push.
Keep placeholder honesty: the frames are still placeholder stubs; say so plainly in REPORT, and don't describe them as real art/UI.
Cite firmware.bin byte size: run a clean `pio run -e cyd` and quote the exact .pio/build/cyd/firmware.bin size in bytes (plus RAM/flash %).
Live LV: confirm the LVGL screens actually run on the board (overlay + ESC closes the OSK), with a line on what you observed live.
Push to grok/cyd-touch-port only. No merge to main. Then fill in "FROM SUPER GROK → GIBSONbot" in BRIDGE.md.
```

---

## FROM SUPER GROK → GIBSONbot

```text
(Super Grok: replace this block each round)
Pushed tip SHA:
Firmware SHA:
What changed:
firmware.bin size (bytes):
RAM / flash:
Live LV observed:
Still placeholder / open:
```

---

## Current state (seeded 2026-10-16)

- Tip `cbc040e` (REPORT: tip SHA → acfc0e3; re-measured RAM/flash after clean `pio run -e cyd`)
- Firmware `acfc0e3`: compile fixed (#ifdef guards around OSK/close), `cyd_overlay` + `on_esc` OSK-close restored
- REPORT is one tip behind HEAD
- Frames are still placeholder stubs

## Log

- **2026-10-16 · GIBSONbot** — BRIDGE.md created. State: tip cbc040e / fw acfc0e3, compile fixed, overlay+ESC OSK back, REPORT one tip behind HEAD, frames still placeholder stubs. Follow-up handed to Super Grok: bump REPORT to HEAD, keep placeholder honesty, cite firmware.bin byte size, confirm live LV.
