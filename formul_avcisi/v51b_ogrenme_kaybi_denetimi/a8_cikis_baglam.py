import sys; sys.path.insert(0,'.'); import numpy as np
import model as M
V=M.V; uni=M.uni
print('== GIRDI/BAGLAM TEMSIL CAKISMALARI')
# gomme tablosu (X'ten) ve ayni/yakin gomme
X=np.fromfile('/tmp/v8data/c2_train_X.f32',dtype=np.float32).reshape(M.n,128); E=np.zeros((V,64),np.float32); seen=np.zeros(V,bool)
for i in range(M.n):
    w=M.w1t[i]
    if not seen[w]: E[w]=X[i,64:]; seen[w]=True
nrm=np.linalg.norm(E,axis=1); print('   gomme tablosu: normu 0 olan kelime %d ; norm<0.5: %d ; tum normlar medyan %.3f'%((nrm<1e-6).sum(),(nrm<0.5).sum(),np.median(nrm)))
En=E/np.maximum(nrm,1e-9)[:,None]; G=En@En.T; np.fill_diagonal(G,-1)
print('   farkli kelimeler arasi kosinus >0.99: %d cift ; >0.95: %d cift ; her kelimenin en yakin baska kelimeyle medyan kosinus %.3f'%((np.triu(G)>0.99).sum(),(np.triu(G)>0.95).sum(),np.median(G.max(1))))
# kelime sayimi vs gomme normu
cn=np.bincount(M.w1t,minlength=V); print('   gomme kalitesi sayima bagli mi: sayim<5 kelimelerde norm medyan %.3f vs sayim>=100: %.3f'%(np.median(nrm[cn<5]),np.median(nrm[cn>=100])))
# C temsil cakismasi
key=np.where(M.pt>=0,M.pt+V,-1)
print('   C blogu temsil tipleri (egitim): pair slotu yok -> satir yalniz (w_{t-1}, sabit): %.1f%% satir; bu satirlarda w_{t-2} bilgisi C\'de YOK (yalniz X/F\'te).'%(100*(M.pt<0).mean()))
for nm,w2,w1,p in (('train',M.w2t,M.w1t,M.pt),('val',M.w2v,M.w1v,M.pv),('test',M.w2e,M.w1e,M.pe)):
    print('   %s: farkli (w_{t-2},w_{t-1}) cifti %d / %d satir ; C\'de ayni vektore (yalniz kelime slotu) inen satir %.1f%% ; pair slotu olan %.1f%%'%(nm,len(set(zip(w2.tolist(),w1.tolist()))),len(w1),100*(p<0).mean(),100*(p>=0).mean()))
print('   w_{t-3} ve daha eski kelimeler: hicbir blokta yok (c=2). Ek: UNK (id0) hem baglamda hem hedefte tek sinif: egitim baglamlarinin %.1f%%\'inde w_{t-1}=UNK'%(100*(M.w1t==0).mean()))
# --- cikis
print('\n== CIKIS ANALIZI (model: lamF=3000, ORTAK rho=0; okuma a=0.85 eps=1e-5; son karisim alpha=0.2)')
F,Fv,Fe=M.load(); m=M.Fit(F,Fv,Fe); W=m.WF(3000); Q=np.load('Q_full.npy').astype(np.float64); a,e=0.85,1e-5
for sp_,w1,y,Z in (('train',M.w1t,M.yt,m.logits(W,None,'train')),('val',M.w1v,M.yv,m.logits(W,None,'val')),('test',M.w1e,M.ye,m.logits(W,None,'test'))):
    nr=len(y); r=np.arange(nr); zy=Z[r,y]
    neg=(Z<0).mean(); lt=(Z<e).mean(); big=(Z>1).mean()
    S=np.maximum(Z,e); mass_add=(S.sum(1)-Z.sum(1)).mean(); negmass=np.maximum(-Z,0).sum(1).mean()
    P=M.read_P(Z,a,e); Ps=0.8*P+0.2*Q[w1] if sp_!='train' else None
    cep=-np.log(P[r,y]); 
    print('\n  [%s] satir %d'%(sp_,nr))
    print('   z<0: %.1f%% ; z<eps(1e-5): %.1f%% ; z>1: %.4f%% ; hedef sinif z_y<eps: %.1f%% satir ; z toplam ort %.3f ; kirpmayla EKLENEN kutle (Sum max(z,eps)-Sum z) ort %.3f ; negatif kutle ort %.3f'%(100*neg,100*lt,100*big,100*(zy<e).mean(),Z.sum(1).mean(),mass_add,negmass))
    c_clip=cep[zy<e].sum()/cep.sum(); print('   CE(okuma): %.4f ; z_y<eps olan satirlar CE payi %.1f%% (satir payi %.1f%%) ; bu satirlarda ort CE %.3f vs digerleri %.3f'%(cep.mean(),100*c_clip,100*(zy<e).mean(),cep[zy<e].mean() if (zy<e).any() else 0,cep[zy>=e].mean()))
    # siralama
    am_z=Z.argmax(1); am_p=P.argmax(1); t5z=np.argsort(-Z,1)[:,:5]; t5p=np.argsort(-P,1)[:,:5]
    acc=lambda top,k: np.mean([(y[i] in top[i][:k]) for i in range(nr)])
    print('   Top-1 / Top-5 doğruluk: ham z (kirpmasiz) %.4f / %.4f ; okuma P (kirpma+unigram) %.4f / %.4f'%(acc(np.argsort(-Z,1)[:,:1],1),acc(t5z,5),acc(t5p[:,:1],1),acc(t5p,5)))
    print('   okumanin (kirpma+unigram karisimi) argmax degistirdigi satir: %.1f%% ; top-5 kumesini degistirdigi satir: %.1f%%'%(100*(am_z!=am_p).mean(),100*np.mean([set(t5z[i])!=set(t5p[i]) for i in range(0,nr,3)])))
    tie=(np.sort(Z,1)[:,-5]<=e).mean(); print('   en iyi 5. skoru da eps altinda (top-5 siralamasi kirpmayla belirlenen) satir: %.2f%%'%(100*tie))
    if Ps is not None:
        t5s=np.argsort(-Ps,1)[:,:5]; print('   P_son: CE %.4f ; Top-1 %.4f ; Top-5 %.4f ; argmax P_model->P_son degisen %.1f%%'%(-np.log(Ps[r,y]).mean(),acc(t5s[:,:1],1),acc(t5s,5),100*(t5s[:,0]!=am_p).mean()))
        unk=(y==0); print('   UNK (id0) hedefli satir %.1f%% ; UNK satirlari CE %.3f vs diger %.3f ; CE payi %.1f%% ; UNK Top-1 dogru %.1f%%'%(100*unk.mean(),-np.log(Ps[r,y])[unk].mean(),-np.log(Ps[r,y])[~unk].mean(),100*(-np.log(Ps[r,y])[unk]).sum()/(-np.log(Ps[r,y])).sum(),100*(t5s[unk,0]==0).mean()))
        p1=np.sort(Ps,1)[:,::-1]; print('   birbirinden ayirt edilemeyen: P1/P2<1.05 olan satir %.1f%% ; top-5 olasilik kutlesi medyan %.3f'%(100*(p1[:,0]/p1[:,1]<1.05).mean(),np.median(p1[:,:5].sum(1))))
        cover=np.zeros(V,bool); cover[np.unique(t5s)]=True; print('   top-5 icinde hic gorunmeyen sinif: %d / 2001 (%.1f%%)'%((~cover).sum(),100*(~cover).mean()))
# okuma duyarliligi
Zv=m.logits(W,None,'val'); r=np.arange(M.nv)
print('\n  OKUMA DUYARLILIGI (val CE; mevcut tanim degismeden eps/a taramasi):')
for e_ in (1e-7,1e-5,1e-3,1e-2): 
    S=np.maximum(Zv,e_); print('   eps=%g: '%e_+' '.join('a=%.2f:%.4f'%(a_,-np.log((1-a_)*uni[M.yv]+a_*S[r,M.yv]/S.sum(1)).mean()) for a_ in (0.7,0.85,0.95,1.0)))
