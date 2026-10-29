#!/bin/sh
# Headless full tour -> shots + contact sheet. Usage: tools/run_tour.sh [OUTDIR]
set -e
HERE=$(cd "$(dirname "$0")/.." && pwd)
OUT=${1:-/workspace/out/cyd-sim}
mkdir -p "$OUT/shots"
rm -f "$OUT"/shots/*.png
"$HERE/build/cydsim" --headless --serial --script "$HERE/scripts/tour.txt" --out "$OUT/shots" > "$OUT/tour.log" 2>&1
python3 "$HERE/tools/sheet.py" "$OUT/sheet.png" "$OUT"/shots/*.png
echo "shots: $OUT/shots  sheet: $OUT/sheet.png  log: $OUT/tour.log"
