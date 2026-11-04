import subprocess,sys
import os
PM=os.environ.get("PM","pm")
vals={"nan":None,"tiny":"2.8369691695966818e-306","huge":"1e300","neg":"-1.5","big":"1e19","sub":"4.450147717014404e-308","pi":"3.141592653589793"}
for n,v in vals.items():
    s="func main() {\n"
    if v is None: s+="    mov u, 0.0\n    fdiv x, u, u\n    fsub x, x, x\n"
    else: s+="    mov x, %s\n"%v
    s+="    host putf, x\n    host putc, 10\n    ret 0\n}\n"
    open("/tmp/d.pma","w").write(s)
    subprocess.check_call([PM,"asm","/tmp/d.pma","-o","/tmp/d.pmb","--strip"],stdout=subprocess.DEVNULL)
    l=subprocess.check_output([PM,"liste","/tmp/d.pmb"]).decode().split("\n")
    # onek: bayrak, sabitler, veri, bss, islev sayisi (1), "nargs nregs ncode", kod...
    assert l[4].strip()=="1", l[:6]
    na,nr,nc=map(int,l[5].split())
    code=l[6:6+nc]; rest=l[6+nc:]
    ret=code[-1]; body=code[:-1]
    assert ret.startswith("35"), ret
    # putc 10 kullandigi yazmac ile cakisma olmasin: dokumden once yeni sabit yazmac
    base=nr
    extra=[f"40 1 {k}" for k in range(nr) if True]  # tum yazmaclar (x dahil: putf sonrasi degeri de dokulur)
    # yeni yazmac nr+1 = 32 ayirici
    sr=nr+1+64; sep=f"1 {sr} 32"; extra2=[]
    for k in range(nr): extra2+= [f"40 1 {k}", f"40 0 {sr}"]
    # ek: 64 yazmaclik bos alan (hicbir zaman yazilmayan) -> dokulur: cerceve sinirinin ustu
    pad=list(range(nr+1,nr+1+64))
    for k in pad: extra2+= [f"40 1 {k}", f"40 0 {sr}"]
    new=body+[sep]+extra2+[ret.replace(ret,"35 %d"%sr)]
    out=l[:5]+[f"{na} {sr+1} {len(new)}"]+new+rest
    open("hostrt/ek_dokum_%s.l"%n,"w").write("\n".join(out))
