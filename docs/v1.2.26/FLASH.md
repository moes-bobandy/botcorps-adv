# BOTcorps-CYD v1.2.26

env `cyd_livescan` (extends `env:cyd`: esp32dev, huge_app.csv, 4MB DIO/40m), now with `-DLV_CYD27_LOGSUB=1 -DLV_CYD27_LOGSUB_B=1`.
Source `fw/cyd-v1.2.26-2026-11-28` at 9a3b3b8 (from v1.2.25 5c88bba + CYD-27 mock 3aa2829/affd1d8). Build ID inside the app: `cyd-livescan v1.2.26 v1019z 9a3b3b8`.

Changes (Mobandy 2026-11-28 7:20 / 7:22 AM ET):
1. LOGbot subscreen (variant B) in firmware: real log ring (engine TTY + ticker events + per-device scan results + boot/SD/save lines), 3x5 engine font, word-wrapped, oldest top / newest bottom highlighted, no EX.DATA or sample rows; real SD free / last tap / ESP.getFreeHeap / millis strip; ALL WIFI BLE SYS, LIVE/PAUSE, CLEAR real; SAVE writes SD /logs/logbot_<n>.txt and reads it back.
2. Home row: LOGbot, BOTpheus, GIBSONbot, CONTROLbot, TERMINALbot, HACKbot (draw, taps, arrows, pips all keyed by page id).
3. CONTROLbot: live RADAR/SLEEP/BRIGHT/RECAL/ROTATE/BAR/EXIT panel even when its 21,584 B buffer can't be malloc'd (scanline fallback); EXIT -> Launcher via deep-sleep timer wake, no otadata rewrite.
4. LOGbot flip-in lag: oil-slick finish from const tables (no per-pixel libm).
5. GIBSONbot face body stretched to the same 108 px + 2 px rim as the other bots.

Images:
- `BOTcorps-CYD-v1.2.26.bin`: app-only Launcher /Firmware image (0x10000). 2,716,336 bytes (cap 2,741,824; 25,488 B spare). sha256 db126416dfaaf2f161d865efd2cc3e04ac7d5af6b4600b56e68dfc7f7e6c3ef3
- `BOTcorps-CYD-v1.2.26-full.bin`: merged at 0x0. 2,781,872 bytes. sha256 ea63507e32fb8cbe1c621a1dad5529772d1d9136fe2156afa7b7739c4fad1a7a
  esptool merge-bin: 0x1000 bootloader.bin, 0x8000 partitions.bin (huge_app.csv), 0xe000 boot_app0.bin, 0x10000 firmware.bin. Slice at 0x10000 is byte-identical to the app-only bin.

RAM 37.9% (124,324 / 327,680; v1.2.25 124,140). Flash 86.3% (2,715,839 / 3,145,728). DRAM .bss is at its limit: the log ring (64 x 44 B = 2,816 B) is heap, allocated at boot.

Not flashed. Nothing merged. To flash later: put the app-only bin on SD /Firmware (done, below) and install from the Launcher. Whole chip (overwrites the Launcher):
    esptool --chip esp32 --port COM5 --baud 460800 write-flash --flash-mode dio --flash-freq 40m --flash-size 4MB 0x0 BOTcorps-CYD-v1.2.26-full.bin

## SD /Firmware upload (2026-11-28 7:45 AM America/New_York)
From machine BOTcorps (99f03531) with webui-v1226-upload.ps1 (same method as v1.2.25): Launcher WebUI 10.0.4.138, login POST /login -> ESP32SESSION, upload POST / folder=/Firmware.
- Before: `Fi:BOTcorps-CYD-v1.2.25-170b24cd.bin:3 MB` only.
- Uploaded: `/Firmware/BOTcorps-CYD-v1.2.26-db126416.bin` (HTTP 200 OK). After: v1.2.25 file + this file. Nothing deleted or overwritten.
- Readback GET /file?name=/Firmware/BOTcorps-CYD-v1.2.26-db126416.bin&action=download: 2,716,336 bytes, sha256 db126416dfaaf2f161d865efd2cc3e04ac7d5af6b4600b56e68dfc7f7e6c3ef3, MATCH.
