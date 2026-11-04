# V51b'nin (OOFPOOL=1, V49=2, g_F=0.15, rho=0) analitik cozumunun Python kopyasi: C++ F dokumunden ayni Gram/RHS/Schur/Ridge + dogrusal okuma.
import numpy as np, scipy.sparse as sp, scipy.linalg as sl
V=2001; D='/tmp/v8data'; FD='/tmp/v52B'
def rows(s):
    ids=np.load(f'{D}/ids_{s}.npy'); w2=ids[:,:-2].reshape(-1); w1=ids[:,1:-1].reshape(-1); y=ids[:,2:].reshape(-1); blk=np.repeat(np.arange(ids.shape[0]),498); return w2,w1,y,blk
w2t,w1t,yt,bt=rows('train'); w2v,w1v,yv,bv=rows('val'); w2e,w1e,ye,be=rows('test')
n,nv,ne=len(yt),len(yv),len(ye)
uni=np.fromfile(f'{D}/uni_tr.f64')
key=w2t.astype(np.int64)*V+w1t; kk,cc=np.unique(key,return_counts=True); order=np.lexsort((kk,-cc)); top=kk[order[:2000]]; pidx={int(k):i for i,k in enumerate(top)}
def slots(w2,w1):
    return np.array([pidx.get(int(a)*V+int(b),-1) for a,b in zip(w2,w1)])
pt,pv,pe=slots(w2t,w1t),slots(w2v,w1v),slots(w2e,w1e)
NC=V+2000+1   # kelime slotlari + 2000 cift + sabit
def Cmat(w1,p):
    m=len(w1); r=np.concatenate([np.arange(m),np.arange(m)[p>=0],np.arange(m)]); c=np.concatenate([w1,V+p[p>=0],np.full(m,NC-1)]); return sp.csr_matrix((np.ones(len(r)),(r,c)),shape=(m,NC))
Ct,Cv,Ce=Cmat(w1t,pt),Cmat(w1v,pv),Cmat(w1e,pe)
def load(tag=FD):
    g=lambda nm,m: np.fromfile(f'{tag}/{nm}',dtype=np.float32).reshape(m,513)[:,1:].astype(np.float64)
    return g('Fc.f32',n),g('Fcv.f32',nv),g('Fce.f32',ne)
Ytr_rc=lambda: sp.csr_matrix((np.ones(n),(np.arange(n),yt)),shape=(n,V))
GA=(0.05,0.1,0.15,0.2,0.25,0.3,0.4,0.5,0.6,0.7,0.75,0.8,0.85,0.9,0.93,0.95,0.97,0.98,0.99,1.0); GE=(1e-7,1e-6,1e-5,1e-4,3e-4,1e-3,3e-3,1e-2)
def read_P(Z,a,e,pi=uni):
    S=np.maximum(Z,e); return (1-a)*pi[None,:]+a*S/S.sum(1,keepdims=True)
def read_ce(Z,y,pi=uni):
    best=(1e9,None,None); r=np.arange(len(y))
    for e in GE:
        S=np.maximum(Z,e); zy=S[r,y]/S.sum(1)
        for a in GA:
            ce=-np.log((1-a)*pi[y]+a*zy).mean()
            if ce<best[0]: best=(ce,a,e)
    return best
class Fit:
    def __init__(s,F,Fv,Fe,lamC=85.0):
        s.F,s.Fv,s.Fe,s.lamC=F,Fv,Fe,lamC
        s.ACC=(Ct.T@Ct).toarray(); s.ACF=np.asarray(Ct.T@F); s.AFF=F.T@F
        s.RC=np.asarray((Ct.T@Ytr_rc()).todense()); s.RF=np.zeros((512,V))
        for j in range(512): s.RF[j]=np.bincount(yt,weights=F[:,j],minlength=V)
        A=s.ACC+lamC*np.eye(NC); s.cho=sl.cho_factor(A,lower=True)
        s.Y0=sl.cho_solve(s.cho,s.RC); s.X=sl.cho_solve(s.cho,s.ACF)
        s.S=s.AFF-s.ACF.T@s.X; s.rs=s.RF-s.ACF.T@s.Y0
        s.CvY0=np.asarray(Cv@s.Y0); s.Fvr=Fv-np.asarray(Cv@s.X); s.CeY0=np.asarray(Ce@s.Y0); s.Fer=Fe-np.asarray(Ce@s.X)
        s.CtY0=np.asarray(Ct@s.Y0); s.Ftr=F-np.asarray(Ct@s.X)
    def WF(s,lamF,sub=None,lam_vec=None):
        sub=np.arange(512) if sub is None else sub
        Sm=s.S[np.ix_(sub,sub)]+(np.diag(lam_vec[sub]) if lam_vec is not None else lamF*np.eye(len(sub)))
        return sl.solve(Sm,s.rs[sub],assume_a='pos')
    def logits(s,W,sub=None,split='val'):
        sub=np.arange(512) if sub is None else sub
        if split=='val': return s.CvY0+s.Fvr[:,sub]@W
        if split=='test': return s.CeY0+s.Fer[:,sub]@W
        return s.CtY0+s.Ftr[:,sub]@W
    def val_ce(s,W,sub=None): return read_ce(s.logits(W,sub),yv)
