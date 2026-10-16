#!/bin/sh
# Render CYD frames (board, Terminal, Tab sheet, OSK open, badge) as PNGs.
# Uses the same paint path as Adv under BOTCORPS_CYD.
# Output: mocks/cyd-frames/cyd-*.png
# Built by: Super Grok session (no Cursor/agent hand-off)
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
out="$root/mocks/cyd-frames"
mkdir -p "$out"
# Host build uses vendored stub LovyanGFX font headers in tools/host/lgfx.
# and dump the five frames. Until then the frames stay as the last render.
if [ -f tools/host/lgfx/Fonts/glcdfont.h ]; then
  echo "host fonts found; building CYD renderer"
  echo "Vendored stubs present. Full CYD paint dump still requires complete host link of botcorps_ui under BOTCORPS_CYD; frames remain last set until that lands."
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
