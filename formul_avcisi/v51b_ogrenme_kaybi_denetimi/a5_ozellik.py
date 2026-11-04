import sys,time; sys.path.insert(0,'.'); import numpy as np, scipy.linalg as sl
import model as M
V=M.V; F,Fv,Fe=M.load(); m=M.Fit(F,Fv,Fe); lam=3000; a0,e0=0.85,1e-5
r=np.arange(M.nv)
def cefix(Z,y=M.yv): S=np.maximum(Z,e0); return -np.log((1-a0)*M.uni[y]+a0*S[np.arange(len(y)),y]/S.sum(1)).mean()
def ev_sub(sub): W=m.WF(lam,sub); return cefix(m.logits(W,sub))
base=ev_sub(None); print('taban val CE (okuma sabit a=%.2f eps=%g): %.4f'%(a0,e0,base))
idx=np.arange(512); typ=np.where(idx<510,idx%3,0)   # 0: A, 1: (A-mu)^2, 2: (A-mu)^3 ; son 2 sutun A kopyasi
print('\n== OZELLIK TURLERI (V49: 170 taban x {A,A^2,A^3} + 2 A) ablation (lamF=3000)')
for nm,sel in (('yalniz A (172 sutun)',typ==0),('A + A^2 (342)',typ<=1),('A + A^3 (342)',(typ==0)|(typ==2)),('A^2 + A^3 (340)',(typ>=1)&(idx<510)),('A^2 (170)',(typ==1)&(idx<510)),('A^3 (170)',(typ==2)&(idx<510)),('hepsi (512)',np.ones(512,bool))):
    sub=np.where(sel)[0]; print('   %-24s val CE %.4f  (df=%.1f)'%(nm,ev_sub(sub),(np.linalg.eigvalsh(m.S[np.ix_(sub,sub)])/(np.linalg.eigvalsh(m.S[np.ix_(sub,sub)])+lam)).sum()))
rng=np.random.default_rng(0)
for fr in (0.25,0.5,0.75):
    cs=[ev_sub(np.sort(rng.choice(512,int(512*fr),replace=False))) for _ in range(3)]; print('   rastgele %%%d sutun: val CE %.4f +- %.4f'%(int(fr*100),np.mean(cs),np.std(cs)))
# --- drop-one
print('\n== SUTUN BAZINDA MARJINAL KATKI: her sutunu tek tek cikar (512 yeniden cozum), delta val CE')
t=time.time(); d=np.zeros(512)
for j in range(512): d[j]=ev_sub(np.delete(idx,j))-base
print('   sure %.0fs'%(time.time()-t)); np.save('dropone.npy',d)
print('   delta CE: min %.2e medyan %.2e maks %.2e ; toplam (tek tek) %.4f'%(d.min(),np.median(d),d.max(),d.sum()))
print('   sutun sayisi: cikarilinca CE IYILESIR (delta<-1e-5): %d ; |delta|<1e-5 (etkisiz): %d ; zarar >1e-5: %d ; >1e-4: %d ; >1e-3: %d'%((d<-1e-5).sum(),(np.abs(d)<1e-5).sum(),(d>1e-5).sum(),(d>1e-4).sum(),(d>1e-3).sum()))
for t_,nm in ((0,'A'),(1,'A^2'),(2,'A^3')): s=(typ==t_)&(idx<510); print('   tur %-4s: medyan delta %.2e ; etkisiz(|d|<1e-5) %d/%d ; iyilestirme-ile-cikarilan %d'%(nm,np.median(d[s]),(np.abs(d[s])<1e-5).sum(),s.sum(),(d[s]<-1e-5).sum()))
# --- yedeklilik
print('\n== YEDEKLILIK / ILISKI')
Fc=F-F.mean(0); sd=Fc.std(0); dead=(sd<1e-6).sum(); R=np.corrcoef(Fc.T); np.fill_diagonal(R,0); ar=np.abs(R)
print('   olu sutun (std<1e-6): %d ; |r|>0.99 ciftleri: %d ; |r|>0.9: %d ; |r|>0.5: %d (toplam cift %d)'%(dead,(np.triu(ar)>0.99).sum(),(np.triu(ar)>0.9).sum(),(np.triu(ar)>0.5).sum(),512*511//2))
print('   her sutunun baska bir sutunla maks |r| >0.99 olan: %d ; >0.9: %d ; medyan maks|r| %.3f'%((ar.max(1)>0.99).sum(),(ar.max(1)>0.9).sum(),np.median(ar.max(1))))
ev=np.linalg.eigvalsh(np.cov(F.T))[::-1]; print('   ham F kovaryans: PR %.2f ; rank(>1e-6 maks) %d ; ilk PC varyans payi %.3f ; ilk 10 PC %.3f ; ilk 50 PC %.4f'%(ev.sum()**2/(ev**2).sum(),(ev>1e-6*ev[0]).sum(),ev[0]/ev.sum(),ev[:10].sum()/ev.sum(),ev[:50].sum()/ev.sum()))
for t_,nm in ((0,'A'),(1,'A^2'),(2,'A^3')):
    s=np.where((typ==t_)&(idx<510))[0]; e_=np.linalg.eigvalsh(np.cov(F[:,s].T))[::-1]; print('   tur %-4s kendi icinde PR %.2f, rank(>1e-6) %d'%(nm,e_.sum()**2/(e_**2).sum(),(e_>1e-6*e_[0]).sum()))
# eta^2 : kelime kimligiyle aciklanan varyans
cn=np.bincount(M.w1t,minlength=V).astype(float); mu_w=np.zeros((V,512)); np.add.at(mu_w,M.w1t,F); mu_w/=np.maximum(cn,1)[:,None]
within=((F-mu_w[M.w1t])**2).sum(0); tot=(Fc**2).sum(0); eta=1-within/tot
print('   F sutunlarinin son-kelimeyle (w_{t-1}) aciklanan varyans orani eta^2: ortalama %.3f ; medyan %.3f ; min %.3f ; maks %.3f (egitim, kelime sayimi kucuk oldugundan ust yanli)'%(eta.mean(),np.median(eta),eta.min(),eta.max()))
muv=np.zeros((V,512)); muv[:]=mu_w; resv=((Fv-mu_w[M.w1v])**2).sum(0); totv=((Fv-F.mean(0))**2).sum(0); print('   dogrulamada egitim kelime-ortalamasiyla aciklanan varyans (hold-out eta^2) ortalama %.3f'%(1-resv/totv).mean())
# --- katki yogunlugu
Wf=m.WF(lam); imp=np.abs(Wf).sum(1)*sd; o=np.argsort(imp)[::-1]; cu=np.cumsum(imp[o])/imp.sum()
print('\n== AGIRLIK KATKI YOGUNLUGU (sum|W_j| * std_j): ilk 10 sutun %.3f, ilk 50 %.3f, ilk 128 %.3f, ilk 256 %.3f ; son 256 sutun payi %.3f'%(cu[9],cu[49],cu[127],cu[255],1-cu[255]))
print('   sutun-agirlik norm orani maks/medyan %.1f'%(imp.max()/np.median(imp)))
