#!/bin/bash
# usage: run_gate.sh THEME
T=$1; mkdir -p /tmp/c123/g/base$T /tmp/c123/g/new$T
cd /tmp/c123/basewt && SIM_THEME=$T /tmp/c123/bin/sim_base /tmp/c123/tour_base.txt /tmp/c123/g/base$T >/dev/null 2>&1
cd /workspace/v1223-build-wt && SIM_THEME=$T /tmp/c123/bin/sim_new /tmp/c123/tour_new.txt /tmp/c123/g/new$T >/dev/null 2>&1
P="45,220,196,290;90,25,150,29"
cd /workspace/v1223-build-wt && python3 tools/gate/v1220_gate.py /tmp/c123/g/base$T /tmp/c123/g/new$T "pager0:$P" "pager1:$P" "pager2:$P" "pager3:$P" "pager4:90,25,150,29" term dojo gib_scan gib_sel_deck dl_intro dl_play
