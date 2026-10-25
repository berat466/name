import sys,time; sys.path.insert(0,'.'); import numpy as np
import model as M
V=M.V; uni=M.uni
def counts(w1,y): C=np.zeros((V,V)); np.add.at(C,(w1,y),1); return C
def graph(Cn,K=10):
    nw=Cn.sum(1); Nn=np.sqrt(Cn); Nn/=np.maximum(np.linalg.norm(Nn,axis=1,keepdims=True),1e-9); S=Nn@Nn.T
    cand=np.where(nw>=5)[0]; nb=-np.ones((V,K),int); wt=np.zeros((V,K)); 
    Sc=S[:,cand].copy(); 
    for u in range(V):
        s=Sc[u].copy(); 
        if nw[u]>=5 or True:
            s[cand==u]=-1
        kk=min(K,len(cand)); o=np.argsort(-s,kind='stable')[:kk]; nb[u,:kk]=cand[o]; w=np.maximum(s[o],0); wt[u,:kk]=w/max(w.sum(),1e-9)
    return nb,wt,S,nw,cand
def qtab(Cn,nb,wt,b=0.9):
    nw=Cn.sum(1); P=Cn/np.maximum(nw,1)[:,None]; Q=np.zeros((V,V)); has=np.zeros(V,bool)
    for u in range(V):
        ok=nb[u]>=0; ws=wt[u][ok].sum()
        N=(wt[u][ok,None]*P[nb[u][ok]]).sum(0)/ws if ws>0 else None
        if nw[u]>0 and N is not None: Q[u]=(1-b)*P[u]+b*N
        elif nw[u]>0: Q[u]=P[u]
        elif N is not None: Q[u]=N
        else: Q[u]=uni
        Q[u]/=Q[u].sum()
    return Q
t=time.time(); Cn=counts(M.w1t,M.yt); nb,wt,S,nw,cand=graph(Cn); Q=qtab(Cn,nb,wt); print('graf+Q',round(time.time()-t,1),'s')
# --- referans modeli (lamF=3000) ve alpha=0.2 son karisim dogrulamasi
F,Fv,Fe=M.load(); m=M.Fit(F,Fv,Fe); W=m.WF(3000); Zv=m.logits(W,None,'val'); Ze=m.logits(W,None,'test'); a,e=0.85,1e-5
Pv=M.read_P(Zv,a,e); r=np.arange(M.nv); 
ce=lambda P,y: -np.log(P[np.arange(len(y)),y]).mean()
print('== SON KARISIM DOGRULAMASI: model okuma CE val %.4f ; P_son=0.8P+0.2Q CE val %.4f (C++ 4.8242) ; test %.4f'%(ce(Pv,M.yv),ce(0.8*Pv+0.2*Q[M.w1v],M.yv),ce(0.8*M.read_P(Ze,a,e)+0.2*Q[M.w1e],M.ye)))
print('\n== TOP-K / KOMSULUK (V39 grafi: Hellinger-kosinus, aday=egitim sayimi>=5, K=10)')
nc=len(cand); print('   adaylar: %d kelime (sayim>=5) / 2001 ; %d kelime (%.1f%% kelime, egitim token payi %.1f%%) HIC KOMSU OLAMAZ'%(nc,V-nc,100*(V-nc)/V,100*Cn.sum(1)[nw<5].sum()/Cn.sum()))
tokw=nw/nw.sum(); 
s_top=np.array([np.sort(S[u,cand][cand!=u])[::-1] for u in range(V)],dtype=object)
r1=np.array([x[0] for x in s_top]); r10=np.array([x[9] for x in s_top]); r11=np.array([x[10] for x in s_top]); r30=np.array([x[29] for x in s_top])
print('   benzerlik: 1.komsu medyan %.3f ; 10. komsu %.3f ; 11. (ilk atilan) %.3f ; 30. %.3f ; son aday %.3f'%(np.median(r1),np.median(r10),np.median(r11),np.median(r30),np.median([x[-1] for x in s_top])))
mass=np.array([x[:10].sum()/max(x.sum(),1e-9) for x in s_top]); print('   top-10\'un toplam benzerlik kutlesi payi (tum adaylar icinde): kelime ort. %.3f ; token-agirlikli %.3f'%(mass.mean(),(mass*tokw).sum()))
gap=r10-r11; near=((r11>=0.98*r10)).mean(); print('   top-10 siniri: 10.-11. fark medyan %.4f ; 11. komsu 10.nun %%98\'inden buyuk olan kelime orani %.1f%% (sinir yakini adaylar neredeyse ayni benzerlikte)'%(np.median(gap),100*near))
sets={}; 
for u in range(V): sets.setdefault(tuple(sorted(nb[u])),[]).append(u)
big=sorted(sets.values(),key=len,reverse=True); print('   farkli komsu listesi sayisi: %d / 2001 kelime ; en buyuk ayni-liste grubu %d kelime ; ayni listeyi paylasan kelime sayisi (grup>=2): %d'%(len(sets),len(big[0]),sum(len(g) for g in big if len(g)>=2)))
fr=np.bincount(nb[nb>=0].ravel(),minlength=V); print('   komsu olarak kullanilan farkli kelime: %d / %d aday ; en cok kullanilan kelime %d kez (toplam 20010 baglanti)'%((fr>0).sum(),nc,fr.max()))
ws=wt.max(1); print('   en buyuk komsu agirligi: medyan %.3f ; 0.1 (tekduze) uzerinde %.1f%%'%(np.median(ws),100*(ws>0.11).mean()))
# kelime sikligina gore: nadir kelimeler icin komsular
rare=nw<5; print('   nadir kelimeler (sayim<5, %d adet): kendi benzerlik vektoru <5 ornekten -> 1.komsu benzerlik medyan %.3f (siki kelimelerde %.3f)'%(rare.sum(),np.median(r1[rare]),np.median(r1[~rare])))
print('\n== Q TABLOSU KALITESI (yalniz son asama; K ve b degisimi) val CE ; model sabit (lamF=3000)')
def fused(Qt): return ce(0.8*Pv+0.2*Qt[M.w1v],M.yv)
P_w=Cn/np.maximum(nw,1)[:,None]; P_w[nw==0]=uni
print('   Q yalniz (val) CE: mix(b=0.9,K=10) %.4f ; P_w tek basina %.4f ; unigram %.4f'%(ce(Q[M.w1v],M.yv),ce(0.999*P_w[M.w1v]+0.001*uni,M.yv),ce(np.tile(uni,(M.nv,1)),M.yv)))
for K in (1,3,5,10,20,40,len(cand)):
    nbk,wtk,_,_,_=graph(Cn,K); Qk=qtab(Cn,nbk,wtk); print('   K=%-4d : Q yalniz val CE %.4f ; son karisim (alpha=.2) val CE %.4f'%(K,ce(Qk[M.w1v],M.yv),fused(Qk)))
z0=(Q[M.w1v][r,M.yv]==0).mean(); print('   val satirlarinda Q[w][y]==0 (hedefe sifir kutle): %.1f%% ; test %.1f%%'%(100*z0,100*(Q[M.w1e][np.arange(M.ne),M.ye]==0).mean()))
# --- OOF
print('\n== OOF (5 kat, belge/blok bazli: blok index %% 5)')
fold=(np.arange(M.n)//498)%5
print('   kat buyuklukleri:',np.bincount(fold))
tot_removed=[];unseen_oof=[];pair_unseen=[];zeroQ=[];ceoof=[];cand_oof=[]
Qf=[]
for f in range(5):
    keep=fold!=f; Cf=counts(M.w1t[keep],M.yt[keep]); nbf,wtf,_,nwf,cf=graph(Cf); Qf_=qtab(Cf,nbf,wtf); Qf.append(Qf_); cand_oof.append(len(cf))
    tr_i=np.where(fold==f)[0]; w=M.w1t[tr_i]; unseen_oof.append((nwf[w]==0).mean()); zeroQ.append((Qf_[w,M.yt[tr_i]]==0).mean()); ceoof.append(-np.log(Qf_[w,M.yt[tr_i]]+1e-12).mean())
    # cift: top-2000 cift icinde olan satirlar icin kat disi cift sayimi
print('   egitim satirlari icin kat-disi sayim kaybi: kelime sayimlarinin %%%.0f\'i kullanilamaz (kendi kat) ; K=10 graf adayi (sayim>=5): tam %d, OOF ort %.0f'%(100*0.2,len(cand),np.mean(cand_oof)))
print('   satirin son kelimesi OOF bellekte HIC gorulmeyen (n_w=0): ort %.2f%% (val/test icin tam egitimde: %.2f%% / %.2f%%)'%(100*np.mean(unseen_oof),100*(nw[M.w1v]==0).mean(),100*(nw[M.w1e]==0).mean()))
print('   OOF Q[w][y]==0 (hedefe sifir kutle): egitim ort %.1f%% ; val (tam tablo) %.1f%%'%(100*np.mean(zeroQ),100*z0))
# egitim OOF Q'nun CE'si vs val
Qoof=np.zeros(M.n); 
for f in range(5): i=np.where(fold==f)[0]; Qoof[i]=Qf[f][M.w1t[i],M.yt[i]]
print('   Q CE: egitim OOF %.4f ; val (tam tablo) %.4f -> OOF Q kalitesi val ile %s'%(-np.log(Qoof+1e-12).mean(),ce(Q[M.w1v],M.yv),'benzer' if abs(-np.log(Qoof+1e-12).mean()-ce(Q[M.w1v],M.yv))<0.1 else 'farkli'))
# C blogu: cift slotlari
pt=M.pt; print('   C blogu: cift slotu (top-2000 cift) olan satir: train %.1f%% val %.1f%% test %.1f%% ; slotu olmayan satirlar yalniz kelime+sabit ile temsil edilir'%(100*(M.pt>=0).mean(),100*(M.pv>=0).mean(),100*(M.pe>=0).mean()))
# gorulmeyen cift (hic egitimde yok)
allp=set((M.w2t.astype(np.int64)*V+M.w1t).tolist()); 
upv=np.mean([int(a)*V+int(b) not in allp for a,b in zip(M.w2v,M.w1v)]); upe=np.mean([int(a)*V+int(b) not in allp for a,b in zip(M.w2e,M.w1e)])
print('   egitimde HIC gorulmeyen (w_{t-2},w_{t-1}) cifti: val %.1f%%, test %.1f%% ; egitimde en az 1 kez: %d farkli cift'%(100*upv,100*upe,len(allp)))
np.save('Q_full.npy',Q.astype(np.float32))
