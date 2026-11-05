# radar-hunt-meter — MOCK ONLY (three designs)

Owner (2026-11-05 ~9:49 AM ET): *"The hunting meter needs to be changed to where the bars go from green to yellow to red up the tower itself that you are hunting."*

## Scope
- **Mock stills + short clips only.** No firmware port, no flash, no lock.
- Charge-lock current **D4 stays as-is** (ice wire on the hunting still is untouched).
- Tap-to-cycle and HOLD TO LOCK stay locked / unchanged.
- Radar itself is not redesigned beyond replacing the **top hunting signal meter** with a meter that **climbs the hunted tower**.

## What changed vs current hunting UI
Today (fw-port-1104 / `lv_hunt_bar`): while hunting, the top banner holds an 11-bar rising meter (red→yellow→green left-to-right by bar index) plus HUNT / name / dBm / STEADY.

These mocks:
1. **Clear the top 11-bar meter** (HUNT + TELLO name + dBm + STEADY stay). A tiny decorative tower glyph marks that the live meter moved onto the target.
2. **Draw the signal fill up the hunted tower** (TELLO, HUNT brackets). Color by height of the fill: **green at the base → yellow mid → red at the tip**. Weak signal = short green stub; mid = green+yellow; full = green+yellow+red tip.

Base still: `src/base_hunting@3x.png` (= `/workspace/out/fw-port-1104/fw_D4_hunting@3x.png`). D4 current + orange WIFI / blue BLE of the ported look. ## Designs

| | look | how the fill reads |
|---|---|---|
| **A** | **Thermometer bands** — horizontal bar segments stacked inside the tower glass | clearest "fuel gauge up the building"; bands themselves are the bars |
| **B** | **Equalizer columns** — 6 vertical rising bars across the tower face (stair heights) | the old top meter relocated onto the tower silhouette |
| **C** | **Edge rails + mercury** — L/R tick marks climb the tower edges; 5 px center fill | keeps most of the glass clear so the D4 current behind the tower stays readable |

Fills rendered per design: `idle` (22%), `mid` (55%, ~the -66 dBm hunting still), `full` (100%).

## Files
```
A/ B/ C/
  {A,B,C}_{idle,mid,full}@{1x,3x}.png
  hunt_meter_{A,B,C}_3x.mp4          # 2.0 s climb 5%→100%, 60f @30fps
compare/current_vs_{A,B,C}_mid@3x.png
sheets/sheet_ABC_mid@3x.png
sheets/ladder_{A,B,C}@3x.png         # idle | mid | full
src/render_hunt_meter.py
src/base_hunting@3x.png
README.md
SHA256SUMS
```

MP4: H.264, yuv420p, +faststart, 720×960, 30/1, 60 frames, 2.000 s.

## Not in this drop
- No change under `/workspace/cyd-lock-store` (D4 charge lock untouched).
- No edit to `src/live/lv_radar.h` / firmware.
- No flash. No new LOCKED entry.

## How to re-render
```
python3 src/render_hunt_meter.py
```
