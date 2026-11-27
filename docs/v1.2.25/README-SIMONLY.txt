BOTcorps-CYD v1.2.25 (cyd_livescan). Built from fw/cyd-v1.2.25-2026-11-27 at fe7a16a (fixes 982ec2e + version string).
String: cyd-livescan v1.2.25 v1019z fe7a16a
RAM 37.9% (124140 / 327680). Flash 85.8% (2700539 / 3145728).
App-only (Launcher /Firmware): BOTcorps-CYD-v1.2.25.bin 2,701,040 B (cap 2,741,824; 40,784 B spare)
Full image (merged 0x0): BOTcorps-CYD-v1.2.25-full.bin 2,766,576 B (bootloader@0x1000 partitions@0x8000 boot_app0@0xe000 app@0x10000)

Fixes vs v1.2.24 (field-test, device wins):
 1. TERMINALbot / CYD-17 typist: after settle, skip per-tick re-raster so SPI/touch are not starved. Look unchanged.
 2. LOGbot: freeze SAMPLE arrivals (no re-materializing loop) and rebuild the panel only on tab/theme change. Look unchanged.
 3. Tap on the TERMINALbot typist panel opens G2.
 4. CONTROLbot: rebuild live RADAR/SLEEP/BRIGHT/RECAL/ROTATE/EXIT panel when the theme slot changes.
 5. G2: keep LV_J_ALL scanning while the typist is up; late finds request a full push before the scene runs so devices appear.
 6. Pager return: band-split + early ticker so the current banner paints first (no stale still banner).

See FLASH.md for esptool / Launcher notes.
