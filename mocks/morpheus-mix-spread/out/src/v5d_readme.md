## What changed in v5d (Mobandy 2:34 - 2:38 PM ET)

1. **Shorter near towers (2:34 PM ET).** Each tower's world height is its group height times a fixed depth factor:
   - `hk(Z) = 0.6 + (1.2 - 0.6) * smoothstep((Z - 1.6) / 3.2)`, from Gibson depth Z (`src/v5d_heights.py`). It is fixed per tower, so nothing breathes.
   - Group order and ratios 1.00 / 0.80 / 0.62 hold at every depth.
   - **Far factor is 1.2, not 1.0.** With 1.0 the far white towers could not keep >= 6 folder rows while the near towers also stayed >= 45%% of the tallest on-screen tower. So the far towers are 20%% taller than v5c, and the near ones are about 40%% shorter.
   - HACK, FLIPPER and SKIMMER are back at their true Gibson depths (1.71 / 1.94 / 2.17); their tops now fit.
   - The front floor still can't take a base. The nearest Gibson depth (Z 1.6) puts a base at about y 205, so the band below is unreachable without leaving Gibson depth.
   - Columns were re-picked for the new heights (`src/opt18/opt18e.py`, depth fixed).
   - No stumps: the shortest tower (%(stump_name)s) is %(stump_min)s%% of the tallest on-screen tower, median over 320 samples.
   - Every tower shows its emblem plus at least %(rows_min)s on-tower folder rows.

   | tower (near -> far) | Z | hk | H | median on-screen px | idle folder rows |
   |---|---|---|---|---|---|
%(hk_table)s

2. **Black Matrix towers (2:35 PM ET).** The 5 HIGH towers (%(black)s) use the existing T1 code-glass branch of `mo11_tower_fx`, the same code path v4 used for the green group.
   - The falling Matrix code is unchanged. Only the glass base is darkened toward black: front `mo3_mx(0x03120a, 0x000201)`, side `0x05160d`, highlight `0x03120a -> 0x3a8a58`.
   - The other 13 stay see-through wood (T3).
   - Emblems use each device's own colour; folders are opaque.

3. **Open sequence (2:36 - 2:38 PM ET).** A tap still pushes the camera in, as in v5c. Then, in order:
   1. **Gibson lock (0 - 1.2 s).** This is the hold / lock bracket branch of `lv_radar.h lv_rd_fx`, line for line, calling the real `lv_rd_box`, `lv_rd_brackets`, `lv_rd_rect_blend`, `lv_dim` and `b_text35`:
      - Brackets close in from pad 15 to pad 3 and brighten over 1 s.
      - Then magenta brackets pulse at 1.6 Hz, with a tick and a LOCK label.
      - The white snap flash is gated by the same `LV_RD_CHARGE` condition as Gibson, so it is off with the shipped charge lock.
      - The hold is driven by the time since the tap, because Morpheus has no HOLD TO LOCK bar.
      - The shipped Gibson locks by charging a HOLD TO LOCK bar, and the locked tower lights in its own colour (`lv_rd_cg_lit`). That part is not used here; `pullup_gibson_vs_morpheus_2x.png` shows both.
   2. **HACKbot search (1.2 s - about 2.6 s).** This uses `lv_scanbot_download.h lv_dl_drop`'s key bar through the real `lvi_rect` / `lvi_box` / `lvi_line` / `lvi_txt` helpers:
      - Dark fill, with the frame blinking amber / cyan and a cyan underline.
      - Amber 3x5 text `[SEARCH // DEAUTH_ // n/11]` types the device name.
      - Then lv_dl_drop's 11-band tower meter (`lvi_band` inks, rows 238 - k*18) lights band by band up the opened tower.
   3. **Folders.** These use lv_dl_drop's folder chrome via the `lvi_*` helpers: tab, neon frame, scanlines, drop shadow, the cubic ease-out drop and the stagger, and the cyan rail at x 84 / 86. Each folder adds, inside that chrome and all from folders18:
      - an `lvi_icon` detection icon and the file name;
      - a HIGH / MED / LOW risk chip in the threat colour;
      - the device tag, the EX.DATA dBm and an 11-step signal meter from the device's level;
      - the action line;
      - a HACKbot-style load bar with `n%%` -> `GOT`;
      - a `T+n.nS` timestamp since the tap.
   - The idle on-tower folders are unchanged.
   - Gate, from the sim's own state: %(sr)s.
