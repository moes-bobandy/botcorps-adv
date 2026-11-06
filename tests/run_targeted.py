from pathlib import Path
import subprocess, json, hashlib
root=Path('/workspace/cyd-fix8-proof');report={}
for name in ['item1','item6']:
 for v in ['before','after','band64']:
  d=root/'targeted'/name/v;d.mkdir(parents=True,exist_ok=True)
  r=subprocess.run([str(root/('sim_'+v)),str(root/(name+'.txt')),str(d)],capture_output=True,cwd='/workspace/cyd-portrait-wt',timeout=300)
  (d/'log.txt').write_bytes(r.stdout+r.stderr);assert r.returncode==0
  if v=='before': a={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in d.glob('*.rgb')}
  else:
   b={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in d.glob('*.rgb')}
   report[name+'_'+v]=dict(frames=len(a),diffs=[n for n in a.keys()|b.keys() if a.get(n)!=b.get(n)])
(root/'targeted.json').write_text(json.dumps(report,indent=2));print(json.dumps(report))
