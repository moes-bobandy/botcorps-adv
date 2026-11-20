#!/usr/bin/env python3
"""qrun.py FLASH OUTPREFIX SECONDS [sample]: QEMU ESP32 icount shift=2 (~250 MIPS, the 240 MHz core at 1 IPC).
Serial -> OUT.log. With 'sample': PC of CPU1 (UI loop core) every ~15 ms wall -> OUT.pcs as 'phase pc'."""
import subprocess, socket, sys, time, os, re, threading
fl, out, secs = sys.argv[1], sys.argv[2], float(sys.argv[3]); samp = len(sys.argv) > 4
env = dict(os.environ, LD_LIBRARY_PATH='/workspace/tools/qemu/lib-extra:/workspace/tools/qemu/lib/x86_64-linux-gnu')
sock = out + '.mon'
if os.path.exists(sock): os.remove(sock)
q = subprocess.Popen(['/workspace/tools/qemu/bin/qemu-system-xtensa', '-nographic', '-machine', 'esp32', '-m', '4M',
     '-drive', f'file={fl},if=mtd,format=raw', '-icount', 'shift=2,align=off,sleep=off', '-nic', 'none',
     '-serial', 'mon:stdio', '-monitor', f'unix:{sock},server,nowait'], stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env)
phase = ['boot']; lf = open(out + '.log', 'wb')
def rd():
    buf = b''
    for line in iter(q.stdout.readline, b''):
        lf.write(line); lf.flush()
        if b'GFX-RAM' in line: phase[0] = 'init'
        m = re.search(rb'\[tour\] (.*)', line)
        if m: phase[0] = m.group(1).decode(errors='replace').strip().replace(' ', '_')
threading.Thread(target=rd, daemon=True).start()
t0 = time.time()
if samp:
    while not os.path.exists(sock): time.sleep(0.05)
    s = socket.socket(socket.AF_UNIX); s.connect(sock); s.settimeout(2)
    pf = open(out + '.pcs', 'w')
    def cmd(c):
        s.sendall((c + '\n').encode()); data = b''
        while True:
            try: d = s.recv(65536)
            except socket.timeout: break
            data += d
            if data.rstrip().endswith(b'(qemu)'): break
        return data.decode(errors='replace')
    while phase[0] == 'boot' and time.time() - t0 < 60: time.sleep(0.05)
    cmd(''); cmd('cpu 1')
    while time.time() - t0 < secs:
        r = cmd('info registers')
        m = re.search(r'PC=([0-9a-fA-F]+)', r)
        if m: pf.write(f'{phase[0]} {m.group(1)}\n')
        time.sleep(float(os.environ.get("QS","0.05")))
    pf.close()
else:
    while time.time() - t0 < secs: time.sleep(0.5)
q.terminate(); q.wait()
