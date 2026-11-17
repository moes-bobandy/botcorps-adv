# BOTcorps CYD v1.2 (big-bot pager) — status, not flashed, not merged

Source: 3891839 (on top of 9e8a7a5 / v1.1). Branch `backup/cyd-bigbot-pager-2026-11-17`.
Env `cyd_livescan` (extends `env:cyd`). Build ID inside the app: `cyd-livescan 1.0 v1019z 3891839`.

- BOTcorps-CYD-v1.2.bin (app-only, 0x10000): 2,678,208 B, sha256 0c05266fdbc194cc98712fac53e0a7ecef6481489529d6d067ab2b49f5f9be1d
  Fits ota_0 2,686,976 B with 8,768 B headroom.
- BOTcorps-CYD-v1.2-full.bin (merged 0x0): 2,743,744 B, sha256 e62e8958c70ebbb2f09aed487b2bafbeda3d3a08f9720c98570a52e209ea356c
  Byte-identical to PlatformIO `.pio/build/cyd_livescan/firmware.factory.bin`.

Copies also live at `/workspace/out/fw-bigbot-1117/`.

## Done

- The CYD home screen is the five-page big-bot pager from the pager-v2 mock: BOTpheus, GIBSONbot, CONTROLbot (default, changeable), HACKbot, TERMINALbot.
- v3.1 faces in the locked THEME_FACE slot (65,88)–(174,197); unique role panels in the locked v6 panel slot; CONTROLbot keeps the live locked v6 controls.
- 5 pips in y=25–29. Ticker swipe, 233 ms ease-in-out cubic full-frame slide, no wrap with an 18 px edge bounce. Theme cycle on the CONTROLbot face recolors the whole pager.
- No BACKbot in the bottom-right corner. The 5V pad and right mounting hole are restored (LV_R_BACKBOT 221,301–239,319 patched from chrome).
- Center taps: BOTpheus -> Dojo EB3 (LV_SC_MO), GIBSONbot -> Gibson scan tower (LV_SC_RADAR), CONTROLbot -> theme cycle, HACKbot -> HACKbot game (LV_SC_DL), TERMINALbot -> TERM (LV_SC_TERM). `go ctl` / LV_SC_CONTROLBOT land on HOME page 2. Every sub-screen returns to the pager through the existing `lv_back()` path (BACKbot or BACK).
- v1.1 fixes kept (907c765 controls, ada2253 routes); both are ancestors of 9e8a7a5.

## Pixel proof (firmware sim vs pager-v2 stills)

Compared `sim_live` RGB dumps to the pager-v2 stills after the same RGB565 fill-expand the sim uses. All 10 grape/holo pages: face=0, panel=0, corner=0, body (y>=30)=0. Remaining diffs are the live ticker (same message, different scroll, y=0–23) and the live-painted pips (86 px grape / 88 px holo in y=25–29). See `proof/pixel_diff.json` and `proof/routes.log`.

## gate.sh

1042/1057 PASS, 15 FAIL, 4 N/A — identical to the v1.1 / c1b5659 baseline. The host renderer is the Cardputer/M5 path; the CYD pager is compiled out of it, so the existing 15 FAILs are the same hub / e7ba692 locks as v1.1. Log: `proof/gate.log`. RAM cardputer elves were not built (reported separately, same as a CYD-only tree).

## Not done (this task)

- Not flashed. Not uploaded to the CYD WebUI. Not merged to main. The 3locks backup branch is still at 9e8a7a5.
