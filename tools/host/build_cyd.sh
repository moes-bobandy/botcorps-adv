#!/bin/bash
# CYD host render build: Adv UI + real rain compiled with -DBOTCORPS_CYD, plus the
# host-only CYD glue (tools/host/cyd/*.cpp). Produces $OUT/rend the same as build.sh.
set -e
cd "$(dirname "$0")/../.."
OUT=${OUT:-/tmp/kcyd}; mkdir -p $OUT/o
if [ -f /home/box/Arduino/libraries/M5GFX/src/lgfx/Fonts/glcdfont.h ]; then
  M5=/home/box/Arduino/libraries/M5GFX/src
else
  M5=${M5GFX_SRC:-third_party/M5GFX/src}
fi
BID="v1019z-cyd $(git rev-parse --short=7 HEAD 2>/dev/null || echo nogit)$( [ -z "$(git status --porcelain --untracked-files=no 2>/dev/null)" ] || echo -dirty)"
printf '#define BOTCORPS_BUILD_ID "%s"\n' "$BID" > $OUT/o/bid.h
B0="-O1 -std=gnu++17 -DBOTCORPS_HOST_RENDER -DBOTCORPS_CYD -w -include $OUT/o/bid.h"
B="$B0 -Isrc -Itools/host -I$M5"; R="$B0 -Itools/host/raindisp -Itools/rainhost/shim -Isrc -Itools/host -I$M5"
g++ $B -Itools/host/idstub -c src/ui/botcorps_ui.cpp       -o $OUT/o/ui.o
g++ $B -Itools/host/idstub -c tools/host/gfx_host.cpp      -o $OUT/o/gfx.o
g++ $B -Itools/host/idstub -c tools/host/render_main.cpp   -o $OUT/o/rm.o
g++ $B -Itools/host/idstub -c tools/host/cyd/cyd_glue.cpp  -o $OUT/o/cydg.o
g++ $B -Itools/host/idstub -c tools/host/cyd/cyd_render.cpp -o $OUT/o/cydr.o
g++ $R -c src/rain/botcorps_rain.cpp     -o $OUT/o/rain.o
g++ $R -c tools/host/rain_bridge.cpp     -o $OUT/o/rb.o
g++ $R -c tools/host/rain_syms.cpp       -o $OUT/o/syms.o
g++ $OUT/o/*.o -o $OUT/rend -lm
