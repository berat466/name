# V53 / D (devam): cift slot sayisini (top-N sinirini) kaldir; A_CC son-kelimeye gore blok-kosegen -> seyrek cozum (C++'ta grup Cholesky ile ayni)
import time, sys, numpy as np, scipy.sparse as sp, scipy.sparse.linalg as spl, scipy.linalg as sl
import d_context as D, model53 as M
V=M.V; F,Fv,Fe=D.F,D.Fv,D.Fe
class FitS:
    def __init__(s,Ct,Cv,Ce,lam):
        s.Ct,s.Cv,s.Ce=Ct,Cv,Ce; nc=Ct.shape[1]; s.nc=nc
        s.ACC=(Ct.T@Ct).tocsc(); s.ACF=np.asarray(Ct.T@F); s.AFF=F.T@F
        Y=sp.csr_matrix((np.ones(M.n),(np.arange(M.n),M.yt)),shape=(M.n,V)); RC=(Ct.T@Y).tocsc(); RF=np.zeros((512,V))
        for j in range(512): RF[j]=np.bincount(M.yt,weights=F[:,j],minlength=V)
        lu=spl.splu((s.ACC+sp.diags(lam)).tocsc()); s.Y0=lu.solve(RC.toarray()); s.X=lu.solve(s.ACF)
        s.S=s.AFF-s.ACF.T@s.X; s.rs=RF-s.ACF.T@s.Y0
        s.CvY0=np.asarray(Cv@s.Y0); s.Fvr=Fv-np.asarray(Cv@s.X); s.CeY0=np.asarray(Ce@s.Y0); s.Fer=Fe-np.asarray(Ce@s.X)
    def WF(s,lf): return sl.solve(s.S+lf*np.eye(512),s.rs,assume_a='pos')
    def val(s,lf):
        W=s.WF(lf); Z=s.CvY0+s.Fvr@W; ce,a,e=M.read_ce(Z,M.yv); Pv=M.read_P(Z,a,e); f=min((M.ce_of((1-al)*Pv+al*D.Q[M.w1v],M.yv),al) for al in (0.1,0.15,0.2,0.25,0.3)); return ce,f
def build_pairs(minc,lamw=85,lamp=85,extra=()):
    key,ok=D.ngram_keys('t',2); kk,cc=np.unique(key[ok],return_counts=True); kk=kk[cc>=minc]; sl_={int(k):i for i,k in enumerate(kk)}
    specs=[('word1',0,lamw)]
    mats={}; lams=[np.full(V,float(lamw))]; off=V
    out={}
    cols={}
    for s in 'tve':
        key,ok=D.ngram_keys(s,2); idx=np.searchsorted(kk,key); idx=np.minimum(idx,len(kk)-1); hit=ok&(kk[idx]==key); cols[s]=np.where(hit,idx,-1)
    nslot=len(kk); lams.append(np.full(nslot,float(lamp))); off2=V+nslot
    cov=(cols['v']>=0).mean()
    if extra:
        for (order,topn,mc,lm) in extra:
            sl3=D.top_slots(order,topn,mc); 
            for s in 'tve':
                key,ok=D.ngram_keys(s,order); c=np.array([sl3.get(int(k),-1) if o else -1 for k,o in zip(key,ok)]); cols[(s,order)]=c
            lams.append(np.full(len(sl3),float(lm))); 
    mm={'t':M.n,'v':M.nv,'e':M.ne}; mats=[]
    sizes=[V,nslot]+[len(l) for l in lams[2:]]; NC=sum(sizes)+1
    for s in 'tve':
        rows=[np.arange(mm[s])]; colsl=[D.H[s][1]]; o=V
        c=cols[s]; ok=c>=0; rows.append(np.where(ok)[0]); colsl.append(c[ok]+o); o+=nslot
        for (order,topn,mc,lm),sz in zip(extra,sizes[2:]):
            c=cols[(s,order)]; ok=c>=0; rows.append(np.where(ok)[0]); colsl.append(c[ok]+o); o+=sz
        rows.append(np.arange(mm[s])); colsl.append(np.full(mm[s],NC-1))
        r=np.concatenate(rows); cc_=np.concatenate(colsl); mats.append(sp.csr_matrix((np.ones(len(r)),(r,cc_)),shape=(mm[s],NC)))
    lam=np.concatenate(lams+[np.array([lamw],float)]); return mats,lam,cov,NC
if __name__=='__main__':
    for minc,lamp in ((5,85),(3,85),(2,85),(2,170),(2,50),(1,170),(1,340)):
        t=time.time(); (Ct,Cv,Ce),lam,cov,NC=build_pairs(minc,85,lamp); t1=time.time(); f=FitS(Ct,Cv,Ce,lam); t2=time.time()
        res=[(lf,)+f.val(lf) for lf in (3000,9000)]
        print('cift count>=%d lam_cift=%d | NC=%d kapsam(val)=%.3f | %s | kurulum+cozum %.1fs'%(minc,lamp,NC,cov,' ; '.join('lamF=%d okuma %.4f son %.4f(a=%.2f)'%(a,b,c[0],c[1]) for a,b,c in res),t2-t1),flush=True)
