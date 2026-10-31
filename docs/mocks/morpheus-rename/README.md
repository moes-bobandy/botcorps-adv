# MORPHEUSbot rename mock (BOOTbot -> MORPHEUSbot). MOCK ONLY.
No firmware, no bins, no lock-store change. Frames come from the LOCKED clips (read-only):
boot = bootbot_bg_v2@3x.mp4 f0-84, home = gibsonbot_bg@3x.mp4 f6-479. Each 1x pixel is a 3x3 block, so edits copy blocks from the same frame.
- Boot chip label: GLCD 5x7 "BOOTbot" -> "MORPHEUSbot", same ink, centered at x119 (x87..151, y155). Silk "BOOTBOT REV C" -> "MORPHEUSBOT REV C", tiny 3x5, right-aligned at x223.
- Home chip: the label does not fit on one line (43 px vs about 35 px), so it is two tiny lines "MORPHEUS" / "bot", following the hub12 TERMINAL/BOT rule. The intro "BOOT" GLCD label also becomes two tiny lines.
- Glasses: pince-nez (two round dark lenses tinted with the face shade, white rim, glint, short bridge, no arms). Big version 9x9 on the boot face, small version 7x5 on the home chip.
- Diff check: every pixel that differs from the original frame is inside the label, silk or glasses box. That holds for all 85 boot and 540 home frames.
- Adoption needs a re-render of both clips, a re-lock approved by Mobandy, and a lock_blob rebuild.
