# Tasks — CYD Touch Port Review

## P0 — Must fix
- [x] Implement full on-screen keyboard for typing (terminal commands, prompts).
- [ ] Map touch zones to navigation: tap icons to select, focus shows label under icon.
- [ ] Ensure no rain behind Terminal; slight rain only on intro.
- [ ] Fold INT ID card fully into EXT without overlap; keep per-bot color badge.
- [ ] Confirm 40 commands in CORE/COMMS/GAME/SYS dropdown, bot replies in bot color.
- [ ] Typing never triggers hotkeys or wipes prompt.

## P1 — Polish
- [ ] Optional BlackBerry-style I2C keyboard support.
- [ ] Visual pixel-match verification against mocks/shots.
- [ ] Calibration for XPT2046 touch mapping.

## Status
Basic single-screen compile succeeds. Drawing code reused. Touch stub (tap = OK) added. Basic OSK (bottom keyboard on terminal tap) implemented. Zone mapping and rain rules next.
