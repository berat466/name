import numpy as np, d2_pairs as P, model53 as M
V=M.V
def run(minc,lamp,tag,extra=(),alpha=0.0,kap=20.0,lamw=85):
    (Ct,Cv,Ce),lam,cov,NC=P.build_pairs(minc,lamw,lamp,extra)
    if alpha!=0:
        ni=np.asarray(Ct.sum(0)).ravel(); lam=lam.copy(); lam[V:]=lam[V:]*(1+ni[V:]/kap)**(-alpha)   # yalniz cift(+ust) slotlar; sabit slot dahil degil
        lam[-1]=lamw
    f=P.FitS(Ct,Cv,Ce,lam); r=[(lf,)+f.val(lf) for lf in (9000,)]
    print('%-52s NC=%d | %s'%(tag,NC,' ; '.join('lamF=%d okuma %.4f son %.4f'%(a,b,c[0]) for a,b,c in r)),flush=True)
run(1,50,'cift>=1 lam=50 (sabit)')
for al in (-0.5,-0.25,0.25,0.5):
    for lp in (50,100):
        run(1,lp,'cift>=1 lam0=%d count-aware alpha=%.2f kappa=20'%(lp,al),alpha=al)
for (o,n,mc,lm) in ((3,5000,2,85),(3,5000,2,170),(3,20000,2,170),(3,20000,1,340)):
    run(1,50,'cift>=1 + uclu top%d count>=%d lam=%d'%(n,mc,lm),extra=((o,n,mc,lm),))
