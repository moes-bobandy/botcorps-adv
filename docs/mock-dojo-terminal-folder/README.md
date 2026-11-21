# Dojo terminal folder mock (sim-only, not yet tested on your CYD)

Branch `mock/dojo-terminal-folder-2026-11-21` off `514df65` (`mock/dojo-10-pillars-2026-11-21`).
Mock only: no bin, upload, flash, or merge. The scan-tower idea was not touched.

Mobandy 2026-11-21 (America/New_York): a Dojo pillar folder that is "our terminal", like TERMINALbot's individual scan. Mock two variants: V4 = a new 4th folder, V3 = the terminal replacing the 3rd folder.

## Variants (sim `mofold` command)

| `mofold` | What it is |
|---|---|
| 0 | Current 10-pillar Dojo, three info scrolls. Default. Pixel-identical to 514df65 (landed EVIL TWIN 0 px; full gate below). |
| 3 | **V3:** scroll 3 becomes the terminal folder. Locked folder frame, tab (`SCROLL 3`), emblem, drop and press flash are the same code as the other scrolls. Content uses the same name / fact / badge widgets: `TERMINAL SWEEP` / `INDIVIDUAL SCAN` / `SWEEP` + radio badge + `EX.DATA` or `LIVE`. Scrolls 1 and 2, the pillar, search bar, panel and footer are unchanged (896 px, only the scroll 3 body, bbox x 98..215, y 199..234). |
| 4 | **V4 packed (least change that actually fits):** a 4th terminal folder. A 4th full-size folder does **not** fit in 240x320 without moving the others (see fit). |
| 5 | **V4 unfitted proof only:** 4 folders at the locked size (FY0=60, FDY=62, FH=50). The 4th tab would be at y=246 and the body (254..304) sits under the open-pillar panel (y=247..285) and the footer terminal (y=287..319). Search bar / footer say `4 SCROLLS`, but the 4th folder is hidden. Not the proposed V4.

## Fit: why a 4th folder cannot keep the locked size

Available band for folders: first tab at y=60, panel top at y=247 (187 px). Each locked folder occupies 58 px from tab top to body bottom (FH=50, body 8 below the tab top), plus a 4 px gap (FDY=62). Four locked folders need 4*58 + 3*4 = 244 px. 244 > 187, short by 57 px. Stealing the 8 px under the search bar is not enough. There is only ~5 px between scroll 3's body (ends y=242) and the panel.

Least change that keeps all four folders above the panel, with the same chrome and touch boxes still >= 47 px tall: pack every folder to FH=35, FDY=47, FY0=60. Last body ends at y=244 (2 px above the panel). The search bar, close-up pillar, panel and footer stay put. This **does** move and shrink scrolls 1-3, and the locked instrument rows (fact + 7-seg / badges) collide inside the shorter body. That is shown honestly in the V4 stills; it is not a restyle, it is the locked content forced into 35 px.

V3 needs no layout change and is the clean variant.

## What a tap does (real scan path)

Tapping the terminal folder (after the usual folder press flash) does the same thing as tapping that bot's chip on TERMINALbot:

1. `lv_press` on the folder rect (same flash as the other scrolls)
2. `lv_go_soon(0, "sweep <hunt>")` — the same call as `lv_core.h` TERMINALbot `HOT[]` chips (~100 ms later `lv_open_tty`)
3. `lv_cmd("sweep eviltwin")` → `lv_job_start(LV_J_WIFI, LV_H_EVILTWIN)`
4. The locked TTY individual-scan UI: `lv_scene_tty` + `lv_tty_banner` (bot name, `HUNTS …`, `SCANNING` bar or `N FOUND / OF M SEEN`) + the real OSK

BACK from that TTY returns to the Dojo (`lv_tty_from` is set to `LV_SC_MO`). The footer bar still opens full TERMINALbot (`lv_term_from_mo`), unchanged.

### EVIL TWIN has no detector in the 10 TERMINALbot chips

`MO_HUNT[EVIL TWIN]` is still -1 (it took KEYFOB's slot; KEYFOB remains a TERM chip). The mock does **not** change `LV_NHUNT` (still 10) or the TERM chip layout / tints, so TERMINALbot stays pixel-identical.

It adds `LV_H_EVILTWIN = 10` and sizes the hunt tables to `LV_NHUNT_ALL = 11`. `sweep eviltwin` (aliases `twin`, `evil`) is the only new entry point. `lv_hunt_match` treats an evil twin as two Wi-Fi finds with the same SSID and different lock types. In this sim, if that pair is not already in the live list, `fk_wifi_poll` appends an EX.DATA `CafeWiFi` open copy next to a WPA2 copy, only while this hunt is running. The TTY log prints `note: EX.DATA if no live same-SSID lock mismatch.` For the other nine pillars, the folder runs the existing `sweep <LV_HUNT_NAME[MO_HUNT[i]]>` job unchanged.

Reuse sources:

- Folder chrome / drop / hit / flash: `mo12_folder` in `src/live/botpheus/mo_mix.inc`; `lv_sel_drop` / `lv_sel_landed` / `lv_sel_hit` in `lv_select.h`; `mo_fold_hit` / `itw_mo_tap` in `src/live/lv_morpheus_v13.h`
- Individual scan: `lv_go_soon` → `lv_open_tty` → `lv_cmd("sweep …")` → `lv_job_start` / `lv_hunt_match` / `lv_hunt_report`; draw `lv_scene_tty` / `lv_tty_banner` in `src/live/lv_core.h`
- Not used: `lv_scanbot_download.h` (HACKbot DOWNLOAD game folders, a different screen)

V3 and V4 opened/scanning stills are the same locked TTY UI. They differ only by scan-bar phase (the V4 open takes one more folder-drop gap, so the sweep starts at a later `lv_now`). The path is the same.

## Stills (@3x nearest-neighbour) and GIF

Outputs: `/workspace/out/fw-bigbot-1117/dojo-terminal-folder/`

- `eviltwin_landed_current_vs_v3_vs_v4_3x.png` — EVIL TWIN open, folders landed, current | V3 | V4 packed
- `eviltwin_v4_fit_proof_3x.png` — current | V4 packed | V4 unfitted (4th folder hidden under the panel)
- `landed_current_3x.png`, `landed_v3_3x.png`, `landed_v4_3x.png`, `landed_v4unfit_3x.png`
- `flash_v3_3x.png`, `flash_v4_3x.png` — press flash on the terminal folder, still on the Dojo
- `scan_v3_3x.png`, `scan_v4_3x.png` — TTY individual scan, `SCANNING`
- `eviltwin_term_scan_v3_vs_v4_3x.png` — those two side by side
- `scan_v3_done_3x.png` — same path after the wifi pass: `2 FOUND OF 22 SEEN`, CafeWiFi OPEN + WPA2, EX.DATA note
- `dojo_terminal_folder_v3.gif` — short V3 story: folders finish landing, tap scroll 3, TTY scanning, hunt report. @2x, under 10 MB.

## Gate

Default `mofold 0`: non-Dojo screens 0 px vs 514df65; Dojo also 0 px at this default (EVIL TWIN landed still 0 px vs 514df65). V3/V4 are opt-in via `mofold`. See `gate_px.txt`.

Host sim only. No bin was built.
