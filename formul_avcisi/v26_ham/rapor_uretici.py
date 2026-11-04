import re,glob,os
base='/home/user/name/formul_avcisi/'
def rd(p): return open(p).read()
def fl(x): return float(x)
D={}
for H in (1024,512):
    for s in (1,2,3):
        p=f'{base}v26_ham/v26log/v26_c2_H{H}_s{s}.txt'
        if not os.path.exists(p): continue
        t=rd(p); d={'m':{}, 'sure':None}
        for l in t.splitlines():
            if l.startswith('MODEL|'):
                f=l.split('|'); nm,eta,rdg=f[1],f[2].split('=')[1],f[3]
                par=f[4]+','+f[5]; tr=re.search(r'CE=([\d.]+) PPL=([\d.]+) top1=([\d.]+) top5=([\d.]+)',f[6]).groups(); va=re.search(r'CE=([\d.]+) PPL=([\d.]+) top1=([\d.]+) top5=([\d.]+)',f[7]).groups(); te=re.search(r'CE=([\d.]+) PPL=([\d.]+) top1=([\d.]+) top5=([\d.]+)',f[8]).groups()
                d['m'][(nm,rdg)]=dict(eta=eta,par=par,tr=[fl(x) for x in tr],va=[fl(x) for x in va],te=[fl(x) for x in te],gap=fl(f[9].rsplit('=',1)[1]),neg=fl(f[10].rsplit('=',1)[1]),yc=fl(f[11].rsplit('=',1)[1]),dev=f[12].rsplit('=',1)[1],rt=f[13].rsplit('=',1)[1])
            elif l.startswith('SURE agirlik'):
                d['sure']=[fl(x) for x in re.findall(r'=([\d.]+)s',l)]
            elif l.startswith('SECIM'): d['sec']=l
            elif l.startswith('TOPLAM'): d['tot']=re.search(r'sure=([\d.]+)s tepe_RAM=(\d+)MB',l).groups()
        D[(H,s)]=d
MN=['V17_Ridge(eta=0)','V20_CE(eta kaba-softmax secimi)','V20_CE(eta dogrusal secimi; FARKLI agirlik)']
RN=['SOFTMAX_KABA','SOFTMAX_INCE','DOGRUSAL']
def avg(x): return sum(x)/len(x)
def agg(H,mn,rn,key,idx=None):
    v=[]
    for s in (1,2,3):
        if (H,s) in D:
            e=D[(H,s)]['m'][(mn,rn)][key]; v.append(e if idx is None else e[idx])
    return v
o=[];P=o.append
P('# TEK HAMLE V26 — Ağırlık okuma hatasını düzelt ve gerçek öğrenmeyi ölç\n')
P('Dal `claude/formul-avcisi` · kodlar `cpp/tek_hamle_v26.cpp` (V17/V20 ağırlıkları, 3 okuma), `cpp/tek_hamle_v26c.cpp` (V25 doğrulaması) · ham loglar/komutlar `v26_ham/`.\n')
P('**Protokol.** Aynı veri/bölme/tohumlar (3 tohum, V17 (γ,λ) seçimleri: H=1024 (4,10),(4,100),(2,1); H=512 (1,1),(2,1),(1,1)). **Tüm ağırlıklar yalnız eğitimden** (V20 dâhil: `W=W0−ηD`, η∈{0,0.003,0.005,0.0075,0.01}), bu yüzden sayılar V17/V20 raporlarındaki eğitim+doğrulama modelli değerlerle birebir karşılaştırılamaz; eski sayılar **yeniden ölçüm değildir**, yalnız referanstır. Aynı ağırlık üç şekilde okunur, parametreler yalnız doğrulama CE ile seçilir, test hiçbir seçime girmez:\n\n- `SOFTMAX_KABA`: V17 protokolü `P=(1−α)π+α·softmax(z/T)`, 10 α × 11 T (V9–V24 kalibrasyonu).\n- `SOFTMAX_İNCE`: aynı formül, 20 α × 22 T (genişletilmiş bütçe; softmax\'ın yalnız kaba ızgara yüzünden kaybetmediğini kontrol etmek için).\n- `DOĞRUSAL`: `P=(1−α)π+α·max(z,ε)/Σ_j max(z_j,ε)`, 20 α × 8 ε (ε∈1e-7…1e-2). Bütçe softmax-kaba ile aynı mertebede (160 vs 110), softmax-ince\'den küçük (480).\n\nOkuma değişirken W değişmez (aynı model nesnesi). Her okumada olasılık toplamı sapması (|Σp−1|) ve z≤0 oranı ölçüldü.\n')
if (1024,1) not in D: raise SystemExit
# ana tablo: PPL
P('## 1. Test PPL — aynı ağırlıklar, farklı okuma (3 tohum ortalaması; yalnız-eğitim protokolü)\n')
P('| H | model (ağırlık) | eski rapor PPL (referans, yeniden ölçüm DEĞİL) | softmax-kaba | softmax-ince | **doğrusal** | doğrusal − softmax-ince |'); P('|---|---|---|---|---|---|---|')
old={(512,MN[0]):'212,3',(1024,MN[0]):'206,8',(512,MN[1]):'202,8',(1024,MN[1]):'198,9'}
for H in (512,1024):
    if not any((H,s) in D for s in (1,2,3)): continue
    n=len([s for s in (1,2,3) if (H,s) in D])
    for mn in MN[:2]:
        vals=[avg(agg(H,mn,rn,'te',1)) for rn in RN]
        P(f'| {H} | {mn} | {old[(H,mn)]} | {vals[0]:.1f} | {vals[1]:.1f} | **{vals[2]:.1f}** | {vals[2]-vals[1]:+.1f} |')
    mn=MN[2]; vals=[avg(agg(H,mn,rn,'te',1)) for rn in RN]
    P(f'| {H} | {mn} | – | {vals[0]:.1f} | {vals[1]:.1f} | {vals[2]:.1f} | {vals[2]-vals[1]:+.1f} |')
P('')
P('(tohum sayısı: '+', '.join(f'H={H}: {len([s for s in (1,2,3) if (H,s) in D])}' for H in (512,1024))+')\n')
P('## 2. Top-1 / Top-5 (test, 3 tohum ort.) — PPL düşerken doğruluk ne oluyor?\n')
P('| H | model | okuma | test PPL | Top-1 | Top-5 | eğitim–doğr. CE farkı | z≤0 oranı | y kırpılan oran | |Σp−1| maks |'); P('|---|---|---|---|---|---|---|---|---|---|')
for H in (512,1024):
    if not any((H,s) in D for s in (1,2,3)): continue
    for mn in MN[:2]:
        for rn in RN:
            P(f'| {H} | {mn} | {rn} | {avg(agg(H,mn,rn,"te",1)):.1f} | {100*avg(agg(H,mn,rn,"te",2)):.1f}% | {100*avg(agg(H,mn,rn,"te",3)):.1f}% | {avg(agg(H,mn,rn,"gap")):.3f} | {avg(agg(H,mn,rn,"neg")):.3f} | {avg(agg(H,mn,rn,"yc")):.3f} | {max(fl(D[(H,s)]["m"][(mn,rn)]["dev"]) for s in (1,2,3) if (H,s) in D):.1e} |')
P('')
P('## 3. Tohum başına test PPL (softmax-kaba / softmax-ince / doğrusal) ve seçilen parametreler\n')
P('| H | tohum | model | η | softmax-kaba | softmax-ince | doğrusal | seçilen (α,T) kaba | (α,T) ince | (α,ε) doğrusal |'); P('|---|---|---|---|---|---|---|---|---|---|')
for (H,s),d in sorted(D.items(), reverse=True):
    for mn in MN[:2]:
        e=[d['m'][(mn,rn)] for rn in RN]
        P(f'| {H} | {s} | {mn} | {e[0]["eta"]} | {e[0]["te"][1]:.2f} | {e[1]["te"][1]:.2f} | {e[2]["te"][1]:.2f} | {e[0]["par"]} | {e[1]["par"]} | {e[2]["par"]} |')
P('')
P('PLACEHOLDER')
open(base+'TEK_HAMLE_V26_RAPOR.md','w').write('\n'.join(o)+'\n')
print(len(D),'runs')
