import re,glob,os
base='/home/user/name/formul_avcisi/'
L=base+'v30_ham/v30log/'
def rd(p): return open(p).read()
def asama(path,prefix='ORTAK'):
    out=[]
    for l in rd(path).replace('\\n','\n').splitlines():
        if l.startswith('ASAMA|'+prefix):
            f=l.split('|'); tg=f[2].split('=')[1]; lab=f[3]; m=re.search(r'lam=(\S+) sID=(\S+)',lab)
            q=re.search(r'dogr/test=(-?[\d.]+)/(-?[\d.]+)',l).groups()
            te=re.search(r'test CE=([\d.]+) PPL=([\d.]+) top1=([\d.]+) top5=([\d.]+)',l).groups()
            out.append(dict(name=f[1],tg=tg,lam=float(m.group(1)) if m else None,sid=float(m.group(2)) if m else None,tr=float(re.search(r'\|egitim PPL=([\d.]+)',l).group(1)),va=float(re.search(r'\|dogr PPL=([\d.]+)',l).group(1)),te=[float(x) for x in te],gap=float(re.search(r'egitim-dogr CE=(-?[\d.]+)',l).group(1)),qe=float(q[1]),al=re.search(r'alpha=(\S+) eps=(\S+)',l).groups(),st=float(re.search(r'asama sure=([\d.]+)s',l).group(1))))
    return out
def kat(path):
    out=[]
    for l in rd(path).splitlines():
        if l.startswith('KATEGORI|'):
            f=l.split('|'); m=re.search(r'n=(\d+)\|test CE=([\d.]+) PPL=([\d.]+) top1=([\d.]+)',l+'')
            mm=re.search(r'n=(\d+)',f[5]); ce=re.search(r'test CE=([\d.]+) PPL=([\d.]+) top1=([\d.]+)',f[6])
            out.append(dict(name=f[1],tg=f[2],cat=f[4],n=int(mm.group(1)),ce=float(ce.group(1)),ppl=float(ce.group(2)),t1=float(ce.group(3))))
    return out
def avg(x): return sum(x)/len(x)
o=[];P=o.append
P('# TEK HAMLE V30 — 4 F + kelime kimliği formülünün optimizasyonu\n')
P('Dal `claude/formul-avcisi` · kod `cpp/tek_hamle_v30.cpp` (V29 kopyası) · ham loglar/komutlar `v30_ham/`. H=512 (4×128), yalnız eğitim protokolü, V26 doğrusal okuma (α,ε yalnız doğrulama). Bütün seçimler **doğrulama PPL/CE ile**; test seçime girmedi. Formül V29\'dakiyle aynı (ortak ridge, `F=[F₁..F₄]`, `X*=[X,ID(w_{t−2}),ID(w_{t−1})]`, F₂..F₄ girdisinde X* ve önceki F\'ler); yalnız λ ve kimlik ölçek çarpanı `s_ID` değişti. `s_ID`: kimlik bloklarının hem F₁ (RMS_X/√nid × s_ID) hem F₂..F₄ (1/√nid × s_ID) girdi ölçeğini çarpar; ölçek temeli yalnız eğitimden.\n')
# lam sweep
A=asama(L+'v30_A_lamsweep_H512_s1.txt')+asama(L+'v30_A2_lamnarrow_sID1_H512_s1.txt')
A=sorted([a for a in A if a['tg']=='Y'],key=lambda a:a['lam'])
P('## 1. Deney A — λ taraması (tohum 1, γ₁=1, γ₂=0,5, s_ID=1; ortak ridge, Y hedefi)\n')
P('| λ | eğitim PPL | doğrulama PPL | (bilgi) test PPL | Top-1 / Top-5 | eğitim–doğr. CE farkı |'); P('|---|---|---|---|---|---|')
bl=min(A,key=lambda a:a['va'])
for a in A: P(f'| {a["lam"]:g}{" ← doğrulamada en iyi" if a is bl else ""} | {a["tr"]:.1f} | {a["va"]:.1f} | {a["te"][1]:.1f} | {100*a["te"][2]:.1f}% / {100*a["te"][3]:.1f}% | {a["gap"]:.3f} |')
P(f'\nSeçilen λ={bl["lam"]:g} (ızgara içinde, kenar değil); ızgara {{0,1; 1; 10; 30; 100; 300}} + daraltma {{3; 5; 15; 20}} (λ=10–20 aralığında doğrulama farkı ≤0,2 PPL: anlamlı kazanç yok).\n')
# sID
B=[]
for f in sorted(glob.glob(L+'v30_B_sID*_H512_s1.txt')): B+=asama(f)
B+=[a for a in A if a['sid']==1]
B=[b for b in B if b['tg']=='Y']
P('## 2. Deney B — kimlik ölçek çarpanı s_ID (tohum 1; her s_ID için komşu λ\'lar)\n')
P('| s_ID | λ | eğitim PPL | doğrulama PPL | (bilgi) test PPL | Top-1 / Top-5 | eğitim–doğr. CE farkı |'); P('|---|---|---|---|---|---|---|')
bs=min([b for b in B if b['lam'] in (5,10,20,40)],key=lambda b:b['va'])
for b in sorted(B,key=lambda b:(b['sid'],b['lam'])):
    if b['sid']==1 and b['lam'] not in (5,10,15,20): continue
    P(f'| {b["sid"]:g} | {b["lam"]:g} | {b["tr"]:.1f} | {b["va"]:.1f} | {b["te"][1]:.1f} | {100*b["te"][2]:.1f}% / {100*b["te"][3]:.1f}% | {b["gap"]:.3f} |')
P('\nGrid {0,5; 1; 2; 4} içinde doğrulama PPL monoton düştü ve s_ID=4 **ızgara kenarındaydı**; bu yüzden ızgara 8 ve 16 ile genişletildi (görev ızgarasının dışında, aynı kuralla). Doğrulama optimumu s_ID=8 (λ=10 ve 20 eşit: 160,6); s_ID=16 daha kötü (161,1) → optimum ızgara içinde. Eşitlikte daha çok düzenlileştirilmiş **λ=20** seçildi (eğitim–doğrulama farkı daha küçük).\n')
# final
P('## 3. Doğrulama: seçilen konfigürasyon (s_ID=8, λ=20), 3 tohum, H=512\n')
F={s:asama(L+f'v30_FINAL_sID8_lam20_H512_s{s}.txt') for s in (1,2,3) if os.path.exists(L+f'v30_FINAL_sID8_lam20_H512_s{s}.txt')}
fy=[f for s in F for f in F[s] if f['tg']=='Y']; fq=[f for s in F for f in F[s] if f['tg']=='Q']
P('| tohum | γ₁ | test PPL (Y) | test CE | Top-1 / Top-5 | eğitim PPL | doğr. PPL | eğitim–doğr. CE farkı | Q açıklanan varyans (test) | Q-hedefli test PPL |'); P('|---|---|---|---|---|---|---|---|---|---|')
for s in F:
    y=[f for f in F[s] if f['tg']=='Y'][0]; q=[f for f in F[s] if f['tg']=='Q'][0]
    P(f'| {s} | {[1,2,1][s-1]} | **{y["te"][1]:.1f}** | {y["te"][0]:.4f} | {100*y["te"][2]:.1f}% / {100*y["te"][3]:.1f}% | {y["tr"]:.0f} | {y["va"]:.0f} | {y["gap"]:.3f} | {q["qe"]:.3f} | {q["te"][1]:.1f} |')
P(f'| **ort.** | | **{avg([f["te"][1] for f in fy]):.1f}** | {avg([f["te"][0] for f in fy]):.4f} | {100*avg([f["te"][2] for f in fy]):.1f}% / {100*avg([f["te"][3] for f in fy]):.1f}% | {avg([f["tr"] for f in fy]):.0f} | {avg([f["va"] for f in fy]):.0f} | {avg([f["gap"] for f in fy]):.3f} | {avg([f["qe"] for f in fq]):.3f} | {avg([f["te"][1] for f in fq]):.1f} |')
P('')
FINAL=avg([f["te"][1] for f in fy])
open('/tmp/v30_final_val.txt','w').write(str(FINAL))
P('PLACEHOLDER')
open(base+'TEK_HAMLE_V30_RAPOR.md','w').write('\n'.join(o)+'\n')
