import sys,time; sys.path.insert(0,'.'); import numpy as np, scipy.linalg as sl, scipy.sparse as sp
import model as M
V=M.V; F,Fv,Fe=M.load(); m=M.Fit(F,Fv,Fe)
def ce_split(m,W,split,fix=None):
    Z=m.logits(W,None,split); y={'val':M.yv,'test':M.ye,'train':M.yt}[split]
    if fix: S=np.maximum(Z,fix[1]); r=np.arange(len(y)); return -np.log((1-fix[0])*M.uni[y]+fix[0]*S[r,y]/S.sum(1)).mean()
    return M.read_ce(Z,y)[0]
lamF=3000; W=m.WF(lamF)
ceV,aV,eV=m.val_ce(W); print('== REFERANS (Python kopyasi, ORTAK rho=0, alpha_son=0): val CE %.4f (okuma a=%s eps=%g)'%(ceV,aV,eV)); fix=(aV,eV)
print('   train CE(okuma sabit) %.4f ; val %.4f ; test %.4f'%(ce_split(m,W,'train',fix),ce_split(m,W,'val',fix),ce_split(m,W,'test',fix)))
# --- 1) blok katkilari
print('\n== 1) BLOK KATKILARI (ayni okuma secimi val ile, her satir kendi okumasini secer)')
def rep(name,Zfun):
    out=[]
    for sp_,y in (('train',M.yt),('val',M.yv),('test',M.ye)):
        Z=Zfun(sp_); ce,a,e=M.read_ce(Z,y) if sp_=='val' else (None,)*3
        if sp_=='val': pa=(a,e); cev=ce
    S=None
    res={}
    for sp_,y in (('train',M.yt),('val',M.yv),('test',M.ye)):
        Z=Zfun(sp_); S_=np.maximum(Z,pa[1]); r=np.arange(len(y)); res[sp_]=-np.log((1-pa[0])*M.uni[y]+pa[0]*S_[r,y]/S_.sum(1)).mean()
    print('   %-34s train CE %.4f | val CE %.4f | test CE %.4f  (okuma a=%s eps=%g)'%(name,res['train'],res['val'],res['test'],pa[0],pa[1]))
rep('yalniz C (kelime+cift+sabit), F yok',lambda s: {'train':m.CtY0,'val':m.CvY0,'test':m.CeY0}[s])
rep('C + F (lamF=3000)  [model]',lambda s: m.logits(W,None,s))
for lam in (300,1000,10000,30000): 
    Wl=m.WF(lam); rep('C + F (lamF=%g)'%lam,lambda s,Wl=Wl: m.logits(Wl,None,s))
Wl=m.WF(1e-3); rep('C + F (lamF=1e-3 ~ cezasiz)',lambda s: m.logits(Wl,None,s))
# F tek basina (sabit + F)
def fonly(lam):
    Z=np.hstack([np.ones((M.n,1)),m.F]); A=Z.T@Z; A[1:,1:]+=lam*np.eye(512); A[0,0]+=85; R=np.zeros((513,V)); 
    for j in range(513): R[j]=np.bincount(M.yt,weights=Z[:,j],minlength=V)
    Wf=sl.solve(A,R,assume_a='pos'); return lambda s: {'train':Z,'val':np.hstack([np.ones((M.nv,1)),Fv]),'test':np.hstack([np.ones((M.ne,1)),Fe])}[s]@Wf
rep('yalniz F + sabit (C yok), lamF=3000',fonly(3000)); rep('yalniz F + sabit (C yok), lamF=300',fonly(300))
# --- 2) C blogu buzulmesi
print('\n== 2) C BLOGU: lam_C=85 ile buzulme faktoru n/(n+85) (kimlik sutunlari)')
cnt=np.asarray(M.Ct.sum(0)).ravel(); w=cnt[:V]; p=cnt[V:V+2000]
for nm,c in (('kelime slotu',w),('cift slotu',p)):
    f=c/(c+85); print('   %s: %d slot ; sayim medyan %d ; buzulme faktoru <0.1: %d (%.1f%%), <0.3: %d (%.1f%%), >0.5: %d (%.1f%%) ; etkin df = %.1f / %d'%(nm,len(c),np.median(c),(f<0.1).sum(),100*(f<0.1).mean(),(f<0.3).sum(),100*(f<0.3).mean(),(f>0.5).sum(),100*(f>0.5).mean(),f.sum(),len(c)))
rw=np.asarray(M.Ct[:, :V].sum(1)).ravel()   # her satir 1 kelime slotu
fw=(w/(w+85))[M.w1t]; print('   egitim satirlari: kelime slotu faktoru medyan %.3f ; satirlarin %.1f%%\'inde <0.3'%(np.median(fw),100*(fw<0.3).mean()))
has=M.pt>=0; fp=(p/(p+85))[M.pt[has]]; print('   cift slotu olan satirlar %.1f%%; bunlarda cift faktoru medyan %.3f, <0.3 olan %.1f%%'%(100*has.mean(),np.median(fp),100*(fp<0.3).mean()))
print('   cift slotlari: sayim dagilimi min %d medyan %d maks %d ; sayim<=3: %d'%(p.min(),np.median(p),p.max(),(p<=3).sum()))
# --- 3) Schur spektrumu, df, rank
print('\n== 3) F BLOGU (C ve sabit kosullu) SCHUR SPEKTRUMU')
ev,U=np.linalg.eigh(m.S); ev=np.maximum(ev[::-1],0); U=U[:,::-1]
tr_S,tr_A=np.trace(m.S),np.trace(m.AFF); print('   iz(S)/iz(AFF) = %.4f -> F enerjisinin %.1f%%\'i C+sabit ile ACIKLANIYOR, %.1f%% F\'e ozgu'%(tr_S/tr_A,100*(1-tr_S/tr_A),100*tr_S/tr_A))
for lam in (300,3000,30000): print('   df(lam=%g) = %.1f / 512 ; ezilen (ev<lam) yon sayisi %d'%(lam,(ev/(ev+lam)).sum(),(ev<lam).sum()))
print('   ozdeger: maks %.4g medyan %.4g min %.3g ; rank(>1e-6 maks)=%d ; rank(>1e-3)=%d ; PR=%.2f ; kosul sayisi S+3000I = %.3g ; S kosul %.3g'%(ev[0],np.median(ev),ev[-1],(ev>1e-6*ev[0]).sum(),(ev>1e-3*ev[0]).sum(),ev.sum()**2/(ev**2).sum(),(ev[0]+3000)/(ev[-1]+3000),ev[0]/max(ev[-1],1e-30)))
ca=np.linalg.cond(m.ACC+85*np.eye(M.NC)); print('   A_CC+85I kosul sayisi %.3g'%ca)
# --- 4) hedef sinyali (rs) PC bazinda ve genelleme
rsU=U.T@m.rs; en=(rsU**2).sum(1)/np.maximum(ev,1e-12)   # tek basina OLS ile aciklanan enerji
cum=np.cumsum(en)/en.sum()
print('\n== 4) HEDEF SINYALI (C-sonrasi artik RHS) Schur ana bilesenlerinde')
for k in (10,35,100,200,300): print('   ilk %d PC: OLS sinyal enerjisi payi %.3f'%(k,cum[k-1]))
# genelleme: val tarafinda ayni projeksiyon (artik hedef = Y - C-tahmini)
Rv=np.zeros((M.nv,V)); Rv[np.arange(M.nv),M.yv]=1; Rv-=m.CvY0           # val artik hedef (C-modeline gore)
rsv=m.Fvr.T@Rv; rsvU=U.T@rsv
def cosg(a,b,lo,hi):
    A=rsU[lo:hi]/M.n; B=rsvU[lo:hi]/M.nv; return (A*B).sum()/np.sqrt((A**2).sum()*(B**2).sum()+1e-30)
print('   egitim ve dogrulama sinyal vektorlerinin kosinus benzerligi (yon grubu icin; 1=ayni yon, 0=iliskisiz):')
for lo,hi in ((0,10),(10,35),(35,100),(100,200),(200,300),(300,512)): print('     PC %3d-%3d : cos = %.3f ; egitim sinyal enerjisi payi %.3f'%(lo,hi,cosg(0,0,lo,hi),en[lo:hi].sum()/en.sum()))
# --- 5) kuyruk lambda duyarliligi
print('\n== 5) RIDGE DUYARLILIGI: PC grubu bazinda lambda carpani (val CE, okuma sabit a=%s eps=%g)'%(aV,eV))
rsU_=U.T@m.rs
def Wtail(K,mult,lam=3000):
    lv=np.full(512,float(lam)); lv[K:]*=mult; Wp=(rsU_/(ev+lv)[:,None]); return U@Wp
base=ce_split(m,W,'val',fix)
print('   taban %.4f'%base)
for K in (35,100,200):
    row=[]
    for mult in (1,0.3,0.1,0.03,0.01):
        Wk=Wtail(K,mult); row.append('x%g: %.4f'%(mult,ce_split(m,Wk,'val',fix)))
    print('   ilk %3d PC lam=3000 sabit, sonrakilerin lam carpani ->'%K,' | '.join(row))
for K in (35,100,200):
    Wk=Wtail(K,1e9); print('   yalniz ilk %d PC (kuyruk tamamen kapali): val CE %.4f'%(K,ce_split(m,Wk,'val',fix)))
np.save('S_ev.npy',ev)
