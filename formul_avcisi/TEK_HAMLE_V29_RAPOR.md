# TEK HAMLE V29 — Kelime kimliği + matematiksel değerler birlikte, 4 bağlantılı F

Dal `claude/formul-avcisi` · kod `cpp/tek_hamle_v29.cpp` (V28 kopyası) · ham loglar/komutlar `v29_ham/`. H=512 (4×128), tohumlar: [1, 2, 3] (n=3), (γ₁,λ): s1 (1,1), s2 (2,1), s3 (1,1); γ₂=0,5 (V28'de doğrulamayla seçilen). Yalnız eğitim protokolü; V26 doğrusal okuma, α ve ε yalnız doğrulama CE ile; softmax yok. Test hiçbir seçime girmedi.

## 1. Yapı

- `X* = [X, ID(w_{t−2}), ID(w_{t−1})]`: kimlikler **sıra duyarlı** (t−1 ve t−2 için ayrı seyrek one-hot blokları, 2001+2001 boyut), yalnız geçmiş bağlam kelimeleri; hedef kelime girdide yok. Seyrek blok hiçbir yerde yoğun matrise açılmadı (satır başına ≤3 aktif indeks; seyrek ağırlık matrisi (8098×128)). C varyantı: ayrıca (w_{t−2},w_{t−1}) çiftinin 4096 kovalı hash kimliği.
- `F₁=tanh(γ₁X*A₁+b₁)`: V17 yön kuralı, kimlik farkları yönün parçası (birim norm), bias V17'deki gibi. `F_i=tanh(γ₂[X*,F₁..F_{i−1}]A_i+b_i)`: kimlik ve X her aşamaya erişir. Yönler yalnız eğitim satır çiftlerinden.
- **Ölçek (yalnız eğitimden):** F₁ girdisinde kimlik bloğu enerjisi X ile eşitlenir (ölçek = RMS_X/√nid: B 1,3176, C 1,0758; RMS_X=1,8634). F₂..F₄ girdisinde her blok (X, F_j, kimlik) satır-norm RMS=1'e ölçeklenir (kimlik 1/√nid = 0,7071 / 0,5774).
- Ağırlıklar: V28'deki ardışık artık çözümü ve ayrıca **tek ortak ridge** `W=(FᵀF+λI)⁻¹FᵀY`, `F=[F₁..F₄]`. T = Y (gerçek etiket) veya Q (V25/V27 trigram öğretici, LOO).

## 2. Ana sonuç (test; doğrusal okuma; tohum ortalaması)

| sistem | hedef | test PPL | test CE | Top-1 | Top-5 | eğitim PPL | doğr. PPL | eğitim–doğr. CE farkı | Q açıklanan varyans (test) |
|---|---|---|---|---|---|---|---|---|---|
| A · V28: kimlik YOK, bağlı 4F (ardışık) | Y | **180.7** | 5.1969 | 13.9% | 32.3% | 95 | 189 | 0.692 | – |
| A · V28: kimlik YOK, bağlı 4F (ardışık) | Q | **176.7** | 5.1742 | 13.3% | 31.6% | 127 | 188 | 0.390 | 0.311 |
| B · V29: sıralı kimlik, bağlı 4F (ardışık) | Y | **170.4** | 5.1384 | 14.7% | 34.0% | 85 | 179 | 0.743 | – |
| B · V29: sıralı kimlik, bağlı 4F (ardışık) | Q | **168.2** | 5.1251 | 14.1% | 32.3% | 113 | 178 | 0.450 | 0.354 |
| C · V29: sıralı kimlik + çift (hash), bağlı 4F (ardışık) | Y | **171.6** | 5.1453 | 14.6% | 33.8% | 84 | 180 | 0.759 | – |
| C · V29: sıralı kimlik + çift (hash), bağlı 4F (ardışık) | Q | **169.0** | 5.1297 | 14.0% | 32.1% | 114 | 178 | 0.445 | 0.349 |
| A · V28 ortak ridge | Y | **176.5** | 5.1732 | 14.6% | 33.1% | 85 | 185 | 0.781 | – |
| A · V28 ortak ridge | Q | **171.0** | 5.1417 | 14.1% | 32.2% | 116 | 182 | 0.446 | 0.357 |
| B · V29 ortak ridge | Y | **166.3** | 5.1135 | 15.4% | 34.9% | 72 | 173 | 0.881 | – |
| B · V29 ortak ridge | Q | **160.4** | 5.0778 | 14.8% | 33.3% | 97 | 170 | 0.561 | 0.433 |
| C · V29 ortak ridge (+çift) | Y | **167.8** | 5.1230 | 15.4% | 34.6% | 73 | 175 | 0.879 | – |
| C · V29 ortak ridge (+çift) | Q | **161.6** | 5.0850 | 14.6% | 33.2% | 99 | 172 | 0.550 | 0.423 |

Referanslar (V28 raporu, γ₂=0,5): ortak ridge 176,5; ardışık 180,7; V26 tek F 182,5; trigram 138,9. Bu raporda A aynı programda yeniden ölçüldü ve V28 sonuçlarını **birebir yeniden üretti** (176,5 / 180,7).

## 3. Kimliğin gerçek katkısı (aynı program, aynı tohum, aynı toplam F boyutu)

| karşılaştırma | Y test PPL farkı | Q açıklanan varyans farkı (test) | Top-5 farkı (Y) |
|---|---|---|---|
| ardışık: B − A | -10.3 | +0.043 | +1.6 pp |
| ardışık: C − A | -9.1 | +0.039 | +1.4 pp |
| ortak ridge: B − A | -10.2 | +0.076 | +1.9 pp |
| ortak ridge: C − A | -8.6 | +0.066 | +1.6 pp |
| ortak ridge: C − B | +1.6 | -0.011 | -0.3 pp |

## 4. Her F'nin katkısı (ardışık çözüm; tohum ort.)

| zincir | aşama | Y: test PPL | Y: eğitim / doğr. PPL | Q: açıklanan varyans (test) | Q: ek | Q: test PPL |
|---|---|---|---|---|---|---|
| A · kimlik yok | F1 | 186.4 | 108 / 196 | 0.260 | +0.260 | 183.9 |
| A · kimlik yok | F2 | 182.9 | 100 / 192 | 0.287 | +0.027 | 180.7 |
| A · kimlik yok | F3 | 181.6 | 96 / 190 | 0.301 | +0.014 | 178.3 |
| A · kimlik yok | F4 | 180.7 | 95 / 189 | 0.311 | +0.010 | 176.7 |
| B · sıralı kimlik | F1 | 179.2 | 107 / 189 | 0.262 | +0.262 | 177.8 |
| B · sıralı kimlik | F2 | 173.5 | 95 / 182 | 0.307 | +0.045 | 172.7 |
| B · sıralı kimlik | F3 | 171.4 | 89 / 180 | 0.335 | +0.028 | 169.9 |
| B · sıralı kimlik | F4 | 170.4 | 85 / 179 | 0.354 | +0.019 | 168.2 |
| C · kimlik+çift | F1 | 179.7 | 107 / 189 | 0.262 | +0.262 | 178.3 |
| C · kimlik+çift | F2 | 174.4 | 95 / 183 | 0.305 | +0.043 | 173.3 |
| C · kimlik+çift | F3 | 172.6 | 90 / 181 | 0.331 | +0.027 | 170.7 |
| C · kimlik+çift | F4 | 171.6 | 84 / 180 | 0.349 | +0.018 | 169.0 |

## 5. Ana soruya cevap

- **Ana sonuç (n=3 tohum):** kimlik + X + dört bağlantılı F, ortak ridge ile test PPL **166.3** (kimliksiz aynı program 176.5; V28 raporu 176,5). Ardışık çözümle 170.4 (kimliksiz 180.7). Çift hash kimliği eklemek (167.8) sıralı kimlikten daha iyi değil. Top-5 34.9%, Top-1 15.4%.
- [Kesin] Q hedefiyle açıklanan trigram varyansı (test) ortak ridge B 0.433 (A 0.357; V25'te tek F 0,40–0,43): kimlik girişi %40–43 sınırını aştı.
- [Kesin] Kimlik her F aşamasının girdisine katıldı (F₁ girdisinde ölçek eşitlemesi, F₂..F₄'te blok RMS=1); yalnız son çıktıya eklenmedi. Dört F ve ağırlık hesabı tek C++ programı içinde, kapalı form.
- **Kimlik = n-gram belleği uyarısı:** kimlik one-hot'ları (t−1, t−2) kelime ve bigram sayım bilgisini doğrudan taşır (V25: kimlik-yalnız ridge doğrusal okumayla ≈137 PPL, yani n-gram tablosunun yeniden ifadesi). Buradaki kazanç, kimliğin ezber/sayım bilgisini F'lere taşımasıdır; **bağımsız genelleme kazancı olarak sunulmuyor**. Eğitim PPL'inin düşmesine rağmen doğrulama/test de düştüğü için bu bilgi görülmemiş metinde de işe yarıyor, ama bu bilgi n-gram istatistiğidir.
- [Muhtemel] Kimlik-yalnız ridge (V25) ≈137 PPL verirken burada ≈166: F'lerin tanh ve λ ile kimlik bilgisini seyreltmesi, ayrıca λ'nın doğrusal okuma için ayarlanmamış olması. Bu ayrıştırma yapılmadı.
- [Kesin] Eğitim PPL belirgin düşük (≈70 vs 80), eğitim–doğrulama farkı büyük (bölüm 2); doğrulama ve test yine iyileşiyor.

## 6. Sınırlamalar

- Yalnız H=512 (4×128); H=1024 yapılmadı. Süre sınırı 12 dk.
- λ, γ₁, γ₂ kimlik için yeniden seçilmedi (V17/V28'den); kimlik bloğunun ölçeği tek bir kural (enerji eşitleme). Alternatif ölçekler ve kimlik için ayrı λ denenmedi. V25'te tek F + kimlik λ=100 gibi büyük λ'ların doğrusal okumada daha iyi olduğu görülmüştü; bu çalışma bunu taramadı.
- A (kimlik yok) bu programda yeniden ölçüldü ve V28 raporundaki 176,5 / 180,7 ile örtüştü (aynı γ₂=0,5, tohum ve protokol).
- Hash çift kimliği (4096 kova) çarpışmalı; en sık çiftleri kimliğe çeviren V25 yöntemi kullanılmadı.
- Q sonuçları öğretici aktarımıdır, bağımsız öğrenme değil. Test n=4 980.
- Yeni yapı ana mimariye eklenmedi.

## 7. Süre ve bellek

| komut | wall s | user CPU s | RAM MB |
|---|---|---|---|
| `./tek_hamle_v29 2 512 1 1 1 0.5 ABCJ` | 89.20 | 158.27 | 907 |
| `./tek_hamle_v29 2 512 2 2 1 0.5 ABCJ` | 92.97 | 165.09 | 907 |
| `./tek_hamle_v29 2 512 3 1 1 0.5 ABCJ` | 87.93 | 162.34 | 907 |

Zincir başına toplam süre (özellik+Gram+RHS+iki hedef+değerlendirme; tohum ort.): V28_KIMLIK_YOK=17.8s; ORTAK_RIDGE_V28_KIMLIK_YOK=12.9s; V29_KIMLIK_SIRALI=16.8s; ORTAK_RIDGE_V29_KIMLIK_SIRALI=12.6s; V29_KIMLIK_SIRALI+CIFT=17.3s; ORTAK_RIDGE_V29_KIMLIK_SIRALI+CIFT=12.5s

Seyrek kimlik bloğu yoğun açılmadı: seyrek ağırlık (8098×128 float) ≈ 4 MB / aşama; tepe RAM tabloda (V28 H=512 ≈ 890 MB).

