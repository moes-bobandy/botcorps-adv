set -e
cd /workspace/out/morpheus-mix/towers3c/src
python3 towers3c_body.py
SIM=/tmp/t3c/sim_E python3 /workspace/out/morpheus-mix/src/drive.py steps.json /tmp/t3c/run_EB &
SIM=/tmp/m18/sim python3 /workspace/out/morpheus-mix/src/drive.py steps.json /tmp/t3c/run_v5f &
wait; echo ALLDONE
