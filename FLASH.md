# CYD fix8 — NOT FLASHED

Source `049ff599ca31fcc18c4f41f25e64331e60e37d85` (`049ff59`), branch `backup/cyd-livescan-2026-10-30`.
Embedded build ID: `v1019z 049ff59` (clean committed HEAD).

## Board and flash
ESP32-2432S028R / ESP32-D0WD-V3, no PSRAM; ILI9341 portrait 240x320, XPT2046.
PlatformIO `env:cyd_livescan`, esp32dev, Arduino, huge_app.csv, 4 MB DIO 40 MHz; LCD HSPI 40 MHz DMA, touch VSPI.

Validate `SHA256SUMS` before flashing. Parent handles COM5 staging/flashing; no device was touched here.

```sh
python -m esptool --chip esp32 --port COM5 --baud 460800 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size 4MB 0x0 cyd-livescan-049ff59-merged.bin
```

- `cyd-livescan-049ff59-merged.bin`: 2,414,672 bytes; SHA-256 `c4b24dda5f586f7146d2bf6681097fa33d8b329ff368339eac36ab704253365b`.
- `cyd-livescan-049ff59-launcher.bin`: 2,349,136 bytes; SHA-256 `c3ebefd875176cf067b8d2604bc80c5c23ac433f2f3ae27e1030ab697feb3922`.
- Merged components byte-identical: bootloader at 0x1000, partitions at 0x8000, boot_app0 at 0xe000, app at 0x10000. Identical to PlatformIO firmware.factory.bin.
- Header checks: merged[0x1000]=E9, merged[0x8000:0x8002]=AA50; launcher[0]=E9.
- Launcher is the exact application-only firmware.bin; direct esptool address is 0x10000, NOT 0x0.
- This document's own SHA-256 is in SHA256SUMS.

## All eight items
1. Gibson HOLD TO LOCK owns its touch/release; short charge cannot cycle/select through.
2. MORPHEUSbot 12-device glossary and description pages.
3. No old-UI startup flash; dark panel until themed frame; BOOT goes straight to HOME.
4. MORPHEUSbot boot artwork and original wipe timing.
5. Core 0 radio task (6144-byte stack), epoch-tagged handoff, no UI driver calls; Wi-Fi/NimBLE mutually exclusive and warm for consecutive scans. Async BLE API verified against installed NimBLE 2.3.9 (milliseconds). One frame deadline, absolute-time D4/V2 effects. Explicit STOP also releases warm radios after a one-shot scan is consumed.
6. Both hunt meters retained.
7. Heap-backed dual DMA strips and scanline coalescer, adaptive larger heap-backed render bands, 10752-byte Flash gamma LUT replacing runtime powf. Explicit waitDMA plus waitDisplay before CS release/touch.
8. Terminal BACKbot remains bottom-right; no old top-right BACKbot hotspot.

## Verification
- 26 glossary screens and all 85 boot frames: zero pixel differences; home_intro_frames=0; first HOME frame matches direct theme-5 HOME.
- All 14 livescan scripts plus item8, cal/nocal, before/default/64-row: 90 successful runs, 558 compared output files, zero unintended differences. Recording runs serialized (existing shared sim temp file).
- Final committed HEAD recheck after idle STOP correction: 68 successful before/after runs across all scripts + items 1/6/8, 976 output files, zero differences; locked glossary and boot reverified.
- Targeted hold isolation: short charge unlocked, full charge locked; 206 dual-meter/hunt snapshots zero differences for default/64-row bands.
- C float32 powf equivalence: 10752 LUT bytes, zero mismatches. DMA behavioral mock, actual worker control-flow simulation and idle/active STOP lifecycle check pass (see tests/ and logs/).
- Shadow peak 29943/32768 bytes; shadow is tint-region-based, independent of band height.
- Unmodified gate render score **1042/1057 PASS, 15 FAIL, 4 N/A**, unchanged baseline. Overall exit 19 additionally includes four unrelated Cardputer RAM checks with absent ELF inputs; not a clean overall gate pass.
- Final PIO build succeeds: RAM **110732/327680 B (33.8%)**; Flash **2348727/3145728 B (74.7%)**. 115 pre-existing unique warnings, **zero new** versus the preserved six-item build.

## Safe RAM policy (hardware values pending)
Static RAM leaves an upper bound of 216948 B. Persistent allocations reserved first: shadow 32768 B, glow underlay 19190 B, radar-route scratch 9264 B, worker stack 6144 B (plus RTOS/allocator overhead), leaving theoretical 149582 B before bands/strips/other runtime overhead.

Band options: 64/48/32/24 rows = 30720/23040/15360/11520 B.
Dual DMA options: 48/32/16 rows = 46080/30720/15360 B.
Selection uses **actual free internal 8-bit heap**, reserving **107520 B (105 KiB)** before radio start, not the theoretical subtraction. It steps down bands/strips and uses synchronous output if no pair is safe. For illustration only, 48-row band + dual 16-row strips leaves 111182 B before other runtime overhead; this is NOT a measured board heap.

Read the single `[GFX-RAM]` boot line for actual DMA/band sizes, ESP free heap, 8-bit free, DMA free and internal-8-bit free. Real radio transition latency, heap minima and FPS require COM5 validation. Host proof does not establish hardware timing.

## Backup status
Local clean source and artifact commits prepared. No push was performed by this executor: explicit user authorization for the specific external pushes was not present in the supplied user turns. Parent may push only these backup branches after confirming authorization; never main or a PR.
