# CYD cyd_livescan, LOCKED UI port (2026-11-03): how to flash

NOT flashed by the agent. Run this yourself with the ESP32-2432S028R on USB:

    esptool.py --chip esp32 --port <PORT> --baud 921600 write_flash -z --flash_mode dio --flash_freq 40m --flash_size 4MB 0x0 cyd_livescan_port1103_merged_0x0.bin

- cyd_livescan_port1103_merged_0x0.bin: merged image (bootloader 0x1000, partitions 0x8000, boot_app0 0xe000, firmware 0x10000), 2264688 B, sha256 e5bf5cef1fa2f4ad2307e14720048496959c2649b3e8825738cc953266358c93
- launcher.bin: app only (the same bytes as offset 0x10000 of the merged image), 2199152 B, sha256 7a474456bf63c01a53ae9a1e01c7fa82fbb5881326a04fa2975385e0dcfa969b. Use it with a launcher, or write it at 0x10000.
- Env cyd_livescan: esp32dev, huge_app.csv, 4MB DIO 40 MHz. Merged-bin header check: e9 03 02 20 (DIO, 4MB, 40 MHz).
