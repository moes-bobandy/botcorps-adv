v1.2.20 BUILT, NOT FLASHED, NOT UPLOADED, SIM-GATED (not yet tested on the CYD). Mobandy OK 4:11 PM ET 11-24 ("then build").
Built from build/v1.2.20-2026-11-24 (15d498e), env cyd_livescan, BUILD_ID v1019z 15d498e. Base: mock/v1220-hitlist-2026-11-24 (5483e3b).
On: item1 gold TERMINALbot + face rain, item1b board rain removed (themes 0-2) + 3x5 face rain, item2 banner band-split + slide 233->100 ms,
    item3 GIBSON independent towers + banner name/colour, item4 Dojo white strip, item5 HACKbot SPEED + independent towers, item6 self-destruct.
OFF: TERMINALbot strip (LV_TERM_STRIP 0): no free band to move it to without covering the hunt-card labels / mini terminal (Mobandy to pick).
App-only (OTA/Launcher, never USB-flash at 0x10000): BOTcorps-CYD-v1.2.20.bin  2775664 B  sha256 91140841ded7c2795c1276215f7672fc47e41deb2d6ac97c5cf11ea75e705af4
Full (factory image, flash at 0x0): BOTcorps-CYD-v1.2.20-full.bin  2841200 B  sha256 e5c42a8045d3d283ee697360c0820f149da75c698e0b4cdf6d1f64c18f5b7e1d
Link fix: item1's 4.6 KB static face buffer overflowed dram0_0_seg by 4,192 B; moved to heap (5483e3b), sim 0 px.
RAM 37.9% (124164 B), Flash 88.2% (2775155 / 3145728 B).
