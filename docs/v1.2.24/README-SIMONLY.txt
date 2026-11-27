BOTcorps-CYD v1.2.24 (cyd_livescan). Built from fw/cyd-v1.2.24-2026-11-27 at b277600. NOT flashed, NOT uploaded, NOT copied to SD/Downloads, NOT merged to main.
Sim-only on device until you flash it. Gate vs v1.2.23: only G2 changed (see gate_px.txt).
String: cyd-livescan v1.2.24 v1019z b277600
RAM 37.9% (124116 / 327680). Flash 85.8% (2700287 / 3145728).
App-only (Launcher /Firmware): BOTcorps-CYD-v1.2.24.bin 2,700,784 B (cap 2,741,824; 41,040 B spare)
Full image (merged 0x0): BOTcorps-CYD-v1.2.24-full.bin 2,766,320 B (bootloader@0x1000 partitions@0x8000 boot_app0@0xe000 app@0x10000)

New vs v1.2.23:
 1. G2 Var F typist is the default TERMINALbot screen (LV_CYD22_G2=1, LV_C22_VAR=6): tmux-style 0:motd / 1:scan / 2:devs* bar, tightened BOTcorps wordmark, scan counts (including 0), name/class/rssi/ch chart, dim "term" glyphs (same hit as the old gold pill), Gibson> prompt.
 2. Stripe Slot back is removed on G2. A ~1 s long-press anywhere that is not another control exits to the same target as the old back (pager home, or Dojo if G2 was opened from there). Short taps do not exit. A hold on "term" does not exit; a tap still opens the live typed terminal.
 3. LIVE empty-state: SCANNING... while the scan job is running, "no devices on grid" when it finished empty. Every device row prints CH/RSSI even when the value is 0. Late finds force a full G2 push.

HACKbot solid lime is not in this bin (it was not in v1.2.23).

See FLASH.md for esptool / Launcher notes. Gate: gate_px.txt.
