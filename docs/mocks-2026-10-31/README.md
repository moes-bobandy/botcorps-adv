# CYD livescan mocks, 2026-10-31 (NOT firmware)

The live firmware changes (no-flash theme taps and the 12 restored themes) are in commit 7ecb753.
Everything in this folder is a mock and pending approval. None of it is built into the firmware.

- `ctl-pictures/`: Issue 3 + steer (A). Each CONTROLbot chip value rect gets a picture per choice instead of text.
  - UI-changing chips use a tinted plate with full-bright art. Non-UI chips (FOCUS, MIX, MULTI) use a grey plate with dimmed art.
  - There is no white selection border.
  - Each chip's art is pasted onto a real sim frame, in exactly the removed rect.
  - The scripts are `chips.py`, `mock.py`, `sheets.py` and `vid.py`.
  - (A) BG STYLE and BG COLOR are shown as a proposal in the RAIN and RAIN COL rects.
- `hunt-banner/`: Issue 5. While a GIBSON hunt is active, the ticker becomes a fixed hunt bar.
  - It is a real sim render from a scratch patch (`hunt_banner.patch`), not applied to `src/`.
- `tty-cheatsheet/`: Issue 4. A CMDS key on the TTY keyboard opens a list of the real parser commands.
  - It is a sim render from a scratch patch (`cmds_cheatsheet.patch`), not applied to `src/`.
- `themes/`: a contact sheet of the 12 themes as built in 7ecb753.
- `flicker-evidence/`: Issue 1 before/after frame-watch evidence.
