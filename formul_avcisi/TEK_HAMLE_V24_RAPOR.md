# TEK HAMLE V24 — Ağırlık üretiminde öğrenme sınırını ölç (ölçüm/teşhis)

Dal `claude/formul-avcisi` · kod `cpp/tek_hamle_v24.cpp` (V23 kopyası; yeni mimari yok, ölçüm aracı) · ham loglar/komutlar `v24_ham/`. Aynı 50 000 tokenlık metin, V=2001, c=2, 39 840 / 4 980 / 4 980 örnek. **Tüm öğrenciler yalnız eğitimden** (W, D, Q* eğitimden; doğrulama yalnız α,T/η seçimi ve n-gram EM ağırlıkları; test bağımsız). Bu yüzden V20 sayıları, V20 raporundaki eğitim+doğrulama modeline göre biraz farklıdır (protokol farkı bilerek korunuyor).

## 0. Ana bulgular (H=1024, tohum 1; ayrıntı aşağıda)

1. [Kesin] **Bilgi sınırı:** bu 50k tokenlık veride en iyi ölçülen sayım modeli trigram, test PPL 138,9 (eğitim sayımları) / 135,4 (eğitim+doğrulama sayımları, V8 protokolü); 137 referansı yeniden üretildi. Bigram 144,9. Trigram bağlamı test satırlarının yalnız %68,1'inde görülmüş, (bağlam, hedef) çifti %23,6'sında. PPL ≤ 100 hiçbir ölçülen modelde yok; eğitimde 100'ün altına inmek yalnız ezberlemeyle mümkün (eğitim PPL 99,8, doğrulama 214). [Muhtemel] Bu veride PPL ≤ 100 bilgi bakımından da ulaşılamaz görünüyor (tahmin edilemeyen test hedefi oranı yüksek); bunu kanıtlayan ölçüm yok, yalnız bu veriyle tutarlı.
2. [Kesin] **Bilgi F'ye çok az aktarılıyor.** Aynı F üzerinde trigram olasılıklarını (Q) kapalı formda regres ettiğimizde açıklanan varyans (1 − göreli hata) yalnız ≈ %43 (eğitim) / %41 (doğrulama, test), KL(Q‖öğrenci) ≈ 0,69–0,75 nat. H=512'de ≈ %39. λ 0,3→300 değişince bu oran neredeyse değişmiyor (0,567→0,585): kayıp **temsilden (F ve karesel kayıp)**, çözücü/düzenlileştirmeden değil.
3. [Kesin] **Çözüm ve genelleme ikincil:** Cholesky çözüm artığı ≈ 1e-7 (V22); Q'yu uydurmada eğitim→test göreli hata artışı yalnız ≈ +0,011 (%2). Etiketle eğitilen A için ise eğitim–doğrulama CE farkı 0,30 (H=1024): burada gerçek bir genelleme açığı var (eğitim CE 5,12, doğrulama 5,42).
4. [Kesin] **Öğreticinin başarısı öğrenciye geçmiyor:** Q (trigram) test CE 4,934; B (Q'dan regresyonla üretilen ağırlık) test CE 5,404 (PPL 222,2); A (etiket) 5,340 (PPL 208,5); V20 5,315 (203,4). Unigram→trigram CE kazancı 0,711 nat; B bunun %34'ünü, A %43'ünü, V20 %46'sını geri alıyor.
5. [Kesin] **Hedef → ağırlık bağı zayıf:** hedef PPL 200/150/100 için Q* değişimi ‖ΔQ‖/‖Q‖ ≤ 0,0019, ağırlık değişimi ‖ΔW‖/‖W‖ ≤ 0,0008, test PPL 211,08 / 211,08 / 211,07. Darboğaz W'ya aktarım değil, hedefin öğretim sinyalini neredeyse hiç değiştirmemesi (CE ortalaması uç satırların tavanlanmasıyla sağlanıyor).

**Ana soruya cevap (sınır nerede?):** (a) *bilgi*: n-gram tavanı ≈ 135–139 PPL, hedef 100 bu veride bilgi bakımından erişilmez görünüyor [Muhtemel]; (b) *özellik matrisi F*: mevcut tanh(γ·X·W1) özellikleri trigram bilgisinin yalnız ≈ %40’ını temsil ediyor — **en büyük ölçülebilir kayıp burada**; (c) *çözüm*: ridge/Cholesky kayıp üretmiyor; (d) *genelleme*: etiketle eğitimde belirgin (0,30 nat), öğreticiyle eğitimde küçük; (e) *hedef kanalı*: hedef CE→Q* haritası ağırlıkları değiştirmiyor.

## 1. Deney A — bağlamsal bilgi

n-gram: lineer interpolasyon (ML trigram/bigram + unigram(+1) + düzgün), karışım ağırlıkları doğrulamada EM (V8 yöntemi; bağlam bulunmayan bileşen ağırlığı yeniden normalleştirilir). EM ağırlıkları: bigram a2=0,530 a1=0,465 a0=0,005; trigram a3=0,046 a2=0,483 a1=0,466 a0=0,005 → **trigram bileşeni karışımda yalnız %4,6**.

| model | eğitim (sayım kendini içerir) PPL | eğitim LOO PPL | doğrulama PPL | test PPL (eğitim sayımları) | test PPL (eğitim+doğr. sayımları) | test Top-1 | test Top-5 |
|---|---|---|---|---|---|---|---|
| unigram | 347.11 | 363.05 | 297.34 | 282.91 | 281.12 | 9.0% | 22.6% |
| bigram | 45.78 | 175.70 | 150.49 | 144.86 | 141.37 | 15.5% | 34.5% |
| trigram | 22.30 | 168.31 | 145.55 | 138.87 | 135.41 | 16.0% | 35.8% |
| V17 Ridge (λ=10) | 166.9 | – | 225.4 | 208.53 | – | 15.7% | 30.0% |
| V20 CE düzeltmesi (η val ile) | 99.8 | – | 214.1 | 203.39 | – | 14.7% | 30.4% |
| V23 hedef-koşullu (CE=ln100, b=8) | 223.0 | – | 228.1 | 211.07 | – | 15.5% | 30.1% |

Not: trigram 137 referansı **yeniden üretildi** (135,4 V8 protokolü; 138,9 yalnız eğitim sayımlarıyla; V8 raporu 137,4). Küçük fark: burada yalnız bloklarda t ≥ c konumları ve bağlam bulunmayan bileşenin yeniden normalleştirilmesi kullanıldı. Eğitim PPL'i sayım kendini içerdiğinde 22,3 (trigram) / 45,8 (bigram): n-gram eğitim başarısı ezberdir; LOO ile 168 / 176.

Kapsama (eğitim sayımlarıyla):

| küme | bigram bağlamı görüldü | trigram bağlamı görüldü | bigram (bağlam,hedef) görüldü | trigram (bağlam,hedef) görüldü |
|---|---|---|---|---|
| doğrulama | %100 | %66,7 | %66,6 | %22,3 |
| test | %100 | %68,1 | %68,0 | %23,6 |

Kelime sıklığına göre test (tohum 1, H=1024):

| model | sık CE / Top-1 | orta CE / Top-1 | nadir CE / Top-1 |
|---|---|---|---|
| unigram | 4.1335 / 16.7% | 6.6267 / 0.0% | 8.6783 / 0.0% |
| bigram | 3.5442 / 26.0% | 5.6125 / 4.9% | 8.3081 / 0.2% |
| trigram | 3.5190 / 26.5% | 5.5470 / 5.7% | 8.2508 / 0.3% |
| V17 Ridge | 3.6300 / 26.4% | 6.4528 / 5.1% | 8.7679 / 0.0% |
| V20 | 3.6800 / 23.3% | 6.3409 / 6.8% | 8.6528 / 1.0% |
| V23 | 3.6415 / 26.1% | 6.4901 / 5.1% | 8.7424 / 0.0% |
| B (Q=trigram→W) | 3.7544 / 25.2% | 6.5712 / 2.9% | 8.5615 / 0.0% |

Bağlam bilgisi arttığında (test, trigram bağlamı görülen 3 392 satır vs yalnız bigram görülen 1 588 satır) CE:

| model | trigram bağlamı GÖRÜLDÜ CE / Top-1 | yalnız bigram görüldü CE / Top-1 |
|---|---|---|
| unigram | 5.7183 / 9.8% | 5.4888 / 7.4% |
| bigram | 4.9875 / 15.4% | 4.9508 / 15.7% |
| trigram | 4.9280 / 16.1% | 4.9454 / 15.7% |
| V17 Ridge | 5.3829 / 16.2% | 5.2485 / 14.7% |
| V20 | 5.3497 / 15.3% | 5.2413 / 13.5% |
| V23 | 5.3997 / 15.9% | 5.2507 / 14.8% |
| B (Q=trigram→W) | 5.4718 / 14.4% | 5.2581 / 14.5% |

- [Kesin] Trigramın bigram'a kazancı yalnız ≈ %4 PPL (144,9 → 138,9); trigram bağlamı görülen satırlarda trigram CE 4,928, bigram 4,988. Ridge/V20/V23 trigram bağlamı **görülen** satırlarda yalnız-bigram satırlarına göre **daha kötü** (V17: 5,383 vs 5,249) — bu modeller görülen trigram bağlamından yararlanmıyor (n-gram'ın tersine).
- [Kesin] Ridge'in n-gram'a göre kaybı orta kelimelerde (CE 6,45 vs bigram 5,61, Top-1 %5,2 vs %4,9 ama Top-5 %6,7 vs %14,1) ve Top-5'te (%30,0 vs %35,8); sık kelimelerde fark küçük (3,63 vs 3,52).

## 2. Deney B — Q (trigram) → F → W bilgi aktarımı (H=1024, tohum 1)

Q = EM ağırlıklı trigram olasılıkları (eğitim sayımları). "tam": eğitim satırları için sayım kendini içerir; "LOO": satırın kendisi çıkarılmış. Göreli hata = ‖FW−Q‖² / ‖Q−π‖² (π eğitim unigram; 0 = mükemmel, 1 = unigram kadar kötü). Q'nun kendi test CE'si 4,934.

| W | λ | göreli hata eğitim / doğr. / test | KL(Q‖öğrenci) eğitim/doğr./test | gerçek etikette CE (PPL): eğitim | doğrulama | **test** | test Top-1 / Top-5 | ‖W‖ | ‖WA−WB‖/‖WA‖ |
|---|---|---|---|---|---|---|---|---|---|
| A (etiket) | 0.3 | 1.8694/2.2465/2.2397 (Q'ya karşı) | 0.7732/0.7371/0.7285 | 5.1108 (165.8) | 5.4182 (225.5) | **5.3398 (208.48)** | 15.7% / 30.0% | 1.9145 | – |
| B (Q tam) | 0.3 | 0.5667/0.5853/0.5789 | 0.7473/0.7029/0.6929 | 5.4714 (237.8) | 5.4702 (237.5) | **5.4026 (221.99)** | 14.5% / 28.2% | 0.5774 | 0.8242 |
| B (Q LOO) | 0.3 | 0.6253/0.5882/0.5814 | 0.7385/0.7037/0.6933 | 5.5058 (246.1) | 5.4758 (238.8) | **5.4101 (223.65)** | 14.4% / 27.9% | 0.5695 | 0.8658 |
| A (etiket) | 10 | 1.8215/2.1826/2.1767 (Q'ya karşı) | 0.7730/0.7366/0.7279 | 5.1176 (166.9) | 5.4179 (225.4) | **5.3401 (208.53)** | 15.7% / 30.0% | 1.8355 | – |
| B (Q tam) | 10 | 0.5668/0.5841/0.5777 | 0.7480/0.7034/0.6933 | 5.4740 (238.4) | 5.4712 (237.7) | **5.4037 (222.22)** | 14.5% / 28.1% | 0.5533 | 0.8243 |
| B (Q LOO) | 10 | 0.6254/0.5870/0.5801 | 0.7393/0.7042/0.6937 | 5.5077 (246.6) | 5.4768 (239.1) | **5.4111 (223.87)** | 14.4% / 27.8% | 0.5464 | 0.8655 |
| A (etiket) | 30 | 1.7384/2.0724/2.0681 (Q'ya karşı) | 0.7728/0.7359/0.7269 | 5.1308 (169.2) | 5.4176 (225.3) | **5.3406 (208.64)** | 15.6% / 29.9% | 1.7148 | – |
| B (Q tam) | 30 | 0.5674/0.5825/0.5759 | 0.7495/0.7045/0.6942 | 5.4789 (239.6) | 5.4732 (238.2) | **5.4056 (222.65)** | 14.5% / 28.0% | 0.5204 | 0.8222 |
| B (Q LOO) | 30 | 0.6260/0.5853/0.5784 | 0.7408/0.7053/0.6947 | 5.5115 (247.5) | 5.4787 (239.5) | **5.4129 (224.27)** | 14.4% / 27.8% | 0.5140 | 0.8627 |
| A (etiket) | 300 | 1.2790/1.4706/1.4715 (Q'ya karşı) | 0.7740/0.7340/0.7237 | 5.2398 (188.6) | 5.4224 (226.4) | **5.3497 (210.54)** | 15.7% / 29.7% | 1.0533 | – |
| B (Q tam) | 300 | 0.5846/0.5867/0.5796 | 0.7627/0.7149/0.7034 | 5.5206 (249.8) | 5.4915 (242.6) | **5.4231 (226.59)** | 14.3% / 27.5% | 0.3470 | 0.7934 |
| B (Q LOO) | 300 | 0.6410/0.5896/0.5822 | 0.7544/0.7157/0.7040 | 5.5447 (255.9) | 5.4967 (243.9) | **5.4295 (228.04)** | 14.2% / 27.4% | 0.3408 | 0.8298 |

Okuma:
- [Kesin] B, Q'yu **temsil edemiyor** (göreli hata 0,57–0,63, yani varyansın ≈ %40'ı açıklanıyor) ve λ'dan neredeyse bağımsız: λ=0,3→300 arasında eğitim hatası 0,567→0,585. En esnek ridge ile bile ≈ 0,567 → **temsil sınırı** (F'nin kapsadığı uzay + karesel kayıp).
- [Kesin] Eğitim–test göreli hata farkı +0,011 → B için genelleme açığı yok; B'nin eğitim–doğrulama CE farkı negatif (−0,003…−0,05): öğretici ezberi aktarılamıyor, ağırlıklar fazla "yumuşak" kalıyor.
- [Kesin] B gerçek etikette A'dan kötü (222,2 vs 208,5): olasılık-uzayı regresyonu, kalibre softmax-CE ile uyuşan skor ayrımı üretmiyor. Fark kalibrasyondan (α=0,25,T=0,01) değil W'dan: ‖WB‖ ≈ 0,55 vs ‖WA‖ ≈ 1,84; ‖WA−WB‖/‖WA‖ ≈ 0,82.
- [Kesin] "LOO" Q ile eğitim CE (5,508) "tam" Q'dan (5,474) kötü ve test da kötü (223,9 vs 222,2): öğretici ezberini çıkarmak aktarımı iyileştirmedi.
- A'nın "Q'ya karşı" hatası (1,8) anlamlı bir uydurma ölçüsü değil, yalnız A'nın Q'dan ne kadar uzak olduğunu gösterir.

**F temsilinden mi, Ridge çözümünden mü?** [Kesin] Ridge çözümü (λ etkisi) toplam Q-uydurma hatasının ≈ %3'ü (0,567→0,585), genelleme ≈ %2'si (+0,011); kalanı (≈ %95) F'nin temsil sınırıdır. Kapasite artışı bunu çok yavaş azaltıyor (Deney C). [Muhtemel] Sebep: girdi X, 64-boyutlu PPMI-SVD gömmelerinden (kelime çifti kimliği kaybolur) + rastgele tanh özellikleri; trigram tablosu 2001² kelime çiftine duyarlı, rastgele özellikler bunu ayıramıyor. Bu girdi darboğazını ayrı ölçmedim (X-üzerinde doğrusal ridge karşılaştırması yapılmadı).

## 3. Deney C — özellik kapasitesi (tohum 1)

| H | model | eğitim CE (PPL) | doğrulama CE | test CE (PPL) | test Top-1 / Top-5 | eğitim–doğr. fark | koşu süresi / RAM (tüm modeller birlikte) |
|---|---|---|---|---|---|---|---|
| 512 | V17 Ridge (A) | 5.3565 (212.0) | 5.4459 | 5.3739 (215.69) | 14.9% / 29.3% | 0.0893 | 77.82 s / 644 MB |
| 512 | V20 | 5.0892 (162.3) | 5.4027 | 5.3236 (205.12) | 15.3% / 30.7% | 0.3134 | 77.82 s / 644 MB |
| 512 | B (Q=trigram, λ=V17) | 5.5400 (254.7) | 5.4991 | 5.4332 (228.87) | 14.3% / 27.2% | -0.0410 | 77.82 s / 644 MB |
| 1024 | V17 Ridge (A) | 5.1176 (166.9) | 5.4179 | 5.3401 (208.53) | 15.7% / 30.0% | 0.3003 | 128.81 s / 791 MB |
| 1024 | V20 | 4.6035 (99.8) | 5.3664 | 5.3151 (203.39) | 14.7% / 30.4% | 0.7629 | 128.81 s / 791 MB |
| 1024 | B (Q=trigram, λ=V17) | 5.4740 (238.4) | 5.4712 | 5.4037 (222.22) | 14.5% / 28.1% | -0.0028 | 128.81 s / 791 MB |

Q-uydurma (B, "tam" Q): H=512 göreli hata eğitim/doğr./test λ=0,3: 0.6037/0.5996/0.5950; λ=1: 0.6070/0.6009/0.5956. H=1024 λ=0,3: 0.5667/0.5853/0.5789; λ=10: 0.5668/0.5841/0.5777. KL: H=512 0.7699/0.7186/0.7080 → H=1024 0.7473/0.7029/0.6929.

- [Kesin] H 512→1024: etiket-A eğitim CE 5,357→5,118 (−0,24), doğrulama 5,446→5,418 (−0,03), test 5,374→5,340 (−0,03); eğitim–doğrulama farkı 0,09→0,30. **Eğitim büyük iyileşiyor, test yok → ezberleme/genelleme.**
- [Kesin] Öğretici aktarımı: Q-uydurma göreli hata 0,604→0,567 (H=512→1024, λ=0,3): kapasite iki katına çıkınca açıklanan varyans yalnız %39,6→%43,3. KL 0,77→0,75. Yani H büyürken öğretici bilgisi **yavaş** aktarılıyor: temsil kapasitesi sınırlayıcı, fakat H artışı yetersiz (log-benzeri). [Tahmin] %60 hatayı ≈ 0,2'ye indirmek için H çok daha büyük olmalı; ölçülmedi.
- Süre/RAM: H=512 78 s, 644 MB; H=1024 129 s, 791 MB (bu koşu n-gram + dağıtım değerlendirmeleri dahil; Ridge+gram tek başına 1,7 s / 0,3 s).

## 4. Deney D — hedef ↔ ağırlık ilişkisi (V23 Q*, b=8 LOO öğretici, λ=V17)

| hedef PPL | CE hedefi | κ (öğretim karışımı) | öğretici eğitim CE (hedefe eşit) | öğrenci eğitim PPL | test PPL | test Top-1 |
|---|---|---|---|---|---|---|
| 200 | 5.2983 | 0.000010 | 5.2983 | 223.1 | 211.08 | 15.54% |
| 150 | 5.0106 | 0.000080 | 5.0106 | 223.1 | 211.08 | 15.54% |
| 100 | 4.6052 | 0.000515 | 4.6052 | 223.0 | 211.07 | 15.54% |

| karşılaştırma | ‖ΔQ*‖/‖Q*‖ | ‖ΔW‖/‖W‖ | ‖FΔW‖/‖FW‖ |
|---|---|---|---|
| 200 vs 150 (H=1024) | 0.00027 | 0.00022 | 0.00028 |
| 200 vs 100 (H=1024) | 0.00191 | 0.00083 | 0.00085 |
| 150 vs 100 (H=1024) | 0.00164 | 0.00073 | 0.00073 |
| 200 vs 100 (H=512) | 0.00191 | 0.00069 | 0.00083 |

- [Kesin] Hedef 200→100: öğretim bilgisi %0,19, ağırlık %0,08 değişti; test PPL 211,08→211,07. Ağırlığa aktarım zincirinde ΔW/W ≈ 0,4–0,8×ΔQ/Q: Q değişiminin yalnız bir kısmı F uzayında.
- [Kesin] Kayıp halka **hedef→öğretim sinyali**: κ yalnız 1e-5…5e-4. CE ortalaması ağır kuyrukludur; κ-karışımı satır kaybını −ln κ ile tavanlayarak ortalamayı hedefe indirir, hangi kelimenin öğretileceğini değiştirmez. Ağırlık aktarım katsayısı (ΔW/ΔQ) ~0,4–0,8 olduğundan tek başına darboğaz değil.
- Not: `‖FΔW‖/‖ΔQ‖` sütunu loglarda var ama değerler (0,92–2,45) teorik üst sınır 1'i aşıyor (float kuadratik form kesinlik hatası olası); **güvenilmez, rapora konmadı** [Kesin: değer geçersiz].

## 5. Tohum tutarlılığı (H=1024, test PPL / Top-1)

| tohum | bigram | trigram | V17 Ridge | V20 | V23 (CE=ln100) | B (Q→W) |
|---|---|---|---|---|---|---|
| 1 | 144.9 | 138.9 | 208.53 / 15.7% | 203.39 / 14.7% | 211.07 / 15.5% | 222.22 / 14.5% |
| 2 | 144.9 | 138.9 | 209.73 / 15.6% | 201.17 / 15.1% | 212.47 / 15.6% | 223.66 / 14.6% |
| 3 | 144.9 | 138.9 | 208.55 / 15.9% | 202.39 / 15.3% | 211.59 / 15.8% | 220.66 / 14.9% |

## 6. Sınırlamalar ve dürüstlük notları

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

