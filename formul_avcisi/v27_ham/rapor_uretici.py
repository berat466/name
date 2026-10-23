import re,glob,os
base='/home/user/name/formul_avcisi/'
def rd(p): return open(p).read()
R={}
for p in sorted(glob.glob(base+'v27_ham/v27log/v27_c2_H*_s*.txt')):
    H=int(re.search(r'_H(\d+)_',p).group(1)); s=int(re.search(r'_s(\d)\.',p).group(1)); t=rd(p)
    if 'TOPLAM' not in t: continue
    d={'st':[], 'time':{}}
    for l in t.splitlines():
        if l.startswith('ASAMA|'):
            f=l.split('|'); name=f[1]; tg=f[2].split('=')[1]; lab=f[3]; w=int(re.search(r'eklenen=(\d+)',l).group(1))
            res=[float(x) for x in re.search(r'test\)=([\d.]+)/([\d.]+)/([\d.]+)\|',l).groups()]
            q=re.search(r'dogr/test=(-?[\d.]+)/(-?[\d.]+) \(onceki asamaya gore ek: ([+-][\d.]+)/([+-][\d.]+)\)',l).groups()
            rp=re.search(r'alpha=(\S+) eps=(\S+)',l).groups()
            trp=float(re.search(r'\|egitim PPL=([\d.]+)',l).group(1)); vap=float(re.search(r'\|dogr PPL=([\d.]+)',l).group(1)); te=re.search(r'test CE=([\d.]+) PPL=([\d.]+) top1=([\d.]+) top5=([\d.]+)',l).groups()
            gap=float(re.search(r'egitim-dogr CE=(-?[\d.]+)',l).group(1)); st=float(re.search(r'asama sure=([\d.]+)s',l).group(1))
            d['st'].append(dict(name=name,tg=tg,lab=lab,w=w,res=res,q=[float(x) for x in q],rp=rp,trp=trp,vap=vap,te=[float(x) for x in te],gap=gap,st=st))
        elif l.startswith('ZINCIR|'):
            f=l.split('|'); d['time'][f[1]]=float(re.search(r'=([\d.]+)s',f[3]).group(1))
        elif l.startswith('ZINCIR_BITTI'):
            f=l.split('|'); d['time'][f[1]+'_toplam']=f[2]
    R[(H,s)]=d
man=[l.split('\t') for l in rd(base+'v27_ham/v27log/manifest.tsv').splitlines()]
def avg(x): return sum(x)/len(x)
o=[];P=o.append
P('# TEK HAMLE V27 — Birbirine bağlı 4 F ile ağırlık üretimi\n')
P('Dal `claude/formul-avcisi` · kod `cpp/tek_hamle_v27.cpp` (V26 kopyası; V26 okuma katmanı) · ham loglar/komutlar `v27_ham/`. Aynı veri/bölme, **yalnız eğitim** protokolü; doğrusal okuma `P=(1−α)π+α·max(z,ε)/Σmax(z,ε)` (α∈20, ε∈8 değer, **yalnız doğrulama CE ile**); softmax kullanılmadı. Tohumlar ve (γ₁,λ): H=512: s1 (1,1), s2 (2,1), s3 (1,1); H=1024: s1 (4,10), s2 (4,100), s3 (2,1) (V17 seçimleri). F₂..F₄ için γ₂=1 sabit, ayrıca ayarlanmadı.\n')
P('''## 1. Yöntem

- `F₁=tanh(γ₁·X·A₁+b₁)` (V17 yönteminin aynısı: eğitim satır çiftlerinden birim yön, `h/4` sütun + sabit sütun); `F_i=tanh(γ₂·F_{i−1}·A_i+b_i)`, `i=2,3,4`. **Her aşamada aynı yön üretme kuralı**: hedefleri farklı iki eğitim satırının önceki-aşama özelliklerinin farkı birim vektöre normalize edilir (`A_i` sütunu), sapma `b_i` = V17'deki gibi çiftin orta noktasından hesaplanan değerlerin standart sapmasıyla N(0,σ²) (önceki aşama sütun sayısı × h/4). Yalnız eğitim satırları kullanılır; val/test yalnız dönüştürülür.
- Zincir: `R₀=T` (T = Y gerçek etiket **veya** Q = trigram öğretici dağılımı, eğitim satırlarında leave-one-out), `W_i=(F_iᵀF_i+λI)⁻¹F_iᵀR_{i−1}`, `R_i=R_{i−1}−F_iW_i`, `Ŷ=Σ F_iW_i`. `F_iᵀR_{i−1}` yoğun artık matrisi kurmadan, `F_iᵀT − Σ_{j<i}(F_iᵀF_j)W_j` ile hesaplanır (tam eşdeğer). Dört çıktı yalnız toplanır; tek algoritma, ardışık dört temsil aşaması.
- C deneyi: aynı dört F tek birleşik matriste, **tek ortak ridge** (`Z=[F₁..F₄]`).
- Kalan hata: Y için `‖Y−Ŷ‖²/n`; Q için trigram dağılımının (π'den sapmasına göre) açıklanan varyansı, `1−‖Ŷ−Q‖²/‖Q−π‖²` (doğrulama/test, Q val/test için eğitim sayımlarından). Her aşamanın "ek" katkısı bir önceki aşamaya göre fark.
''')
P('## 2. Ana sonuç (test PPL, doğrusal okuma; 3 tohum ortalaması)\n')
P('| H | yapı | hedef | test PPL | Top-1 | Top-5 | eğitim PPL | doğrulama PPL | eğitim–doğr. CE farkı | Q açıklanan varyans (test) |'); P('|---|---|---|---|---|---|---|---|---|---|')
for H in (512,1024):
    ss=[s for s in (1,2,3) if (H,s) in R]
    if not ss: continue
    names=[]
    for e in R[(H,ss[0])]['st']:
        k=(e['name'],e['tg'],e['lab'])
        if k not in names: names.append(k)
    for (nm,tg,lab) in names:
        es=[x for s in ss for x in R[(H,s)]['st'] if (x['name'],x['tg'],x['lab'])==(nm,tg,lab)]
        P(f'| {H} (n={len(ss)}) | {nm} · {lab} | {tg} | {avg([e["te"][1] for e in es]):.1f} | {100*avg([e["te"][2] for e in es]):.1f}% | {100*avg([e["te"][3] for e in es]):.1f}% | {avg([e["trp"] for e in es]):.1f} | {avg([e["vap"] for e in es]):.1f} | {avg([e["gap"] for e in es]):.3f} | {(("%.3f"%avg([e["q"][1] for e in es])) if tg=="Q" else "–")} |')
P('')
P('PLACEHOLDER')
open(base+'TEK_HAMLE_V27_RAPOR.md','w').write('\n'.join(o)+'\n')
