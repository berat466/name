import numpy as np, b_countaware as B, model53 as M
ni=B.ni
def rowloss(lam,lamF=9000):
    B.m.set_lam(lam); W=B.m.WF(lamF); Z=B.m.logits(W,'val'); ce,a,e=M.read_ce(Z,M.yv); Pv=M.read_P(Z,a,e)
    best=min(((M.ce_of((1-al)*Pv+al*B.Q[M.w1v],M.yv),al) for al in (0.1,0.15,0.2,0.25,0.3))); al=best[1]; P=(1-al)*Pv+al*B.Q[M.w1v]
    return -np.log(P[np.arange(M.nv),M.yv]), ce, best
l0,_,b0=rowloss(np.full(M.NC,85.0))
for name,lam in (('alpha=+0.25 (varsayim: sik az buzulsun) lam0=85',85*(1+ni/100)**-0.25),('alpha=-0.25 lam0=85 (ters yon)',85*(1+ni/100)**0.25),('alpha=-0.25 lam0=60',60*(1+ni/100)**0.25)):
    l1,ce,b1=rowloss(lam); d=l1-l0; print('%-55s fused %.4f vs sabit %.4f | fark %.4f +- %.4f (z=%.1f)'%(name,b1[0],b0[0],d.mean(),d.std()/np.sqrt(len(d)),d.mean()/(d.std()/np.sqrt(len(d)))))
# df raporu
for name,lam in (('sabit',np.full(M.NC,85.0)),('alpha=+0.25',85*(1+ni/100)**-0.25),('alpha=-0.25',85*(1+ni/100)**0.25)):
    d=B.df(lam); print('df',name,[round(float(x),1) for x in d],'toplam %.1f'%sum(d))
