# CONTROLbot live panel on theme change (sim-only)

Mobandy 2:55 PM ET 2026-11-25: "control bot panel keeps reverting to the old sound one"
on BOTcorps-CYD-v1.2.21 (d7b5545).

## Root cause

The approved v1.2.12 panel (RADAR / SLEEP / BRIGHT / RECAL TOUCH / ROTATE / BAR / EXIT)
is drawn live into `lv_cb_pan` and overlaid on the locked v6 stills. Those stills still
carry the old sound panel (SCAN B.RAIN / VOL 40% / CHIME ON / TIME RTC / TONE BEEP).

`lv_pg_blit` (`src/live/pager/lv_pager.h`) rebuilt the live panel only when `lv_cb_pan`
was null or `lv_cb_panel_dirty`. A CONTROLbot face tap calls `lv_set_theme_any`, which
changes `lv_cb_slot()` but does not mark the panel dirty. `lv_pg_compose_row` then
skips the overlay because `lv_cb_pan_slot != slot`, and the baked sound panel shows
through. The standalone `lv_cb_frame` path already rebuilt on slot mismatch; the pager
(HOME) did not.

Confirmed on the sim: first open shows the v1.2.12 panel; one face tap (theme 0 GRAPE
→ slot HOLO FOIL, lktheme=24) reverts the panel to SCAN/VOL/CHIME/TIME/TONE.

Commits:
- newer approved panel: `fb46c13` (v1.2.12; SLEEP itself landed in `178b4d5` v1.2.3)
- old sound panel stills + live v6: `c1b5659` (fw-3locks, 2026-11-16)

NVS keys (`cbscan` / `cbvol` / … / `lktheme`) do not store a panel layout. Sleep /
idle timeout do not themselves restore the old labels. The unrouted `LV_SC_CTL` screen
is still in `lv_core.h` and is not this bug (`go ctl` already remaps to the pager page
when `LV_CB` is on). Locked stills were left in place; only the missed rebuild route
was closed.

A failed `malloc` of the 21,584 B live panel can still reveal the baked still (known
since `178b4d5`). This fix does not change that path.

## Fix

In `lv_pg_blit`, rebuild when `need_cb && (!lv_cb_pan || lv_cb_panel_dirty || lv_cb_pan_slot != lv_cb_slot())`,
matching `lv_cb_frame`. No stills deleted, `LV_SC_CTL` code not deleted.

## Proof (HOST SIM ONLY)

- `ctl_open@3x.png` — first open, v1.2.12 panel (same 0 px with or without the fix)
- `ctl_before_trigger@3x.png` — after face tap on unfixed v1.2.21: old sound panel
- `ctl_after_fix@3x.png` — same tap after the fix: v1.2.12 panel, HOLO FOIL ink
- `gate_px.txt` — tour 0 px, trigger diff only inside the panel rect

No firmware bin, not flashed, not uploaded.
