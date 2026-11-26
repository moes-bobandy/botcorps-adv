# CYD Theme Refresh Mocks (2026-11-25)

**MOCK ONLY** — no bin, no build, no SD upload, no flash, no merge.
Base commit: `d7b55459` (v1.2.21).

## Replaced Theme Targets (kept untouched in code for 0-px default gate)
1. **The radar theme:** `SONAR` (`lv_theme = 23`, `LV_CB_FW` slot 15, `ext_theme_at(13)`)
2. **The regular purple theme:** `NEON GRAPE` (`lv_theme = 0`, `LV_CB_FW` slot 11) — **Default-theme flag:** `lv_theme = 0` is currently the NVS/boot default (`src/live/lv_ui.cpp:725,729` `lktheme` default `0`, `src/live/lv_core.h:271,2655`). Kept untouched in this mock branch so `v1220_tour` passes 0 px on default theme; when removing `GRAPE` for firmware, recommend switching default `lktheme` to `ICE` (`lv_theme = 3`, native `T3` clip) or `MATRIX RAIN` (`lv_theme = 26`, via `LV_THEME_BASE = 3` re-ink).
3. **The non-camo green grid theme:** `CIRCUIT` (`lv_theme = 22`, `LV_CB_FW` slot 0, `ext_theme_at(12)`)

## 3 Primary Replacement THEMED UIs + 3 Bonus Candidates
| Slot | Index | Short | Full Name | Type | ACC | ACCD | HDR | FRAME | Replaces / Role |
|---|---|---|---|---|---|---|---|---|---|
| 24 | 26 | `MATRX` | `MATRIX RAIN` | Multi-color | `#22E868` | `#0A5228` | `#9EFFC0` | `#22E868` | Primary #1 (replaces `CIRCUIT`) |
| 25 | 27 | `STEEL` | `CHROME STEEL` | Multi-color | `#A8C4E0` | `#3C4B5C` | `#E8F4FF` | `#A8C4E0` | Primary #2 (replaces `NEON GRAPE`) |
| 26 | 28 | `CARBN` | `CRIMSON CARBON` | Multi-color | `#FF244C` | `#5E0A1A` | `#FF889E` | `#FF244C` | Primary #3 (replaces `SONAR`) |
| 27 | 29 | `AMBER` | `AMBER CRT` | Multi-color | `#FFB010` | `#683C00` | `#FFE082` | `#FFB010` | Bonus #1 (film/CRT phosphor) |
| 28 | 30 | `TEAL` | `CYBER TEAL` | Single-color | `#00E0A8` | `#005C44` | `#78FFD8` | `#00E0A8` | Bonus #2 (solid emerald-teal) |
| 29 | 31 | `VAPOR` | `VAPOR GRID` | Multi-color | `#00E8FF` | `#3B1866` | `#FF78E0` | `#00E8FF` | Bonus #3 (outrun horizon grid) |

## Gate Result
`v1220_tour` 11-screen gate vs `/tmp/v1220/gate121s`: **0 px diff across all 11 screens (PASS)**.
