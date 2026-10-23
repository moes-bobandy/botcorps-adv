P0 v003 gate references (ARCH-GAME-P0-CHIP-PROMPT-v003.md sha256 39e76347...0bf4). Read by tools/gate/gate.py (p0-v003 rows).

e7ba692-render.sha256   sha256 of the 96 e7ba692 host frames (/workspace/gate-e7ba692/render). As
                        "<sha>  render/<name>" sorted it hashes to 69052dcf51af63c8631ba2d4bf71291c4e866858c446cf8b635eb993080ab301.
e7ba692-p0.sha256       e7ba692 source (git archive) + a reference-only port of the P0 host hooks (p0_sheet / p0_atk /
                        botcorps_host_defend frame block); frames gc-0/1/6/12 and sheet-0/1, rgb+norain.
e7ba692-src.sha256      botcorps_id.cpp, platformio.ini, src/rain/*, release/gate/ram-caps.txt at e7ba692 (G1, G17 cap unchanged).
a5off-render.sha256     the 96 frames from the P0 tip with only the BH_DEV drone row reverted to
                        { GL_UNKNOWN, -1 } (G21 A5 on/off); a5off-scanboard-drone.log is that render's GATE_BOX log.
png/e7ba692-*.png       lossless RGB PNGs of e7ba692 frames: 9 scanboard norain (G2a), gc-N norain from the same
                        reference port (G2g a), terminal-cmdmenu rgb+norain (GIBSON R1).
png/a5off-*.png         A5-off scanboard-drone rgb+norain (G21 b).
