# BOTcorps-CYD v1.2.24 — BUILD ONLY, NOT FLASHED

Nothing was written to a device, an SD card, Downloads, or the CYD WebUI. This branch was not merged to main.

env `cyd_livescan` (extends `env:cyd`: esp32dev, huge_app.csv, 4MB DIO/40m). Source `fw/cyd-v1.2.24-2026-11-27` at b277600. Build ID string inside the app: `cyd-livescan v1.2.24 v1019z b277600`.

v1.2.23 (a87eb83) + CYD-22 long-press exit / empty-list / CH=0 (00961b5) + G2 Var F typist as the default (518db9d, LV_CYD22_G2=1, LV_C22_VAR=6). HACKbot solid lime is not in this bin (it was not in v1.2.23).

Images (also under `/workspace/out/fw-bigbot-1117/v1.2.24/`):

- `BOTcorps-CYD-v1.2.24.bin` — app-only Launcher /Firmware image (flash at 0x10000).
  2,700,784 bytes (cap 2,741,824; 41,040 B spare). sha256 37330c231fdfe8f858f53662151a8317088ee47a31d9fa923cf796591de47ed3
- `BOTcorps-CYD-v1.2.24-full.bin` — merged image at 0x0.
  2,766,320 bytes. sha256 b999411566107a38b4c67017237d3d29ef4db51761a9eb04542c305af988af94
  esptool v5.2.0 merge_bin: 0x1000 bootloader.bin, 0x8000 partitions.bin (huge_app.csv), 0xe000 boot_app0.bin, 0x10000 firmware.bin. The slice at 0x10000 of app length is byte-identical to the app-only bin.

How one would flash later (not performed):

    # whole chip at 0x0 (overwrites the Launcher; your CYD on COM5):
    esptool --chip esp32 --port COM5 --baud 460800 write-flash --flash-mode dio --flash-freq 40m --flash-size 4MB 0x0 BOTcorps-CYD-v1.2.24-full.bin

    # App-only: do NOT esptool it. Put BOTcorps-CYD-v1.2.24.bin on the SD under /Firmware via the Launcher WebUI and verify the readback sha256.

RAM 37.9% (124116 / 327680). Flash 85.8% (2700287 / 3145728).
