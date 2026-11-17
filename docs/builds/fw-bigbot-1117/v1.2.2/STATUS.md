# BOTcorps CYD v1.2.2 — status (BUILD ONLY: not flashed, not uploaded, not merged)

Source on `backup/cyd-bigbot-pager-2026-11-17` (parent 9b85518 / v1.2.1). Env `cyd_livescan`.
Change only: pager cycle-button art, page order + start page, and flip smoothness. BOTpheus Dojo internals were not touched.

- BOTcorps-CYD-v1.2.2.bin (app-only, 0x10000): 2,686,976 B, sha256 90e6b3ccec4d4f199ba684541a8b7fe3051ac23cb8035089894711c45038a527
  Fits ota_0 2,686,976 B with 0 B headroom (v1.2.1 had 512 B). The glossy Matrix-pill shader was removed so the arrow chips + slide cache could fit; do not add more app code without freeing flash first.
- BOTcorps-CYD-v1.2.2-full.bin (merged 0x0): 2,752,512 B, sha256 a20b77abf6bc1be986aa84e8f312dab868b35d3542c59cadc80108ee6843dae7 (= PlatformIO firmware.factory.bin)
- Static RAM 121,668 B (v1.2.1 121,004 B, +664 B for the board RLE row table). gate.sh 1042/1057 (15 FAIL, 4 N/A) = baseline.

## 1. Arrow chips (same 24x36 boxes)

Replaces the glossy red/blue Matrix pills. Painted rects stay (5,32)-(28,67) and (211,32)-(234,67); hit rects stay (3,30)-(30,69) and (209,30)-(236,69). 130 ms press flash (lv_hi). Dark IC body, six metallic pins per side, pin-1 notch, theme-ink arrow (LV_ACC / LV_ACCD of the current CONTROLbot theme). Left chip points left (previous page), right chip points right (next page). Coil (32,30)-(207,69) and face (65,88)-(174,197) are not covered. Drawn with integer rounded-rect + pin rects + a 10x15 bit-mask triangle (no libm).

Pixels vs v1.2.1, real C++ sim_live renderer: BOTpheus grape (same page index, same tick) is 0 px different outside the two chip rects. Like-for-like GIBSON / HACKbot / TERMINALbot / CONTROLbot pages are 0 px outside the chip rects + ticker (y=0..23, scroll phase) + pip band (page index moved). Holo (theme 24, a non-default v6 slot) recolors the arrows and matches the same way.

## 2. Page order and start page

Was: BOTpheus, GIBSONbot, CONTROLbot (default, index 2), HACKbot, TERMINALbot.
Now: BOTpheus, HACKbot, GIBSONbot (default, index 2), TERMINALbot, CONTROLbot (far right).
Face/panel blobs are unchanged; only the page-index map changed (`lv_pg_art`). `go ctl` still lands on the CONTROLbot page (`LV_PG_CTL` = 4). Center taps are unchanged per bot. Coil swipe and arrow chips follow the new order, no wrap.

Verified in sim stills: boot dump (t=7710) is pixel-identical to the GIBSONbot page once the ticker is masked; it is not the CONTROLbot page (face box differs). The five grape pages in order are BOTpheus, HACKbot, GIBSONbot, TERMINALbot, CONTROLbot, with pip 0..4 lit in that order (proof/order_grape@2x.png). Mid-slide frame from GIBSONbot to TERMINALbot shows both pages at once (proof/new_mid_slide@3x.png).

## 3. Why flips were not smooth, and the fix

v1.2.1 re-inflated all 20 board strips and every face/panel 16-row chunk, and malloc/free'd the second decode set, on every animation frame (lv_pg_blit reset decode keys and lv_cb_cached). A 233 ms ease-in-out slide at 30 ms ticks is 7 frames; that is 308 inflates for a two-overlay flip (20 board + 24 overlay, times 7) or 224 when one side is CONTROLbot.

v1.2.2 builds a budgeted RLE of the current theme board once at slide/drag start (32,970 B for grape; cap 44 KB), keeps the second 8,416 B chunk set for the incoming page, holds the live CONTROLbot panel if needed, and leaves decode keys alone during the flip. The cache is freed when the slide ends or HOME is left. BOTpheus Dojo internals were not changed.

Host sim_live, same script, clock_gettime around each blit (x86 times are not device times; inflate counts are the work units):

| Flip | v1.2.1 | v1.2.2 |
| --- | --- | --- |
| Two overlay pages (e.g. BOTpheus→neighbour) | 7 frames, avg 419 us, max 428 us, 308 inflates | 7 frames, avg 321–341 us, max 330–349 us, 168 inflates (board RLE + 8,416 B set; cache 41,386 B) |
| Onto CONTROLbot | 7 frames, avg 436 us, max 444 us, 224 inflates | 7 frames, avg 273 us, max 363 us, 84 inflates (RLE only; cache 32,970 B) |

Inflate work per flip is 45% lower (308→168) and 62% lower (224→84). The 20 board inflates now happen once at the start of the flip, not on every frame. Mid-slide dumps differ from both end pages (not a snap). QEMU was not re-run this build; the counts above do not depend on the host clock.

## Not proven on hardware

Heap and frame times on the CYD can differ from the host sim. Read `[GFX-RAM]`, `[live] heap`, and any `PAGER:` lines from a device log after install if you want on-board numbers. This bin was not installed or flashed.
