BOTcorps-CYD v1.2.23 (cyd_livescan). Built from build/v1.2.23-2026-11-26 at b5b1ecf. NOT flashed, NOT uploaded, NOT merged to main.
Sim-only, not yet tested on your CYD.
String: cyd-livescan v1.2.23 v1019z b5b1ecf
RAM 37.9% (124092 / 327680). Flash 85.7% (2696983 / 3145728).
App-only (Launcher /Firmware): BOTcorps-CYD-v1.2.23.bin 2,697,488 B (cap 2,741,824; 44,336 B spare)
Full image (merged 0x0): BOTcorps-CYD-v1.2.23-full.bin 2,763,024 B (bootloader@0x1000 partitions@0x8000 boot_app0@0xe000 app@0x10000)

New vs v1.2.22:
 1. CYD-17 terminal-prompt readout panels on BOTpheus / HACKbot / GIBSONbot / TERMINALbot (LV_CYD17_PANEL_TERM=1, source 6f031b0). Ink follows the live theme per pixel. CONTROLbot untouched (LV_C17_CTL=0).
 2. CYD-19 LOGbot page between TERMINALbot and CONTROLbot (LV_CYD19_LOGBOT=1, source 696080e): oil-slick, no mouth, normal chip-bot eyes. Pips 5 -> 6. SCAN/SYS tabs flip on tap.
    Log rows are generated SAMPLE rows (EX.DATA tag on the panel). No live log feed.
 3. G2 "Wake up, Neo" TERMINALbot screen (LV_TPS_G2_ONLY=1, source 0d64cf3 incl 46e5ede, branch mock/v1221-term-ps-matrix-2026-11-25). Tapping the TERMINALbot face on the pager opens it; tap anywhere returns to the TERM screen.
    Only G2 is linked (A-F, F2, G removed; TERM's LIST button is back to the stock list). Rows come from the real scan list on the CYD (tag "LIVE"); the host sim shows the canned set (tag "SAMPLE").
    While G2 is up: backlight dims to 20% and a WiFi+BLE scan starts if none is running (both from the source mock).
Port changes (not in the mocks): CYD-17/19 21,584 B panel buffer moved from static .bss to heap (static overflowed dram0 by 21,088 B); lazy malloc on the first pager panel draw, never freed;
 on malloc failure the locked panel is left as is. G2 data tag keyed on BOTCORPS_CYD. LOGbot sample BOOT line says v1.2.23.
Gate: gate_px.txt (v1220_gate.py, base b7496bf flags off vs build, SIM_THEME 22/23/9; G2 frame vs source mock 0d64cf3).

Flash:
  Full image, USB, whole chip at 0x0 (your CYD on COM5):  esptool --chip esp32 --port COM5 --baud 460800 write-flash 0x0 BOTcorps-CYD-v1.2.23-full.bin
  (it overwrites the Launcher too; use the app-only route to keep it)
  App-only: do NOT esptool it. Put BOTcorps-CYD-v1.2.23.bin on the SD under /Firmware via the Launcher WebUI and verify the readback sha256.
