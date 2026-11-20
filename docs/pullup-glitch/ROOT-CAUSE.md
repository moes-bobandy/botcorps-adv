# Pull-up glitch and motion jumps on the CYD (v1.2.16, code fd8efa8): root cause

SIM ONLY. No firmware bin was built, uploaded or flashed. Nothing here has been tested on the CYD yet.

## What Mobandy saw on the board
"the scan tower is a little better and it pulls up the tower but with glitch and still overall jumps when movement"

## Root cause (fd8efa8 line numbers)
1. **The 1-row INFO-tower fade costs about 13x a whole frame.** `src/live/lv_radar.h:1364-1371`: `lv_rd_info_tower_fade`
   redraws the whole INFO monolith once for **every one of its ~210 rows** (a 1-row `LvBand one`). Inside it,
   `lv_radar.h:1340-1341` draws 103 full-height column strokes with `lvi_line`, and `lvi_line` (`:1314-1317`) walks all
   ~199 Bresenham steps of each stroke even when the band is a single row. `lvi_micro` (`:1326`) also runs `snprintf` for
   all 22 plates on every row. That is about 4.3 M clipped pixel tests per frame instead of about 20 k.
   - Sim (host) CPU per pull frame: **6.3 ms vs 0.53 ms** once landed (`devrec` logs). On the device timeline model, that is
     **~800-1,350 ms per pull frame** (about 8 frames over 7 s).
   - The panel is drawn band by band with no full-frame buffer. On the CYD each pull frame therefore paints slowly from
     top to bottom (the top shows the new frame and the bottom still shows the old one). That is the glitch.
2. **Overall jumps.** Motion is already time-based (`lv_now = millis()` at frame start; radar `t` is set at
   `lv_radar.h:1005`, the rain at `:1058`, and the pull-up runs on `LvSelClk` with a 50 ms/frame cap at `:1600`). So a
   slow frame does not skip animation time. It shows up as one big step (the camera, grid, rain and towers all move the
   whole frame time at once). Even v1.2.16's *landed* close-up ran at ~65-70 ms per frame (~14 fps) because of the same
   full-length strokes (13 bands x 103 strokes x 199 steps). The idle radar also pushes all 147 KB per frame over a
   40 MHz SPI bus, which is 30 ms of bus time per frame.

## Why the sim showed 0 px and missed it
- The sim compares **finished frames** on a fixed 30 ms clock (`tick` = +30 ms). It never measured how long a frame
  takes, and it never showed a frame half-drawn. Every pixel was right, just about 25x too slow on the board.
- QEMU (16 ms/frame for GIBSON) was measured on v1.2.12, before the INFO pull-up existed. QEMU also does not model SPI time.
- **Panel readback (suspect 1) is not the cause.** The fade composites from the band buffer in RAM (`B.px`), not from the
  panel. There is no `readRect` or `readPixel` anywhere in the live UI (`rg` over `src/live`). The panel has no MISO
  (`src/display/lgfx_ili9341.h:94 pin_miso = -1`), so a readback could not work anyway.
- **Two-pass draws (suspect 3) are not the cause.** Each band is cleared and drawn in RAM, then pushed once by DMA.
  Nothing is cleared on the panel first.

## Fix (branch fix/pullup-glitch-2026-11-20)
1. `lvi_line`: vertical strokes visit only this band's rows (Bresenham with dx = 0 is exactly the rows y..yy). `lvi_micro`
   and the name plate skip when their rows miss the band. **Same pixels**, and the 1-row fade becomes cheap. No RAM is
   added (the dram0 fix in fd8efa8 stays).
2. Dirty rows (`lv_core.h` `lv_dr_*`): GIBSON (`lv_radar_frame`), HACKbot DOWNLOAD (`lv_dl_frame`) and HOT-COLD
   (`lv_hc_frame`) push only the panel rows whose final pixels changed. The row hash is taken after the theme re-ink.
   Every other way of writing to the panel (`lv_blit_hw`, `lv_raw_draw`) is wrapped so it invalidates the rows it touches.
   `lv_go` and ORIENT invalidate all rows. Each row is also force-pushed once every 32 frames, in case of a hash collision.
   RAM cost is 1,320 B of BSS. There is no panel readback.
3. DMA overlap was already in place: v1.2.11 ping-pong bands render band N+1 while band N is sent. Time-based motion was
   also already in place. Neither was changed.

## Sim mode that reproduces the device failure
`devrec N NAME` (`tools/livescan/dev_model.h`, host only) replays the firmware timeline:
- CPU: host thread time x `SIM_DEV_K`, calibrated so the idle radar costs QEMU's 16.0 ms.
- SPI: 40 MHz x 16 bit/px + 8 us per address window.
- DMA: `ui_blit`'s waitDMA plus async push, and the end-of-render drain.
- Loop: `lv_ui_loop`'s 30 ms deadline.
- Panel: a second framebuffer that receives each band only when its transfer finishes. It is sampled by a "camera"
  every 20 ms of device time.
In this mode the sim reproduces the glitch before the fix (45-171 of 180 pull frames over 100 ms) and not after it
(0-11 of 180, single-frame outliers from host timing noise).
