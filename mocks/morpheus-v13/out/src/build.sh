#!/bin/bash
# usage: build.sh TREE OUT [flags]   (botcorps-cyd-sim skill: zig c++ host build)
cd $1; O=$2; shift 2
exec /tmp/rci/zig/ziglang/zig c++ -target x86_64-linux-gnu.2.36 -I/tmp/zinc -O2 -std=gnu++17 -w "$@" -o $O tools/livescan/sim_live.cpp /usr/lib/x86_64-linux-gnu/libz.so.1
