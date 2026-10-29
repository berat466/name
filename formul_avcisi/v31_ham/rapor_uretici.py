import re,glob,os
base='/home/user/name/formul_avcisi/'; L=base+'v31_ham/v31log/'
def rd(p): return open(p).read()
def avg(x): return sum(x)/len(x)
def sonuc(path):
    d={}
    for l in rd(path).splitlines():
        if l.startswith('SONUC|'):
            f=l.split('|'); tag=f[1].strip(); lc=re.search(r'lamC=(\S+) lamF=(\S+)',f[2]).groups()
            te=re.search(r'test CE=([\d.]+) PPL=([\d.]+) top1=([\d.]+) top5=([\d.]+)',l).groups()
            d[tag]=dict(lc=lc,tr=float(re.search(r'egitim PPL=([\d.]+)',l).group(1)),va=float(re.search(r'dogr CE=([\d.]+) PPL=([\d.]+)',l).group(2)),vce=float(re.search(r'dogr CE=([\d.]+)',l).group(1)),te=[float(x) for x in te],gap=float(re.search(r'farki=(-?[\d.]+)',l).group(1)),cs=re.search(r'GORULDU: PPL=([\d.]+) top1=([\d.]+) \(n=(\d+)\)',l).groups(),cb=re.search(r'yalniz bigram: PPL=([\d.]+) top1=([\d.]+) \(n=(\d+)\)',l).groups(),dev=re.search(r'sapma=(\S+)',l).group(1),al=re.search(r'alpha=(\S+) eps=(\S+)',f[3]).groups() if False else None)
    return d
F={1:sonuc(L+'v31_sweep2_H512_s1.txt')}
for s in (2,3):
    p=L+f'v31_final_H512_s{s}.txt'
    if os.path.exists(p) and 'TOPLAM' in rd(p): F[s]=sonuc(p)
ID='YALNIZ_KIMLIK(secilen)'; CB='ORTAK_KIMLIK+4F(secilen)'; PC='ayni W: yalniz C kismi (logit)'; PF='ayni W: yalniz F kismi (logit)'
o=[];P=o.append
ns=len(F)
P('# TEK HAMLE V31 — Kelime kimliğini koru, 4 F ile eksik bilgiyi tamamla\n')
P(f'Dal `claude/formul-avcisi` · kod `cpp/tek_hamle_v31.cpp` (V30 kopyası) · ham loglar/komutlar `v31_ham/`. H=512 (4×128), yalnız eğitim protokolü, V26 doğrusal okuma (α,ε yalnız doğrulama). Seçimler doğrulama CE ile; test seçime girmedi. Tamamlanan tohumlar: {sorted(F)}.\n')
P('''## 1. Formül ve hesap

`Z=[C,F₁,F₂,F₃,F₄]`, `C=[ID(w_{t−2}),ID(w_{t−1})]` (4002 seyrek sütun; yoğun one-hot **veri** matrisi yok), F zinciri V28 gibi: `F₁=tanh(γ₁X A₁+b₁)`, `F_i=tanh(γ₂[X,F₁..F_{i−1}]A_i+b_i)` (γ₂=0,5); **F girdisine kimlik girmez** (kimlik yalnız ortak çözümde, doğrudan sütun olarak).

Tek ortak normal denklem: `(ZᵀZ+Λ)W=ZᵀY`, `Λ=diag(λ_C I_C, λ_F I_F)`:

```
[ A_CC+λ_C I   A_CF        ] [W_C]   [R_C]
[ A_FC         A_FF+λ_F I  ] [W_F] = [R_F]
```
Blok eliminasyon (Schur): `W_F=(A_FF+λ_F I−A_FC(A_CC+λ_C I)⁻¹A_CF)⁻¹(R_F−A_FC(A_CC+λ_C I)⁻¹R_C)`, `W_C=(A_CC+λ_C I)⁻¹(R_C−A_CF W_F)`. `A_CC` (4002²) sayım matrisi (diyagonal = kelime sayıları, çapraz = (t−2,t−1) çift sayıları) çift duyarlıkla Cholesky; F bloğu 513×513. Bu, iki bağımsız ridge\'in toplamı **değildir**: çapraz terimler `A_CF` Schur tamamlayıcıda ve `W_C`\'nin F\'ye bağlı düzeltmesinde yer alır. Çözücü doğrudandır (yaklaşık yöntem yok); rezidü bölüm 6\'da.
''')
P('## 2. Deney A/B — taramalar (tohum 1; doğrulama CE ile)\n')
P('Yalnız kimlik `W_C=(CᵀC+λ_C I)⁻¹CᵀY`:\n')
P('| λ_C | doğrulama CE (PPL) |\n|---|---|')
ids={}
for p in glob.glob(L+'v31_sweep*_H512_s1.txt'):
    for l in rd(p).splitlines():
        m=re.match(r'YALNIZ_KIMLIK_TARAMA\|lamC=(\S+)\|dogr CE=([\d.]+) PPL=([\d.]+)',l)
        if m: ids[float(m.group(1))]=(float(m.group(2)),float(m.group(3)))
for k in sorted(ids): P(f'| {k:g} | {ids[k][0]:.4f} ({ids[k][1]:.1f}) |')
P('\nOrtak çözüm (λ_C × λ_F), doğrulama PPL (`1e9` = F katkısı kapalı kontrolü):\n')
P('| λ_C \\ λ_F | '+' | '.join(['1','20','100','300','1000','3000','10000','1e9'])+' |'); P('|---|'+'---|'*8)
grid={}
for p in glob.glob(L+'v31_sweep*_H512_s1.txt'):
    for l in rd(p).splitlines():
        m=re.match(r'ORTAK_TARAMA\|lamC=(\S+) lamF=(\S+)\|dogr CE=([\d.]+) PPL=([\d.]+)',l)
        if m: grid[(float(m.group(1)),float(m.group(2)))]=(float(m.group(3)),float(m.group(4)))
lcs=sorted({k[0] for k in grid}); lfs=[1,20,100,300,1000,3000,10000,1e9]
bestk=min([k for k in grid if k[1]<1e8],key=lambda k:grid[k][0])
for lc in lcs:
    P(f'| {lc:g} | '+' | '.join((('**%.1f**'%grid[(lc,lf)][1]) if (lc,lf)==bestk else ('%.1f'%grid[(lc,lf)][1])) if (lc,lf) in grid else '–' for lf in lfs)+' |')
P(f'\nSeçilen: **λ_C={bestk[0]:g}, λ_F={bestk[1]:g}** (iki eksende de ızgara içinde: ilk taramada λ_C=100 ve λ_F=1000 kenardaydı, bu yüzden λ_C∈{{300,1000}} ve λ_F∈{{3000,10000}} eklendi). Yalnız kimlik için λ_C=20 (komşular 10 ve 100 daha kötü). F kapalı kontrol (`λ_F=1e9`) her λ_C için yalnız-kimlik doğrulama CE\'sini **aynen** veriyor (ör. λ_C=100: 5,0317 = 5,0317): ortak sistem F katkısı kapatılınca yalnız-kimliğe dönüyor.\n')
P('## 3. Sonuçlar (test; doğrusal okuma)\n')
P('| tohum | sistem | λ_C / λ_F | test CE | test PPL | Top-1 | Top-5 | eğitim PPL | doğr. PPL | eğitim–doğr. CE farkı |\n|---|---|---|---|---|---|---|---|---|---|')
for s in sorted(F):
    for tag,nm in ((ID,'yalnız kimlik C'),(CB,'**V31 kimlik + 4F (ortak)**')):
        r=F[s][tag]; P(f'| {s} | {nm} | {r["lc"][0]} / {r["lc"][1] if tag==CB else "–"} | {r["te"][0]:.4f} | **{r["te"][1]:.1f}** | {100*r["te"][2]:.1f}% | {100*r["te"][3]:.1f}% | {r["tr"]:.0f} | {r["va"]:.0f} | {r["gap"]:.3f} |')
P(f'| **ort. (n={ns})** | yalnız kimlik C | | | **{avg([F[s][ID]["te"][1] for s in F]):.1f}** | {100*avg([F[s][ID]["te"][2] for s in F]):.1f}% | {100*avg([F[s][ID]["te"][3] for s in F]):.1f}% | {avg([F[s][ID]["tr"] for s in F]):.0f} | {avg([F[s][ID]["va"] for s in F]):.0f} | {avg([F[s][ID]["gap"] for s in F]):.3f} |')
P(f'| **ort. (n={ns})** | **V31 kimlik + 4F** | | | **{avg([F[s][CB]["te"][1] for s in F]):.1f}** | {100*avg([F[s][CB]["te"][2] for s in F]):.1f}% | {100*avg([F[s][CB]["te"][3] for s in F]):.1f}% | {avg([F[s][CB]["tr"] for s in F]):.0f} | {avg([F[s][CB]["va"] for s in F]):.0f} | {avg([F[s][CB]["gap"] for s in F]):.3f} |')
P('')
P('PLACEHOLDER')
open(base+'TEK_HAMLE_V31_RAPOR.md','w').write('\n'.join(o)+'\n')
