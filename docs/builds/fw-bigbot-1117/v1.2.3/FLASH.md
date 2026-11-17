# fw-bigbot-1117 v1.2.3 — CYD 2.8" — BUILD ONLY, NOT FLASHED

Nothing was written to a device, an SD card, or the CYD WebUI. Nothing was merged.

- `BOTcorps-CYD-v1.2.3.bin`: app-only at 0x10000, or install it from the SD card in Launcher.
- `BOTcorps-CYD-v1.2.3-full.bin`: merged image at 0x0.

How it would be flashed later (not performed):

    esptool.py --chip esp32 --baud 921600 write_flash 0x0 BOTcorps-CYD-v1.2.3-full.bin
    esptool.py --chip esp32 --baud 921600 write_flash 0x10000 BOTcorps-CYD-v1.2.3.bin

Navigation is unchanged from v1.2.2 (arrow chips or coil swipe; start on GIBSONbot; CONTROLbot is the far-right page). On the CONTROLbot panel, the old VOL slot is now SLEEP: tap to cycle OFF / 30s / 1m / 5m. When the timeout hits, GPIO21 goes off; any touch wakes the backlight, and that first wake-tap does not fire a control.
