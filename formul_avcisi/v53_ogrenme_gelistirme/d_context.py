# V53 / Asama D: baglam genisletme (C blogunda ek slotlar; ayni ortak havuz, tek analitik cozum). Python kopyasi uzerinde olcum.
import time, sys, itertools, numpy as np, scipy.sparse as sp, model53 as M
V=M.V
# Q (C asamasi kabul: Dirichlet kendi sayim, kappa=3, KN devam eden unigram)
Cn=M.counts(M.w1t,M.yt); nb,wt=M.graph(Cn); Q0=M.qtab(Cn,nb,wt); nw=Cn.sum(1); nc_=(Cn>0).sum(0).astype(float); pkn=(nc_+1)/(nc_+1).sum()
lam=nw/(nw+3.0); Q=lam[:,None]*Q0+(1-lam)[:,None]*pkn[None,:]; Q/=Q.sum(1,keepdims=True)
def hist(ids,k):   # k. onceki kelime (k=1 -> w_{t-1}); satir t=2..499 ; yoksa -1
    out=np.full((ids.shape[0],498),-1,np.int64)
    for r in range(ids.shape[0]):
        for t in range(2,500):
            if t-k>=0: out[r,t-2]=ids[r,t-k]
    return out.reshape(-1)
H={s:{k:hist(i,k) for k in (1,2,3,4)} for s,i in (('t',M.idt),('v',M.idv),('e',M.ide))}
def ngram_keys(s,order):   # order=2: (w2,w1); 3: (w3,w2,w1); 4: ...
    ks=[H[s][k] for k in range(order,0,-1)]; ok=np.all([k>=0 for k in ks],axis=0); key=np.zeros(len(ks[0]),np.int64)
    for k in ks: key=key*V+np.maximum(k,0)
    return key,ok
def top_slots(order,topn,mincount=2):
    key,ok=ngram_keys('t',order); kk,cc=np.unique(key[ok],return_counts=True); sel=cc>=mincount; kk,cc=kk[sel],cc[sel]; o=np.lexsort((kk,-cc))[:topn]; return {int(k):i for i,k in enumerate(kk[o])}
def build(specs):
    """specs: [(tur, param, lam)] tur in 'word1','pair','skip2','tri','quad'; ilk eleman her zaman word1 + sabit eklenir."""
    mats={'t':[], 'v':[], 'e':[]}; lams=[]; off=0; cov={}
    for tur,par,lm in specs:
        if tur=='word1': cols={s:H[s][1].copy() for s in mats}; nslot=V
        elif tur=='skip2': cols={s:H[s][2].copy() for s in mats}; nslot=V
        else:
            order={'pair':2,'tri':3,'quad':4}[tur]; sl=top_slots(order,par); nslot=len(sl); cols={}
            for s in mats:
                key,ok=ngram_keys(s,order); cols[s]=np.array([sl.get(int(k),-1) if o else -1 for k,o in zip(key,ok)])
        for s in mats:
            c=cols[s]; ok=c>=0; m=len(c); mats[s].append(sp.csr_matrix((np.ones(ok.sum()),(np.where(ok)[0],c[ok]+off)),shape=(m,0 if False else off+nslot)) if False else (np.where(ok)[0],c[ok]+off))
        cov[tur+str(par)]=(mats['v'][-1][0].size/len(H['v'][1]))
        lams.append(np.full(nslot,float(lm))); off+=nslot
    # sabit slot
    NC=off+1; out=[]
    for s,mm in (('t',len(H['t'][1])),('v',len(H['v'][1])),('e',len(H['e'][1]))):
        r=np.concatenate([a for a,_ in mats[s]]+[np.arange(mm)]); c=np.concatenate([b for _,b in mats[s]]+[np.full(mm,NC-1)]); out.append(sp.csr_matrix((np.ones(len(r)),(r,c)),shape=(mm,NC)))
    lams.append(np.array([float(specs[0][2])]))
    return out,np.concatenate(lams),cov
F,Fv,Fe=M.load()
def run(specs,lamF=9000,tag=''):
    t0=time.time(); (Ct,Cv,Ce),lamv,cov=build(specs); t1=time.time()
    m=M.Fit(F,Fv,Fe,lamv,Ct_=Ct,Cv_=Cv,Ce_=Ce); t2=time.time(); res=[]
    for lf in ((lamF,) if np.isscalar(lamF) else lamF):
        W=m.WF(lf); Z=m.logits(W,'val'); ce,a,e=M.read_ce(Z,M.yv); Pv=M.read_P(Z,a,e)
        f=min((M.ce_of((1-al)*Pv+al*Q[M.w1v],M.yv),al) for al in (0.1,0.15,0.2,0.25,0.3)); 
        top1=(Z.argmax(1)==M.yv).mean(); res.append((lf,ce,f[0],f[1]))
    nnz=Ct.nnz; print('%-46s NC=%-6d kapsam(val)=%s | %s | Fit %.1fs bellek(ACC)=%.0fMB'%(tag,Ct.shape[1],{k:round(v,3) for k,v in cov.items() if k!='word1'+str(V) and not k.startswith('word1')},' ; '.join('lamF=%d okuma %.4f son %.4f'%(a,b,c) for a,b,c,d in res),t2-t1,Ct.shape[1]**2*8/1e6),flush=True)
    return res
if __name__=='__main__':
    base=[('word1',0,85),('pair',2000,85)]
    run(base,tag='taban: kelime+2000 cift (V51b)')
    for n in (5000,10000): run([('word1',0,85),('pair',n,85)],tag='cift top-%d'%n)
    run(base+[('skip2',0,85)],tag='taban+w_{t-2} slotu')
    for n in (2000,5000):
        for lh in (85,170,340): run(base+[('tri',n,lh)],tag='taban+uclu top-%d lam=%d'%(n,lh))
    for lh in (85,170,340): run(base+[('tri',3000,lh),('quad',3000,lh)],tag='taban+uclu3000+dortlu3000 lam=%d'%lh)
