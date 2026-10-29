set -e
cd /workspace/scratch
P=/workspace/cydpaint-venv/bin/python
$P v8art.py 8
(cd hb/gp && make -C build -j16 cydsim_gp >/dev/null && ./build/cydsim_gp --script full.script --out out --headless | tail -1)
ffmpeg -loglevel error -y -framerate 100/3 -i hb/gp/out/v8/f%04d.png -vf scale=720:960:flags=neighbor -c:v libx264 -crf 18 -preset medium -pix_fmt yuv420p deliv/t8all.mp4
ffmpeg -loglevel error -y -i deliv/t8all.mp4 -vf scale=240:320:flags=area -start_number 1 dec/mine/f%04d.png
$P v8art.py 8 choose
(cd hb/gp && make -C build -j16 cydsim_gp >/dev/null && ./build/cydsim_gp --script full.script --out out --headless | tail -1)
ffmpeg -loglevel error -y -framerate 100/3 -i hb/gp/out/v8/f%04d.png -vf scale=720:960:flags=neighbor -c:v libx264 -crf 18 -preset medium -pix_fmt yuv420p -movflags +faststart deliv/try8.mp4
mkdir -p dec/try8
ffmpeg -loglevel error -y -i deliv/try8.mp4 -vf scale=240:320:flags=area -start_number 1 dec/try8/f%04d.png
for th in 40 16; do $P score.py dec/try8 $th | tail -11 | cut -c1-22; done
ls -l hb/gp/sim/gib_v8_art.inc
echo T8DONE
