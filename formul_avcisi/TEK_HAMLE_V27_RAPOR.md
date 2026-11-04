# TEK HAMLE V27 — Birbirine bağlı 4 F ile ağırlık üretimi

Dal `claude/formul-avcisi` · kod `cpp/tek_hamle_v27.cpp` (V26 kopyası; V26 okuma katmanı) · ham loglar/komutlar `v27_ham/`. Aynı veri/bölme, **yalnız eğitim** protokolü; doğrusal okuma `P=(1−α)π+α·max(z,ε)/Σmax(z,ε)` (α∈20, ε∈8 değer, **yalnız doğrulama CE ile**); softmax kullanılmadı. Tohumlar ve (γ₁,λ): H=512: s1 (1,1), s2 (2,1), s3 (1,1); H=1024: s1 (4,10), s2 (4,100), s3 (2,1) (V17 seçimleri). F₂..F₄ için γ₂=1 sabit, ayrıca ayarlanmadı.

## 1. Yöntem

- `F₁=tanh(γ₁·X·A₁+b₁)` (V17 yönteminin aynısı: eğitim satır çiftlerinden birim yön, `h/4` sütun + sabit sütun); `F_i=tanh(γ₂·F_{i−1}·A_i+b_i)`, `i=2,3,4`. **Her aşamada aynı yön üretme kuralı**: hedefleri farklı iki eğitim satırının önceki-aşama özelliklerinin farkı birim vektöre normalize edilir (`A_i` sütunu), sapma `b_i` = V17'deki gibi çiftin orta noktasından hesaplanan değerlerin standart sapmasıyla N(0,σ²) (önceki aşama sütun sayısı × h/4). Yalnız eğitim satırları kullanılır; val/test yalnız dönüştürülür.
- Zincir: `R₀=T` (T = Y gerçek etiket **veya** Q = trigram öğretici dağılımı, eğitim satırlarında leave-one-out), `W_i=(F_iᵀF_i+λI)⁻¹F_iᵀR_{i−1}`, `R_i=R_{i−1}−F_iW_i`, `Ŷ=Σ F_iW_i`. `F_iᵀR_{i−1}` yoğun artık matrisi kurmadan, `F_iᵀT − Σ_{j<i}(F_iᵀF_j)W_j` ile hesaplanır (tam eşdeğer). Dört çıktı yalnız toplanır; tek algoritma, ardışık dört temsil aşaması.
- C deneyi: aynı dört F tek birleşik matriste, **tek ortak ridge** (`Z=[F₁..F₄]`).
- Kalan hata: Y için `‖Y−Ŷ‖²/n`; Q için trigram dağılımının (π'den sapmasına göre) açıklanan varyansı, `1−‖Ŷ−Q‖²/‖Q−π‖²` (doğrulama/test, Q val/test için eğitim sayımlarından). Her aşamanın "ek" katkısı bir önceki aşamaya göre fark.

## 2. Ana sonuç (test PPL, doğrusal okuma; 3 tohum ortalaması)

| H | yapı | hedef | test PPL | Top-1 | Top-5 | eğitim PPL | doğrulama PPL | eğitim–doğr. CE farkı | Q açıklanan varyans (test) |
|---|---|---|---|---|---|---|---|---|---|
| 512 (n=3) | TEK_F(H=512) · F1..F1(zincir) | Y | 182.5 | 14.8% | 33.3% | 75.9 | 190.8 | 0.922 | – |
| 512 (n=3) | TEK_F(H=512) · F1..F1(zincir) | Q | 170.3 | 14.4% | 32.7% | 105.5 | 180.4 | 0.537 | 0.397 |
| 512 (n=3) | BAGLI_4F(4x128) · F1..F1(zincir) | Y | 186.4 | 13.2% | 31.5% | 108.4 | 195.9 | 0.591 | – |
| 512 (n=3) | BAGLI_4F(4x128) · F1..F2(zincir) | Y | 187.6 | 13.4% | 32.2% | 102.3 | 198.0 | 0.661 | – |
| 512 (n=3) | BAGLI_4F(4x128) · F1..F3(zincir) | Y | 193.0 | 13.4% | 32.1% | 94.9 | 202.8 | 0.759 | – |
| 512 (n=3) | BAGLI_4F(4x128) · F1..F4(zincir) | Y | 194.6 | 13.5% | 32.1% | 91.3 | 204.7 | 0.807 | – |
| 512 (n=3) | BAGLI_4F(4x128) · F1..F1(zincir) | Q | 183.9 | 12.6% | 30.5% | 143.8 | 195.3 | 0.306 | 0.260 |
| 512 (n=3) | BAGLI_4F(4x128) · F1..F2(zincir) | Q | 178.9 | 12.9% | 31.2% | 133.0 | 192.0 | 0.368 | 0.284 |
| 512 (n=3) | BAGLI_4F(4x128) · F1..F3(zincir) | Q | 178.2 | 13.2% | 31.3% | 128.7 | 192.2 | 0.401 | 0.290 |
| 512 (n=3) | BAGLI_4F(4x128) · F1..F4(zincir) | Q | 178.6 | 13.1% | 31.3% | 129.3 | 192.3 | 0.397 | 0.292 |
| 512 (n=3) | ORTAK_RIDGE_4F(4x128) · F1..F4(ORTAK tek ridge) | Y | 197.3 | 13.6% | 32.1% | 84.2 | 206.1 | 0.895 | – |
| 512 (n=3) | ORTAK_RIDGE_4F(4x128) · F1..F4(ORTAK tek ridge) | Q | 177.8 | 13.3% | 31.5% | 123.3 | 190.4 | 0.434 | 0.310 |
| 1024 (n=3) | TEK_F(H=1024) · F1..F1(zincir) | Y | 192.2 | 15.3% | 33.7% | 69.0 | 198.6 | 1.057 | – |
| 1024 (n=3) | TEK_F(H=1024) · F1..F1(zincir) | Q | 172.3 | 14.4% | 32.9% | 98.0 | 181.5 | 0.616 | 0.430 |
| 1024 (n=3) | BAGLI_4F(4x256) · F1..F1(zincir) | Y | 189.2 | 13.6% | 32.2% | 99.2 | 197.4 | 0.688 | – |
| 1024 (n=3) | BAGLI_4F(4x256) · F1..F2(zincir) | Y | 194.5 | 13.8% | 32.8% | 86.7 | 202.7 | 0.849 | – |
| 1024 (n=3) | BAGLI_4F(4x256) · F1..F3(zincir) | Y | 198.0 | 13.7% | 32.7% | 83.1 | 205.5 | 0.907 | – |
| 1024 (n=3) | BAGLI_4F(4x256) · F1..F4(zincir) | Y | 199.4 | 13.7% | 32.4% | 81.8 | 206.4 | 0.926 | – |
| 1024 (n=3) | BAGLI_4F(4x256) · F1..F1(zincir) | Q | 180.6 | 13.0% | 31.2% | 130.1 | 191.7 | 0.388 | 0.292 |
| 1024 (n=3) | BAGLI_4F(4x256) · F1..F2(zincir) | Q | 177.8 | 13.4% | 31.7% | 123.0 | 189.2 | 0.431 | 0.312 |
| 1024 (n=3) | BAGLI_4F(4x256) · F1..F3(zincir) | Q | 177.9 | 13.3% | 31.7% | 120.3 | 188.8 | 0.451 | 0.318 |
| 1024 (n=3) | BAGLI_4F(4x256) · F1..F4(zincir) | Q | 178.0 | 13.3% | 31.8% | 118.7 | 188.9 | 0.465 | 0.320 |
| 1024 (n=3) | ORTAK_RIDGE_4F(4x256) · F1..F4(ORTAK tek ridge) | Y | 199.9 | 13.8% | 32.5% | 76.8 | 206.7 | 0.991 | – |
| 1024 (n=3) | ORTAK_RIDGE_4F(4x256) · F1..F4(ORTAK tek ridge) | Q | 178.0 | 13.5% | 31.6% | 114.9 | 187.6 | 0.491 | 0.333 |

## 3. Aşama aşama katkı (3 tohum ortalaması): gerçek ek bilgi ve %25 kapasite payı

Hedef Q (trigram dağılımı) için her aşama sonrası açıklanan varyans; "ek" = bir önceki aşamaya göre artış. Y hedefi için kalan regresyon hatası `‖Y−Ŷ‖²/n` (test).

| H | aşama | eklenen sütun | kümülatif sütun | Q açıklanan varyans (test) | ek açıklanan (test) | Q-zincir test PPL | Y-zincir kalan hata (test) | Y-zincir test PPL | Y-zincir eğitim / doğr. PPL |
|---|---|---|---|---|---|---|---|---|---|
| 512 | F1 | 129 | 129 | 0.260 | +0.260 | 183.9 | 0.9722 | 186.4 | 108 / 196 |
| 512 | F2 | 128 | 257 | 0.284 | +0.024 | 178.9 | 0.9716 | 187.6 | 102 / 198 |
| 512 | F3 | 128 | 385 | 0.290 | +0.006 | 178.2 | 0.9728 | 193.0 | 95 / 203 |
| 512 | F4 | 128 | 513 | 0.292 | +0.002 | 178.6 | 0.9737 | 194.6 | 91 / 205 |
| 512 | *tek F (512+1)* | 513 | 513 | 0.397 | – | 170.3 | 0.9708 | 182.5 | 76 / 191 |
| 512 | *ortak ridge 4 F* | 513 | 513 | 0.310 | – | 177.8 | 0.9759 | 197.3 | 84 / 206 |
| 1024 | F1 | 257 | 257 | 0.292 | +0.292 | 180.6 | 0.9715 | 189.2 | 99 / 197 |
| 1024 | F2 | 256 | 513 | 0.312 | +0.021 | 177.8 | 0.9733 | 194.5 | 87 / 203 |
| 1024 | F3 | 256 | 769 | 0.318 | +0.006 | 177.9 | 0.9754 | 198.0 | 83 / 206 |
| 1024 | F4 | 256 | 1025 | 0.320 | +0.002 | 178.0 | 0.9769 | 199.4 | 82 / 206 |
| 1024 | *tek F (1024+1)* | 1025 | 1025 | 0.430 | – | 172.3 | 0.9777 | 192.2 | 69 / 199 |
| 1024 | *ortak ridge 4 F* | 1025 | 1025 | 0.333 | – | 178.0 | 0.9783 | 199.9 | 77 / 207 |

## 4. Karşılaştırma ve sonuç

- **H=512 (3 tohum), doğrusal okuma, test PPL:** tek F: Y 182.5, Q 170.3; bağlı 4F: Y (F1 / F1..F4 / en iyi aşama) 186.4 / 194.6 / 186.4, Q (F1..F4) 178.6; ortak ridge: Y 197.3, Q 177.8. Q açıklanan varyans (test): tek F 0.397, bağlı 4F 0.292, ortak ridge 0.310.
- **H=1024 (3 tohum), doğrusal okuma, test PPL:** tek F: Y 192.2, Q 172.3; bağlı 4F: Y (F1 / F1..F4 / en iyi aşama) 189.2 / 199.4 / 189.2, Q (F1..F4) 178.0; ortak ridge: Y 199.9, Q 178.0. Q açıklanan varyans (test): tek F 0.430, bağlı 4F 0.320, ortak ridge 0.333.

**Okuma (yalnız yukarıdaki ölçümler):**
- [Kesin] Aynı toplam sütun sayısında, bağlı 4 F tek F'den **daha az** Q bilgisi açıklıyor ve test PPL'i düşürmüyor. Başarı hedefi (V26 referansı 182,5'in altı) Y hedefiyle sağlanmadı; aşağıdaki tablo ve seed ortalamaları bunu gösteriyor.
- [Kesin] En iyi bağlı-F Y sonucu H=512'de 186,4 (yalnız F₁), H=1024'te 189,2 (yalnız F₁): 182,5 referansının altına inilemedi. H=1024'te F₁ (257 sütun) tek F'den (1025 sütun, 192,2) iyi çıkması zincirden değil, daha küçük modelin doğrusal okumada daha az aşırı uyum yapmasından geliyor (H=512 tek F 182,5 hâlâ daha iyi) [Muhtemel].
- [Kesin] Q-hedefli zincirde F₂ test PPL'ini 183,9→178,9 (H=512) ve 180,6→177,8 (H=1024) düşürüyor, F₃/F₄ düz kalıyor: ek aşama yalnız küçük bir kazanç sağlıyor, tek F'nin (170,3 / 172,3) gerisinde.
- [Kesin] F₂, F₃, F₄ eklediği açıklanan Q varyansı çok küçük ve hızla sönüyor (ek ≈ +0,02 → +0,005 → +0,002); %25 sütun payı %25 benzersiz bilgi taşımıyor: toplam açıklanan varyans (≈0,29–0,32) tek F'nin (≈0,40–0,42) altında, yani toplam "%100'e yaklaşma" ölçütünde zincir tek F'nin gerisinde.
- [Kesin] Y hedefli zincirde F₂..F₄ eğitim PPL'ini düşürüyor ama doğrulama/test PPL'ini kötüleştiriyor: ek aşamalar eğitime uyuyor, genelleme kazandırmıyor (eğitim–doğrulama farkı büyüyor).
- [Muhtemel] Neden: (a) F₁ yalnız h/4 yön alıyor, ham girdi X'ten yön sayısı azalıyor (kapasite bölünüyor); (b) F₂..F₄ girdisi tanh çıktıları olduğundan yeni bilgi eklemiyor, F₁'in doğrusal olmayan yeniden karışımı; ham X ve kelime kimliği bilgisi (V25: eksik bilginin yeri) hiçbir aşamaya girmiyor. Bu açıklama ölçülmedi, doğrulanması için aşama başına X'in de verildiği (skip) varyant gerekir.
- [Kesin] Ortak tek ridge, aşamalı (ardışık artık) çözümden Q açıklanan varyansı biraz artırıyor (çapraz ilişkiler) ama tek F düzeyine ulaşmıyor.

## 5. Sınırlamalar

- γ₂=1 ve λ (V17'den) F₂..F₄ için ayrıca seçilmedi; aşama başına λ/γ taraması yapılırsa sonuç değişebilir. Yön sayısı h/4 sabit (tek F ile aynı toplam sütun).
- Tohum sayısı tablo başlıklarındaki n; test n=4 980. Q hedefi leave-one-out trigram öğreticidir (V25); Q-zincirin test PPL'i etiketle ölçülür, ama Q hedefi gerçek öğrenme değil öğretici bilgisinin aktarımı ölçümüdür.
- Süre/RAM ölçümleri aşağıda; çekirdekler başka iş çalıştırmadı (tek süreç).
- Zincir bir "yeni mimari" değildir; ana algoritmaya eklenmedi.

## 6. Süre ve bellek

| komut | wall s | user CPU s | RAM MB |
|---|---|---|---|
| `./tek_hamle_v27 2 512 1 1 1 1 SCJ` | 38.70 | 74.80 | 888 |
| `./tek_hamle_v27 2 1024 1 4 10 1 SCJ` | 58.47 | 142.70 | 1021 |
| `./tek_hamle_v27 2 512 2 2 1 1 SCJ` | 38.35 | 77.49 | 888 |
| `./tek_hamle_v27 2 512 3 1 1 1 SCJ` | 41.70 | 74.97 | 887 |
| `./tek_hamle_v27 2 1024 2 4 100 1 SCJ` | 53.93 | 132.90 | 1021 |
| `./tek_hamle_v27 2 1024 3 2 1 1 SCJ` | 52.67 | 133.40 | 1021 |

Zincir başına süre (özellik üretimi + Gram + RHS, sonra aşama çözümleri+değerlendirme), tohum 1: H=512: TEK_F=2.93, TEK_F=toplam=11.2s tepe_RAM=871MB, BAGLI_4F=2.51, BAGLI_4F=toplam=14.8s tepe_RAM=887MB, ORTAK_RIDGE_4F=2.53, ORTAK_RIDGE_4F=toplam=12.5s tepe_RAM=888MB; H=1024: TEK_F=6.99, TEK_F=toplam=18.3s tepe_RAM=1021MB, BAGLI_4F=5.42, BAGLI_4F=toplam=20.6s tepe_RAM=1021MB, ORTAK_RIDGE_4F=6.19, ORTAK_RIDGE_4F=toplam=19.4s tepe_RAM=1021MB

