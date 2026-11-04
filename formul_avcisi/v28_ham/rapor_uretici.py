import re,glob,os
base='/home/user/name/formul_avcisi/'
def rd(p): return open(p).read()
G=rd(base+'v28_ham/v28log/G2_SECILEN').strip()
def parse(p):
    t=rd(p); d={'st':[],'sc':[],'time':{},'tot':{}}
    for l in t.splitlines():
        if l.startswith('ASAMA|'):
            f=l.split('|'); name=f[1]; tg=f[2].split('=')[1]; lab=f[3]; w=int(re.search(r'eklenen=(\d+)',l).group(1))
            res=[float(x) for x in re.search(r'test\)=([\d.]+)/([\d.]+)/([\d.]+)\|',l).groups()]
            q=re.search(r'dogr/test=(-?[\d.]+)/(-?[\d.]+) \(onceki asamaya gore ek: ([+-][\d.]+)/([+-][\d.]+)\)',l).groups()
            trp=float(re.search(r'\|egitim PPL=([\d.]+)',l).group(1)); vap=float(re.search(r'\|dogr PPL=([\d.]+)',l).group(1)); te=re.search(r'test CE=([\d.]+) PPL=([\d.]+) top1=([\d.]+) top5=([\d.]+)',l).groups()
            gap=float(re.search(r'egitim-dogr CE=(-?[\d.]+)',l).group(1)); st=float(re.search(r'asama sure=([\d.]+)s',l).group(1))
            d['st'].append(dict(name=name,tg=tg,lab=lab,w=w,res=res,q=[float(x) for x in q],trp=trp,vap=vap,te=[float(x) for x in te],gap=gap,st=st))
        elif l.startswith('OLCEK|'): d['sc'].append(l)
        elif l.startswith('ZINCIR|'):
            f=l.split('|'); d['time'][f[1]]=float(re.search(r'=([\d.]+)s',f[3]).group(1))
        elif l.startswith('ZINCIR_BITTI'):
            f=l.split('|'); d['tot'][f[1]]=float(re.search(r'toplam=([\d.]+)s',f[2]).group(1))
        elif l.startswith('TOPLAM'): d['all']=l
    return d if 'all' in d else None
R={}
for p in sorted(glob.glob(base+'v28_ham/v28log/v28_c2_H*_s*_g2_*.txt')):
    m=re.search(r'_H(\d+)_s(\d)(?:_g2)?_g2_([\d.]+)\.txt',p) or re.search(r'_H(\d+)_s(\d)_g2_([\d.]+)\.txt',p)
    H,s,g=int(m.group(1)),int(m.group(2)),m.group(3)
    d=parse(p)
    if d: R[(H,s,g)]=d
sel={(H,s):R[(H,s,G)] for (H,s,g) in R if g==G}
def avg(x): return sum(x)/len(x)
def pick(H,prefix,tg,lab=None):
    es=[]
    for (h,s),d in sel.items():
        if h!=H: continue
        for x in d['st']:
            if x['name'].startswith(prefix) and x['tg']==tg and (lab is None or x['lab'].startswith(lab)): es.append(x)
    return es
def m(es,f): return avg([f(e) for e in es]) if es else float('nan')
man=[l.split('\t') for l in rd(base+'v28_ham/v28log/manifest.tsv').splitlines()]
o=[];P=o.append
P('# TEK HAMLE V28 — 4 bağlantılı F ile (X erişimli) bilgi kaybını giderme\n')
P(f'Dal `claude/formul-avcisi` · kod `cpp/tek_hamle_v28.cpp` (V27 kopyası) · ham loglar/komutlar `v28_ham/`. Yalnız eğitim protokolü; V26 doğrusal okuma (`P=(1−α)π+α·max(z,ε)/Σmax(z,ε)`, α∈20, ε∈8 değer yalnız **doğrulama CE ile**); softmax yok. (γ₁,λ) V17 seçimleri; **γ₂** (F₂..F₄ tanh ölçeği) doğrulamayla seçildi: **γ₂={G}** (bölüm 4). Tohum 1/2/3 için H=512: (1,1),(2,1),(1,1); H=1024: (4,10),(4,100),(2,1).\n')
P('''## 1. Yapı

- `F₁=tanh(γ₁XA₁+b₁)` (V17 yöntemi). `F_i=tanh(γ₂·[X̃,F̃₁..F̃_{i−1}]·A_i+b_i)`, `i=2,3,4`; `~` = blok ölçeklendirme. Yön üretimi (her aşamada aynı kural): hedefleri farklı iki eğitim satırının birleşik girişleri farkı birim vektöre normalize edilir, bias V17'deki gibi çift orta noktasından (std ile N(0,σ²)). Yalnız eğitim satırları kullanılır; val/test yalnız dönüştürülür; etiketler özelliğe girmez (etiket yalnız "farklı hedef" çift seçiminde, V17 ile aynı).
- **Ölçek:** ham X ile F blokları farklı ölçekte; her blok, eğitim satır-normu RMS'i 1 olacak şekilde ölçeklendi (ölçekler bölüm 5'te). V27 (X yok) ölçeksiz, birebir yeniden üretildi (`V27_X_YOK`).
- Ağırlıklar: `R₀=T`, `W_i=(F_iᵀF_i+λI)⁻¹F_iᵀR_{i−1}` (yoğun artık yerine `F_iᵀT−Σ_{j<i}F_iᵀF_jW_j`), `Ŷ=ΣF_iW_i`. E: dört F tek ortak ridge. T = Y (gerçek etiket) veya Q (trigram öğretici, eğitim satırlarında LOO). Q'dan gelen sonuçlar bağımsız öğrenme olarak sunulmaz.
''')
P('## 2. Ana sonuç (doğrusal okuma; test PPL; tohum ort.)\n')
P('| H | sistem | hedef | test PPL | Top-1 | Top-5 | eğitim PPL | doğr. PPL | eğitim–doğr. CE farkı | Q açıklanan varyans (test) |'); P('|---|---|---|---|---|---|---|---|---|---|')
for H in (512,1024):
    ns=len([1 for (h,s) in sel if h==H])
    if not ns: continue
    rows=[('A · tek F','TEK_F','F1..F1'),('B · V27 bağlı 4F (X yok), F₁..F₄','V27_X_YOK','F1..F4'),('C/D · V28 X-bağlantılı 4F, F₁..F₄','V28_X_BAG','F1..F4'),('V28 X-bağlantılı, yalnız F₁','V28_X_BAG','F1..F1'),('E · V28 ortak ridge (4F tek çözüm)','ORTAK','F1..F4')]
    for nm,pre,lab in rows:
        for tg in ('Y','Q'):
            es=pick(H,pre,tg,lab)
            if not es: continue
            P(f'| {H} (n={ns}) | {nm} | {tg} | **{m(es,lambda e:e["te"][1]):.1f}** | {100*m(es,lambda e:e["te"][2]):.1f}% | {100*m(es,lambda e:e["te"][3]):.1f}% | {m(es,lambda e:e["trp"]):.0f} | {m(es,lambda e:e["vap"]):.0f} | {m(es,lambda e:e["gap"]):.3f} | {(("%.3f"%m(es,lambda e:e["q"][1])) if tg=="Q" else "–")} |')
P('')
P('## 3. Aşama aşama katkı (3 tohum ort.; V28 X-bağlantılı vs V27)\n')
P('| H | zincir | aşama | eklenen sütun | Y: test PPL | Y: eğitim / doğr. PPL | Y: kalan hata (test) | Q: açıklanan varyans (test) | Q: ek | Q: test PPL |'); P('|---|---|---|---|---|---|---|---|---|---|')
for H in (512,1024):
    if not any(h==H for (h,s) in sel): continue
    for pre,nm in (('V28_X_BAG','V28 (X erişimli)'),('V27_X_YOK','V27 (X yok)')):
        for k in range(1,5):
            lab=f'F1..F{k}'; y=pick(H,pre,'Y',lab); q=pick(H,pre,'Q',lab)
            P(f'| {H} | {nm} | F{k} | {q[0]["w"]} | {m(y,lambda e:e["te"][1]):.1f} | {m(y,lambda e:e["trp"]):.0f} / {m(y,lambda e:e["vap"]):.0f} | {m(y,lambda e:e["res"][2]):.4f} | {m(q,lambda e:e["q"][1]):.3f} | {m(q,lambda e:e["q"][3]):+.3f} | {m(q,lambda e:e["te"][1]):.1f} |')
P('')
P('## 4. γ₂ seçimi (H=512, tohum 1; seçim doğrulama PPL ile, test yalnız bilgi)\n')
P('| γ₂ | V28 Y F₁..F₄ doğr. PPL | (bilgi) test PPL | V28 Q F₁..F₄ doğr. PPL |'); P('|---|---|---|---|')
for g in sorted({g for (h,s,g) in R if h==512 and s==1},key=float):
    d=R[(512,1,g)]; y=[x for x in d['st'] if x['name'].startswith('V28_X') and x['tg']=='Y' and x['lab'].startswith('F1..F4')][0]; q=[x for x in d['st'] if x['name'].startswith('V28_X') and x['tg']=='Q' and x['lab'].startswith('F1..F4')][0]
    P(f'| {g}{" ← seçilen" if g==G else ""} | {y["vap"]:.1f} | {y["te"][1]:.1f} | {q["vap"]:.1f} |')
P('')
P('## 5. Blok ölçekleri (tohum 1, H=512; ham satır-normu RMS ve uygulanan ölçek)\n')
d=sel.get((512,1))
for l in d['sc']:
    if l.startswith('OLCEK|V28'): P('- `'+l.replace('OLCEK|','')+'`')
P('')
P('PLACEHOLDER')
open(base+'TEK_HAMLE_V28_RAPOR.md','w').write('\n'.join(o)+'\n')
