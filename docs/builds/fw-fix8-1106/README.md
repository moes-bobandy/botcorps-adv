# CYD fix8 verification (2026-11-06)

Preserves Items 1,2,3,4,6,8 and adds off-core warm radios + graphics throughput (5,7).

- `regression.json`: all 14 livescan scripts plus item8, saved/missing calibration, 90 successful runs across pre-Slice-B/default/64-row simulators. 558 output files, no unintended differences (including recordings).
- `targeted.json`: short-charge stays unlocked; full charge locks; 206 dual-meter hunt snapshots, zero default/64-row differences.
- `verify_glossary.json`: all 26 locked screens and 85 boot frames zero difference; HOME_INTRO count zero.
- `performance.json`: unmodified gate render score unchanged 1042/1057; four unrelated firmware RAM ELF inputs absent (overall exit 19). Shadow peak 29943/32768; shadow stores tint-region pixels, not a band-sized allocation.
- Exact C libm float32 check: 10752/10752 LUT bytes match original `powf` loop. DMA behavioral mock passes caller lifetime, wire order, coalescing, fallback and idle-before-CS tests. LCD `waitDisplay()` is a no-op in this LovyanGFX version, so firmware explicitly calls `waitDMA()` too.

ESP32 bands and both DMA strips are heap-backed. Persistent shadow, glow-underlay and radar-route scratch plus the Core 0 task are allocated before selecting graphics sizes; 105 KiB actual internal 8-bit heap is reserved. Bands try 64/48/32/24 rows, strips try 48/32/16 rows. Strip DMA can fall back to synchronous drawing if headroom does not permit two buffers. No speculative runtime heap value is claimed: read `[GFX-RAM]` on the CYD before accepting hardware performance.

Host dependencies were isolated outside the repository. Gate used unmodified scripts, Zig Clang in place of absent g++, and `HOST_EXTRA=-fconstexpr-steps=100000000`. Record-producing simulator checks were serialized because the existing sim uses shared `/tmp/.sim_rec_part`.
