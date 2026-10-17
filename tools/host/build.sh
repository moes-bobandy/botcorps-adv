#!/bin/bash
# host render build: tools/host/build.sh
set -e
cd "$(dirname "$0")/../.."
OUT=${OUT:-/tmp/k}; mkdir -p $OUT/o
M5=/home/box/Arduino/libraries/M5GFX/src
B0="-O1 -std=gnu++17 -DBOTCORPS_HOST_RENDER -w ${HOST_EXTRA}"; B="$B0 -Isrc -Itools/host -I$M5"; R="$B0 -Itools/host/raindisp -Itools/rainhost/shim -Isrc -Itools/host -I$M5"
g++ $B -Itools/host/idstub -c src/ui/botcorps_ui.cpp -o $OUT/o/ui.o
g++ $B -Itools/host/idstub -c tools/host/gfx_host.cpp -o $OUT/o/gfx.o
g++ $B -Itools/host/idstub -c tools/host/render_main.cpp -o $OUT/o/rm.o
g++ $R -c src/rain/botcorps_rain.cpp -o $OUT/o/rain.o
g++ $R -c tools/host/rain_bridge.cpp -o $OUT/o/rb.o
g++ $R -c tools/host/rain_syms.cpp -o $OUT/o/syms.o
g++ $OUT/o/*.o -o $OUT/rend -lm
