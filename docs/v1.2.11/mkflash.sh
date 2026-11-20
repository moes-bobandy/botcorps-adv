#!/bin/bash
# mkflash.sh BUILD_DIR OUT
D=$1; ~/.platformio/penv/bin/python -m esptool --chip esp32 merge_bin -o $2 --flash_mode dio --flash_freq 40m --flash_size 4MB --fill-flash-size 4MB 0x1000 $D/bootloader.bin 0x8000 $D/partitions.bin 0xe000 /home/box/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin 0x10000 $D/firmware.bin
