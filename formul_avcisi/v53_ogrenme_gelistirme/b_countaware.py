# V53 / Asama B: sayim-bilincli C-blogu ridge  lam_i = lam0 * (1 + n_i/kappa)^(-alpha)   (alpha=0 -> sabit lambda)
import time, itertools, numpy as np, model53 as M
F,Fv,Fe=M.load(); m=M.Fit(F,Fv,Fe,85.0)
Cn=M.counts(M.w1t,M.yt); nb,wt=M.graph(Cn); Q=M.qtab(Cn,nb,wt)
ni=np.asarray(M.Ct.sum(0)).ravel()            # slot basina egitim satiri sayisi
def metric(lam,lamF):
    m.set_lam(lam); W=m.WF(lamF); Z=m.logits(W,'val'); ce,a,e=M.read_ce(Z,M.yv); Pv=M.read_P(Z,a,e)
    best=min((M.ce_of((1-al)*Pv+al*Q[M.w1v],M.yv),al) for al in (0.1,0.15,0.2,0.25,0.3))
    return ce,best
def df(lam):   # etkin serbestlik derecesi tr(A (A+L)^-1) (kelime / cift / sabit)
    d=np.diag(m.ACC)/(np.diag(m.ACC)+lam)          # A_CC blok-kosegen degil ama diag yaklasimi yerine tam iz:
    A=m.ACC; H=np.linalg.solve(A+np.diag(lam),A); t=np.diag(H); V=M.V
    return t[:V].sum(), t[V:V+2000].sum(), t[-1]
if __name__=='__main__':
    import sys
    base=metric(np.full(M.NC,85.0),9000); print('sabit lam=85 lamF=9000:',base,flush=True)
    print('df sabit:',df(np.full(M.NC,85.0)),flush=True)
    res=[]
    for lam0,al,kap in itertools.product((85,170,340,680,1360),(0.25,0.5,0.75,1.0),(20,100)):
        lam=lam0*(1+ni/kap)**(-al)
        for lf in (3000,9000,27000):
            ce,(fce,fal)=metric(lam,lf); res.append((fce,lam0,al,kap,lf,ce,fal)); print('lam0=%d alpha=%.2f kappa=%d lamF=%d | okuma %.4f | son karisim %.4f (a=%.2f)'%(lam0,al,kap,lf,ce,fce,fal),flush=True)
    res.sort(); print('EN IYI 5:'); [print(r) for r in res[:5]]
