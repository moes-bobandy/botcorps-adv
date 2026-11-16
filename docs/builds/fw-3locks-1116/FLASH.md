# fw-3locks-1116 — CYD 2.8" (ESP32-2432S028) — BUILD ONLY, NOT FLASHED

Firmware: CONTROLbot v6 (replaces the old CONTROLbot screen, live control panel) + BOTpheus EB3 + HACKbot game.
env `cyd_livescan`, source commit c1b5659 (branch backup/cyd-fw-3locks-2026-11-16). Build ID string inside the app: `cyd-livescan 1.0 v1019z c1b5659`.

Image: `cyd_fw3locks_c1b5659_merged_0x0.bin` (also in the repo as bins/cyd_fw3locks_c1b5659_merged_0x0.bin)
- size 2,697,776 bytes
- sha256 56b347d506e26db1611ff55d02e5e66f6da3f1e1bc8f0525a4d91e132ca6a3e2
- esptool v5.2.0 merge_bin: 0x1000 bootloader.bin, 0x8000 partitions.bin (huge_app.csv), 0xe000 boot_app0.bin, 0x10000 firmware.bin
- byte-identical to PlatformIO's .pio/build/cyd_livescan/firmware.factory.bin

Flash (whole image at offset 0x0):

    esptool.py --chip esp32 --baud 921600 write_flash 0x0 cyd_fw3locks_c1b5659_merged_0x0.bin

Navigation: home CONTROLbot chip and terminal `go ctl` / `go controlbot` / `go control` open CONTROLbot v6. `go botpheus` / `go towers`, `go hackbot` / `go hack`.
First boot with no saved CONTROLbot settings: backlight 80 % (the panel default), VOL 40 %, CHIME ON, TONE BEEP.
