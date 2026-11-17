# fw-bigbot-1117 v1.2.2 — CYD 2.8" — BUILD ONLY, NOT FLASHED

Nothing was written to a device, an SD card, or the CYD WebUI. Nothing was merged.

- `BOTcorps-CYD-v1.2.2.bin`: app-only at 0x10000, or install it from the SD card in Launcher.
- `BOTcorps-CYD-v1.2.2-full.bin`: merged image at 0x0.

How it would be flashed later (not performed):

    esptool.py --chip esp32 --baud 921600 write_flash 0x0 BOTcorps-CYD-v1.2.2-full.bin
    esptool.py --chip esp32 --baud 921600 write_flash 0x10000 BOTcorps-CYD-v1.2.2.bin

Navigation: tap the left arrow chip for the previous page and the right arrow chip for the next page, or swipe on the coil strip. There is no wrap. Pages are BOTpheus, HACKbot, GIBSONbot, TERMINALbot, CONTROLbot, and the pager starts on GIBSONbot. A tap on the center opens BOTpheus EB3, the HACKbot game, GIBSON radar or TERM, and on CONTROLbot cycles the theme.
