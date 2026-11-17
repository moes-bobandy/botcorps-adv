# BOTcorps CYD v1.2.3 — status (BUILD ONLY: not flashed, not uploaded, not merged)

Source on `backup/cyd-bigbot-pager-2026-11-17` at 178b4d5 (parent 5ac6426 / v1.2.2). Env `cyd_livescan`. Build id `cyd-livescan 1.0 v1019z 178b4d5`.
Change only: CONTROLbot touch/panel lifetime, and VOL → SLEEP. Look is unchanged outside the SLEEP slot and the live control states. BOTpheus Dojo internals were not touched.

- BOTcorps-CYD-v1.2.3.bin (app-only, 0x10000): 2,686,704 B, sha256 e699abcf4f3141d81bb1d245cb7ac5e4b2f528333ddbae8e437d2469b3034040
  Fits ota_0 2,686,976 B with 272 B headroom (v1.2.2 was 0 B). SLEEP plus the keep-panel fix first overran by 288 B; the bytes came back from compacting `lv_controlbot_tap` onto the existing `LV_CB_BLK` switch (no duplicate hit rects / lambda) and looping NVS get/put over `LV_CB_NVS[8]`.
- BOTcorps-CYD-v1.2.3-full.bin (merged 0x0): 2,752,240 B, sha256 144d71c19aa5af92fc64994648347bb7e840cafaa89c5e06967c89313790fb29 (= PlatformIO firmware.factory.bin)
- Static RAM 121,676 B (v1.2.2 121,668 B, +8 B for `sleep` / `lv_cb_slp` / `lv_cb_idle_t`). gate.sh 1042/1057 (15 FAIL, 4 N/A) = baseline.

## 1. Why the CONTROLbot controls were dead on the CYD

On pager HOME, a release goes `lv_ui_loop` → `lv_read_tap` → `lv_tap` → `lv_pg_tap`. If `lv_pg_page == LV_PG_CTL` (now index 4), that calls `lv_controlbot_tap` (`lv_pager.h:629`). `go ctl` remaps `LV_SC_CONTROLBOT` to HOME+page 4, so the standalone `lv_tap` branch at `lv_core.h` is unused. Hit rects already matched the drawn bars/buttons (`LV_CB_BLK` / `lv_cb_bar` / `lv_cb_btn`); `lv_pad_snap` was already applied; IRQ36 is already off (`lgfx_cyd.h` `cfg.pin_int = -1`, VSPI polled). Coil swipe only accepts y 30–69, so it does not swallow the panel at y 221–289. The routing and geometry were not the bug.

The bug was the live panel's lifetime. v1.2.1 stopped holding `lv_cb_pan` (21,584 B) forever, and `lv_pg_blit` malloc'd it, composed, then called `lv_pg_cb_release()` after every still frame. The next dirty frame had to malloc 21,584 B again. If that alloc fails, compose uses the locked still's default panel, so SCAN/VOL/BRIGHT/CHIME/TIME/BAR/TONE appear stuck even though `lv_cb_set` and NVS still change underneath.

That is a device-only failure mode:

- The host sim has unlimited heap, so the 21,584 B alloc always succeeds and a direct `tap X Y` (screen coords, no XPT2046 / c0–c7 cal) always updates the pixels.
- The v1.2.1 QEMU "tour cb tap" (`docs/builds/fw-bigbot-1117/v1.2.1/proof/qemu_tour_patch.py` step 10) only set `lv_cb_panel_dirty = true` and logged heap (`free=50,788`, `largest=36,852`). It never injected a touch at a control coordinate. QEMU Wi-Fi init also fails with `ESP_ERR_NO_MEM` on these builds, so QEMU is richer in heap than a CYD where the radio stack actually comes up.

Fix (`lv_pager.h:237` and `:530`): keep `lv_cb_pan` allocated while the still page is CONTROLbot; still free it when leaving that page, ending a slide away from it, or leaving HOME (`lv_pg_leave_cache`). The panel is rebuilt only when settings or the theme change.

## 2. VOL → SLEEP

Same slot as VOL: draw `lv_cb_bar(46, 244, ...)` (`lv_controlbot_panel.h:117`), tap block 1 (`lv_controlbot_screen.h:113`), 46,244–113,265, same size and style. Only the label/value text in that slot changes. Default is OFF. Tap cycles OFF / 30S / 1M / 5M. Value is `cbsleep` in NVS namespace `botcorps` (with the other panel keys). Speaker volume stays at the saved `cbvol` (default 40%), with no UI.

When the selected timeout elapses with no touch (`lv_cb_idle_tick` from `lv_tick`, `lv_core.h:2271`), backlight duty goes to 0 via the existing `setBrightness` hook (GPIO21). Any finger-down wakes and restores BRIGHT (`lv_cb_finger` in `lv_touch`). The first wake-tap is eaten (`lv_cb_eat` at `lv_core.h:2479` returns `sleep-wake`) and does not step a control, start a coil swipe, or cycle the theme.

## 3. Per-control tests (host sim_live, page 4, drawn coordinates)

Script and log: `proof/ctl_sim.txt`, `proof/ctl_sim.log`. `tap` calls `lv_tap` at screen coords. `rhold` also feeds `lv_touch` (press/release), like `lv_ui_loop`.

| Control | Coord | Result | Visible |
| --- | --- | --- | --- |
| SCAN | 80,231 | `cb-scan` B.RAIN → S.RAIN, then rhold → OFF | panel value + meter |
| SLEEP | 80,254 | `cb-sleep` OFF → 30S → 1M → 5M → OFF, NVS `cbsleep` | slot label SLEEP, values OFF/30S/1M/5M |
| BRIGHT | 60,277 then 100,277 | `cb-bright-` 80% → 70%, `cb-bright+` → 80%; `BACKLIGHT` 80 → 70 → 80 | meter + percent |
| CHIME | 135,237 | `cb-chime` ON → OFF | badge OFF |
| TIME | 176,237 | `cb-time` RTC → NTP | badge NTP |
| BAR | 135,272 | `cb-bar` HAZRD → HUD | badge HUD |
| TONE | 176,272 | `cb-tone` BEEP → 8BIT, speaker hook fired | badge 8BIT |
| Face / theme | 120,145 | `controlbot-theme` HOLO FOIL | panel re-inks |

SLEEP timeout and wake-eat: arm 30S at t=11280; `BACKLIGHT 0%` at t=41280 (exactly 30,000 ms). `rhold 135,237` (CHIME) returns `sleep-wake` and turns the backlight back to 80%; the CHIME value does not change. A second `rhold` at the same point returns `cb-chime` and toggles it. A later 1M arm plus 30 s of idle does not blank the screen, and the next face tap is a real theme cycle, not a wake-eat.

Pixels vs v1.2.2, same boot script, t=8010, default theme, page 4: 181 px differ, all inside the old VOL slot (46,244)–(113,265). The rest of the panel, and the rest of the frame including the ticker, is 0 px (proof/pixel_diff.txt). Renders: `v1.2.3_controlbot_sleep_off@3x.png`, `v1.2.3_panel_sleep_{off,30s,1m,5m}@8x.png`, `v1.2.3_panel_sleep_cycle@4x.png`, `v1.2.3_per_control_pairs@3x.png`.

## Not proven on hardware

The keep-panel fix and the SLEEP timeout/wake path were proven in the host sim, not on the CYD or in QEMU this build. Heap on the device can still differ after Wi-Fi/NimBLE come up. This bin was not installed or flashed.
