# BOTcorps CYD v1.2.1 — status (BUILD ONLY: not flashed, not uploaded, not merged)

Source 9f01f0a on `backup/cyd-bigbot-pager-2026-11-17` (parent b681f7c / v1.2 3891839). Env `cyd_livescan`.
Build ID in the app: `cyd-livescan 1.0 v1019z 9f01f0a`.

- BOTcorps-CYD-v1.2.1.bin (app-only, 0x10000): 2,686,464 B, sha256 1579474d2e3b9081f76e81ae614eb06afc8aa8667bdcf55d3b9c2db843724f8a
  Fits ota_0 2,686,976 B with 512 B headroom (v1.2 had 8,768 B; the chunked overlays cost 6,211 B of flash). VOL's replacement must stay small or free flash elsewhere.
- BOTcorps-CYD-v1.2.1-full.bin (merged 0x0): 2,752,000 B, sha256 39724c25b2b98cc79f27ff21ddc61cf553e3e7d1ea8aa82fe09294c020457771 (= PlatformIO firmware.factory.bin)
- Static RAM 121,004 B (v1.2 120,964 B). gate.sh 1042/1057 (15 FAIL, 4 N/A) = baseline.

## Root causes (evidence in proof/)

1. BOTpheus force close — PROVEN in QEMU + sim. `src/live/botpheus/mo_emblem.h:93` (`new MoEmb`, 7,112 B each, never freed). The first Dojo frame builds 18 emblems = 128,016 B (proof/sim_emblem_allocs_v1.2.log). The CYD UI leaves ~53 KB. `operator new` throws -> std::terminate -> abort -> reboot (proof/qemu_tour_v1.2.1_pre-emblem-fix_abort.log, backtrace mo_emb_get <- mo_emb_draw <- mo11_tower_fx <- lv_scene_radar <- itw_mo_scene). The same code is in v1.1, so v1.1's EB3 route would have crashed too. After the reboot the CYD showed CALbot and then the MORPHEUSbot BOOTING clip, which is the "morpheus intro".
2. Wrong panel on every page, and dead CONTROLbot controls — PROVEN in QEMU. v1.2 kept the face (24,200 B) and panel (21,584 B) in heap forever, because lv_pg_leave_cache had no caller. After one non-CONTROLbot page: free 26,252 B, largest block 13,812 B (proof/qemu_tour_v1.2.log). The role panel and lv_cb_pan (21,584 B each) could not allocate, so the still's CONTROLbot panel showed through on every page, and the live CONTROLbot panel never redrew or flashed. Settings still changed underneath. The host sim has unlimited heap, so the v1.2 proof could not see this.
3. RADIO SCAN FAILED (`src/live/lv_core.h:558`, set when the radio task publishes -2 after `WiFi.mode`/`NimBLEDevice::init` fails) — SUSPECTED, same heap. v1.2 left about 46 KB less heap for the radio stack. QEMU cannot confirm: QEMU Wi-Fi init fails with ESP_ERR_NO_MEM on every build.
4. "Cycles but glitchy" — PROVEN in sim. One shared overlay buffer meant a slide between two non-CONTROLbot pages showed the outgoing page's face and panel on the incoming page (proof/v1.2_slide_hackbot_to_terminal_wrong_art.png).

## Fixes

- Emblems: exact-size entries (2·S² + (S+2)²), nothrow malloc, 20,000 B budget, freed when leaving the Dojo. Dojo byte-identical over 170 sim frames.
- Pager: 16-row deflate chunks decoded into the free tail of the static io->buf. A still page uses 0 B of heap; the second page of a slide uses 8,416 B for that frame only. lv_cb_pan exists only while drawing, as in v1.1. Leaving HOME frees everything. QEMU v1.2.1 on HOME: free 50,788 B, largest 36,852 B; in the Dojo: 43,956 B free, no abort; DL OK.
- Pip ink is computed once per slot.
- Swipe works only on the coil strip (32,30)-(207,69). The banner is inert.
- Matrix pills: BLUE on the left rail = previous page, RED on the right rail = next page. Fixed colours, glossy, 24x36 at (5,32)/(211,32) over pins 1-3 only, 130 ms pressed flash, no wrap (edge bounce).
- Power-on goes CALbot -> pager. The MORPHEUSbot BOOTING clip no longer plays; `go boot` still replays it. There was no intro on LV_SC_MO entry itself: the first Dojo frame comes 100 ms after the tap.
- Serial prints one `[live] heap scr=N free= min= largest=` line per screen change.
- The VOL slot is marked in lv_controlbot_screen.h (the tap line and the lv_cb_bar call).

Pixels: all 10 grape/holo pages are identical to v1.2 except inside the two pill boxes (1,544 px). The ticker differs only by scroll phase, because boot timing changed. The Dojo, the HACKbot game and the CONTROLbot panel art are unchanged.

## Not proven on hardware

Heap numbers are from QEMU (esp32 machine) running the real firmware with a tour-only build flag. The release bin does not include the tour. Device free heap can differ, so read the `[GFX-RAM]` and `[live] heap` lines from a device log to confirm.

## Capturing a serial crash log (if needed)

1. Plug the CYD into the PC over USB-C. Close anything else using the COM port (Arduino IDE, Launcher WebUI tools).
2. Run `pio device monitor -b 115200 --filter esp32_exception_decoder` (or `python -m serial.tools.miniterm COMx 115200`, or PuTTY at 115200 8N1).
3. Press RST, then reproduce the problem: tap the BOTpheus face, swipe, start a scan.
4. Save everything from `rst:` through the `Backtrace:` line, plus the `[GFX-RAM]` and `[live] heap ...` lines. Decode with `xtensa-esp32-elf-addr2line -pfiaC -e firmware.elf <addresses>`, using the firmware.elf from this exact build (9f01f0a).
