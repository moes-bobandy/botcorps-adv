BOTcorps-CYD v1.2.23 (cyd_livescan). Built from build/v1.2.23-2026-11-26 at 4832ae7. NOT flashed, NOT uploaded, NOT merged to main.
Sim-only, not yet tested on your CYD.
String: cyd-livescan v1.2.23 v1019z 4832ae7
RAM 37.9% (124092 / 327680). Flash 85.7% (2697439 / 3145728).
App-only (Launcher /Firmware): BOTcorps-CYD-v1.2.23.bin 2,697,936 B (cap 2,741,824; 43,888 B spare)
Full image (merged 0x0): BOTcorps-CYD-v1.2.23-full.bin 2,763,472 B (bootloader@0x1000 partitions@0x8000 boot_app0@0xe000 app@0x10000)
Previous pair (brightness fix only, before G2-replaces-TERM), kept in v1.2.23/_prev/: app 2,697,488 B sha256 d801eaf86a8afff2a3df6031e6c837b7fa205a2d98a0a85bd4d17ed115d671ca, full sha256 437b02d90355a7c08c4b0af5f5e61dd1693242fbcd2cf33671b0743b0ffe152d (built at 9cad4a2, packed at ae0e2f2).

New vs v1.2.22:
 1. CYD-17 terminal-prompt readout panels on BOTpheus / HACKbot / GIBSONbot / TERMINALbot (LV_CYD17_PANEL_TERM=1, source 6f031b0). Ink follows the live theme per pixel. CONTROLbot untouched (LV_C17_CTL=0).
 2. CYD-19 LOGbot page between TERMINALbot and CONTROLbot (LV_CYD19_LOGBOT=1, source 696080e): oil-slick, no mouth, normal chip-bot eyes. Pips 5 -> 6. SCAN/SYS tabs flip on tap. Log rows are generated SAMPLE rows (EX.DATA tag). No live log feed.
 3. G2 is THE TERMINALbot screen (LV_TPS_G2_ONLY=1, source 0d64cf3 incl 46e5ede). Every former TERM route opens G2: pager TERMINALbot face, "go term", legacy home TERMINALbot chip, and the Dojo footer mo-terminal path (that path is currently shadowed by the Dojo strip, which owns y>=247; the route itself now points at G2).
    How to get around G2:
      - Back to the pager: the uniform Stripe Slot back in the top-left (same control as every other sub-screen). If G2 was opened from the Dojo footer, that back goes to the Dojo instead.
      - Live typed terminal (old TERM, code kept): the small gold-rim "TERM" pill in the unused bottom-right corner. From that screen, the banner back returns to G2.
    G2 does not change the backlight: it keeps the user's normal brightness setting. The dimmest G2 text (the no-grid / "[ -- ] NO … ON GRID" lines) uses an 80% theme-accent floor (was 60%).
    The typist starts at y=25 when it still fits so the Stripe Slot does not cover the first line. Rows come from the real scan list on the CYD (tag "LIVE"); the host sim shows the canned set (tag "SAMPLE").

Port changes (not in the mocks): CYD-17/19 21,584 B panel buffer moved from static .bss to heap; G2 data tag keyed on BOTCORPS_CYD; LOGbot sample BOOT line says v1.2.23; G2 backlight dim removed; MID text floor raised; G2 is the TERMINALbot screen with Stripe Slot back and a TERM corner control.
Gate: gate_px.txt (v1220_gate.py, base b7496bf flags off vs build, SIM_THEME 22/23/9, 33 PASS; plus G2 routing tour).

Flash:
  Full image, USB, whole chip at 0x0 (your CYD on COM5):  esptool --chip esp32 --port COM5 --baud 460800 write-flash 0x0 BOTcorps-CYD-v1.2.23-full.bin
  (it overwrites the Launcher too; use the app-only route to keep it)
  App-only: do NOT esptool it. Put BOTcorps-CYD-v1.2.23.bin on the SD under /Firmware via the Launcher WebUI and verify the readback sha256.
