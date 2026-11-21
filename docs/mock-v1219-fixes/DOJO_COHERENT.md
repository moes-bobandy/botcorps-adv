# Dojo idle no-tear / no-split (LV_MO_COHERENT_PRESENT)

HOST SIM ONLY, no bin. Own droppable flags on mock/v1219-fixes-2026-11-21.

## What Mobandy saw

The towers "split the screen" **as they move about**, not when a pillar comes forward.
Idle camera sway paints a new pillar pose every frame, but the panel is updated in dirty-row
SPI windows (~14–16 windows, ~22 ms of 40 MHz SPI at K=1; ~90 ms on the CYD). A mid-push
sample of the ILI9341 GRAM (the device-timeline camera in `tools/livescan/dev_model.h`,
every 20 ms) therefore shows **the top of one pose and the bottom of the previous one**.
That is the split. Faster frames were already tried in the perf-all pass; they shrink
the window, they do not remove mixed geometry. DMA/SPI still races the visible GRAM
because each band is pushed as soon as it is drawn.

## The fix

Flag `LV_MO_COHERENT_PRESENT` (default 1) in `src/live/lv_morpheus_v13.h`.

Each Dojo frame (`itw_mo_frame`, idle full-field and drop-only) is wrapped in
`lv_present_begin` / `lv_present_end`. The host sim wires those hooks to
`dev_present_begin` / `dev_present_end`. The panel model **holds the last completed
image until the group's last SPI byte has landed, then latches every window of that
frame at once**. Mid-burst camera samples are never a mix of old and new pillar geometry.
Composed pixels are unchanged (the fence is a present rule, not a redraw).

On the real CYD the hook is currently null (no second GRAM / no 142 KB field buffer in
DRAM). The firmware now *declares* the present fence so a later flashed path can flip
one back buffer at present-complete. This sim proof is of that contract, not of a
flashed board. The ILI9341 will still show mid-write GRAM until that path exists.

## Pull-forward speed (separate flag, same commit)

`MO_PUSH_FAST` (default 1) sets `MO12_PUSH_MS` from 400 to 240. The ease law and the
u=0 / u=1 poses are the same; only how far the ease has travelled at a given wall
time during the pull changes. Folder drop still keys off time-since-tap (`MO12_DROP0`),
not off `PUSH_MS`. Close duration is unchanged.

## Gates

See `gate_dojo_coherent_vs_c70ef04.txt`. Idle, open-done and settled completed frames
are 0 px vs c70ef04. Device-timeline idle: 21/36 mixed cam frames before the fence,
0/36 after. Mid-pull at 180 ms after tap differs (27367 px) as intended.

Renders: `/workspace/out/fw-bigbot-1117/v1219-fixes/dojo-coherent/`.
