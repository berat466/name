import re,glob,os
base='/home/user/name/formul_avcisi/'
def rd(p): return open(p).read()
R={}
for p in sorted(glob.glob(base+'v29_ham/v29log/v29_c2_H512_s*.txt')):
    s=int(re.search(r'_s(\d)\.txt',p).group(1)); t=rd(p)
    if 'TOPLAM' not in t: continue
    d={'st':[],'kim':[],'time':{},'tot':{}}
    for l in t.splitlines():
        if l.startswith('ASAMA|'):
            f=l.split('|'); name=f[1]; tg=f[2].split('=')[1]; lab=f[3]
            q=re.search(r'dogr/test=(-?[\d.]+)/(-?[\d.]+) \(onceki asamaya gore ek: ([+-][\d.]+)/([+-][\d.]+)\)',l).groups()
            trp=float(re.search(r'\|egitim PPL=([\d.]+)',l).group(1)); vap=float(re.search(r'\|dogr PPL=([\d.]+)',l).group(1)); te=re.search(r'test CE=([\d.]+) PPL=([\d.]+) top1=([\d.]+) top5=([\d.]+)',l).groups()
            gap=float(re.search(r'egitim-dogr CE=(-?[\d.]+)',l).group(1)); st=float(re.search(r'asama sure=([\d.]+)s',l).group(1)); al=re.search(r'alpha=(\S+) eps=(\S+)',l).groups()
            d['st'].append(dict(name=name,tg=tg,lab=lab,q=[float(x) for x in q],trp=trp,vap=vap,te=[float(x) for x in te],gap=gap,st=st,al=al))
        elif l.startswith('KIMLIK|'): d['kim'].append(l)
        elif l.startswith('ZINCIR|'):
            f=l.split('|'); d['time'][f[1]]=float(re.search(r'=([\d.]+)s',f[3]).group(1))
        elif l.startswith('ZINCIR_BITTI'):
            f=l.split('|'); d['tot'][f[1]]=float(re.search(r'toplam=([\d.]+)s',f[2]).group(1))
    R[s]=d
def avg(x): return sum(x)/len(x)
def pick(prefix,tg,lab,exact=False):
    es=[]
    for s,d in R.items():
        for x in d['st']:
            ok=(x['name']==prefix) if exact else x['name'].startswith(prefix)
            if ok and x['tg']==tg and x['lab'].startswith(lab): es.append(x)
    return es
def m(es,f): return avg([f(e) for e in es]) if es else float('nan')
N=len(R)
man=[l.split('\t') for l in rd(base+'v29_ham/v29log/manifest.tsv').splitlines()]
SYS=[('A · V28: kimlik YOK, bağlı 4F (ardışık)','V28_KIMLIK_YOK(4x128)','F1..F4'),('B · V29: sıralı kimlik, bağlı 4F (ardışık)','V29_KIMLIK_SIRALI(4x128)','F1..F4'),('C · V29: sıralı kimlik + çift (hash), bağlı 4F (ardışık)','V29_KIMLIK_SIRALI+CIFT(4x128)','F1..F4'),
('A · V28 ortak ridge','ORTAK_RIDGE_V28_KIMLIK_YOK(4x128)','F1..F4'),('B · V29 ortak ridge','ORTAK_RIDGE_V29_KIMLIK_SIRALI(4x128)','F1..F4'),('C · V29 ortak ridge (+çift)','ORTAK_RIDGE_V29_KIMLIK_SIRALI+CIFT(4x128)','F1..F4')]
o=[];P=o.append
P('# TEK HAMLE V29 — Kelime kimliği + matematiksel değerler birlikte, 4 bağlantılı F\n')
P(f'Dal `claude/formul-avcisi` · kod `cpp/tek_hamle_v29.cpp` (V28 kopyası) · ham loglar/komutlar `v29_ham/`. H=512 (4×128), tohumlar: {sorted(R)} (n={N}), (γ₁,λ): s1 (1,1), s2 (2,1), s3 (1,1); γ₂=0,5 (V28\'de doğrulamayla seçilen). Yalnız eğitim protokolü; V26 doğrusal okuma, α ve ε yalnız doğrulama CE ile; softmax yok. Test hiçbir seçime girmedi.\n')
P('''## 1. Yapı

- `X* = [X, ID(w_{t−2}), ID(w_{t−1})]`: kimlikler **sıra duyarlı** (t−1 ve t−2 için ayrı seyrek one-hot blokları, 2001+2001 boyut), yalnız geçmiş bağlam kelimeleri; hedef kelime girdide yok. Seyrek blok hiçbir yerde yoğun matrise açılmadı (satır başına ≤3 aktif indeks; seyrek ağırlık matrisi (8098×128)). C varyantı: ayrıca (w_{t−2},w_{t−1}) çiftinin 4096 kovalı hash kimliği.
- `F₁=tanh(γ₁X*A₁+b₁)`: V17 yön kuralı, kimlik farkları yönün parçası (birim norm), bias V17'deki gibi. `F_i=tanh(γ₂[X*,F₁..F_{i−1}]A_i+b_i)`: kimlik ve X her aşamaya erişir. Yönler yalnız eğitim satır çiftlerinden.
- **Ölçek (yalnız eğitimden):** F₁ girdisinde kimlik bloğu enerjisi X ile eşitlenir (ölçek = RMS_X/√nid: B 1,3176, C 1,0758; RMS_X=1,8634). F₂..F₄ girdisinde her blok (X, F_j, kimlik) satır-norm RMS=1'e ölçeklenir (kimlik 1/√nid = 0,7071 / 0,5774).
- Ağırlıklar: V28'deki ardışık artık çözümü ve ayrıca **tek ortak ridge** `W=(FᵀF+λI)⁻¹FᵀY`, `F=[F₁..F₄]`. T = Y (gerçek etiket) veya Q (V25/V27 trigram öğretici, LOO).
''')
P('## 2. Ana sonuç (test; doğrusal okuma; tohum ortalaması)\n')
P('| sistem | hedef | test PPL | test CE | Top-1 | Top-5 | eğitim PPL | doğr. PPL | eğitim–doğr. CE farkı | Q açıklanan varyans (test) |'); P('|---|---|---|---|---|---|---|---|---|---|')
for nm,pre,lab in SYS:
    for tg in ('Y','Q'):
        es=pick(pre,tg,lab,True)
        if not es: continue
        P(f'| {nm} | {tg} | **{m(es,lambda e:e["te"][1]):.1f}** | {m(es,lambda e:e["te"][0]):.4f} | {100*m(es,lambda e:e["te"][2]):.1f}% | {100*m(es,lambda e:e["te"][3]):.1f}% | {m(es,lambda e:e["trp"]):.0f} | {m(es,lambda e:e["vap"]):.0f} | {m(es,lambda e:e["gap"]):.3f} | {(("%.3f"%m(es,lambda e:e["q"][1])) if tg=="Q" else "–")} |')
P('')
P('Referanslar (V28 raporu, γ₂=0,5): ortak ridge 176,5; ardışık 180,7; V26 tek F 182,5; trigram 138,9. Bu raporda A aynı programda yeniden ölçüldü ve V28 sonuçlarını **birebir yeniden üretti** (176,5 / 180,7).\n')
P('## 3. Kimliğin gerçek katkısı (aynı program, aynı tohum, aynı toplam F boyutu)\n')
P('| karşılaştırma | Y test PPL farkı | Q açıklanan varyans farkı (test) | Top-5 farkı (Y) |'); P('|---|---|---|---|')
def d(a,b,tg,f): 
    ea,eb=pick(a,tg,'F1..F4',True),pick(b,tg,'F1..F4',True); return m(eb,f)-m(ea,f)
for (lab,a,b) in (('ardışık: B − A','V28_KIMLIK_YOK(4x128)','V29_KIMLIK_SIRALI(4x128)'),('ardışık: C − A','V28_KIMLIK_YOK(4x128)','V29_KIMLIK_SIRALI+CIFT(4x128)'),('ortak ridge: B − A','ORTAK_RIDGE_V28_KIMLIK_YOK(4x128)','ORTAK_RIDGE_V29_KIMLIK_SIRALI(4x128)'),('ortak ridge: C − A','ORTAK_RIDGE_V28_KIMLIK_YOK(4x128)','ORTAK_RIDGE_V29_KIMLIK_SIRALI+CIFT(4x128)'),('ortak ridge: C − B','ORTAK_RIDGE_V29_KIMLIK_SIRALI(4x128)','ORTAK_RIDGE_V29_KIMLIK_SIRALI+CIFT(4x128)')):
    P(f'| {lab} | {d(a,b,"Y",lambda e:e["te"][1]):+.1f} | {d(a,b,"Q",lambda e:e["q"][1]):+.3f} | {100*d(a,b,"Y",lambda e:e["te"][3]):+.1f} pp |')
P('')
P('## 4. Her F\'nin katkısı (ardışık çözüm; tohum ort.)\n')
P('| zincir | aşama | Y: test PPL | Y: eğitim / doğr. PPL | Q: açıklanan varyans (test) | Q: ek | Q: test PPL |'); P('|---|---|---|---|---|---|---|')
for nm,pre in (('A · kimlik yok','V28_KIMLIK_YOK(4x128)'),('B · sıralı kimlik','V29_KIMLIK_SIRALI(4x128)'),('C · kimlik+çift','V29_KIMLIK_SIRALI+CIFT(4x128)')):
    for k in range(1,5):
        lab=f'F1..F{k}'; y=pick(pre,'Y',lab,True); q=pick(pre,'Q',lab,True)
        P(f'| {nm} | F{k} | {m(y,lambda e:e["te"][1]):.1f} | {m(y,lambda e:e["trp"]):.0f} / {m(y,lambda e:e["vap"]):.0f} | {m(q,lambda e:e["q"][1]):.3f} | {m(q,lambda e:e["q"][3]):+.3f} | {m(q,lambda e:e["te"][1]):.1f} |')
P('')
P('PLACEHOLDER')
open(base+'TEK_HAMLE_V29_RAPOR.md','w').write('\n'.join(o)+'\n')
