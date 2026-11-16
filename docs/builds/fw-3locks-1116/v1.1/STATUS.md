# BOTcorps CYD v1.1 (fw-3locks) — status, not flashed, not merged
Source: 907c765 (on top of ada2253, 9bc3569). Env cyd_livescan.
- BOTcorps-CYD-v1.1.bin (app-only, 0x10000): 2,633,184 B, sha256 1663d60f0b3c7716bf87c714ccf2d5938f591d465346c441bfd5f9426e65aa92 (limit 2,686,976)
- BOTcorps-CYD-v1.1-full.bin (merged 0x0): 2,698,720 B, sha256 32b4802bb0099c3a203a1cc1d11251d5dde4deedd3e59bc723428a528c20d6d4
## Done
- Home MORPHEUS chip -> BOTpheus EB3 (006aa03). Big slot -> HACKbot game (3503679/856709a). The label reads HACKbot (61 px, inside x46-68 y241-247 only).
- CONTROLbot v6 panel: taps snap to the nearest block, 150 ms debounce, and the tapped block flashes inverted for 200 ms (that block only).
- gate.sh 1042/1057, identical to the c1b5659 baseline. The 24-theme v6 route diff is 0 px.
## Pending (mock first, not half-shipped)
- Theme the BOTpheus bottom terminal board, the HACKbot board and the radar chrome.
- Clone the v6 coil/pin pattern code onto the home and other boards.
- The old 12-theme carry-over (7ecb753/3240b03 multi-ink re-ink) is intact. Themes 0..11 are byte-identical to 049ff59 on home/term/scan/radar/tty/gloss.
