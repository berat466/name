#!/usr/bin/env python3
"""Sure denetimi: klasik MLP (id+X, V32'de secilen sabit konfigurasyon) icin TEK egitim kosusunun zaman dokumu.
V32 betigiyle ayni model/veri/optimizer/erken durdurma (patience 6); yalniz zamanlayicilar eklendi. Yeni model egitimi degil: V32'nin secilen konfigurasyonunun olcumu."""
import time; t_imp0=time.time()
import numpy as np, torch, torch.nn as nn, json
t_imp=time.time()-t_imp0
torch.set_num_threads(4); V=2001; d='/tmp/v8data'
t0=time.time()
def load(s):
    n={'train':39840,'val':4980,'test':4980}[s]
    X=np.fromfile(f'{d}/c2_{s}_X.f32',dtype=np.float32).reshape(n,-1); ctx=np.fromfile(f'{d}/c2_{s}_ctx.i32',dtype=np.int32).reshape(n,2); y=np.fromfile(f'{d}/c2_{s}_y.i32',dtype=np.int32)
    return torch.tensor(X),torch.tensor(ctx,dtype=torch.long),torch.tensor(y,dtype=torch.long)
D={s:load(s) for s in ('train','val','test')}; t_load=time.time()-t0
Kx=D['train'][0].shape[1]
class MLP(nn.Module):
    def __init__(s,emb,hid,drop):
        super().__init__(); s.E=nn.Embedding(V,emb); s.net=nn.Sequential(nn.Dropout(drop),nn.Linear(2*emb+Kx,hid),nn.Tanh(),nn.Dropout(drop),nn.Linear(hid,V))
    def forward(s,X,ctx): return s.net(torch.cat([s.E(ctx).flatten(1),X],1))
def ce(m,s):
    m.eval(); X,c,y=D[s]; tot=0.
    with torch.no_grad():
        for i in range(0,len(y),2048): tot+=nn.functional.cross_entropy(m(X[i:i+2048],c[i:i+2048]),y[i:i+2048],reduction='sum').item()
    return tot/len(y)
cfg=dict(emb=32,hid=256,drop=0.3,lr=2e-3,wd=1e-2)
torch.manual_seed(0); t0=time.time(); m=MLP(cfg['emb'],cfg['hid'],cfg['drop']); opt=torch.optim.AdamW(m.parameters(),lr=cfg['lr'],weight_decay=cfg['wd']); t_init=time.time()-t0
X,c,y=D['train']; n=len(y); best=1e9; bad=0; ep_log=[]; T_train=0.; T_val=0.; updates=0; t_all0=time.time(); t_to_q=None
for ep in range(80):
    m.train(); perm=torch.randperm(n); ta=time.time()
    for i in range(0,n,256):
        idx=perm[i:i+256]; opt.zero_grad(); loss=nn.functional.cross_entropy(m(X[idx],c[idx]),y[idx]); loss.backward(); opt.step(); updates+=1
    dt=time.time()-ta; T_train+=dt; tb=time.time(); v=ce(m,'val'); dv=time.time()-tb; T_val+=dv
    ep_log.append((ep+1,dt,dv,float(np.exp(v))))
    if t_to_q is None and np.exp(v)<=147.0: t_to_q=time.time()-t_all0   # V31 dogrulama PPL = 147.0 seviyesine ilk ulasma (egitim+dogrulama suresi dahil)
    if v<best-1e-4: best=v; bad=0
    else:
        bad+=1
        if bad>=6: break
T_run=time.time()-t_all0
tt=time.time(); te=ce(m,'test'); t_test=time.time()-tt
print(json.dumps(dict(torch_import_s=t_imp,veri_yukleme_s=t_load,model_kurma_s=t_init,epok=ep+1,agirlik_guncelleme=updates,batch=256,egitim_adim_s=T_train,dogrulama_degerlendirme_s=T_val,egitim_kosusu_toplam_s=T_run,test_degerlendirme_s=t_test,test_PPL=float(np.exp(te)),en_iyi_dogr_PPL=float(np.exp(best)),V31_dogr_PPL_seviyesine_ilk_ulasma_s=t_to_q,epok_ayrintisi=ep_log),ensure_ascii=False))
