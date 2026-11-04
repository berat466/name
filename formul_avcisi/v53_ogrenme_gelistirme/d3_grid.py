import time, numpy as np, d2_pairs as P
for minc,lamw,lamp in ((2,85,35),(2,85,25),(2,85,15),(1,85,50),(1,85,35),(1,85,25),(3,85,35),(3,85,25),(2,60,35),(2,120,35),(2,170,35)):
    (Ct,Cv,Ce),lam,cov,NC=P.build_pairs(minc,lamw,lamp); f=P.FitS(Ct,Cv,Ce,lam); res=[(lf,)+f.val(lf) for lf in (3000,9000)]
    print('cift count>=%d lam_kelime=%d lam_cift=%d | NC=%d kapsam=%.3f | %s'%(minc,lamw,lamp,NC,cov,' ; '.join('lamF=%d okuma %.4f son %.4f(a=%.2f)'%(a,b,c[0],c[1]) for a,b,c in res)),flush=True)
