import concurrent.futures, hashlib, json, subprocess, threading, contextlib
from pathlib import Path
ROOT=Path('/workspace/cyd-portrait-wt')
OUT=Path('/workspace/cyd-fix8-proof/regression')
scripts=list((ROOT/'tools/livescan/scripts').glob('*.txt'))+[Path('/tmp/item8.txt')]

rec_lock=threading.Lock()

def run(task):
    version,script,cal=task
    dst=OUT/version/(script.stem+'.'+cal);dst.mkdir(parents=True,exist_ok=True)
    with rec_lock if "recstart" in script.read_text() else contextlib.nullcontext():
        r=subprocess.run(['/workspace/cyd-fix8-proof/sim_'+version,str(script),str(dst)]+(['nocal'] if cal=='nocal' else []),cwd=ROOT,capture_output=True,timeout=300)
    (dst/'log.txt').write_bytes(r.stdout+r.stderr)
    return [version,script.stem,cal,r.returncode]
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as ex:
    runs=list(ex.map(run,[(v,s,c) for v in ['after'] for s in scripts for c in ['cal','nocal']]))
    old=json.loads((OUT/'report.json').read_text())['runs']
    keys={(r[0],r[1],r[2]) for r in runs}
    runs += [r for r in old if tuple(r[:3]) not in keys]
diffs=[];files=0
for s in scripts:
 for c in ['cal','nocal']:
    sub=s.stem+'.'+c
    a={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (OUT/'before'/sub).glob('*.rgb')}
    files+=len(a)
    for v in ['after','band64']:
        b={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (OUT/v/sub).glob('*.rgb')}
        diffs.extend([v,sub,n] for n in a.keys()|b.keys() if a.get(n)!=b.get(n))
report=dict(runs=runs,baseline_frames=files,unintended_diffs=diffs)
(OUT/'report.json').write_text(json.dumps(report,indent=2))
print(json.dumps(dict(runs=len(runs),failed=[r for r in runs if r[-1]],baseline_frames=files,diffs=diffs),indent=2))
