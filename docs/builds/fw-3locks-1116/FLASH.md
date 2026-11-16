# fw-3locks-1116 — CYD 2.8" (ESP32-2432S028) — BUILD ONLY, NOT FLASHED

Firmware: CONTROLbot Color UI v6 + BOTpheus EB3 + HACKbot game, env `cyd_livescan`, source commit 10e5b12
(branch backup/cyd-fw-3locks-2026-11-16). Build ID string inside the app: `cyd-livescan 1.0 v1019z 10e5b12`.

Image: `cyd_fw3locks_10e5b12_merged_0x0.bin` (also in the repo as bins/cyd_fw3locks_10e5b12_merged_0x0.bin)
- size 2,691,216 bytes
- sha256 b755c209100386cbb1459e17b9e2e81311bca5b660ccc412e76fcd069d46b68e
- made with esptool v5.2.0 merge_bin: 0x1000 bootloader.bin, 0x8000 partitions.bin (huge_app.csv), 0xe000 boot_app0.bin, 0x10000 firmware.bin
- byte-identical to PlatformIO's own .pio/build/cyd_livescan/firmware.factory.bin

Flash (whole image at offset 0x0):

    esptool.py --chip esp32 --baud 921600 write_flash 0x0 cyd_fw3locks_10e5b12_merged_0x0.bin

How to reach the new screens (terminal): `go botpheus` (or `go towers`), `go hackbot` (or `go hack`), `go color` / `colorui` / `themes`.
Home CONTROLbot still opens the locked CONTROLbot screen. `go morpheus` still opens the glossary (unchanged).
