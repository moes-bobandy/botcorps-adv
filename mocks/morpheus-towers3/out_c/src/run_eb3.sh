set -e
cd /workspace/out/morpheus-mix/towers3c/src
python3 eb3_body.py
SIM=/tmp/t3c3b/sim_EB3 python3 /workspace/out/morpheus-mix/src/drive.py steps.json /tmp/t3c3b/run_EB3
echo ALLDONE
