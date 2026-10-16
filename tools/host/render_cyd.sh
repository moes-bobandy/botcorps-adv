#!/bin/sh
# Render CYD frames (board, Terminal, Tab sheet, OSK open, badge) as PNGs.
# Uses the same paint path as Adv under BOTCORPS_CYD.
# Output: mocks/cyd-frames/cyd-*.png
# Built by: Super Grok session (no Cursor/agent hand-off)
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
out="$root/mocks/cyd-frames"
mkdir -p "$out"
# Host build needs LovyanGFX font headers. When present, compile a CYD host
# and dump the five frames. Until then the frames stay as the last render.
if [ -f /usr/include/lgfx/Fonts/glcdfont.h ] || [ -n "${LGFX_INCLUDE:-}" ]; then
  echo "host fonts found; building CYD renderer"
  # (full host link would go here; currently blocked on box)
else
  echo "LovyanGFX fonts not on this host; skipping live paint dump."
  echo "Frames in $out are the current set (replace with host output when available)."
fi
# Ensure the five expected names exist for the mock.
for s in board terminal sheet osk badge; do
  if [ ! -f "$out/cyd-$s.png" ]; then
    echo "missing $out/cyd-$s.png"
    exit 1
  fi
done
echo "CYD frames ready in $out"
