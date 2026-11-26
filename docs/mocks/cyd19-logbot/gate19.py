import numpy as np, subprocess, os, sys
names=[f"p{i}_{k}" for i in range(5) for k in ("mid","full")]
def run(binp, script, theme, out):
    os.makedirs(out, exist_ok=True)
    env=dict(os.environ, SIM_THEME=str(theme))
    subprocess.run(f"cd /workspace/v1222-build-wt && {binp} {script} {out}", shell=True, env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
def ld(out,n): return np.fromfile(f"{out}/{n}.rgb",np.uint8)[:230400].reshape(320,240,3)
lines=[]
ok=True
for th,tn in ((22,"Vapor Grid"),(23,"Amber CRT"),(9,"Prism")):
    run("/tmp/c19/bin/sim_base","/tmp/c19/gate_base.txt",th,f"/tmp/c19/g/base{th}")
    run("/tmp/c19/bin/sim_off","/tmp/c19/gate_base.txt",th,f"/tmp/c19/g/off{th}")
    for fin in (0,1,2):
        os.environ["LB_FINISH"]=str(fin)
        run("/tmp/c19/bin/sim_lb","/tmp/c19/gate_lb.txt",th,f"/tmp/c19/g/lb{th}_{fin}")
    lines.append(f"== SIM_THEME={th} ({tn})")
    lines.append("A) default-OFF build (LV_CYD19_LOGBOT=0) vs base (6f031b0 tree, unmodified): every page 0 px")
    for n in names:
        d=(ld(f"/tmp/c19/g/base{th}",n)!=ld(f"/tmp/c19/g/off{th}",n)).any(-1).sum()
        lines.append(f"  {n:9s} diff_px={d}  {'PASS' if d==0 else 'FAIL'}"); ok&=d==0
    lines.append("B) LOGBOT=1 build vs base, existing pages (CONTROLbot is page 5 now): diff must be 0 outside pip band y=25..29")
    for fin in (0,1,2):
        for n in names:
            a=ld(f"/tmp/c19/g/base{th}",n); b=ld(f"/tmp/c19/g/lb{th}_{fin}",n)
            m=(a!=b).any(-1); outside=m.copy(); outside[25:30,:]=False
            ys,xs=np.nonzero(m)
            bb=f"pip band x{xs.min()}-{xs.max()} y{ys.min()}-{ys.max()}" if m.any() else "none"
            res=outside.sum()==0
            ok&=res
            if fin==0 or not res: lines.append(f"  fin{fin} {n:9s} total={m.sum():4d} inside_pip_band={m.sum()-outside.sum():4d} OUTSIDE={outside.sum()}  {'PASS' if res else 'FAIL'}  {bb}")
        lines.append(f"  (finish {fin}: all 10 screens checked)")
open('/tmp/c19/gate19.txt','w').write("\n".join(lines)+"\nOVERALL "+("PASS" if ok else "FAIL")+"\n")
print("\n".join(lines[:30])); print("OVERALL","PASS" if ok else "FAIL")
