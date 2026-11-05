# LOCKED: ui_gibson_radar_hunt_meter_interact (Gibson radar current-to-meter interaction, V2 ARC)

- **Locked:** 2026-11-05 3:51 PM ET. Mobandy picked "Lock V2 arc and band ripple" on "Which current-to-meter interaction should I keep?" (following his 2026-11-05 12:51 PM ET direction: "Yes make sure the electric current is connecting and interacting while the tower hunt meter is o[n]").
- **Source:** `057279f21ba76286061194de1ed313b9a10de93f` on `backup/cyd-livescan-2026-10-30` (botcorps-adv), `docs/mocks/radar-current-meter-interact/V2/`.
- **This is a mock only.** No firmware was built for the hunt meter or this interaction, and nothing was flashed. Porting it into firmware needs Mobandy's word.
- Adds to the locked `ui_gibson_radar`, `ui_gibson_radar_charge` (D4, `bd10318`), and `ui_gibson_radar_hunt_meter` (A thermometer bands, `0320479`). Those locked pieces and the rest of the radar are unchanged (0 px changed outside the hunted tower and contact area across all 200 sim frames).

## What is locked
- **V2 ARC (arc + band ripple):** a soft ice contact port glows where the locked D4 current meets the right edge of the hunted tower's glass and breathes with D4.
- Every arriving D4 packet (every ~180–200 ms) throws a short stepped D4-style ice arc (1x grid, no forks, ~120 ms) from the contact port into one of the lowest three green bands.
- Those low green bands flash and a brightness ripple climbs the lit thermometer stack (25 ms per band). A new band lights when a packet's ripple reaches the top; once the stack reaches the signal level, arriving packets flash the top band.
- Brightness stays below the top banner's peak (~203 luma): pixels outside the glass stay under D4's 176 ceiling, and band flashes inside the glass are capped at 200 luma.
- Clip: `V2_3x.mp4`, 180 frames, 6.0 s, 30 fps, 720x960 (3x), H.264 yuv420p faststart, sha256 `34a177a72758c64ddf18b9839b8270aa1a5d8ecc274871e1c416f4a9d8cb09cc`.

## Rules
- Never change this screen; only add.
- Keep `ui_gibson_radar_charge` (D4) and `ui_gibson_radar_hunt_meter` (A thermometer bands) unchanged.
