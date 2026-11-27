# BOTcorps-CYD v1.2.25

env `cyd_livescan` (extends `env:cyd`: esp32dev, huge_app.csv, 4MB DIO/40m). Source `fw/cyd-v1.2.25-2026-11-27` at fe7a16a (fixes 982ec2e + version string). Build ID string inside the app: `cyd-livescan v1.2.25 v1019z fe7a16a`.

v1.2.24 (b277600) + field-test fixes for TERMINALbot typist settle / LOGbot frozen arrivals / TERMINALbot panel tap to G2 / CONTROLbot panel rebuild on theme slot / G2 keep-scan + late-find full push / early ticker on pager return. Look of locked screens unchanged.

Images (also under `/workspace/out/fw-bigbot-1117/v1.2.25/`):

- `BOTcorps-CYD-v1.2.25.bin` — app-only Launcher /Firmware image (flash at 0x10000).
  2,701,040 bytes (cap 2,741,824; 40,784 B spare). sha256 170b24cd53276902dddf0be36b684dc045985c38e60caaa48f3834a92c55670c
- `BOTcorps-CYD-v1.2.25-full.bin` — merged image at 0x0.
  2,766,576 bytes. sha256 c0788ea7fa659596aa82f437d212c4439fc9aed9563c34eb5e41bce7ded08e3e
  esptool v5.2.0 merge_bin: 0x1000 bootloader.bin, 0x8000 partitions.bin (huge_app.csv), 0xe000 boot_app0.bin, 0x10000 firmware.bin. The slice at 0x10000 of app length is byte-identical to the app-only bin.

How one would flash later (app-only was uploaded to SD /Firmware; the device was not esptool-flashed from this build):

    # whole chip at 0x0 (overwrites the Launcher; your CYD on COM5):
    esptool --chip esp32 --port COM5 --baud 460800 write-flash --flash-mode dio --flash-freq 40m --flash-size 4MB 0x0 BOTcorps-CYD-v1.2.25-full.bin

    # App-only: do NOT esptool it. Put BOTcorps-CYD-v1.2.25.bin on the SD under /Firmware via the Launcher WebUI and verify the readback sha256.

RAM 37.9% (124140 / 327680). Flash 85.8% (2700539 / 3145728).

## SD /Firmware upload (2026-11-27 12:44 PM America/New_York)

Uploaded ONLY the app-only bin to the work Launcher WebUI at 10.0.4.138, folder `/Firmware`, remote name `BOTcorps-CYD-v1.2.25-170b24cd.bin` (unique, same convention as v1.2.24). Login POST /login -> ESP32SESSION; upload POST / with folder=/Firmware. Device was not flashed. Other SD files were not written or deleted.

`/Firmware` listed empty immediately before the upload (`pa:/Firmware:0`, no `Fi:` entries — the earlier v1.2.24 bin was not present on this card). After upload and readback the folder listed only this file: `Fi:BOTcorps-CYD-v1.2.25-170b24cd.bin:3 MB`.

Readback via GET /file?name=/Firmware/BOTcorps-CYD-v1.2.25-170b24cd.bin&action=download:
- size 2,701,040 bytes
- sha256 170b24cd53276902dddf0be36b684dc045985c38e60caaa48f3834a92c55670c
- MATCH vs the built app-only bin.
