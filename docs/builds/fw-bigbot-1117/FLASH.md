# fw-bigbot-1117 — CYD 2.8" (ESP32-2432S028) — BUILD ONLY, NOT FLASHED

This document is a build record. Nothing in this task was written to a device, an SD card, or the CYD WebUI, and this branch was not merged.

Firmware: the five-page big-bot pager is the new CYD home (pager-v2 mock: v3.1 faces and panels, no BACKbot). CONTROLbot keeps locked v6 controls. Center taps open EB3 / Gibson scan / theme cycle / HACKbot game / TERM; every sub-screen has a way back to the pager.
env `cyd_livescan` (extends `env:cyd`), source commit 3891839 (branch `backup/cyd-bigbot-pager-2026-11-17`). Build ID string inside the app: `cyd-livescan 1.0 v1019z 3891839`.

Images (also under `/workspace/out/fw-bigbot-1117/`):

- `BOTcorps-CYD-v1.2.bin` — app-only, flash at 0x10000 (Launcher ota slot).
  2,678,208 bytes. sha256 0c05266fdbc194cc98712fac53e0a7ecef6481489529d6d067ab2b49f5f9be1d
  Fits ota_0 2,686,976 bytes (8,768 bytes headroom).
- `BOTcorps-CYD-v1.2-full.bin` — merged image at 0x0.
  2,743,744 bytes. sha256 e62e8958c70ebbb2f09aed487b2bafbeda3d3a08f9720c98570a52e209ea356c
  esptool v5.2.0 merge_bin: 0x1000 bootloader.bin, 0x8000 partitions.bin (huge_app.csv), 0xe000 boot_app0.bin, 0x10000 firmware.bin
  Byte-identical to PlatformIO's `.pio/build/cyd_livescan/firmware.factory.bin`.

How one would flash later (not performed):

    # whole image at offset 0x0
    esptool.py --chip esp32 --baud 921600 write_flash 0x0 BOTcorps-CYD-v1.2-full.bin

    # or app-only into the Launcher ota slot
    esptool.py --chip esp32 --baud 921600 write_flash 0x10000 BOTcorps-CYD-v1.2.bin

Navigation: home is the pager, default page CONTROLbot (changeable). Swipe the top ticker to change pages (no wrap; edges bounce). `go ctl` / `go controlbot` / `go control` open the CONTROLbot page of the pager. `go botpheus` / `go towers`, `go hackbot` / `go hack`, `go term`, and the radar / scan-tower routes still work and return to the pager via BACKbot / BACK.
