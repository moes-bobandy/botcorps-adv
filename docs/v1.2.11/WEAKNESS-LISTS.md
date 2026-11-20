# v1.2.11 sub-screen weakness lists (MOCK ONLY review; no firmware change implied)

Measured in QEMU (240 MHz approx., no radio, SPI bus time not modelled) + sim_live real renderer.

## DOJO (MORPHEUSbot Construct)
1. Heaviest screen: ~93 ms idle / ~148 ms open per frame (QEMU, v1.2.11; v1.2.10 was 194 / 285). Still ~7-10 fps on the CYD.
2. First open frame stalls ~0.7 s (close-up emblems built on demand). v1.2.11 no longer lets that stall skip the fly-in, but the freeze itself remains.
3. Before v1.2.11: tap = hard cut to tower + folders (push swallowed by slow frames) — FIXED (dt-capped Dojo clock).
4. Before v1.2.11: folders were decoration ("mo-open") — FIXED (tap = flash + open frame + footer facts).
5. Folder facts are text only: no action (e.g. PROTECT has no "do it" path); footer is the only detail surface.
6. No hardware-key style navigation: pillars only by touch, small far pillars hard to hit even with the nearest-centre assist.
7. Code rain + parchment wash are recomputed every frame even when the camera is still (no dirty-region reuse).

## GIBSON radar
1. Before v1.2.11: lock showed HUNT/IGNORE/WATCH as 11 px tall deck buttons (46x16 targets, below the 36-40 px resistive floor) — v1.2.11 moves them into 151x66 folders.
2. Centre stack replaced by SPEED (1X/2X/3X) per Mobandy; speed only scales the scene clock (camera sway / rain), not scan rate.
3. Advance is a camera pan (DOWNLOAD law), not the Dojo's zoom-in; the tower does not grow.
4. Hunt meter still lives on the charge route; folder shows read #, dBm, WARMER/COLDER/STEADY, level bar.
5. Rocker is L/R only: no up/down, so no way to step folders without touch.

## HACKbot (SCANbot DOWNLOAD)
1. No d-pad / A-B: every action is a touch on a tower or folder; game feel is weak.
2. Open frame 46 ms (v1.2.10 101 ms) but intro typing screen is fine; open-tower full redraw ~150 KB/frame.
3. Folder targets 152x59 are fine; tower targets are narrow when towers are far.
4. Feedback for wrong file = red banner only; no haptic-like press state on folders.

## Proposed (mock only, see *.gif / *.png here)
- HACKbot: theme-coloured game d-pad (UP/DOWN/LEFT/RIGHT) + A (OPEN/DOWNLOAD) + B (BACK) in the deck, 40x40+ targets.
