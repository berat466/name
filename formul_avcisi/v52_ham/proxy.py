import numpy as np, sys
tag=sys.argv[1]; n=39840; nv=4980; V=2001; m=513
rd=lambda nm,shape,dt=np.float32: np.fromfile(f'/tmp/{tag}/{nm}',dtype=dt).reshape(shape)
F=rd('Fc.f32',(n,m))[:,1:].astype(np.float64); Fv=rd('Fcv.f32',(nv,m))[:,1:].astype(np.float64)
y=rd('ytr.i32',(n,),np.int32); yv=rd('yva.i32',(nv,),np.int32)
uni=np.bincount(y,minlength=V)/n
mu=F.mean(0); Fc=F-mu; ym=np.bincount(y,minlength=V)/n
RHS=np.zeros((512,V))
for j in range(512): RHS[j]=np.bincount(y,weights=Fc[:,j],minlength=V)    # Fc' (Y - ym) (Fc merkezli => ym katkisi 0)
C=Fc.T@Fc
def ce_read(Zv,yv):
    best=1e9
    for a in (0.3,0.5,0.7,0.8,0.9,0.95,1.0):
        S=np.maximum(Zv,1e-5); P=(1-a)*uni+a*S/S.sum(1,keepdims=True); ce=-np.log(P[np.arange(len(yv)),yv]).mean(); best=min(best,ce)
    return best
def fit_eval(T,lam):      # F' = Fc T  (T 512x512 tersinir); ridge izotropik lam
    A=T.T@C@T; R=T.T@RHS; W=np.linalg.solve(A+lam*np.eye(512),R)
    Zv=ym+ (Fv-mu)@T@W
    return ce_read(Zv,yv)
lams=[100,300,1000,3000,10000,30000]
ev,U=np.linalg.eigh(C/n); ev=np.maximum(ev[::-1],0); U=U[:,::-1]
print('== proxy (yalniz F blogu, intercept var, C blogu YOK) :',tag)
dfs=[(lam,(n*ev/(n*ev+lam)).sum()) for lam in (300,3000,30000)]; print('serbestlik derecesi df(lam):',', '.join('lam=%g: %.1f'%d for d in dfs))
# her PC'nin sinyal enerjisi: ||U_k' RHS||^2 / (n*ev_k)  (tek basina OLS ile aciklanan hedef enerjisi) 
sig=((U.T@RHS)**2).sum(1)/ (n*np.maximum(ev,1e-18)); tot=sig.sum()
cum=np.cumsum(sig)/tot; print('PC bazinda sinyal enerjisi payi: ilk 10 PC %.3f, ilk 33 PC %.3f, ilk 133 %.3f, ilk 228 %.3f, son 284 PC %.3f'%(cum[9],cum[32],cum[132],cum[227],1-cum[227]))
base={lam:fit_eval(np.eye(512),lam) for lam in lams}; print('taban (F cikti):',{k:round(v,4) for k,v in base.items()})
rng=np.random.default_rng(0); Q,_=np.linalg.qr(rng.normal(size=(512,512)))
from scipy.linalg import hadamard
Hd=hadamard(512)/np.sqrt(512)
for nm,T in [('rastgele ortogonal',Q),('Hadamard',Hd),('PCA rotasyonu (ortogonal)',U)]:
    r={lam:fit_eval(T,lam) for lam in (1000,3000)}; print('%-26s'%nm,{k:round(v,4) for k,v in r.items()},' maks fark taban:',max(abs(r[l]-base[l]) for l in r))
print('kismi beyazlatma Z=Fc U diag((ev+tau*ev1)^-p) (tersinir, ortogonal DEGIL), olcek: toplam varyans korunur')
best=(base[min(base,key=base.get)],'taban',None)
for p in (0.1,0.25,0.5):
    for tau in (1e-4,1e-3,1e-2):
        s=(ev+tau*ev[0])**(-p); s*=np.sqrt(ev.sum()/(s**2*ev).sum()); T=U*s
        r={lam:fit_eval(T,lam) for lam in lams}; lb=min(r,key=r.get); print('  p=%.2f tau=%g : en iyi lam=%g CE=%.4f'%(p,tau,lb,r[lb]))
        if r[lb]<best[0]: best=(r[lb],(p,tau),lb)
print('proxy en iyi:',best, ' taban en iyi:',min(base.values()))
