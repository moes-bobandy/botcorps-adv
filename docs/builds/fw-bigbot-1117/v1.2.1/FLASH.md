# fw-bigbot-1117 v1.2.1 — CYD 2.8" — BUILD ONLY, NOT FLASHED

Nothing was written to a device, an SD card, or the CYD WebUI. Nothing was merged.

- `BOTcorps-CYD-v1.2.1.bin`: app-only at 0x10000, or install it from the SD card in Launcher.
- `BOTcorps-CYD-v1.2.1-full.bin`: merged image at 0x0.

How it would be flashed later (not performed):

    esptool.py --chip esp32 --baud 921600 write_flash 0x0 BOTcorps-CYD-v1.2.1-full.bin
    esptool.py --chip esp32 --baud 921600 write_flash 0x10000 BOTcorps-CYD-v1.2.1.bin

Navigation: tap the BLUE pill (left) for the previous page and the RED pill (right) for the next page, or swipe on the coil strip. There is no wrap. A tap on the center opens BOTpheus EB3, GIBSON radar, HACKbot game or TERM, and on CONTROLbot cycles the theme.
