# FLASH: cyd_livescan port 1105 (hunt meter A + V2 on the D4 charge lock)

NOT FLASHED. Flash only on Mobandy's go for this exact bin.

- Board: ESP32-2432S028R (CYD), ILI9341 240x320 portrait + XPT2046. Env `cyd_livescan` (esp32dev, huge_app.csv, 4MB DIO 40m).
- Source: `b29f8abf17aa4c8ea0542dad9b574844e4e62807` on `backup/cyd-livescan-2026-10-30`. Build ID in the bin: `v1019z b29f8ab`.
- `cyd_livescan_port1105_merged_0x0.bin` 2,275,312 B, sha256 `09b2ca752fe8ddc8a386b7ee164e55f233ed74824a684a9268c983ab24bfc04c`
  (bootloader 0x1000, partitions 0x8000, boot_app0 0xe000, app 0x10000; == PlatformIO's firmware.factory.bin)
- `launcher.bin` (app only, for 0x10000) 2,209,776 B, sha256 `ece72d53a6b4f66d45511da080c0f14e55893c65d28fa3762fb45d046dc3b78a`

Whole image at 0x0 (recommended):

    esptool.py --chip esp32 -b 460800 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size 4MB 0x0 cyd_livescan_port1105_merged_0x0.bin

App only (bootloader + partitions already from a cyd_livescan build):

    esptool.py --chip esp32 -b 460800 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size 4MB 0x10000 launcher.bin

Check before flashing: `sha256sum -c SHA256SUMS`.
