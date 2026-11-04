import sys; sys.path.insert(0,'.'); import numpy as np
import model as M
F,Fv,Fe=M.load('/tmp/v52Q'); m=M.Fit(F,Fv,Fe); a0,e0=0.85,1e-5
def cefix(Z,y=M.yv): S=np.maximum(Z,e0); return -np.log((1-a0)*M.uni[y]+a0*S[np.arange(len(y)),y]/S.sum(1)).mean()
def ev_sub(sub,lam=3000): W=m.WF(lam,sub); return cefix(m.logits(W,sub))
print('== V49 ATILAN YONLER (duz F + gate g_F=0.15, V49 donusumu YOK; 512 ayri taban yon)')
print('   duz F, 512 yon: val CE (ORTAK, okuma sabit) %.4f   [V49 B (170x3) ayni olcumde: 4.8527]'%ev_sub(None))
for k in (172,342,512):
    print('   duz F ilk %d yon: %.4f'%(k,ev_sub(np.arange(k))))
print('   duz F, V49\'un ATTIGI 340 yon (170..509) tek basina: %.4f'%ev_sub(np.arange(170,510)))
for lam in (1000,3000,10000): print('   lamF=%g duz F 512: %.4f ; ilk 172: %.4f'%(lam,ev_sub(None,lam),ev_sub(np.arange(172),lam)))
ev=np.linalg.eigvalsh(m.S)[::-1]; print('   duz F Schur: df(3000) = %.1f ; PR %.2f ; rank(>1e-3) %d'%((ev/(ev+3000)).sum(),ev.sum()**2/(ev**2).sum(),(ev>1e-3*ev[0]).sum()))
# agirlik sifira yakinligi
W=m.WF(3000); aw=np.abs(W); print('   F agirliklari: |W|<1e-3*maks olan oge orani %.1f%% ; satir normu (sutun) medyan/maks %.3g / %.3g'%(100*(aw<1e-3*aw.max()).mean(),np.median(np.linalg.norm(W,axis=1)),np.linalg.norm(W,axis=1).max()))
W0=m.Y0-m.X@W; a0w=np.abs(W0); print('   C agirliklari (kelime/cift/sabit): |W_C|<1e-3*maks olan oge orani %.1f%% ; kelime slotu satir normu medyan %.3g, cift slotu medyan %.3g'%(100*(a0w<1e-3*a0w.max()).mean(),np.median(np.linalg.norm(W0[:2001],axis=1)),np.median(np.linalg.norm(W0[2001:4001],axis=1))))
