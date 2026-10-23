import re,glob,math
base='/home/user/name/formul_avcisi/'
def rd(p): return open(p).read()
t1=rd(base+'v24_ham/v24log/v24_c2_H1024_s1.txt'); t5=rd(base+'v24_ham/v24log/v24_c2_H512_s1.txt')
def line(t,pat): 
    m=re.search(r'^'+pat+r'.*$',t,re.M); return m.group(0) if m else ''
def og(t,k):
    m=re.search(r'^OGRENCI\|'+re.escape(k)+r'\|lambda=(\S+)\|alpha=(\S+)\|T=(\S+)\|egitim CE=([\d.]+) PPL=([\d.]+) top1=([\d.]+) top5=([\d.]+)\|dogr CE=([\d.]+) PPL=([\d.]+) top1=([\d.]+) top5=([\d.]+)\|test CE=([\d.]+) PPL=([\d.]+) top1=([\d.]+) top5=([\d.]+)\|dogr-egitim CE=(-?[\d.]+)',t,re.M)
    return m.groups() if m else None
def di(t,k):
    m=re.search(r'^DISTIL\|'+re.escape(k)+r'\|.*?rel_hata egitim/dogr/test=([\d.]+)/([\d.]+)/([\d.]+)\|KL\(Q\|\|ogrenci\) ([\d.]+)/([\d.]+)/([\d.]+)',t,re.M); return m.groups() if m else None
def ngl(t,model,split):
    m=re.search(r'^NG\|'+model+r'\|'+re.escape(split)+r'\|CE=([\d.]+)\|PPL=([\d.]+)\|top1=([\d.]+)\|top5=([\d.]+)',t,re.M); return m.groups()
o=[];P=o.append
P('# TEK HAMLE V24 — Ağırlık üretiminde öğrenme sınırını ölç (ölçüm/teşhis)\n')
P('Dal `claude/formul-avcisi` · kod `cpp/tek_hamle_v24.cpp` (V23 kopyası; yeni mimari yok, ölçüm aracı) · ham loglar/komutlar `v24_ham/`. Aynı 50 000 tokenlık metin, V=2001, c=2, 39 840 / 4 980 / 4 980 örnek. **Tüm öğrenciler yalnız eğitimden** (W, D, Q* eğitimden; doğrulama yalnız α,T/η seçimi ve n-gram EM ağırlıkları; test bağımsız). Bu yüzden V20 sayıları, V20 raporundaki eğitim+doğrulama modeline göre biraz farklıdır (protokol farkı bilerek korunuyor).\n')
P('## 0. Ana bulgular (H=1024, tohum 1; ayrıntı aşağıda)\n')
P('''1. [Kesin] **Bilgi sınırı:** bu 50k tokenlık veride en iyi ölçülen sayım modeli trigram, test PPL 138,9 (eğitim sayımları) / 135,4 (eğitim+doğrulama sayımları, V8 protokolü); 137 referansı yeniden üretildi. Bigram 144,9. Trigram bağlamı test satırlarının yalnız %68,1'inde görülmüş, (bağlam, hedef) çifti %23,6'sında. PPL ≤ 100 hiçbir ölçülen modelde yok; eğitimde 100'ün altına inmek yalnız ezberlemeyle mümkün (eğitim PPL 99,8, doğrulama 214). [Muhtemel] Bu veride PPL ≤ 100 bilgi bakımından da ulaşılamaz görünüyor (tahmin edilemeyen test hedefi oranı yüksek); bunu kanıtlayan ölçüm yok, yalnız bu veriyle tutarlı.
2. [Kesin] **Bilgi F'ye çok az aktarılıyor.** Aynı F üzerinde trigram olasılıklarını (Q) kapalı formda regres ettiğimizde açıklanan varyans (1 − göreli hata) yalnız ≈ %43 (eğitim) / %41 (doğrulama, test), KL(Q‖öğrenci) ≈ 0,69–0,75 nat. H=512'de ≈ %39. λ 0,3→300 değişince bu oran neredeyse değişmiyor (0,567→0,585): kayıp **temsilden (F ve karesel kayıp)**, çözücü/düzenlileştirmeden değil.
3. [Kesin] **Çözüm ve genelleme ikincil:** Cholesky çözüm artığı ≈ 1e-7 (V22); Q'yu uydurmada eğitim→test göreli hata artışı yalnız ≈ +0,011 (%2). Etiketle eğitilen A için ise eğitim–doğrulama CE farkı 0,30 (H=1024): burada gerçek bir genelleme açığı var (eğitim CE 5,12, doğrulama 5,42).
4. [Kesin] **Öğreticinin başarısı öğrenciye geçmiyor:** Q (trigram) test CE 4,934; B (Q'dan regresyonla üretilen ağırlık) test CE 5,404 (PPL 222,2); A (etiket) 5,340 (PPL 208,5); V20 5,315 (203,4). Unigram→trigram CE kazancı 0,711 nat; B bunun %34'ünü, A %43'ünü, V20 %46'sını geri alıyor.
5. [Kesin] **Hedef → ağırlık bağı zayıf:** hedef PPL 200/150/100 için Q* değişimi ‖ΔQ‖/‖Q‖ ≤ 0,0019, ağırlık değişimi ‖ΔW‖/‖W‖ ≤ 0,0008, test PPL 211,08 / 211,08 / 211,07. Darboğaz W'ya aktarım değil, hedefin öğretim sinyalini neredeyse hiç değiştirmemesi (CE ortalaması uç satırların tavanlanmasıyla sağlanıyor).
''')
P('**Ana soruya cevap (sınır nerede?):** (a) *bilgi*: n-gram tavanı ≈ 135–139 PPL, hedef 100 bu veride bilgi bakımından erişilmez görünüyor [Muhtemel]; (b) *özellik matrisi F*: mevcut tanh(γ·X·W1) özellikleri trigram bilgisinin yalnız ≈ %40’ını temsil ediyor — **en büyük ölçülebilir kayıp burada**; (c) *çözüm*: ridge/Cholesky kayıp üretmiyor; (d) *genelleme*: etiketle eğitimde belirgin (0,30 nat), öğreticiyle eğitimde küçük; (e) *hedef kanalı*: hedef CE→Q* haritası ağırlıkları değiştirmiyor.\n')
# A
P('## 1. Deney A — bağlamsal bilgi\n')
P('n-gram: lineer interpolasyon (ML trigram/bigram + unigram(+1) + düzgün), karışım ağırlıkları doğrulamada EM (V8 yöntemi; bağlam bulunmayan bileşen ağırlığı yeniden normalleştirilir). EM ağırlıkları: bigram a2=0,530 a1=0,465 a0=0,005; trigram a3=0,046 a2=0,483 a1=0,466 a0=0,005 → **trigram bileşeni karışımda yalnız %4,6**.\n')
P('| model | eğitim (sayım kendini içerir) PPL | eğitim LOO PPL | doğrulama PPL | test PPL (eğitim sayımları) | test PPL (eğitim+doğr. sayımları) | test Top-1 | test Top-5 |'); P('|---|---|---|---|---|---|---|---|')
for md in ('unigram','bigram','trigram'):
    a=ngl(t1,md,'egitim(sayim kendini icerir)'); b=ngl(t1,md,'egitim(LOO)'); c=ngl(t1,md,'dogrulama'); d=ngl(t1,md,'test(egitim sayimlari)'); e=ngl(t1,md,'test(egitim+dogrulama sayimlari; V8 protokolu)')
    P(f'| {md} | {a[1]} | {b[1]} | {c[1]} | {d[1]} | {e[1]} | {100*float(d[2]):.1f}% | {100*float(d[3]):.1f}% |')
for nm,k in (('V17 Ridge (λ=10)','V17_Ridge_WA'),('V20 CE düzeltmesi (η val ile)','V20_CE_duzeltme'),('V23 hedef-koşullu (CE=ln100, b=8)','V23_hedef_PPL100_kappa=0.000515')):
    r=og(t1,k); P(f'| {nm} | {r[4]} | – | {r[8]} | {r[12]} | – | {100*float(r[13]):.1f}% | {100*float(r[14]):.1f}% |')
P('')
P('Not: trigram 137 referansı **yeniden üretildi** (135,4 V8 protokolü; 138,9 yalnız eğitim sayımlarıyla; V8 raporu 137,4). Küçük fark: burada yalnız bloklarda t ≥ c konumları ve bağlam bulunmayan bileşenin yeniden normalleştirilmesi kullanıldı. Eğitim PPL\'i sayım kendini içerdiğinde 22,3 (trigram) / 45,8 (bigram): n-gram eğitim başarısı ezberdir; LOO ile 168 / 176.\n')
P('Kapsama (eğitim sayımlarıyla):\n\n| küme | bigram bağlamı görüldü | trigram bağlamı görüldü | bigram (bağlam,hedef) görüldü | trigram (bağlam,hedef) görüldü |\n|---|---|---|---|---|\n| doğrulama | %100 | %66,7 | %66,6 | %22,3 |\n| test | %100 | %68,1 | %68,0 | %23,6 |\n')
P('Kelime sıklığına göre test (tohum 1, H=1024):\n')
P('| model | sık CE / Top-1 | orta CE / Top-1 | nadir CE / Top-1 |'); P('|---|---|---|---|')
def gr(t,k):
    out=[]
    for g in ('sik','orta','nadir'):
        m=re.search(r'^GRUP\|'+re.escape(k)+r'\|'+g+r'\|n=\d+\|CE=([\d.]+)\|top1=([\d.]+)',t,re.M); out.append(f'{m.group(1)} / {100*float(m.group(2)):.1f}%')
    return out
for nm,k in (('unigram','unigram'),('bigram','bigram'),('trigram','trigram'),('V17 Ridge','V17_Ridge'),('V20','V20'),('V23','V23'),('B (Q=trigram→W)','B_Qtam')):
    g=gr(t1,k); P(f'| {nm} | {g[0]} | {g[1]} | {g[2]} |')
P('')
P('Bağlam bilgisi arttığında (test, trigram bağlamı görülen 3 392 satır vs yalnız bigram görülen 1 588 satır) CE:\n')
P('| model | trigram bağlamı GÖRÜLDÜ CE / Top-1 | yalnız bigram görüldü CE / Top-1 |'); P('|---|---|---|')
def bg(t,k):
    r=[]
    for lab in ('GORULDU','yalniz bigram goruldu'):
        m=re.search(r'^BAGLAM\|'+re.escape(k)+r'\|trigram baglami '+lab+r'\|n=\d+\|CE=([\d.]+)\|top1=([\d.]+)',t,re.M); r.append(f'{m.group(1)} / {100*float(m.group(2)):.1f}%')
    return r
for nm,k in (('unigram','unigram'),('bigram','bigram'),('trigram','trigram'),('V17 Ridge','V17_Ridge'),('V20','V20'),('V23','V23'),('B (Q=trigram→W)','B_Qtam')):
    g=bg(t1,k); P(f'| {nm} | {g[0]} | {g[1]} |')
P('')
P('- [Kesin] Trigramın bigram\'a kazancı yalnız ≈ %4 PPL (144,9 → 138,9); trigram bağlamı görülen satırlarda trigram CE 4,928, bigram 4,988. Ridge/V20/V23 trigram bağlamı **görülen** satırlarda yalnız-bigram satırlarına göre **daha kötü** (V17: 5,383 vs 5,249) — bu modeller görülen trigram bağlamından yararlanmıyor (n-gram\'ın tersine).')
P('- [Kesin] Ridge\'in n-gram\'a göre kaybı orta kelimelerde (CE 6,45 vs bigram 5,61, Top-1 %5,2 vs %4,9 ama Top-5 %6,7 vs %14,1) ve Top-5\'te (%30,0 vs %35,8); sık kelimelerde fark küçük (3,63 vs 3,52).\n')
# B
P('## 2. Deney B — Q (trigram) → F → W bilgi aktarımı (H=1024, tohum 1)\n')
P('Q = EM ağırlıklı trigram olasılıkları (eğitim sayımları). "tam": eğitim satırları için sayım kendini içerir; "LOO": satırın kendisi çıkarılmış. Göreli hata = ‖FW−Q‖² / ‖Q−π‖² (π eğitim unigram; 0 = mükemmel, 1 = unigram kadar kötü). Q\'nun kendi test CE\'si 4,934.\n')
P('| W | λ | göreli hata eğitim / doğr. / test | KL(Q‖öğrenci) eğitim/doğr./test | gerçek etikette CE (PPL): eğitim | doğrulama | **test** | test Top-1 / Top-5 | ‖W‖ | ‖WA−WB‖/‖WA‖ |'); P('|---|---|---|---|---|---|---|---|---|---|')
for lam in ('0.3','10','30','300'):
    kA='A_etiket_lam'+lam; r=og(t1,kA); d=di(t1,kA)
    w=re.search(r'^W_NORM\|lam='+lam+r'\|Q=tam\|\|\|WA\|\|=([\d.]+)\|\|\|WB\|\|=([\d.]+)\|\|\|WA-WB\|\|/\|\|WA\|\|=([\d.]+)',t1,re.M).groups()
    P(f'| A (etiket) | {lam} | {d[0]}/{d[1]}/{d[2]} (Q\'ya karşı) | {d[3]}/{d[4]}/{d[5]} | {r[3]} ({r[4]}) | {r[7]} ({r[8]}) | **{r[11]} ({r[12]})** | {100*float(r[13]):.1f}% / {100*float(r[14]):.1f}% | {w[0]} | – |')
    for mode,lab in (('tam','B_Qtam'),('LOO','B_QLOO')):
        kB=lab+'_lam'+lam; r=og(t1,kB); d=di(t1,kB)
        w=re.search(r'^W_NORM\|lam='+lam+r'\|Q='+mode+r'\|\|\|WA\|\|=([\d.]+)\|\|\|WB\|\|=([\d.]+)\|\|\|WA-WB\|\|/\|\|WA\|\|=([\d.]+)',t1,re.M).groups()
        P(f'| B (Q {mode}) | {lam} | {d[0]}/{d[1]}/{d[2]} | {d[3]}/{d[4]}/{d[5]} | {r[3]} ({r[4]}) | {r[7]} ({r[8]}) | **{r[11]} ({r[12]})** | {100*float(r[13]):.1f}% / {100*float(r[14]):.1f}% | {w[1]} | {w[2]} |')
P('')
P('''Okuma:
- [Kesin] B, Q\'yu **temsil edemiyor** (göreli hata 0,57–0,63, yani varyansın ≈ %40\'ı açıklanıyor) ve λ\'dan neredeyse bağımsız: λ=0,3→300 arasında eğitim hatası 0,567→0,585. En esnek ridge ile bile ≈ 0,567 → **temsil sınırı** (F\'nin kapsadığı uzay + karesel kayıp).
- [Kesin] Eğitim–test göreli hata farkı +0,011 → B için genelleme açığı yok; B\'nin eğitim–doğrulama CE farkı negatif (−0,003…−0,05): öğretici ezberi aktarılamıyor, ağırlıklar fazla "yumuşak" kalıyor.
- [Kesin] B gerçek etikette A\'dan kötü (222,2 vs 208,5): olasılık-uzayı regresyonu, kalibre softmax-CE ile uyuşan skor ayrımı üretmiyor. Fark kalibrasyondan (α=0,25,T=0,01) değil W\'dan: ‖WB‖ ≈ 0,55 vs ‖WA‖ ≈ 1,84; ‖WA−WB‖/‖WA‖ ≈ 0,82.
- [Kesin] "LOO" Q ile eğitim CE (5,508) "tam" Q\'dan (5,474) kötü ve test da kötü (223,9 vs 222,2): öğretici ezberini çıkarmak aktarımı iyileştirmedi.
- A\'nın "Q\'ya karşı" hatası (1,8) anlamlı bir uydurma ölçüsü değil, yalnız A\'nın Q\'dan ne kadar uzak olduğunu gösterir.
''')
P('**F temsilinden mi, Ridge çözümünden mü?** [Kesin] Ridge çözümü (λ etkisi) toplam Q-uydurma hatasının ≈ %3\'ü (0,567→0,585), genelleme ≈ %2\'si (+0,011); kalanı (≈ %95) F\'nin temsil sınırıdır. Kapasite artışı bunu çok yavaş azaltıyor (Deney C). [Muhtemel] Sebep: girdi X, 64-boyutlu PPMI-SVD gömmelerinden (kelime çifti kimliği kaybolur) + rastgele tanh özellikleri; trigram tablosu 2001² kelime çiftine duyarlı, rastgele özellikler bunu ayıramıyor. Bu girdi darboğazını ayrı ölçmedim (X-üzerinde doğrusal ridge karşılaştırması yapılmadı).\n')
# C
P('## 3. Deney C — özellik kapasitesi (tohum 1)\n')
P('| H | model | eğitim CE (PPL) | doğrulama CE | test CE (PPL) | test Top-1 / Top-5 | eğitim–doğr. fark | koşu süresi / RAM (tüm modeller birlikte) |'); P('|---|---|---|---|---|---|---|---|')
man=[l.split('\t') for l in rd(base+'v24_ham/v24log/manifest.tsv').splitlines()]
mm={}
for m_ in man:
    kv=dict(x.split('=') for x in m_[2:]); h=re.search(r'v24 2 (\d+) 1 ',m_[0]) or re.search(r'tek_hamle_v24 2 (\d+) (\d)',m_[0])
    mm[(m_[0].split()[2], m_[0].split()[3])]=f'{kv["wall_s"]} s / {kv["maxrss_MB"]} MB'
for H,t,lamv in ((512,t5,'1'),(1024,t1,'10')):
    for nm,k in (('V17 Ridge (A)','V17_Ridge_WA'),('V20','V20_CE_duzeltme'),('B (Q=trigram, λ=V17)','B_Qtam_lam'+lamv)):
        r=og(t,k)
        P(f'| {H} | {nm} | {r[3]} ({r[4]}) | {r[7]} | {r[11]} ({r[12]}) | {100*float(r[13]):.1f}% / {100*float(r[14]):.1f}% | {r[15]} | {mm.get((str(H),"1"),"")} |'.replace(f'({r[11]})',f'({r[12]})'))
P('')
r5=di(t5,'B_Qtam_lam1'); r10=di(t1,'B_Qtam_lam10'); r5b=di(t5,'B_Qtam_lam0.3'); r10b=di(t1,'B_Qtam_lam0.3')
P(f'Q-uydurma (B, "tam" Q): H=512 göreli hata eğitim/doğr./test λ=0,3: {"/".join(r5b[:3])}; λ=1: {"/".join(r5[:3])}. H=1024 λ=0,3: {"/".join(r10b[:3])}; λ=10: {"/".join(r10[:3])}. KL: H=512 {"/".join(r5b[3:])} → H=1024 {"/".join(r10b[3:])}.\n')
P('''- [Kesin] H 512→1024: etiket-A eğitim CE 5,357→5,118 (−0,24), doğrulama 5,446→5,418 (−0,03), test 5,374→5,340 (−0,03); eğitim–doğrulama farkı 0,09→0,30. **Eğitim büyük iyileşiyor, test yok → ezberleme/genelleme.**
- [Kesin] Öğretici aktarımı: Q-uydurma göreli hata 0,604→0,567 (H=512→1024, λ=0,3): kapasite iki katına çıkınca açıklanan varyans yalnız %39,6→%43,3. KL 0,77→0,75. Yani H büyürken öğretici bilgisi **yavaş** aktarılıyor: temsil kapasitesi sınırlayıcı, fakat H artışı yetersiz (log-benzeri). [Tahmin] %60 hatayı ≈ 0,2'ye indirmek için H çok daha büyük olmalı; ölçülmedi.
- Süre/RAM: H=512 78 s, 644 MB; H=1024 129 s, 791 MB (bu koşu n-gram + dağıtım değerlendirmeleri dahil; Ridge+gram tek başına 1,7 s / 0,3 s).
''')
# D
P('## 4. Deney D — hedef ↔ ağırlık ilişkisi (V23 Q*, b=8 LOO öğretici, λ=V17)\n')
P('| hedef PPL | CE hedefi | κ (öğretim karışımı) | öğretici eğitim CE (hedefe eşit) | öğrenci eğitim PPL | test PPL | test Top-1 |'); P('|---|---|---|---|---|---|---|')
for ppl,ce in ((200,'5.2983'),(150,'5.0106'),(100,'4.6052')):
    k=[mm_ for mm_ in re.findall(r'^OGRENCI\|V23_hedef_PPL'+str(ppl)+r'_kappa=([\d.]+)',t1,re.M)][0]; r=og(t1,f'V23_hedef_PPL{ppl}_kappa={k}')
    P(f'| {ppl} | {ce} | {k} | {ce} | {r[4]} | {r[12]} | {100*float(r[13]):.2f}% |')
P('')
P('| karşılaştırma | ‖ΔQ*‖/‖Q*‖ | ‖ΔW‖/‖W‖ | ‖FΔW‖/‖FW‖ |'); P('|---|---|---|---|')
for l in re.findall(r'^D_FARK\|hedef (\d+) vs (\d+)\|kappa [\d.]+ vs [\d.]+\|\|\|dQ\|\|/\|\|Q\|\|=([\d.]+)\|\|\|dW\|\|/\|\|W\|\|=([\d.]+)\|\|\|F dW\|\|/\|\|dQ\|\|\(aktarim\)=([\d.]+)\|\|\|F dW\|\|/\|\|F W\|\|=([\d.]+)',t1,re.M):
    P(f'| {l[0]} vs {l[1]} (H=1024) | {l[2]} | {l[3]} | {l[5]} |')
for l in re.findall(r'^D_FARK\|hedef (\d+) vs (\d+)\|kappa [\d.]+ vs [\d.]+\|\|\|dQ\|\|/\|\|Q\|\|=([\d.]+)\|\|\|dW\|\|/\|\|W\|\|=([\d.]+)\|\|\|F dW\|\|/\|\|dQ\|\|\(aktarim\)=([\d.]+)\|\|\|F dW\|\|/\|\|F W\|\|=([\d.]+)',t5,re.M):
    if l[0]=='200' and l[1]=='100': P(f'| {l[0]} vs {l[1]} (H=512) | {l[2]} | {l[3]} | {l[5]} |')
P('''
- [Kesin] Hedef 200→100: öğretim bilgisi %0,19, ağırlık %0,08 değişti; test PPL 211,08→211,07. Ağırlığa aktarım zincirinde ΔW/W ≈ 0,4–0,8×ΔQ/Q: Q değişiminin yalnız bir kısmı F uzayında.
- [Kesin] Kayıp halka **hedef→öğretim sinyali**: κ yalnız 1e-5…5e-4. CE ortalaması ağır kuyrukludur; κ-karışımı satır kaybını −ln κ ile tavanlayarak ortalamayı hedefe indirir, hangi kelimenin öğretileceğini değiştirmez. Ağırlık aktarım katsayısı (ΔW/ΔQ) ~0,4–0,8 olduğundan tek başına darboğaz değil.
- Not: `‖FΔW‖/‖ΔQ‖` sütunu loglarda var ama değerler (0,92–2,45) teorik üst sınır 1'i aşıyor (float kuadratik form kesinlik hatası olası); **güvenilmez, rapora konmadı** [Kesin: değer geçersiz].
''')
# seeds
rows=[]
for p in sorted(glob.glob(base+'v24_ham/v24log/v24_c2_H1024_s*.txt')):
    t=rd(p); s=re.search(r'_s(\d)\.txt',p).group(1); lam=re.search(r'lambda_V17=(\S+)',t).group(1)
    a=og(t,'V17_Ridge_WA'); b=og(t,'V20_CE_duzeltme'); c=og(t,'V23_hedef_PPL100_kappa=0.000515'); d=og(t,'B_Qtam_lam'+lam); e=ngl(t,'trigram','test(egitim sayimlari)'); f=ngl(t,'bigram','test(egitim sayimlari)')
    rows.append((s,a,b,c,d,e,f))
P('## 5. Tohum tutarlılığı (H=1024, test PPL / Top-1)\n')
P('| tohum | bigram | trigram | V17 Ridge | V20 | V23 (CE=ln100) | B (Q→W) |'); P('|---|---|---|---|---|---|---|')
for s,a,b,c,d,e,f in rows: P(f'| {s} | {float(f[1]):.1f} | {float(e[1]):.1f} | {a[12]} / {100*float(a[13]):.1f}% | {b[12]} / {100*float(b[13]):.1f}% | {c[12]} / {100*float(c[13]):.1f}% | {d[12]} / {100*float(d[13]):.1f}% |')
P('')
P('''## 6. Sınırlamalar ve dürüstlük notları

- Tek metin, tek bölme; test n = 4 980 (Top-1 standart hatası ≈ %0,5). Ana ayrıntı tohum 1; tohum tablosu (bölüm 5) yalnız test PPL/Top-1 içerir, H=512 yalnız tohum 1.
- n-gram: EM ağırlıkları doğrulamada, sayımlar eğitimde (test PPL 138,9) veya eğitim+doğrulamada (135,4, yalnız V8 protokolünü yeniden üretmek için). V8'in 137,4'ü tam örtüşmedi; fark ≈ ±2 PPL (düzgünleştirme ayrıntısı).
- B deneyi olasılık uzayında karesel kayıpla yapıldı; "F temsil sınırı" ile "karesel kayıp / olasılık uzayı" etkisini ayıramadım. Bunun için logit uzayında veya CE ile bir öğrenci gerekirdi — yeni sistem sayıldığı için kurulmadı. [Kesin: ayrım yapılamadı]
- Girdi X (192-boyutlu PPMI-SVD) ile rastgele tanh özelliklerinin payı ayrı ölçülmedi; H > 1024 denenmedi. "Temsil sınırı" bulgusu F+karesel kayıp bileşkesi için geçerlidir. [Muhtemel] X darboğazı baskın.
- `‖FΔW‖/‖ΔQ‖` aktarım oranı geçersiz (bkz. bölüm 4 notu), kullanılmadı.
- V20 sayıları bu raporda yalnız-eğitim protokolüyle (η: {0,0.003,0.005,0.0075,0.01}, α,T doğrulama CE): test PPL 203,4 (V20 raporu 198,9 eğitim+doğrulama modeliyle; karıştırılmamalı).

## 7. Somut öneri (uygulanmadı, ana mimariye eklenmedi)

1. **Ulaşılabilirlik sertifikası (tek Cholesky, W çözülmeden):** Q (öğretim dağılımı) ve F verildiğinde, F uzayında ulaşılabilecek en küçük göreli hata `e_min = (‖Q‖² − tr(RᵀS⁻¹R)) / ‖Q−π‖²`, `R=FᵀQ`, `S=FᵀF` (λ→0). Bu veride λ=0,3 ile ölçülen hata 0,567 (H=1024) / 0,604 (H=512); e_min bunların altında kalamaz ve λ duyarsızlığı nedeniyle ≈ aynıdır: hedef olarak "Q'nun %X'i temsil edilsin" denirse yalnız `X ≤ %43` ulaşılabilir. Hedef değerleri formüle bu sertifikayla girmeli: hedef ε < e_min ise formül hedefi karşılayamaz ve bunu hesaplamadan raporlar. Böylece "100 PPL yaz, sonuç hedefe uymasa da sessiz kal" yerine matematiksel olarak ulaşılamazlık kanıtı üretilir.
2. **Hedefi CE ortalamasına değil, dağılım bütçesine bağla:** CE ortalaması ağır kuyrukludur (V23/V24 D: κ 1e-5…5e-4 ağırlığı değiştirmedi). Kuadratikleştirilebilir bütçe: `KL(Q‖P_öğrenci) ≤ ε` ≈ `‖FW−Q‖²_Σ ≤ ε'`; λ, `df(λ)` ve eğitim hatası `e(λ)` ile `e(λ)=ε'` eşitliğinden (monoton; Cholesky taramasıyla) seçilir. Bu, hedefi doğrudan Λ'ya katar; bu veride `e(λ)` λ'ya duyarsız olduğu için **önce F zenginleştirilmelidir**, yoksa hedef ağırlıkları yönlendiremez.
3. **Darboğaz F:** ölçülen en büyük kayıp F'nin trigram bilgisini taşıyamamasıdır (açıklanan varyans %43; H iki katına çıkınca +%4). Bir sonraki ölçüm: (i) X-üzerinde doğrusal ridge vs F ile `e_min` karşılaştırması (X mi, rastgele özellik mi sınırlıyor); (ii) H = 2048/4096 ile `e_min(H)` eğrisi; (iii) kelime-çifti kimliğini taşıyan seyrek blokların `e_min`'e etkisi (V10–V11 bu yönde kısmi denendi). `e_min` bir kez Cholesky ile hesaplandığından bu ölçümler ucuzdur. [Tahmin] `e_min` ancak çift-kimlik bilgisi F'ye girerse belirgin düşer.
4. **Gerçekçi tavan:** bu 50k token için n-gram tavanı ≈ 135–139 PPL; hedef 100 yerine ≤ 140–150 aralığı gerçek bilgiyle tutarlı bir hedeftir. [Muhtemel]
''')
open(base+'TEK_HAMLE_V24_RAPOR.md','w').write('\n'.join(o)+'\n')
