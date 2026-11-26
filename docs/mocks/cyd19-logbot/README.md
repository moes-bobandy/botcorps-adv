# MOCK 2026-11-26 (CYD-19): LOGbot home pager page, three special finishes

Mock only, sim-only, not yet tested on your CYD. Not in any bin. No build, no flash.
Branch mock/cyd19-logbot-page-2026-11-26 on mock/cyd17-panel-terminal-2026-11-26 (6f031b0, v1.2.22, Vapor Grid default).
Flag: LV_CYD19_LOGBOT (default 0). With 0 the firmware path and every render are byte-identical to the base.

Page order with the flag: BOTpheus, HACKbot, GIBSONbot, TERMINALbot, **LOGbot**, CONTROLbot (CONTROLbot = page 5 in the sim `page N`).
Cloned verbatim from the locked pager page: board, chrome, pins, banner/ticker, arrow chips, rain face box, role-panel plate geometry.
Added: LOGbot's face + the log content. Pip row: 6 pips instead of 5 (re-centred on x=120), the only change on existing pages.

- Face: no LOGbot sprite/face exists in source or docs (rg LOGbot: 0 hits), so it is the house chip plate (blob 2, the same art TERMINALbot uses)
  through a metal finish + smooth sheen, with the same black code rain. Its two locked eye sockets are the two TABS (lit = open log);
  the recessed slot holds the two log counters (sample numbers).
- Panel: two-tab terminal (SCAN LOG: scanned devices MAC/SSID/RSSI + HUNT start/lock/end; SYSTEM LOG: BOOT/FLASH/WARN/ERR). CYD-17 F2 look,
  ink = live theme per pixel. Tap the face or the panel to flip tabs. Newest row at the bottom, new row every 1.8 s.
- All rows are generated SAMPLE data (EX.DATA tag on the panel); no live log is read.
- Finishes: chrome blue (non-monotone ramp = dark horizon band between reflections), metallic copper, oil-slick (dark substrate + drifting
  thin-film hue, animated). All three add a smooth anisotropic sheen on top of the art's banded shading, so no band is a flat fill.

Gate: gate_c19.txt (themes 22/23/9, 3 finishes). OFF build vs base: 0 px on all 10 screens. ON build vs base: 0 px outside pip band y=25..29.
