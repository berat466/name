import re,os,glob
base='/home/user/name/formul_avcisi/'
def fl(x): return float(x)
S={}
for p in sorted(glob.glob(base+'v23_ham/v23log/v23_c2_H1024_s*.txt')):
    s=int(re.search(r'_s(\d)\.txt',p).group(1)); d={'cfg':{}, 'wd':{}, 'kap':[], 'ogr':[], 'order':[]}
    txt=open(p).read().splitlines()
    if not any(l.startswith('TOPLAM') or l.startswith('SURE') for l in txt): continue
    for l in txt:
        if l.startswith('CFG|'):
            f=l.split('|'); name=f[1]; kv={}
            for x in f[2:9]:
                k,v=x.split('=',1); kv[k]=v
            def grab(t): 
                m=re.search(r'CE=([\d.]+) PPL=([\d.]+) top1=([\d.]+) top5=([\d.]+)',t); return [fl(z) for z in m.groups()]
            kv['tr']=grab(f[9]); kv['va']=grab(f[10]); kv['te']=grab(f[11]); kv['gap']=fl(f[12].split('=')[1]); kv['sec']=f[13].split('=')[1]
            d['cfg'][name]=kv; d['order'].append(name)
        elif l.startswith('WDIFF|'):
            f=l.split('|'); d['wd'][(f[1],f[2])]=fl(f[3])
        elif l.startswith('HEDEF_KAPPA'): d['kap'].append(l)
        elif l.startswith('OGRETICI'): d['ogr'].append(l)
        elif l.startswith('SURE'): d['sure']=l
        elif l.startswith('CP|'): d.setdefault('cp',[]).append(l)
    S[s]=d
seeds=sorted(S)
man=[l.split('\t') for l in open(base+'v23_ham/v23log/manifest.tsv').read().splitlines()]
def avg(x): return sum(x)/len(x)
names=S[seeds[0]]['order']
DESC={'ridge_hot_lamV17':'Ridge one-hot, λ=V17 seçimi (referans, bu koşuda eğitim-yalnız)','ridge_hot_Cp':'Ridge one-hot, λ=Cp ile',
'A_PPL200':'V23 hedef CE=ln200, λ=Cp','A_PPL150':'V23 hedef CE=ln150, λ=Cp','A_PPL100':'V23 hedef CE=ln100, λ=Cp','B_PPL100_Top1_Top5':'V23 CE=ln100 + Top-1≥%20 + Top-5≥%40, λ=Cp','K0_saf_sayim_ogretici':'V23 κ=0 (saf sayım öğretici), λ=Cp',
'A_PPL100_lamV17':'V23 hedef CE=ln100, λ=V17 (aynı λ)','B_PPL100_Top1_Top5_lamV17':'V23 CE+Top-1+Top-5, λ=V17 (aynı λ)','K0_lamV17':'V23 κ=0, λ=V17 (aynı λ)'}
o=[];P=o.append
P('# TEK HAMLE V23 — Öğrenme hedeflerini doğrudan ağırlık üretim formülüne katma\n')
P(f'Dal `claude/formul-avcisi` · kod `cpp/tek_hamle_v23.cpp` · ham loglar/komutlar `v23_ham/`. H=1024, c=2, aynı metin/bölme. Tamamlanan tohumlar: {seeds}. Tüm modeller **yalnız eğitimden**; doğrulama (α,T) seçimi ve kontrol için, test bağımsız. Sonradan CE gradyanı/η düzeltmesi **yok**; tek ağırlık hesabı = tek ridge çözümü.\n')
P('## 1. Türetilen formül\n')
P('''Aday çerçeve `W=(FᵀΩF+Λ)⁻¹FᵀΩZ*` bu problem için şu forma indirildi:

`W* = (FᵀF + Λ(Q*))⁻¹ Fᵀ Q*`,  `Q* = Q*(κ)`,  `κ = max(κ_CE, κ_1, κ_5)`,  `Λ = λ*·I`, `λ* = argmin_λ Cp(λ; Q*)`

- **Öğretim matrisi Q\\* (veriden, ilk çözümden önce):** her eğitim satırı için **kendisi hariç (leave-one-out)** eğitim sayımlarından interpolasyonlu trigram/bigram/unigram dağılımı `P̂_i` (geri çekilme gücü b eğitim LOO CE'siyle seçilir). Böylece öğretim sinyali bağlamsal bilgiyi (hangi kelime hangi 2-kelimelik bağlamdan sonra geldi) taşır ve satırın kendi etiketini içermez (sızıntı yok). `Q*_i=(1−κ)P̂_i+κ·e_{y_i}` (κ>0) veya `(1+κ)P̂_i−κ·π` (κ<0, düzleştirme).
- **CE hedefi → κ_CE:** Q\\*'ın eğitim CE'si hedefe eşitlenir: `(1/n)Σ −ln[(1−κ)P̂_i(y_i)+κ] = CE*`. κ'da monoton tek skaler denklem (ikiye bölme, ağırlıklar hesaplanmadan çözülür). CE\\* ve PPL aynı kısıt: tek denklem.
- **Top-k hedefleri → κ_1, κ_5 (kapalı form):** y_i, Q\\*_i'de ilk-1 ⇔ `κ/(1−κ) > g1_i = max_{v≠y}(P̂_v−P̂_y)`; ilk-5 ⇔ `κ/(1−κ) > g5_i` (5. en büyük fark). Oranı ≥ hedef yapan en küçük κ = `r/(1+r)`, `r` = g'nin hedef-kuantili. (Marj koşulu `Q_y−Q_v>0`; ridge'in karesel kaybı ile kısmen taşınabilen karesel/doğrusal vekil.)
- **Genelleme → Λ:** Mallows Cp: `Cp(λ)=SSE(λ)+2σ̂²·df(λ)·V`, `SSE(λ)=‖Q\\*‖²−⟨W,R⟩−λ‖W‖²` (ridge normal denkleminden, ek gemm yok), `df(λ)=m−λ·tr((FᵀF+λI)⁻¹)`, `σ̂²` en esnek modelden. Cp eğitimden hesaplanır; `Q*` değiştiği için `λ*` da hedefe bağlıdır.
- **Ω = I:** örnek ağırlıklandırma V12'de başarısızdı, tekrarlanmadı; hedefler Ω'ya sokulmadı. (Bu bir eksiklik olarak not edilir.)

**Tek denklem kurulabildi mi?** Evet: `(FᵀF+λ*I)W=FᵀQ*` doğrusal. Ancak bunun yolu CE ve Top-k kısıtlarını **öğrencinin logitlerine** değil **öğreticinin dağılımına** koymaktır. Öğrencinin gerçek CE'si/Top-k'si üzerindeki kısıtlar W'da doğrusal olmayan ve (Top-k için) kesiklidir: `CE(W)=−ln softmax` dışbükey ama hedef eşitliği `CE(W)≤c` dışbükey kısıt olup W için kapalı formlu çözümü yok (Lagrange çarpanı → softmax'lı doğrusal olmayan denklem); Top-k sıralama koşulları `z_y−z_v≥δ` n·V doğrusal eşitsizlik (SVM/QP) — tek Cholesky'ye indirgenemez. Bu yüzden: kontrollü yaklaşım = hedefi öğreticiye koy, ridge ile en küçük kareler izdüşümünü al; sonucu **ölç**.\n''')
# main table
P('## 2. Kritik kontrol: yalnız hedef CE değişince ağırlıklar değişiyor mu? (tohum ortalaması)\n')
def row(name):
    ks=[S[s]['cfg'][name] for s in seeds]
    return ks
P('| yapılandırma | κ | λ | öğretici eğitim CE (PPL) | öğretici Top-1 / Top-5 (eğitim) | ‖ΔW‖/‖W‖ (A_PPL200\'e göre) | öğrenci eğitim PPL | doğr. PPL | **test PPL** | test Top-1 | test Top-5 |'); P('|---|---|---|---|---|---|---|---|---|---|---|')
for n in ['A_PPL200','A_PPL150','A_PPL100','B_PPL100_Top1_Top5','K0_saf_sayim_ogretici']:
    if n not in S[seeds[0]]['cfg']: continue
    ks=row(n); wd=[S[s]['wd'].get(('A_PPL200',n),S[s]['wd'].get((n,'A_PPL200'),0.0)) for s in seeds]
    P(f'| {DESC[n]} | {avg([fl(k["kappa"].replace("kappa=","")) if False else fl(k["kappa"]) for k in ks]):.5f} | {ks[0]["lambda"]} | {avg([fl(k["ogretici_egitim_CE"]) for k in ks]):.3f} ({2.718281828**avg([fl(k["ogretici_egitim_CE"]) for k in ks]):.0f}) | {100*avg([fl(k["ogretici_Top1"]) for k in ks]):.1f}% / {100*avg([fl(k["ogretici_Top5"]) for k in ks]):.1f}% | {avg(wd):.4f} | {avg([k["tr"][1] for k in ks]):.1f} | {avg([k["va"][1] for k in ks]):.1f} | **{avg([k["te"][1] for k in ks]):.2f}** | {100*avg([k["te"][2] for k in ks]):.2f}% | {100*avg([k["te"][3] for k in ks]):.2f}% |')
P('')
P('Not: ‖ΔW‖/‖W‖ ≈ 1e-3 (hedef 200→100): hedef CE gerçekten öğretim matrisini değiştiriyor ama çok az (aşağıda neden), Top-1/Top-5 hedefleri ise (κ≈0,03) ağırlıkları ≈%5 değiştiriyor.\n')
P('**Neden CE hedefi neredeyse hiç κ üretmiyor?** CE ortalaması ağır kuyrukludur: öğreticinin (LOO sayım) eğitim CE\'si ≈ 5,5; PPL 100 (CE 4,605) için yalnız κ≈5e-4 etiket karışımı yeterli, çünkü ortalama CE\'yi öğreticinin hiç görmediği örneklerdeki `−ln p≈10` tipi uç kayıplar belirliyor; bunları `−ln κ≈7,6` ile sınırlamak ortalamayı hedefe indiriyor. Yani **CE hedefi matematiksel olarak sağlanıyor ama öğretim sinyalinin içeriğini (hangi kelime) değiştirmiyor**; "100 PPL" bu şekilde bir öğrenme başarısı değil, uç kayıp sınırlaması. [Kesin: κ ve WDIFF değerleri loglarda]\n')
P('## 3. Tüm yapılandırmalar (tohum ortalaması; eğitim / doğrulama / test ayrı)\n')
P('| yapılandırma | λ | (α,T) | eğitim CE (PPL) top1 / top5 | doğr. CE (PPL) top1 / top5 | **test CE (PPL)** top1 / top5 | genelleme farkı (doğr.−eğitim CE) |'); P('|---|---|---|---|---|---|---|')
for n in names:
    ks=row(n)
    def f3(key): 
        return f'{avg([k[key][0] for k in ks]):.4f} ({avg([k[key][1] for k in ks]):.1f}) {100*avg([k[key][2] for k in ks]):.1f}% / {100*avg([k[key][3] for k in ks]):.1f}%'
    P(f'| {DESC[n]} | {ks[0]["lambda"]} | α={ks[0]["alpha"]},T={ks[0]["T"]} | {f3("tr")} | {f3("va")} | **{f3("te")}** | {avg([k["gap"] for k in ks]):.3f} |')
P('')
P('## 4. Öğreticinin kendi kalitesi (yalnız eğitim sayımları, LOO)\n')
for l in S[seeds[0]]['ogr']: P('- `'+l+'`')
P('')
P('Öğretici (LOO sayım) kendi başına zayıf: eğitim-LOO CE ≈ 5,5 (PPL ≈ 250) — ölçülmüş trigram (PPL 137) sayım+düzgünleştirmesinden çok daha kötü; yani burada kullanılan basit interpolasyon, güçlü bir öğretmen değil. [Muhtemel: b ızgarasının kenarı genişletildi, bkz. log]\n')
P('## 5. κ ve hedefler\n')
for l in S[seeds[0]]['kap']: P('- `'+l+'`')
P('')
P('## 6. Ağırlık farkları (tohum 1, ‖Wa−Wb‖/‖Wb‖)\n')
P('| a | b | fark |'); P('|---|---|---|')
for (a,b),v in S[seeds[0]]['wd'].items(): P(f'| {a} | {b} | {v:.4f} |')
P('')
P('## 7. Karşılaştırma (H=1024, test PPL)\n')
P('| model | test PPL | not |'); P('|---|---|---|')
h=row('ridge_hot_lamV17'); a=row('A_PPL100_lamV17') if 'A_PPL100_lamV17' in names else None; b=row('B_PPL100_Top1_Top5_lamV17') if 'B_PPL100_Top1_Top5_lamV17' in names else None
P('| V17 Ridge (eğitim+doğrulama modeli) | 206,8 | rapor |'); P('| V20 CE düzeltmesi | 198,9 | rapor |'); P('| trigram referansı | 137 | ölçülmüş |')
P(f'| bu koşuda one-hot Ridge (eğitim-yalnız, λ=V17) | {avg([k["te"][1] for k in h]):.1f} | doğru karşılaştırma tabanı |')
if a: P(f'| **V23** (CE=ln100 hedefli, λ=V17) | {avg([k["te"][1] for k in a]):.1f} | Top-1 {100*avg([k["te"][2] for k in a]):.1f}% |')
if b: P(f'| **V23** (CE+Top-1+Top-5 hedefli, λ=V17) | {avg([k["te"][1] for k in b]):.1f} | Top-1 {100*avg([k["te"][2] for k in b]):.1f}%, Top-5 {100*avg([k["te"][3] for k in b]):.1f}% |')
P('')
P('## 8. Sınırlar ve dürüst değerlendirme\n')
P('PLACEHOLDER_SINIR')
P('\n## 9. Çalıştırma\n')
P('Derleme: `g++ -O3 -march=native -fopenmp -std=c++17 -o tek_hamle_v23 cpp/tek_hamle_v23.cpp`; komut: `./tek_hamle_v23 2 1024 TOHUM GAMMA LAMBDA [top1=0.20] [top5=0.40]` ((γ,λ)=(4,10),(4,100),(2,1) V17 seçimi).\n')
P('| komut | wall s | user CPU s | RAM MB |'); P('|---|---|---|---|')
for m in man:
    kv=dict(x.split('=') for x in m[2:]); P(f'| `{m[0]}` | {kv["wall_s"]} | {kv["user_cpu_s"]} | {kv["maxrss_MB"]} |')
open(base+'TEK_HAMLE_V23_RAPOR.md','w').write('\n'.join(o)+'\n')
