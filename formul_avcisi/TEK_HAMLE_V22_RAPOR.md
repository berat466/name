# TEK HAMLE V22 — Hedef hata oranından doğrudan ağırlık hesaplama

Dal `claude/formul-avcisi` · kod `cpp/tek_hamle_v22.cpp` (V21 kopyası; önceki dosyalara dokunulmadı) · ham loglar/komutlar `v22_ham/`. H=1024, c=2, aynı metin/bölme, tohum 1/2/3 (γ,λ = V17: (4,10),(4,100),(2,1)).

**Yöntem.** Ridge `W0` (Cholesky, eğitim), tek CE gradyanı → `D=A⁻¹FᵀG` (α0=0,2, T0=0,01 sabit). Eğitim CE'sinin doğrultu boyunca ikinci derece modeli `CE(W0−ηD) ≈ L0 + aη + ½bη²`: `L0` ve `a=−⟨D,FᵀG⟩/n` eğitimden **analitik/doğrudan**, `b` eğitim CE'sinin h=0,001 merkezi sonlu farkından (±h için 2 ek eğitim CE değerlendirmesi, ağırlık güncellemesi değil). Hedef denklemi çözülür, `W_hedef = W0 − η_hesaplanan·D` bir kez üretilir. **Bu raporda tüm modeller yalnız eğitimden üretildi** (V20/V21'deki eğitim+doğrulama son modeli kullanılmadı) ki doğrulama kontrol, test tamamen bağımsız kalsın; bu yüzden Ridge test PPL'i burada ≈208,5 (V17: 206,8 eğitim+doğrulama modeliyle). Sabit α0,T0 CE'si (kalibrasyon taraması yok) denklemin tanımlandığı niceliktir; ek olarak doğrulama-kalibre (α,T) test PPL'i de verilir. Test hiçbir η/parametre seçimine girmedi.

## 0. Özet (3 tohum ort., test)

Görev denklemi (eğitim CE'si) için sonuç:

| hedef PPL (CE) | η_hesaplanan (s1/s2/s3) | tahmin edilen eğitim CE | gerçek eğitim PPL | doğrulama PPL | **test PPL** | test Top-1 | doğr.-kalibre test PPL |
|---|---|---|---|---|---|---|---|
| 190 (5,247) | 0.00000/0.00000/0.00000 | 5.1367 | 170.2 | 225.2 | **208.9** | 15.7% | 208.9 |
| 175 (5,165) | 0.00000/0.00008/0.00000 | 5.1347 | 169.8 | 225.1 | **208.8** | 15.7% | 208.8 |
| 150 (5,011) | 0.00122/0.00204/0.00134 | 5.0106 | 150.7 | 220.2 | **204.9** | 15.8% | 204.7 |
| 137 (4,920) | 0.00222/0.00313/0.00237 | 4.9200 | 140.2 | 217.8 | **203.3** | 15.6% | 203.1 |
| 100 (4,605) | 0.00545/0.00657/0.00561 | 4.6052 | 118.9 | 214.6 | **202.4** | 15.2% | 202.0 |

Ridge (η=0, tüm veri eğitim-yalnız, sabit α0,T0): eğitim PPL 170.2, doğrulama PPL 225.2, **test PPL 208.9**.

Bulgular:

- [Kesin] Denklem **eğitim** CE'sine yazıldığı için hedefe eğitimde yaklaşılıyor, test PPL'i hedeften bağımsız olarak ≈ 203–208'de kalıyor: hiçbir hedef test'te gerçekleşmedi. Hedef 100 için denklem eğitim PPL'ini yalnız 116–122'ye indirdi (model 100 öngördü), test PPL 201,7–202,9. Modelden bağımsız gerçek-eğri kökünde (bölüm 3) eğitim PPL 100'e inebiliyor (tohum 1 ve 3), ama test PPL 205–206'ya çıkıyor ve Top-1 %14'e düşüyor; tohum 2'de bu doğrultuda eğitim PPL'i 103,9'un altına inemiyor (hedef 100 ulaşılamaz).
- [Kesin] 190, 175 hedefleri eğitim verisinde Ridge ile zaten aşılmış (eğitim PPL ≈ 167–176; 175 hedefi tohum 2'de η≈0,00008 ile, diğerlerinde η=0 ile): denklem `L0 ≤ hedef` verir, η=0. Bu, eğitim CE'sinin genelleme için iyimser olduğunu gösteriyor (Ridge eğitim–test CE farkı ≈ 0,22, tohum 1).
- [Kesin] Yerel ikinci derece model **geçersiz**: eğitim eğriliği negatif (b≈−3000; parabol aşağı açık, minimumu yok), gerçek eğri ise η≈0,02'de dönüp yükseliyor. Model, uzak köklerde CE'yi sistematik olarak fazla iyimser tahmin ediyor (hedef 100 için tahmin–gerçek −0,15…−0,20 nat, 3 tohum). Doğrulama eğriliği tohuma ve h'ye göre işaret/büyüklük değiştiriyor (ör. −142…+411): parabol değil.
- [Kesin] Sayısal çözüm sorun değil: `‖AD−FᵀG‖/‖FᵀG‖ ≈ 3e-7`; analitik `a` ile sonlu fark `a` h→0'da örtüşüyor (−88,3 vs −87,6, h=0,0005). Engel sayısal değil.
- [Kesin] Tek doğrultuda doğrulama/test CE'sinin alt sınırı belli: tohum 1'de sabit α0,T0 ile doğrulama CE minimumu ≈ 5,367 (PPL 214,2; η≈0,006), test CE minimumu ≈ 5,3125 (PPL ≈ 203,1; η≈0,004–0,005); ölçüm yalnız tohum 1 için tablolandı (bölüm 5), diğer tohumlarda sonuçlar aynı yönde (bölüm 2). Hiçbir η test PPL'ini 190'a, 137'ye, 100'e götüremez; bunun için yeni bilgi/yön gerekir (V20'nin kalibrasyonlu en iyi sonucu ≈ 199–200 PPL, aynı sınıf).
- [Muhtemel] Engelin ana kaynağı **aşırı uyum / genelleme açığı**: eğitim PPL'i tohum 1'de 94'e kadar iniyor (η=0,02), doğrulama/test aynı anda yükseliyor. Özellik kapasitesi eğitim verisini ezberlemeye yetiyor (H=1024 rastgele özellik, 39 840 örnek, 2001 sınıf); eksik olan, **yeni bağlam bilgisi** (trigram 137 PPL sayımlardan geliyor).
- [Kesin] Aynı `D` yönünü kullanan tüm hedefler yalnız farklı bir η seçimidir; yeni öğrenme bilgisi üretmezler. Hedef değeri bir "öğrenme girdisi" değil, tek boyutlu bir ayar noktasıdır.

**Ana soruya cevap:** İstenen PPL'i doğrudan denkleme koymak, **eğitim CE'si** için kısmen çalışır (η hesaplanır, eğitim PPL'i hedefe yaklaşır; hata modelin eğrilik hatasından gelir), fakat **test PPL'ini** hedefe götürmez. Engel hesaplama değil: (1) hedef eğitim kaybında tanımlı, test kaybında değil; (2) tek doğrultu `−D` boyunca test CE'sinin minimumu ≈ 5,31 (PPL ≈ 203) ve bu doğrultuda aşırı uyum başlar; (3) ikinci derece model eğrilik değiştiği için uzak köklerde yanlış.

## 1. Katsayılar ve sayısal doğrulama

| tohum | L0 eğitim / doğr. / test (PPL) | a (analitik) | a (FD h=0,0005 / 0,001 / 0,0025) | b eğitim (h=0,0005 / 0,001 / 0,0025) | b doğrulama | çözüm artığı | Ridge s | CE düzeltme s |
|---|---|---|---|---|---|---|---|---|
| 1 | 166.94 / 225.40 / 208.53 | -88.2781 | -87.6 / -85.9 / -77.3 | -3034 / -3004 / -2993 | 158 / 257 / 386 | 3.3e-07 | 1.681 | 5.779 |
| 2 | 176.06 / 225.95 / 209.73 | -76.8339 | -76.3 / -74.9 / -67.9 | -3553 / -3427 / -3010 | 401 / 411 / 227 | 2.1e-07 | 1.739 | 6.010 |
| 3 | 167.60 / 224.16 / 208.55 | -81.9751 | -81.5 / -80.1 / -72.4 | -4405 / -4302 / -3560 | -142 / -55 / 256 | 6.6e-07 | 1.239 | 5.794 |

## 2. Deney A/B — görev denklemi (eğitim CE'si) — her tohum ayrı

Sütunlar: η_hesaplanan; ikinci derecenin öngördüğü eğitim CE; gerçek eğitim CE/PPL; (tahmin−gerçek); doğrulama PPL; test CE/PPL/Top-1 (sabit α0,T0); doğrulama-kalibre (α,T) test PPL.

| tohum | hedef | η | tahmin CE | gerçek eğitim CE (PPL) | tahmin−gerçek | doğr. PPL | test CE | **test PPL** | test Top-1 | kalibre (α,T) → test PPL / Top-1 |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | 190 | 0.00000 | 5.11761 | 5.11761 (166.94) | +0.0000 | 225.40 | 5.34006 | **208.53** | 15.7% | α=0.2,T=0.01 → 208.53 / 15.7% |
| 1 | 175 | 0.00000 | 5.11761 | 5.11761 (166.94) | +0.0000 | 225.40 | 5.34006 | **208.53** | 15.7% | α=0.2,T=0.01 → 208.53 / 15.7% |
| 1 | 150 | 0.00122 | 5.01064 | 5.01191 (150.19) | -0.0013 | 220.77 | 5.32386 | **205.17** | 15.8% | α=0.2,T=0.01 → 205.17 / 15.8% |
| 1 | 137 | 0.00222 | 4.91998 | 4.93507 (139.08) | -0.0151 | 218.03 | 5.31701 | **203.77** | 15.6% | α=0.2,T=0.01 → 203.77 / 15.6% |
| 1 | 100 | 0.00545 | 4.60517 | 4.75471 (116.13) | -0.1495 | 214.32 | 5.31293 | **202.94** | 15.2% | α=0.25,T=0.01 → 202.50 / 15.0% |
| 2 | 190 | 0.00000 | 5.17082 | 5.17082 (176.06) | +0.0000 | 225.95 | 5.34583 | **209.73** | 15.6% | α=0.2,T=0.01 → 209.73 / 15.6% |
| 2 | 175 | 0.00008 | 5.16479 | 5.16463 (174.97) | +0.0001 | 225.66 | 5.34456 | **209.47** | 15.6% | α=0.2,T=0.01 → 209.47 / 15.6% |
| 2 | 150 | 0.00204 | 5.01064 | 5.02080 (151.53) | -0.0102 | 219.80 | 5.32068 | **204.52** | 15.8% | α=0.25,T=0.01 → 204.07 / 15.8% |
| 2 | 137 | 0.00313 | 4.91998 | 4.95494 (141.87) | -0.0350 | 217.59 | 5.31331 | **203.02** | 15.6% | α=0.25,T=0.01 → 202.36 / 15.7% |
| 2 | 100 | 0.00657 | 4.60517 | 4.80620 (122.27) | -0.2010 | 214.56 | 5.30695 | **201.73** | 15.2% | α=0.25,T=0.01 → 200.96 / 15.1% |
| 3 | 190 | 0.00000 | 5.12160 | 5.12160 (167.60) | +0.0000 | 224.16 | 5.34020 | **208.55** | 15.9% | α=0.2,T=0.01 → 208.55 / 15.9% |
| 3 | 175 | 0.00000 | 5.12160 | 5.12160 (167.60) | +0.0000 | 224.16 | 5.34020 | **208.55** | 15.9% | α=0.2,T=0.01 → 208.55 / 15.9% |
| 3 | 150 | 0.00134 | 5.01064 | 5.01262 (150.30) | -0.0020 | 219.94 | 5.32249 | **204.89** | 15.9% | α=0.2,T=0.01 → 204.89 / 15.9% |
| 3 | 137 | 0.00237 | 4.91998 | 4.93826 (139.53) | -0.0183 | 217.63 | 5.31417 | **203.20** | 15.7% | α=0.2,T=0.01 → 203.20 / 15.7% |
| 3 | 100 | 0.00561 | 4.60517 | 4.77204 (118.16) | -0.1669 | 214.86 | 5.31095 | **202.54** | 15.3% | α=0.2,T=0.01 → 202.54 / 15.3% |

## 3. Modelden bağımsız kontrol: gerçek eğitim eğrisinin hedefi kestiği η (yalnız değerlendirme)

İkinci derece modelin hatasını ayırmak için aynı tek doğrultu boyunca gerçek eğitim CE eğrisi 17 η noktasında hesaplandı; hedefi kestiği η doğrusal enterpolasyonla bulundu ve orada gerçek değerler ölçüldü (yeni ağırlık = yine `W0−ηD`, tek ağırlık hesabı; η için yalnız ileri değerlendirme kullanıldı, gradyan adımı yok).

| tohum | hedef | eğri minimumu (eğitim PPL, η) | kesen η | eğitim PPL | doğr. PPL | test PPL | test Top-1 |
|---|---|---|---|---|---|---|---|
| 1 | 190 | 94.24 | η=0 (zaten sağlandı) | – | – | – | – |
| 1 | 175 | 94.24 | η=0 (zaten sağlandı) | – | – | – | – |
| 1 | 150 | 94.24 | 0.00124 | 149.95 | 220.71 | 205.14 | 15.8% |
| 1 | 137 | 94.24 | 0.00245 | 136.82 | 217.53 | 203.56 | 15.5% |
| 1 | 100 | 94.24 | 0.01072 | 99.80 | 215.27 | 205.10 | 14.4% |
| 2 | 190 | 103.93 | η=0 (zaten sağlandı) | – | – | – | – |
| 2 | 175 | 103.93 | 0.00008 | 175.00 | 225.67 | 209.47 | 15.6% |
| 2 | 150 | 103.93 | 0.00221 | 149.89 | 219.42 | 204.24 | 15.8% |
| 2 | 137 | 103.93 | 0.00380 | 136.91 | 216.55 | 202.41 | 15.6% |
| 2 | 100 | 103.93 | ULAŞILAMAZ (eğri minimumu hedefin üstünde) | – | – | – | – |
| 3 | 190 | 99.07 | η=0 (zaten sağlandı) | – | – | – | – |
| 3 | 175 | 99.07 | η=0 (zaten sağlandı) | – | – | – | – |
| 3 | 150 | 99.07 | 0.00137 | 149.97 | 219.86 | 204.83 | 15.9% |
| 3 | 137 | 99.07 | 0.00267 | 136.83 | 217.12 | 202.90 | 15.7% |
| 3 | 100 | 99.07 | 0.01420 | 99.85 | 219.10 | 206.19 | 14.1% |

## 4. Doğrulama denklemi (tanı; doğrulama a,b,L0 için kullanıldı; test bağımsız)

Aynı denklem doğrulama CE'sine yazıldı: hedeflere "genelleme" açısından bakmak için. Doğrulama burada kontrol/tanı amaçlıdır, test hiçbir kararda yok.

| tohum | hedef | durum | η | öngörülen min CE (PPL) | gerçek doğr. PPL | tahmin−gerçek | test PPL | test Top-1 |
|---|---|---|---|---|---|---|---|---|
| 1 | 190 | kok var (en kucuk pozitif) | 0.01055 | 4.81868 (PPL 123.80) | 215.20 | -0.125 | 205.03 | 14.4% |
| 1 | 175 | kok var (en kucuk pozitif) | 0.01639 | 4.81868 (PPL 123.80) | 219.25 | -0.225 | 208.67 | 13.4% |
| 1 | 150 | kok var (en kucuk pozitif) | 0.02965 | 4.81868 (PPL 123.80) | 233.59 | -0.443 | 221.85 | 10.9% |
| 1 | 137 | kok var (en kucuk pozitif) | 0.04022 | 4.81868 (PPL 123.80) | 247.69 | -0.592 | 232.35 | 9.3% |
| 1 | 100 | ULASILAMAZ (diskriminant<0) | 0.06831 | 4.81868 (PPL 123.80) | 271.03 | -0.784 | 258.19 | 6.9% |
| 2 | 190 | kok var (en kucuk pozitif) | 0.01410 | 5.13984 (PPL 170.69) | 217.65 | -0.136 | 205.23 | 14.4% |
| 2 | 175 | kok var (en kucuk pozitif) | 0.02592 | 5.13984 (PPL 170.69) | 228.55 | -0.267 | 216.91 | 12.1% |
| 2 | 150 | ULASILAMAZ (diskriminant<0) | 0.03694 | 5.13984 (PPL 170.69) | 240.27 | -0.342 | 227.97 | 10.0% |
| 2 | 137 | ULASILAMAZ (diskriminant<0) | 0.03694 | 5.13984 (PPL 170.69) | 240.27 | -0.342 | 227.97 | 10.0% |
| 2 | 100 | ULASILAMAZ (diskriminant<0) | 0.03694 | 5.13984 (PPL 170.69) | 240.27 | -0.342 | 227.97 | 10.0% |
| 3 | 190 | kok var (yalniz buyuk kok pozitif) | 0.01111 | nan (PPL nan) | 216.97 | -0.133 | 204.76 | 14.4% |
| 3 | 175 | kok var (yalniz buyuk kok pozitif) | 0.01647 | nan (PPL nan) | 221.26 | -0.235 | 207.78 | 13.7% |
| 3 | 150 | kok var (yalniz buyuk kok pozitif) | 0.02626 | nan (PPL nan) | 231.72 | -0.435 | 218.16 | 11.6% |
| 3 | 137 | kok var (yalniz buyuk kok pozitif) | 0.03186 | nan (PPL nan) | 237.53 | -0.550 | 224.11 | 10.9% |
| 3 | 100 | kok var (yalniz buyuk kok pozitif) | 0.05056 | nan (PPL nan) | 255.66 | -0.939 | 241.75 | 8.6% |

## 5. Gerçek CE eğrisi (tohum 1; sabit α0,T0; ikinci derece tahmin)

| η | eğitim CE | doğrulama CE | test CE | ikinci derece tahmin (eğitim) |
|---|---|---|---|---|
| 0 | 5.11761 | 5.41787 | 5.34006 | 5.11761 |
| 0.0005 | 5.07341 | 5.40881 | 5.33229 | 5.07429 |
| 0.001 | 5.03023 | 5.40045 | 5.32606 | 5.03023 |
| 0.0015 | 4.98921 | 5.39318 | 5.32147 | 4.98541 |
| 0.002 | 4.95079 | 5.38700 | 5.31814 | 4.93984 |
| 0.003 | 4.88199 | 5.37756 | 5.31409 | 4.84645 |
| 0.004 | 4.82358 | 5.37150 | 5.31254 | 4.75006 |
| 0.005 | 4.77430 | 5.36821 | 5.31256 | 4.65066 |
| 0.006 | 4.73236 | 5.36698 | 5.31363 | 4.54826 |
| 0.0075 | 4.68026 | 5.36723 | 5.31656 | 4.38903 |
| 0.01 | 4.61684 | 5.37056 | 5.32193 | 4.10861 |
| 0.0125 | 4.57608 | 5.37581 | 5.32791 | 3.80942 |
| 0.015 | 4.55322 | 5.38416 | 5.33575 | 3.49145 |
| 0.02 | 4.54585 | 5.40543 | 5.35526 | 2.79918 |
| 0.025 | 4.57152 | 5.42878 | 5.37883 | 2.03180 |
| 0.03 | 4.61671 | 5.45558 | 5.40375 | 1.18932 |
| 0.04 | 4.72661 | 5.51118 | 5.44723 | -0.72098 |

## 6. Karşılaştırma (H=1024, test PPL)

| yöntem | PPL | not |
|---|---|---|
| V17 Ridge (eğitim+doğrulama modeli, kalibre) | 206,8 | referans |
| V20 CE düzeltmesi (A / B) | 198,9 / 199,8 | α,T doğrulamayla seçildi |
| V21 | ≈198,9–199,3 | maske, Top-1 dengeleri |
| V22, hedef 100 (eğitim denklemi), sabit α0,T0 | 202.4 | hedefe test'te ulaşılmadı; eğitim-yalnız model (Ridge burada 208.9) |
| trigram (ölçülmüş referans) | 137 | sayım tabanlı, bu yöntem değil |

## 7. Sınırlar ve ne değişmeli

- Tek metin, tek bölme; test n=4 980. Tohumlar arası farklar küçük ama sonuçlar yalnız bu veri için geçerli.
- Denklem tek boyutlu: yalnız `−D` doğrultusu araştırıldı; tüm hedefler aynı yön, yalnız farklı η. Yeni öğrenme bilgisi içermez. [Kesin]
- b, yerel sonlu farktan; hedefe uzak köklerde model güvenilmez (negatif eğrilik → minimum yok). Daha iyi modelleme (3. derece / gerçek 1-B eğriyi doğrudan çözme) hedefi **eğitimde** daha iyi tutturur ama test sorununu çözmez.
- Hedef eğitim CE'sinde tanımlı → genelleme açığını (eğitim–test CE farkı Ridge'de ≈0,22, η arttıkça ≈0,7'ye çıkar) hesaba katmıyor. [Kesin]
- [Tahmin] Değişmesi gerekenler: (1) hedef denklemi **dışarıda-bırakılmış** (leave-one-out / çapraz uydurma) CE üzerinde yazmak — Ridge için LOO artığı kapalı formda `e_i/(1−h_ii)` ve mevcut Cholesky ile hesaplanabilir; bu denenmedi; (2) tek doğrultu yerine hedefe göre **yeni bilgi** ekleyen özellikler (kelime kimliği / n-gram sayım özellikleri; V10–V11'de kısmen denendi); (3) test için gerçekçi hedef: bu aile ≈199 PPL civarında doyuyor.
- Süre: tohum başı ≈ 50 s, RAM ≈ 1,56 GB (eğitim/doğr./test logitleri bellekte).

| komut | wall s | user CPU s | RAM MB |
|---|---|---|---|
| `./tek_hamle_v22 2 1024 1 4 10` | 63.96 | 142.96 | 1560 |
| `./tek_hamle_v22 2 1024 2 4 100` | 66.71 | 146.78 | 1560 |
| `./tek_hamle_v22 2 1024 3 2 1` | 66.71 | 143.04 | 1560 |
