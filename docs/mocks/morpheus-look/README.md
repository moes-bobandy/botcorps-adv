# MORPHEUSbot "Morpheus look" chip variants (MOCK ONLY)

Mock only: no firmware change, no bins, no flash, no merge. Locked clips untouched.
These are edits on the rename-mock frames (docs/mocks/morpheus-rename) and the idea C frame (docs/mocks/morpheus-pills).
Each edit writes whole 3x3 blocks at 1x and stays inside the chip box. The render script checks this with a diff assert.
The label MORPHEUSbot / BOOTING and the REV C silk are unchanged.

Every variant gets:
- A dark-brown bald head with a dome highlight, ears, a nose shadow and a calm mouth line.
- Solid black round pince-nez lenses with a white glint, a dim grey rim and a thin dark bridge.

The variants build on each other:
- **M1, subtle:** the new face and glasses only. The chip body stays as it is now.
- **M2, medium:** M1 plus an oxblood leather-coat body (3-tone diagonal shading), a dark leather bevel and slant, and coat lapels with a V-neck at the bottom of the screen.
- **M3, full:** M2 plus collar lapel lines on the lower plate (boot size), a black header with a Matrix-green meander and glow, a dark side and bottom frame, and a gunmetal face bezel with a green inner glow line. The pins are unchanged.

Files:
- `M#_boot_f070_3x.png`, `M#_home_f200_3x.png` and `M#_ideaC_pulse_f0100_3x.png` are full 720x960 frames.
- `*_chip_zoom.png` files are chip crops.
- `side_by_side_*` files compare the current BOOTbot with M1, M2 and M3.

Rebuild with `python3 tool/render.py`. It needs mlib.py from the morpheus-rename tool.

Adopting a variant needs a re-render of the locked boot and home clips, a re-lock approved by Mobandy, and a lock_blob rebuild.
lv_core's runtime BOOTbot tint would also need review, because it tints the chip black and grey.
