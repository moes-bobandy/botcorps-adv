# BOTcorps-CYD v1.2.26 memory breakdown

Read-only survey of the v1.2.26 device image built from commit `9a3b3b8`. No source was changed, no firmware was rebuilt, no bins were written.

**ELF / image used (measured):**
- `/workspace/cyd-v1225-wt/.pio/build/cyd_livescan/firmware.elf` (mtime 2026-11-28 07:42 America/New_York, the `cyd_livescan` link of 9a3b3b8)
- App-only bin 2,716,336 B, sha256 `db126416dfaaf2f161d865efd2cc3e04ac7d5af6b4600b56e68dfc7f7e6c3ef3` (matches `/workspace/out/fw-bigbot-1117/v1.2.26/BOTcorps-CYD-v1.2.26.bin` and the SD copy)
- Build ID inside the image: `cyd-livescan v1.2.26 v1019z 9a3b3b8`
- Tools: `xtensa-esp32-elf-size` / `nm -S` / `readelf`, the GNU map `firmware.map`, and on-box compression of the raw ELF bytes with zlib-9 (raw deflate, same family as the ROM `tinfl` already in the firmware), LZ4 block (level 12), and a simple byte RLE

Every byte count below is **measured** unless a sentence is marked **estimated**. Compression ratios are measured on the exact flash bytes of each symbol. Locked looks are assumed to stay pixel-for-pixel: only lossless encodings are discussed.

---

## 0. The three budgets (measured)

| Region | Section | Size (B) | Notes |
|---|---|---:|---|
| Flash image (app-only bin) | — | 2,716,336 | Cap 2,741,824; 25,488 B spare. PIO "Flash used" 2,715,839 / 3,145,728 (86.3%) |
| `.flash.rodata` | mapped 0x3F400120 | 1,419,540 | Art, clips, palettes, tables, strings |
| `.flash.text` | mapped 0x400D0020 | 1,142,044 | Code in flash (cached) |
| `.iram0.vectors` + `.iram0.text` | 0x40080000 | 1,028 + 125,895 = 126,923 | Copied from flash into IRAM at boot |
| `.dram0.data` | 0x3FFBDB60 | 27,332 | Initialized DRAM, also stored in the flash image |
| `.flash.appdesc` | | 256 | IDF app descriptor |
| `.dram0.bss` | 0x3FFC4628 | 96,992 | Zeroed DRAM, not in the flash image |
| DRAM as PIO reports it | `.data` + `.bss` | 124,324 / 327,680 (37.9%) | Matches 27,332 + 96,992 |
| IRAM window | 0x40080000–0x400A0000 | 126,923 used / 131,072; **4,148 B spare** | Ends at 0x4009EFCC |
| DRAM leftover before runtime allocs | 327,680 − 124,324 | **203,356** | Then Wi‑Fi / BT / NimBLE / LwIP, the 105 KiB radio reserve, DMA bands, etc. take most of it |

`.flash.rodata_noload` (23,429 B) is NOBITS and is not in the bin.

The flash image is roughly: flash text + flash rodata + appdesc + the IRAM image + `.dram0.data` (+ a few hundred bytes of headers / alignment). That is why "Flash used" is larger than rodata+text alone.

---

## 1. Flash: top symbols, then groups

### 1.1 Top 15 flash-resident objects (measured)

Addresses in `.flash.rodata` unless noted. `lk_blob_live` is a raw assembly blob; the rest are `nm` symbols.

| Rank | Symbol | Bytes | What it is | Already compressed? |
|---:|---|---:|---|---|
| 1 | `lk_blob_live` | 315,100 | Live lock-clip blob (boot/home/term/scan/ctl), raw deflate chunks | Yes. 1,338 chunks inflate to **2,005,067** B (max 16,352, under the 16,384 dest). Ratio **6.36×**. The full (non-live) blob was 1,843,947 B; this live subset already dropped 1,528,847 B of unused clips (`lock_blob_live.json`). |
| 2 | `LV_CB_Z` | 213,778 | 27 locked CONTROLbot v6 stills, 20 strips × 16 rows, raw deflate | Yes. 540 strips inflate to **4,147,200** B (exactly 27 × 240 × 320 × 2), max 7,680 B per strip. Ratio **19.40×**. |
| 3 | `LV_PORT_HOME_GLOW` | 115,140 | Home-screen glow records, 9,595 × 12 B structs, uncompressed | No |
| 4 | `LV_PORT_SCAN_RAIN` | 49,000 | Scan-screen rain frames, uncompressed | No |
| 5 | `LV_PORT_TERM_RAIN` | 42,080 | Term-screen rain frames, uncompressed | No |
| 6 | `LV_GL_P0_DAT` | 41,579 | Glossary pillar 0 pixel data, uncompressed | No |
| 7 | `LV_PORT_HOME_C0` | 38,306 | Home clip-region 0 indices, uncompressed | No |
| 8 | `LVC_VAR` | 37,550 | CALbot element variants (already a palette-RLE, still hot) | Partial (custom RLE, not deflate) |
| 9 | `LV_PORT_SCAN_GLOW` | 36,348 | Scan glow records, uncompressed | No |
| 10 | `LV_PORT_SCAN_C0` | 36,252 | Scan clip-region 0, uncompressed | No |
| 11 | `LVC_BASE` | 27,594 | CALbot "CAL OK" base frame, custom RLE | Partial |
| 12 | `MO_EMBT_BITS` | 27,378 | BOTpheus emblem bitmaps | No |
| 13 | `LV_TNR_RGB` | 26,496 | Old TERMINALbot "no rain" patch (3 themes × 4,416 RGB565, mostly zeros) | No |
| 14 | `LV_PORT_HOME_RAIN` | 24,000 | Home rain frames, uncompressed | No |
| 15 | `LV_GL_BASE_DAT` | 22,098 | Glossary base art | No |

`lv_lb_sin_t` and the other LOGbot oil-slick tables (8,124 B together) sit in flash on purpose, as of 9a3b3b8, so they would not overflow DRAM `.bss`.

### 1.2 Flash grouped (measured)

**Art / clips / stills / palettes** (app `.flash.rodata` from `lv_ui.cpp.o` 952,653 + `lock_blob_live.S.o` 315,100 = **1,267,753** of the 1,419,540 rodata section):

| Group | Flash bytes (sum of named symbols, measured) | Encoding today |
|---|---:|---|
| Lock-clip blob `lk_blob_live` + `LK_CHUNK` / `LK_OP` tables | 315,100 + 15,720 = 330,820 | raw deflate, 16,384 B dest (`s_buf`) + 10,992 B `tinfl` state |
| CONTROLbot stills `LV_CB_Z` + `LV_CB_OFF` | 213,778 + 2,164 = 215,942 | raw deflate, 7,680 B dest per strip |
| Port-layer overlays (`LV_PORT_*`: rain, glow, clip indices, palettes, LUTs) | 405,264 | **uncompressed** (glow is an array of floats+ids) |
| Glossary (`LV_GL_*`) | 118,917 | uncompressed |
| CALbot (`LVC_BASE` + `LVC_VAR` + row/off tables) | ~66,000 | custom palette-RLE |
| BOTpheus emblems (`MO_EMB*` + DRAM cache is separate) | 42,426 in flash (`MO_EMBT_BITS` + `MO_EMB` + `MO_EMBT_OFF`) | uncompressed / packed bits |
| Pager faces `LV_PG_FACE_Z` + `LV_PG_CORNER_Z` | 2,207 + 2,076 | already deflate (face 1.04× more, corner already 3.11× and still shrinks because some of it is a table) |
| Old TERM norain `LV_TNR_RGB` (+ small bit/offset tables) | 26,496 + ~1.5 KB | uncompressed, almost empty |
| Icons, HACKbot face, tokens (`LV_ICON_ART`, `LV_HC_FACE`, `LV_BM_TOK`) | 4,896 + 4,418 + 4,432 | uncompressed |
| LOGbot oil-slick tables | 8,124 | const float tables, already in flash |

**Fonts** (measured, small):
- Engine 5×7 `LV_GLCD` 475 B, 3×5 `LV_F35` 190 B
- LovyanGFX default `Font0` / `font` 1,280 + 16 B, plus a few hundred bytes of font code
- No unused large font set is linked. `lgfx_fonts.cpp.o` is 1,343 B of rodata.

**App code** (measured from the map): `lv_ui.cpp.o` `.flash.text` 152,881 B (the live UI, pager, games, typist, radar, all compiled through that one TU). Other app objects are tiny.

**Libraries, flash image contribution** (measured from the map: `.flash.rodata` + `.flash.text` + `.iram0.text` + `.dram0.data` + vectors/appdesc — i.e. what actually occupies flash). Top of the list:

| Bucket | Flash bytes | DRAM `.bss` (extra) |
|---|---:|---:|
| App live UI + art (`lv_ui.cpp.o`) | 1,108,424 | 65,118 |
| App lock-clip blob | 315,100 | 0 |
| Wi‑Fi stack (`libnet80211`, `libpp`, `libesp_wifi`, coexist) | 229,652 | 10,101 |
| Other IDF (drivers, system, MM, coredump, …) | 148,483 | 1,737 |
| BT controller (`libbtdm_app`) | 136,049 | 2,918 |
| LwIP | 114,654 | 3,113 |
| NimBLE | 87,578 | 5,324 |
| libc / libstdc++ | 76,723 | 817 |
| mbedTLS | 70,186 | 77 |
| WPA | 56,870 | 1,330 |
| ESP drivers (UART, I2C, LEDC, SPI, GPIO, …) | 48,329 | 181 |
| PHY / RTC | 45,713 | 49 |
| LovyanGFX | 33,081 | 512 |
| Arduino core | 29,978 | 2,276 |
| FreeRTOS | 24,275 | 809 |
| NVS | 19,983 | 24 |
| FATFS | 18,332 | 38 |
| SPI flash HAL | 14,954 | 16 |
| Heap (tlsf, …) | 13,405 | 8 |

Radio-related flash (Wi‑Fi + BT controller + NimBLE + PHY + WPA + LwIP) is about **670 KB measured**. That is the cost of real radios, not art. It is not a "delete this array" save.

---

## 2. IRAM: what fills it, and what could move

**Measured** IRAM use: 126,923 B of the 131,072 B window, 4,148 B spare.

**Top IRAM objects by file** (map, measured):

| File / lib | IRAM bytes | Why it is there |
|---|---:|---|
| `libfreertos.a(tasks.c)` | 8,545 | Scheduler, must stay |
| `libbtdm_app.a(lld_evt.o)` | 7,005 | BT event ISR path |
| `libesp_ringbuf`, `tlsf` heap, `libphy` cal, `ld_acl` / `ld_fm` / `lld_pdu` | 3–4 KB each | Heap + BT + PHY, must stay |
| SPI flash API / HAL | ~7 KB combined | Must stay (cache is off during flash ops) |
| `rtc_clk`, coexist, BT `rwbt` / `rwble` | 2–2.6 KB each | Must stay |

**Top IRAM symbols** (measured): `r_lld_evt_restart` 2,559, `r_rwble_isr` 1,964, `r_rwbt_isr` 1,765, `r_lld_evt_schedule` 1,514, `r_lld_evt_end` 1,310, `tlsf_memalign_offs` 1,263, **`_tzset_unlocked_r` 1,145**, `r_lld_pdu_rx_handler` 1,140, `tlsf_realloc` 973, **`mktime` 897**, `tx_pwctrl_cal` 894, `lld_pdu_tx_prog_llcp` 878, `tlsf_malloc` 814, `tlsf_free` 797, **`__sfvwrite_r` 693**, `rtc_clk_cpu_freq_set_config` 695, **`localtime_r` 587**, plus the Wi‑Fi/BT ISRs and `i2c_master_isr_handler_default` 628.

App code in IRAM is essentially none (`lv_ui.cpp.o` contributes 78 B). The live UI already runs from flash.

**Candidates that could *maybe* move to flash** (measured sizes; this is a review list, not a recommendation to move them blindly):

| Symbol | IRAM bytes | Comment |
|---|---:|---|
| `_tzset_unlocked_r` | 1,145 | libc time; unusual in IRAM |
| `mktime` | 897 | same |
| `__sfvwrite_r` | 693 | libc stdio |
| `localtime_r` | 587 | libc time |
| **Together** | **3,322** | Only move after proving no IRAM / cache-disabled caller. If any of these is used from a flash-op or ISR path, they must stay. |

Do **not** move: FreeRTOS, tlsf, SPI flash, PHY, BT/Wi‑Fi ISRs, coexist, `i2c_master_isr_handler_default`.

---

## 3. DRAM and heap

### 3.1 Static DRAM (measured)

`.dram0.data` 27,332 + `.dram0.bss` 96,992 = **124,324 B**. PIO 37.9%. `.bss` is at its limit (the 9a3b3b8 commit moved the LOGbot tables to flash and the log ring to the heap for that reason).

**Largest DRAM objects** (measured):

| Symbol | Bytes | Section | Role |
|---|---:|---|---|
| `s_buf` | 16,384 | bss | Inflate dest (`LK_INFL_CAP`). One lock chunk or one CONTROLbot strip. |
| `s_infl` | 10,992 | bss | ROM `tinfl_decompressor` state |
| `port_IntStack` | 4,192 | data | FreeRTOS / Xtensa interrupt stack |
| `g_cnxMgr` | 3,880 | bss | Wi‑Fi connection manager |
| `mo_emb_cache` | 3,528 | bss | BOTpheus emblem decode cache (same size as `MO_EMBT_OFF`) |
| `lv_tty_text::lg` | 3,200 | bss | TTY glyph scratch |
| `lv_tl` | 3,120 | bss | TTY lines (`LV_TLINES` 80) |
| `lv_hc_w` | 2,908 | bss | HOT/COLD hunt working state |
| `lv_f` / `lv_tmp` / `s_radio_batch` / `s_radio_work` | 2,496 × 4 | bss | Scan results + radio worker batches (`LV_MAXF` 48) |
| `lv_rd_tw` | 2,304 | bss | Radar towers |
| `vflash_mem` | 2,048 | bss | BT controller |
| `mo_emb_at` | 1,764 | bss | Emblem occupancy |
| `s_line` + `s_pal` | 480 + 512 | bss | One RGB565 line + clip palette |

`lv_ui.cpp.o` alone owns 64,518 B of `.bss` (measured from the map) — the render/tty/radio/cache buffers above.

### 3.2 Runtime heap (measured from the source of this same build; not a device run of v1.2.26)

These are allocated on the heap, not in the 124,324 B static total:

| Buffer | Size | When |
|---|---:|---|
| Radio reserve, kept free | **105 × 1024 = 107,520** | `lv_ui.cpp`: grow DMA/band only while this remains |
| Radio worker task stack | 6,144 | pinned to core 0 |
| DMA bounce (2 buffers) | 2 × 240 × rows × 2 B, rows tried 48 / 32 / 16 | only if the reserve still fits. 2×48 rows = 46,080 B; 2×16 rows = 15,360 B |
| Render band `lv_bandbuf` | 12 or 24 rows, ping-pong (2×) if possible | 2 × 24 × 240 × 2 = 23,040 B at the large size |
| `lv_shadow` | 32,768 | first tint; **vacated before radio init**, then re-allocated on the next tint |
| `lv_port_under` | `LV_PORT_MAXGLOW` × 2 = **19,190** | glow underlay, first port-layer screen |
| CONTROLbot live panel `lv_cb_pan` | 21,584 | malloc, with a scanline fallback if it fails (v1.2.26 item 3) |
| c17 / LOGbot panel buffer | 21,584 | heap, first panel draw, never freed in v1.2.25 notes |
| Pager slide cache | up to `LV_PG_SLIDE_CAP` 45,056 | under a tight HOME heap |
| LOGbot ring `lv_log_r` | 64 × 44 = **2,816** | calloc at the first `lv_log_push` (boot) |

A v1.2.11 comment in `lv_ui.cpp` cites a field fps-log line of `heap=41292 largest=29684` once the UI is running with radios. That is **not** a v1.2.26 measurement; treat it only as "HOME heap after radios is tens of kilobytes, not the 203 KB theoretical leftover."

**Consequence for compression:** a dest larger than `s_buf` (16,384 B) is not a safe default. One 240×320 RGB565 frame is 153,600 B and will not fit. The schemes that already work (`LV_CB_Z` strips of 7,680 B, lock chunks ≤ 16,352 B) are the right size.

---

## 4. Lossless compression (measured)

Method: take the exact flash bytes of each symbol from `.flash.rodata` / `lock_blob_live.bin`. Compress with zlib-9 (zlib wrapper and raw deflate), LZ4 block-12, and a simple byte RLE. For assets that are already raw deflate, also inflate every official chunk/strip with raw inflate (`zlib.decompress(..., -15)`, the same contract as `tinfl` with `TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF`).

For **already-deflate** data, a second zlib pass is useless (~1.02–1.03×). For **uncompressed** data, a single zlib stream of the whole array is the tightest, but the dest would be the full array (often 20–115 KB), which does **not** fit `s_buf`. So the practical number is **chunked raw deflate in 8,192 B pieces**, plus a small offset table. That dest fits `s_buf`. Those are still measured.

### 4.1 Already compressed (no meaningful further save)

| Asset | Flash | Inflated (sum) | Ratio | Max dest | Second zlib-9 |
|---|---:|---:|---:|---:|---:|
| `LV_CB_Z` 27 stills | 213,778 | 4,147,200 | 19.40× | 7,680 | 208,276 (1.03×, not worth it) |
| `lk_blob_live` | 315,100 | 2,005,067 | 6.36× | 16,352 | 309,701 (1.02×, not worth it) |
| `LV_PG_FACE_Z` | 2,207 | 3,520 (one stream; already tight) | ~1.6× | — | 2,126 (1.04×) |

RLE made these slightly larger. They are already the right encoding.

### 4.2 Uncompressed art — whole-array zlib (tightest, dest = full array, usually too big)

| Asset | Flash | zlib-9 | Ratio | Save | Entropy | Fits 16,384 dest? |
|---|---:|---:|---:|---:|---:|---|
| `LV_PORT_HOME_GLOW` | 115,140 | 2,463 | 46.75× | 112,677 | 3.12 | **No** (115,140) |
| `LV_PORT_SCAN_RAIN` | 49,000 | 2,215 | 22.12× | 46,785 | 2.46 | No |
| `LV_PORT_TERM_RAIN` | 42,080 | 2,119 | 19.86× | 39,961 | 2.54 | No |
| `LV_PORT_SCAN_GLOW` | 36,348 | 1,182 | 30.75× | 35,166 | 3.39 | No |
| `LV_PORT_HOME_RAIN` | 24,000 | 431 | 55.68× | 23,569 | 2.34 | No (24,000) |
| `LVC_VAR` | 37,550 | 2,051 | 18.31× | 35,499 | 3.37 | No |
| `LVC_BASE` | 27,594 | 4,230 | 6.52× | 23,364 | 3.79 | No |
| `LV_TNR_RGB` | 26,496 | 487 | 54.41× | 26,009 | **0.17** | No (26,496) |
| `LV_PORT_PAL` | 18,864 | 3,054 | 6.18× | 15,810 | 5.55 | No |
| `LV_PORT_LUTS` | 10,752 | 829 | 12.97× | 9,923 | 7.99 | Yes (10,752) |
| `LV_HC_FACE` | 4,418 | 329 | 13.43× | 4,089 | 4.21 | Yes |
| `LV_GL_P0_DAT` | 41,579 | 9,218 | 4.51× | 32,361 | 5.76 | No |
| `MO_EMB` | 11,520 | 1,345 | 8.57× | 10,175 | 1.62 | Yes |
| `LV_ICON_ART` | 4,896 | 834 | 5.87× | 4,062 | 2.64 | Yes |

LZ4 is consistently looser than raw deflate here (e.g. HOME_GLOW 4,955 vs 2,457). Simple RLE is poor on these (HOME_GLOW RLE 87,281). Deflate is the one to use; the ROM already has `tinfl`.

Whole-group one-stream (measured, dest = the group — too big unless split):

- All `LV_PORT_*` 405,264 → 59,307 (6.83×), save **345,957**
- All `LV_GL_*` 118,917 → 41,631 (2.86×), save **77,286**
- `LVC_VAR`+`LVC_BASE` 65,144 → 6,327 (10.30×), save **58,817**

### 4.3 Same assets, 8,192 B chunks (dest fits `s_buf`) — the practical number

Measured raw-deflate + a 4-byte offset per chunk:

| Asset | Flash | Chunked deflate | Ratio | Save | Chunks |
|---|---:|---:|---:|---:|---:|
| `LV_PORT_HOME_GLOW` | 115,140 | 3,590 | 32.07× | 111,550 | 15 |
| `LV_PORT_SCAN_RAIN` | 49,000 | 3,213 | 15.25× | 45,787 | 6 |
| `LV_PORT_TERM_RAIN` | 42,080 | 3,439 | 12.24× | 38,641 | 6 |
| `LV_PORT_SCAN_GLOW` | 36,348 | 2,028 | 17.92× | 34,320 | 5 |
| `LV_PORT_HOME_RAIN` | 24,000 | 550 | 43.64× | 23,450 | 3 |
| `LV_PORT_PAL` | 18,864 | 3,168 | 5.95× | 15,696 | 3 |
| `LV_PORT_LUTS` | 10,752 | 1,281 | 8.39× | 9,471 | 2 |
| `LV_PORT_HOME_C0` | 38,306 | 15,276 | 2.51× | 23,030 | 5 |
| `LV_PORT_SCAN_C0` | 36,252 | 10,419 | 3.48× | 25,833 | 5 |
| `LV_PORT_TERM_C0` | 15,288 | 4,057 | 3.77× | 11,231 | 2 |
| `LVC_VAR` | 37,550 | 3,368 | 11.15× | 34,182 | 5 |
| `LVC_BASE` | 27,594 | 4,782 | 5.77× | 22,812 | 4 |
| `LV_TNR_RGB` | 26,496 | 559 | 47.40× | 25,937 | 4 |
| `LV_GL_P0_DAT` | 41,579 | 14,016 | 2.97× | 27,563 | 6 |
| `LV_GL_BASE_DAT` | 22,098 | 9,497 | 2.33× | 12,601 | 3 |
| `MO_EMBT_BITS` | 27,378 | 11,674 | 2.35× | 15,704 | 4 |
| `MO_EMB` | 11,520 | 1,447 | 7.96× | 10,073 | 2 |
| `LV_HC_FACE` | 4,418 | 331 | 13.35× | 4,087 | 1 |
| `LV_ICON_ART` | 4,896 | 836 | 5.86× | 4,060 | 1 |

`LV_PORT_HOME_C1` / `C2` (5,618 B each) only shrink to ~4,390 (1.28×); small.

**Runtime fit:** every row above inflates into ≤ 8,192 B, which already fits `s_buf` (16,384) and the existing `tinfl` state (10,992 B static). No new DRAM buffer is required if decode goes through the same `lv_infl` path. Glow/rain are random-access today, so a chunk cache (one 8 KB piece, or a handful) would replace direct array indexing. That is a code change, not a look change, if the inflate is lossless.

---

## 5. Unused or dead items (measured bytes)

These are the only things I would call dead, leftover, or "only barely used," with exact sizes.

| Item | Bytes | Verdict |
|---|---:|---|
| Full (non-live) lock blob | 1,528,847 dropped already | **Already gone** from this bin. `lock_blob_live.json`: live blob 315,100 B, `dropped_bytes` 1,528,847. Nothing to do. |
| Sample SSIDs in `lv_lb_row` (`CafeGuest5G`, `xfinitywifi`, …) | 24 B for the SSID table symbol + ~70 B of names | Used only by the **locked** LOGbot *pager plate* (the five SAMPLE rows + `EX.DATA` on that plate were deliberately left). Not the v1.2.26 subscreen, which is a real log. Removing them would change that locked plate. |
| Other `EX.DATA` UI format strings (glossary pillars, HACKbot slots, …) | 397 B total for every `EX.DATA` / sample SSID / `BOOT v1.2.26` string in `.flash.rodata` | Almost all of this is locked copy on other screens. Not a flash win, and not removable without a look change. |
| `chip-debug-report.cpp.o` (Arduino `printBeforeSetupInfo`) | **4,050** (2,058 text + 1,992 rodata) | Linked because Arduino `main` always prints a boot dump. Not needed for the live UI. Removable only by changing the Arduino startup, not by deleting a BOTCORPS array. |
| mbedTLS Camellia (`camellia.c.obj` + the cipher_wrap table entries) | **4,127** (2,823 text + 1,304 rodata) | Pulled in by mbedTLS `cipher_wrap.c`, not by any BOTCORPS call I can see. Wi‑Fi/WPA on this device does not obviously need Camellia. **Likely unused by the app; not proven unused at runtime.** A custom mbedTLS cipher list could drop it. |
| Old TERMINALbot typist path / `LV_TNR_RGB` | typist code in nm ~5,163; `LV_TNR_RGB` 26,496 | **Not dead.** G2 is the TERMINALbot screen (`LV_TPS_G2_ONLY=1`), but the source keeps the old `LV_SC_TERM` path and TNR still applies when that screen is opened. TNR is a packing candidate (section 4), not a delete candidate. |
| Games (HOT COLD, DOWNLOAD, inside-towers, typist G2) | hunt/download ~26,312 incl. 2,908 bss + 4,418 face; typist ~5,163 | **Used** (HACKbot / TERMINALbot / Dojo). Not dead. |
| Unused fonts | — | **None of any size.** Only `LV_GLCD`, `LV_F35`, and LovyanGFX Font0 are linked. |
| mbedTLS SHA-512 (~5 KB class) | esp_sha512 / sha512 objects ~5,000 | Pulled by `bootloader_sha` and `esp_srp`. Do not treat as dead. |
| 27 CONTROLbot stills | 213,778 already-deflated | All 27 slots are mapped (`LV_CB_N` 27, `LV_CB_FW[]`). Some names were re-inked in v1.2.22, but the stills are still the locked bases. Not dead. |
| libc `printf` / `scanf` (`_vfprintf_r` 11,520, `_svfprintf_r` 11,305, `_ssvfiscanf_r` 7,600, `_dtoa_r` 3,289) | ~33,700 text | Used by `Serial.printf`. Not dead. |

There is no large "excluded game still linked" blob. Feature flags for the games and inside-towers are on in this build, and the symbols are the live screens.

---

## 6. Ranked consumers, then the top savings

### Biggest flash consumers (measured)

1. App art + clips + stills + palettes in `lv_ui.cpp` / `lock_blob_live` — about **1.27 MB** of rodata, of which ~529 KB is already deflate (`lk_blob_live` + `LV_CB_Z`) and ~400 KB is uncompressed port-layer data.
2. Radio stacks (Wi‑Fi + BT + NimBLE + PHY + WPA + LwIP) — about **670 KB** of the flash image. Not compressible art.
3. IDF / libc / mbedTLS / LovyanGFX / Arduino / FreeRTOS — the rest of the code.

### Biggest DRAM consumers (measured)

1. Static render/radio/tty/cache in `lv_ui.cpp` `.bss` (64,518 B), topped by `s_buf` 16,384 + `s_infl` 10,992.
2. Runtime: 105 KiB radio reserve, DMA/band buffers, optional 32 KiB tint shadow, 19 KiB glow underlay, 21 KiB panels.

### Top savings options (measured, lossless, dest fits the existing 16,384 B `s_buf`)

These keep locked looks pixel-for-pixel if the bytes inflate back identically (raw deflate via the ROM `tinfl` already in the firmware). They need a chunk table and a fetch-through-`lv_infl` in place of direct array indexing. That is a later code change; this survey does not do it.

1. **Port-layer rain + glow** (`HOME_GLOW`, `SCAN_GLOW`, `HOME_RAIN`, `SCAN_RAIN`, `TERM_RAIN`): 266,568 → 12,820 chunked deflate, save **253,748 B**. Best dollars-per-line. Glow is 9,595 records; keep a one-chunk cache so you never inflate the whole 115 KB.
2. **The rest of the port layer** (palettes, LUTs, clip-index `C0`/`C1`/`C2`): about 119,462 → ~34,200, save ~**85,000 B**. Together with (1), the whole `LV_PORT_*` set is ~405 KB → ~58 KB chunked, save on the order of **340 KB** (the one-stream figure 345,957 is the ceiling if dest were free; chunked is a few KB looser).
3. **CALbot `LVC_VAR` + `LVC_BASE`**: 65,144 → 8,150 chunked, save **56,994 B**. Access is already "decode this row/element," so a deflate chunk is a natural fit.
4. **`LV_TNR_RGB`** (old TERM norain): 26,496 → 559, save **25,937 B**. Almost empty. Still used if the old TERM screen opens.
5. **Glossary large arrays** (`LV_GL_P0_DAT` + `LV_GL_BASE_DAT` at minimum): 63,677 → 23,513, save **40,164 B**. The whole glossary group one-stream save is 77,286 B if you are willing to split all of it.

**Options 1+3+4 alone, chunked, measured: 253,748 + 56,994 + 25,937 = 336,679 B** off the app image, dest always ≤ 8,192 B. Adding the rest of the port layer and the two glossary arrays pushes the realistic total toward **~450–470 KB**. The app-only bin would fall from 2,716,336 to roughly 2.25 MB in that case (**estimated** from the measured chunk sizes; it does not include a small amount of new decoder/index code, which is tiny next to the art).

**Not worth doing:** another deflate pass on `LV_CB_Z` or `lk_blob_live` (~5–7 KB). Deleting sample `EX.DATA` strings (~397 B) or the SSID table (24 B). Moving IRAM libc (~3.3 KB) without a caller audit. Dropping Camellia or the Arduino boot dump (~4 KB each) is real but small, and needs a framework-config change rather than an art change.

**Do not do:** inflate a whole 115 KB glow array, a whole 405 KB port group, or a 240×320 still into a new DRAM buffer. That dest does not fit the measured HOME heap once the radios are up.

---

## 7. How this was measured

- Sections: `xtensa-esp32-elf-size -A` / `readelf -S` on `firmware.elf` from the 9a3b3b8 `cyd_livescan` build. App size and sha256 from the bin that that link produced.
- Symbols: `xtensa-esp32-elf-nm -C -S --size-sort`, classified by ESP32 address (IRAM 0x40080000, flash text 0x400D0000, flash rodata 0x3F400000, DRAM 0x3FF80000).
- Per-file / per-lib totals: GNU ld map `firmware.map`, allocations under `.flash.rodata`, `.flash.text`, `.iram0.text`, `.dram0.data`, `.dram0.bss`.
- Compression: slices of `.flash.rodata` at the symbol addresses (and `src/lock/live/lock_blob_live.bin` for the clip blob). Already-deflate assets were inflated chunk-by-chunk with raw inflate; every `LV_CB_Z` strip and every live lock chunk succeeded (`ok=540` and `ok=1338`, zero failures).
- Heap/reserve figures are read from the same tree's `lv_ui.cpp` / `lv_core.h` as compiled into this ELF, not from a new device run.

*End of survey. No firmware, no flash, no look change.*
