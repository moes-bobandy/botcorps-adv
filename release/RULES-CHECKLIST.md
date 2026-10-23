# BOTcorps Adv — Dirt's rules checklist (v1016f-4)

Dirt's rules OVERRIDE the mock wherever they conflict.

| # | Rule | Where it applies |
|---|------|------------------|
| R1 | ~~Gibson is ALWAYS a square control-panel block (`chip_block`) in every chip-form view — including the hub/scan-menu centre core. No round disc or ring anywhere.~~ **SUPERSEDED** by release/SPEC-LOCKED.md: the hub centre is the moving rings core (H1); the Gibson square chip stays on the scan board and game boards. | intro, hub/scan menu, scan board |
| R2 | Black background inside Gibson's square (and inside every bot chip). | all chip views |
| R3 | Black chip border like the control panel, with a subtle silk outline so the edge reads on a black PCB. Single shared `chip_block()`. | all chip views |
| R4 | ALL bots inside chips fit the square exactly like Gibson, bot filling the square, each keeping its FIXED colour. | scan board, radar cards |
| R5 | Board colour is PERMANENT MATTE BLACK (ESP32 shield: black PCB, white silkscreen, black 3-pin headers with gold pins, silver WROOM) — FIXED regardless of UI colour. Applies to scan board AND settings board. | scan board, settings |
| R6 | Rain behind every screen that is not a solid panel (shown behind the boards via an inset). | intro, hub, settings (device); scan/terminal are panels |
| R7 | Crown is the ONLY back control (crown = back); terminal button present on boards. | all |
| R8 | No instruction/hint text. | all |
| R9 | Bot colours are fixed per bot. | all |
| R10 | ID badges stay on bot cards. | bot cards |
| R11 | Keyboard only (no touch targets added). | all |
| R12 | Tab hard-edge is locked (sheet opens with a hard edge). | overlays |
| R13 | Settings keeps all 8 settings + NVS values, keyboard-reachable. | settings |
