#!/usr/bin/env python3
"""V32: klasik (gradyanla egitilmis) taban cizgileri, AYNI veri/bolme/degerlendirme konumlari.
Degerlendirme: her 500-token blokta t>=2 konumlari (egitim 39840 / dogrulama 4980 / test 4980 ornek), PPL=exp(ort. CE).
Secim (hiperparametre, erken durdurma) YALNIZ dogrulamada; test yalniz secilen modelde bir kez.
Modeller: mlp_id (sira duyarli 2 kelime kimligi, ogrenilen gomme), mlp_X (sabit PPMI-SVD X, bizim girdi), mlp_id_X (ikisi), lstm (blok ici tam gecmis; DAHA FAZLA BAGLAM)."""
import numpy as np, torch, torch.nn as nn, time, json, sys, itertools, random
torch.set_num_threads(4)
d='/tmp/v8data'; V=2001
ids={s:np.load(f'{d}/ids_{s}.npy') for s in ('train','val','test')}
def load(s):
    n={'train':39840,'val':4980,'test':4980}[s]
    X=np.fromfile(f'{d}/c2_{s}_X.f32',dtype=np.float32).reshape(n,-1); ctx=np.fromfile(f'{d}/c2_{s}_ctx.i32',dtype=np.int32).reshape(n,2); y=np.fromfile(f'{d}/c2_{s}_y.i32',dtype=np.int32)
    return torch.tensor(X),torch.tensor(ctx,dtype=torch.long),torch.tensor(y,dtype=torch.long)
D={s:load(s) for s in ('train','val','test')}
# hizalama kontrolu: ctx/y bloklardaki konumlarla ayni mi
for s in ('train','val','test'):
    b=ids[s]; yy=np.concatenate([b[i,2:] for i in range(len(b))]); cc=np.concatenate([np.stack([b[i,:-2],b[i,1:-1]],1) for i in range(len(b))])
    assert (yy==D[s][2].numpy()).all() and (cc==D[s][1].numpy()).all(), 'hizalama bozuk'
print('HIZALAMA|ctx/y, bloklardaki t>=2 konumlariyla birebir ayni', flush=True)
Kx=D['train'][0].shape[1]
class MLP(nn.Module):
    def __init__(s,use_id,use_x,emb,hid,drop):
        super().__init__(); s.use_id,s.use_x=use_id,use_x; din=0
        if use_id: s.E=nn.Embedding(V,emb); din+=2*emb
        if use_x: din+=Kx
        s.net=nn.Sequential(nn.Dropout(drop),nn.Linear(din,hid),nn.Tanh(),nn.Dropout(drop),nn.Linear(hid,V))
    def forward(s,X,ctx):
        parts=[]
        if s.use_id: parts.append(s.E(ctx).flatten(1))
        if s.use_x: parts.append(X)
        return s.net(torch.cat(parts,1))
def ppl_mlp(m,s):
    m.eval(); X,c,y=D[s]; tot=0.
    with torch.no_grad():
        for i in range(0,len(y),2048): tot+=nn.functional.cross_entropy(m(X[i:i+2048],c[i:i+2048]),y[i:i+2048],reduction='sum').item()
    return tot/len(y)
def train_mlp(kind,cfg,seed,maxep=80,pat=6):
    torch.manual_seed(seed); m=MLP(kind in('id','id_X'),kind in('X','id_X'),cfg['emb'],cfg['hid'],cfg['drop']); opt=torch.optim.AdamW(m.parameters(),lr=cfg['lr'],weight_decay=cfg['wd']); X,c,y=D['train']; n=len(y); best=(1e9,None,0); t0=time.time(); bad=0
    for ep in range(maxep):
        m.train(); perm=torch.randperm(n)
        for i in range(0,n,256):
            idx=perm[i:i+256]; opt.zero_grad(); loss=nn.functional.cross_entropy(m(X[idx],c[idx]),y[idx]); loss.backward(); opt.step()
        v=ppl_mlp(m,'val')
        if v<best[0]-1e-4: best=(v,{k:t.clone() for k,t in m.state_dict().items()},ep); bad=0
        else:
            bad+=1
            if bad>=pat: break
    m.load_state_dict(best[1]); return m,best[0],best[2],time.time()-t0,ep+1
class LSTM(nn.Module):
    def __init__(s,emb,hid,drop,layers=1):
        super().__init__(); s.E=nn.Embedding(V,emb); s.l=nn.LSTM(emb,hid,layers,batch_first=True,dropout=drop if layers>1 else 0); s.dr=nn.Dropout(drop); s.o=nn.Linear(hid,V)
    def forward(s,x): h,_=s.l(s.dr(s.E(x))); return s.o(s.dr(h))
def lstm_ce(m,s):
    m.eval(); b=torch.tensor(ids[s],dtype=torch.long); tot=0.; cnt=0
    with torch.no_grad():
        lg=m(b[:,:-1])   # konum t-1 -> token t ; t>=2 degerlendirilir (indeks 1..)
        ce=nn.functional.cross_entropy(lg[:,1:].reshape(-1,V),b[:,2:].reshape(-1),reduction='sum').item(); return ce/(b.shape[0]*(b.shape[1]-2))
def train_lstm(cfg,seed,maxep=80,pat=6):
    torch.manual_seed(seed); m=LSTM(cfg['emb'],cfg['hid'],cfg['drop'],cfg.get('layers',1)); opt=torch.optim.AdamW(m.parameters(),lr=cfg['lr'],weight_decay=cfg['wd']); b=torch.tensor(ids['train'],dtype=torch.long); best=(1e9,None,0); t0=time.time(); bad=0
    for ep in range(maxep):
        m.train(); perm=torch.randperm(len(b))
        for i in range(0,len(b),8):
            x=b[perm[i:i+8]]; opt.zero_grad(); lg=m(x[:,:-1]); loss=nn.functional.cross_entropy(lg.reshape(-1,V),x[:,1:].reshape(-1)); loss.backward(); nn.utils.clip_grad_norm_(m.parameters(),1.0); opt.step()
        v=lstm_ce(m,'val')
        if v<best[0]-1e-4: best=(v,{k:t.clone() for k,t in m.state_dict().items()},ep); bad=0
        else:
            bad+=1
            if bad>=pat: break
    m.load_state_dict(best[1]); return m,best[0],best[2],time.time()-t0,ep+1
random.seed(0)
out=open('/home/user/name/formul_avcisi/v32_ham/klasik_sonuclar.jsonl','a')
def log(**k): print(json.dumps(k,ensure_ascii=False),flush=True); out.write(json.dumps(k,ensure_ascii=False)+'\n'); out.flush()
which=sys.argv[1:] or ['id','X','id_X','lstm']
for kind in which:
    if kind=='lstm':
        grid=[dict(emb=128,hid=256,drop=0.3,lr=2e-3,wd=1e-2),dict(emb=128,hid=256,drop=0.5,lr=2e-3,wd=1e-2),dict(emb=64,hid=128,drop=0.3,lr=3e-3,wd=1e-2),dict(emb=128,hid=512,drop=0.5,lr=1e-3,wd=1e-2)]
    else:
        grid=[dict(emb=e,hid=h,drop=dr,lr=lr,wd=wd) for e,h,dr,lr,wd in [(64,256,0.3,2e-3,1e-2),(64,512,0.5,2e-3,1e-2),(128,512,0.5,1e-3,1e-2),(128,256,0.5,2e-3,1e-2),(64,1024,0.5,1e-3,1e-2),(32,256,0.3,2e-3,1e-2)]]
        if kind=='X': grid=[g for g in grid if g['emb']==64]
    sel=[]
    for cfg in grid:
        m,v,bep,t,ne=(train_lstm(cfg,0) if kind=='lstm' else train_mlp(kind,cfg,0)); sel.append((v,cfg,bep,t,ne)); log(asama='secim',model=kind,cfg=cfg,dogr_CE=v,dogr_PPL=float(np.exp(v)),en_iyi_epok=bep,epok=ne,sure_s=t)
    v,cfg,_,_,_=min(sel,key=lambda z:z[0]); log(asama='secilen',model=kind,cfg=cfg,dogr_PPL=float(np.exp(v)))
    for seed in (0,1,2):
        m,v,bep,t,ne=(train_lstm(cfg,seed) if kind=='lstm' else train_mlp(kind,cfg,seed))
        te=(lstm_ce(m,'test') if kind=='lstm' else ppl_mlp(m,'test')); tr=(lstm_ce(m,'train') if kind=='lstm' else ppl_mlp(m,'train'))
        # Top-1/Top-5 (test)
        m.eval()
        with torch.no_grad():
            if kind=='lstm':
                b=torch.tensor(ids['test'],dtype=torch.long); lg=m(b[:,:-1])[:,1:].reshape(-1,V); yy=b[:,2:].reshape(-1)
            else: X,c,yy=D['test']; lg=m(X,c)
            top=lg.topk(5,1).indices; t1=(top[:,0]==yy).float().mean().item(); t5=(top==yy[:,None]).any(1).float().mean().item()
        npar=sum(p.numel() for p in m.parameters())
        log(asama='sonuc',model=kind,tohum=seed,cfg=cfg,egitim_PPL=float(np.exp(tr)),dogr_PPL=float(np.exp(v)),test_CE=te,test_PPL=float(np.exp(te)),top1=t1,top5=t5,epok=ne,en_iyi_epok=bep,egitim_suresi_s=t,parametre=npar)
