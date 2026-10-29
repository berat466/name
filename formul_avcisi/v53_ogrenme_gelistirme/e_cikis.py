# V53 / Asama E: dogrusal okumadaki max(z,eps) kirpmasina alternatifler. Yalniz dogrulama ile secim (test kullanilmaz).
import sys, os, numpy as np
os.environ['M53_FD']=sys.argv[1]
import d_context as D, d2_pairs as P, model53 as M
V=M.V; pmin=int(sys.argv[2]); lamp=float(sys.argv[3]); lamF=float(sys.argv[4])
(Ct,Cv,Ce),lam,cov,NC=P.build_pairs(pmin,85,lamp); f=P.FitS(Ct,Cv,Ce,lam); W=f.WF(lamF); Zv=f.CvY0+f.Fvr@W; Ze=f.CeY0+f.Fer@W
Q=D.Q; r=np.arange(M.nv); pi=M.uni; y=M.yv
def evalT(name,posf,params):
    best=None; Qy=Q[M.w1v][r,y]; piy=pi[y]
    for prm in params:
        S=posf(Zv,prm); S=S/S.sum(1,keepdims=True)
        assert np.isfinite(S).all() and (S>=0).all()
        Sy=S[r,y]
        for a in M.GA:
            Pvy=(1-a)*piy+a*Sy
            for al in (0.0,0.1,0.15,0.2,0.25,0.3):
                ce=-np.log((1-al)*Pvy+al*Qy).mean()
                if best is None or ce<best[0]: best=(ce,prm,a,al,S)
    ce,prm,a,al,S=best; Pf=(1-al)*((1-a)*pi[None,:]+a*S)+al*Q[M.w1v]; top1=(Pf.argmax(1)==y).mean(); rk=(Pf>Pf[r,y][:,None]).sum(1); top5=(rk<5).mean()
    print('%-34s val CE %.4f | param=%s a=%.2f alpha_Q=%.2f | Top1 %.4f Top5 %.4f | satir sum sapma %.1e'%(name,ce,prm,a,al,top1,top5,np.abs(Pf.sum(1)-1).max()),flush=True)
    return ce
print('kirpilan sutun payi (val, z<=1e-5): %.3f ; hedef sutunu kirpilan satir: %.3f ; z<0 payi %.3f'%((Zv<=1e-5).mean(),(Zv[r,y]<=1e-5).mean(),(Zv<0).mean()))
evalT('A) max(z,eps) [mevcut]',lambda Z,e: np.maximum(Z,e),M.GE)
evalT('B) softplus(z/t)*t',lambda Z,t: t*np.logaddexp(0,Z/t),(1e-4,3e-4,1e-3,3e-3,1e-2,3e-2))
evalT('C) max(z,eps)^p',lambda Z,p: np.maximum(Z,1e-5)**p,(0.5,0.75,1.0,1.25,1.5))
evalT('D) max(z,eps)^p eps=1e-4',lambda Z,p: np.maximum(Z,1e-4)**p,(0.5,0.75,1.0,1.25,1.5))
evalT('E) softmax(z/T)',lambda Z,T: np.exp((Z-Z.max(1,keepdims=True))/T),(0.005,0.01,0.02,0.05,0.1,0.2))
evalT('F) exp-yumusak: max(z,0)+eps*exp(z/eps)',lambda Z,e: np.maximum(Z,0)+e*np.exp(np.minimum(Z,0)/e),(1e-5,1e-4,1e-3))
evalT('G) (relu(z)+eps)^p, p=.75',lambda Z,e: (np.maximum(Z,0)+e)**0.75,(1e-5,1e-4,1e-3,3e-3))
