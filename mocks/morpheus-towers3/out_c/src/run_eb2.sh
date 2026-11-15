set -e
cd /workspace/out/morpheus-mix/towers3c/src
python3 eb2_body.py
SIM=/tmp/t3c2/sim_EB2 python3 /workspace/out/morpheus-mix/src/drive.py steps.json /tmp/t3c2/run_EB2
echo ALLDONE
