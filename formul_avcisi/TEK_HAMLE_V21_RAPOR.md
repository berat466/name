# TEK HAMLE V21 — Seçici CE ağırlık düzeltmesi (`W = W0 − η (D ⊙ M)`)

Dal `claude/formul-avcisi` · kod `cpp/tek_hamle_v21.cpp` (V20 kopyası, önceki dosyalara dokunulmadı) · ham loglar/komutlar `v21_ham/`.

Yöntem: tek Ridge (`W0`), tek CE gradyanı (eğitim, α0=0,2/T0=0,01), tek `D=A⁻¹FᵀG`; maske `M` D'nin **çıkış sütunlarına** kelime sıklık grubuna göre katsayı verir (V20 grup tanımı aynen: eğitim hedef kitlesinin kümülatif %50'si = sık (44 tür), %50–80 orta (284), kalan nadir (1673)). Maske değiştirmek yeni Cholesky/gradyan istemez: logitler `Z = Z0 − η·(Zd ⊙ m)` olarak hesaplanır (Z0=F·W0, Zd=F·D bir kez). Her (maske, η) için α,T doğrulama CE ile seçildi; test yalnız seçimden sonra. Maske katsayısı 0 → o sütunun Ridge ağırlığı bit düzeyinde değişmez. Seed-1 A maskesi V20 sonuçlarını birebir yeniden üretti (PPL 199,84 / 199,33) — [Kesin] kod doğrulaması.

## 0. Özet (H=1024, test, 3 tohum ort.; her sayı aşağıdaki tablolardan)

- [Kesin] Maske mekanik olarak çalışıyor: sık kelime katsayısı azaldıkça Top-1 kaybı küçülüyor, PPL kazancı da azalıyor (kural A, maske başına η doğrulamadan): 

| sık katsayısı | maske | PPL | Top-1 | net Top-1 geçişi (s1/s2/s3) |
|---|---|---|---|---|
| 1 | A (V20) | 198,89 | %15,43 | −49/−40/−23 |
| 0,5 | D | 198,88 | %15,81 | −16/−23/−16 |
| 0,25 | C | 199,50 | %16,08 | −8/−4/−3 |
| 0 | B | 202,04 | %16,35 | +14/−1/+12 |
| Ridge | – | 206,82 | %16,18 | – |

- [Kesin] Ana hedef (PPL ≤ 199 **ve** Top-1 ≥ %16,2) birlikte sağlanamadı. En yakınlar: D (198,88 / %15,81) PPL hedefini tutar, Top-1 hedefini tutmaz; B (202,04 / %16,35) Top-1'i tutar, V20'nin 7,9 puanlık PPL kazancının 4,8 puanını (%61) korur; C/E kural B (≈199,7 / %16,16–16,18) Top-1'i Ridge düzeyinde tutar, PPL V20 B ile aynı (199,8) — yani V20 B'ye göre Top-1 +0,3 pp, PPL farksız.
- [Muhtemel] D, V20 A ile aynı PPL'de Top-1'i +0,4 pp yükseltiyor ve üç tohumda da net geçişler daha iyi (yukarıdaki tablo). Ancak test n=4 980 (SE ≈ %0,5), tek metin; fark tek başına anlamlı değil, tutarlılığı (3/3 tohum) kanıt sayılır ama yeterli değil.
- [Kesin] Doğrulama-seçim kuralları (global) çoğunlukla maskesiz A veya D'yi seçti: V21 global kural A → PPL 198,95 / Top-1 %15,54; global kural B → 199,29 / %15,91. V20 A/B'ye göre fark ihmal edilebilir (PPL +0,06 / −0,54; Top-1 +0,1 / 0,0 pp). Yani dürüst "doğrulamayla seç" protokolü altında V21 V20'yi anlamlı biçimde geçmedi; maske kazancı yalnızca Pareto cephesini (PPL–Top-1 takası) görünür kılıyor.
- [Kesin] H=512: maskeler PPL'yi kötüleştirdi (202,75 → 203,77–207,70), Top-1 +0,2…+0,3 pp (%15,40 → %15,64–15,72). B–E maskelerinde net geçişler üç tohumda pozitif; ama PPL bedeli yüksek.
- [Kesin] Train–val CE farkı: Ridge 0,28; V20 A 0,67; maskeli varyantlar 0,48–0,64. Fark düzeltme miktarıyla birlikte büyüyor; maske bunu ücretsiz azaltmıyor.
- [Kesin] Ek maliyet yok: aynı tek Ridge + tek gradyan + tek D. H=1024: Ridge ≈1,3 s, CE düzeltmesi ≈6,0 s (V20 ile aynı); maske yalnız eleman-bazlı bir çarpma.
- [Kesin] Genişletilmiş η (0,015–0,03; görev dışı ek, bölüm 5): maskeli varyantlarda doğrulama CE biraz düşse de test PPL/Top-1 iyileşmedi (ör. C kural A 199,50 → 199,81; Top-1 %16,08 → %15,97). Büyük η yardım etmedi.

**Ana soruya cevap:** Sıklığa göre seçici düzeltme Top-1 kaybını azaltıyor (hatta B ile Ridge üstüne çıkarıyor), fakat PPL ve Top-1 aynı anda hedef değerlere (≤199 ve ≥%16,2) ulaşmıyor; bir takas var. Doğrulamayla otomatik seçim bu takasın iyi noktasını güvenilir şekilde bulamıyor.

## 1. Ana sonuç — doğrulama kuralıyla seçilen (maske, η) ve test sonucu

Seçim iki aşamalı ve yalnız doğrulamadan: önce her maske için η, sonra tüm maskeler arasında en iyi (GLOBAL). Kural A: en düşük val CE; kural B: en düşük val CE ve val Top-1 ≥ Ridge − 0,2 pp.

**H=1024** (3 tohum, γ/λ V17 seçimi)

| yöntem | seçilen (maske, η) s1 / s2 / s3 | PPL s1 / s2 / s3 | PPL ort. | Top-1 ort. | Top-5 ort. | test CE ort. | train–val CE farkı ort. |
|---|---|---|---|---|---|---|---|
| Ridge (η=0) | – | 206.37 / 207.82 / 206.28 | **206.82** | 16.18% | 29.99% | 5.3318 | 0.280 |
| GLOBAL kural A | A_V20 η=0.0075 / D_sik.5 η=0.01 / A_V20 η=0.005 | 199.84 / 197.73 / 199.27 | **198.95** | 15.54% | 30.82% | 5.2930 | 0.662 |
| GLOBAL kural B | A_V20 η=0.005 / D_sik.5 η=0.0075 / D_sik.5 η=0.005 | 199.33 / 198.37 / 200.16 | **199.29** | 15.91% | 30.79% | 5.2947 | 0.584 |

**H=512** (3 tohum, γ/λ V17 seçimi)

| yöntem | seçilen (maske, η) s1 / s2 / s3 | PPL s1 / s2 / s3 | PPL ort. | Top-1 ort. | Top-5 ort. | test CE ort. | train–val CE farkı ort. |
|---|---|---|---|---|---|---|---|
| Ridge (η=0) | – | 213.10 / 211.43 / 212.48 | **212.34** | 15.29% | 29.48% | 5.3582 | 0.096 |
| GLOBAL kural A | A_V20 η=0.01 / A_V20 η=0.01 / A_V20 η=0.01 | 202.31 / 203.42 / 202.52 | **202.75** | 15.40% | 30.41% | 5.3120 | 0.328 |
| GLOBAL kural B | D_sik.5 η=0.01 / A_V20 η=0.0075 / A_V20 η=0.0075 | 203.73 / 203.28 / 202.95 | **203.32** | 15.49% | 30.56% | 5.3148 | 0.287 |

## 2. Her maske ayrı (doğrulamayla η seçilmiş, test sonucu; 3 tohum ort.)

**H=1024**

| maske (sık/orta/nadir) | kural | η (s1/s2/s3) | PPL ort. | Top-1 ort. | Top-5 ort. | test CE | train–val farkı | net Top-1 geçişi (s1/s2/s3) |
|---|---|---|---|---|---|---|---|---|
| A: 1/1/1 (V20) | A | 0.0075/0.0075/0.005 | 198.89 | 15.43% | 30.80% | 5.2928 | 0.666 | -49/-40/-23 |
| A: 1/1/1 (V20) | B | 0.005/0.003/0.003 | 199.83 | 15.90% | 30.85% | 5.2975 | 0.542 | -24/-7/-10 |
| B: 0/1/1 | A | 0.01/0.01/0.01 | 202.04 | 16.35% | 30.26% | 5.3084 | 0.476 | +14/-1/+12 |
| B: 0/1/1 | B | 0.01/0.01/0.01 | 202.04 | 16.35% | 30.26% | 5.3084 | 0.476 | +14/-1/+12 |
| C: 0,25/1/1 | A | 0.01/0.01/0.01 | 199.50 | 16.08% | 30.65% | 5.2958 | 0.591 | -8/-4/-3 |
| C: 0,25/1/1 | B | 0.01/0.01/0.0075 | 199.66 | 16.18% | 30.71% | 5.2966 | 0.572 | -8/-4/+13 |
| D: 0,5/1/1 | A | 0.01/0.01/0.0075 | 198.88 | 15.81% | 30.71% | 5.2927 | 0.637 | -16/-23/-16 |
| D: 0,5/1/1 | B | 0.01/0.0075/0.005 | 199.32 | 15.96% | 30.70% | 5.2949 | 0.588 | -16/-15/-1 |
| E: 0,25/0,75/1 | A | 0.01/0.01/0.01 | 199.52 | 16.10% | 30.77% | 5.2959 | 0.579 | -5/-6/+0 |
| E: 0,25/0,75/1 | B | 0.01/0.01/0.0075 | 199.70 | 16.16% | 30.75% | 5.2968 | 0.559 | -5/-6/+9 |

**H=512**

| maske (sık/orta/nadir) | kural | η (s1/s2/s3) | PPL ort. | Top-1 ort. | Top-5 ort. | test CE | train–val farkı | net Top-1 geçişi (s1/s2/s3) |
|---|---|---|---|---|---|---|---|---|
| A: 1/1/1 (V20) | A | 0.01/0.01/0.01 | 202.75 | 15.40% | 30.41% | 5.3120 | 0.328 | +19/+2/-4 |
| A: 1/1/1 (V20) | B | 0.005/0.0075/0.0075 | 203.65 | 15.48% | 30.52% | 5.3164 | 0.278 | +25/+10/-6 |
| B: 0/1/1 | A | 0.01/0.01/0.01 | 207.70 | 15.64% | 29.85% | 5.3361 | 0.160 | +17/+24/+12 |
| B: 0/1/1 | B | 0.01/0.01/0.01 | 207.70 | 15.64% | 29.85% | 5.3361 | 0.160 | +17/+24/+12 |
| C: 0,25/1/1 | A | 0.01/0.01/0.01 | 205.42 | 15.71% | 30.03% | 5.3250 | 0.200 | +19/+26/+18 |
| C: 0,25/1/1 | B | 0.01/0.01/0.01 | 205.42 | 15.71% | 30.03% | 5.3250 | 0.200 | +19/+26/+18 |
| D: 0,5/1/1 | A | 0.01/0.01/0.01 | 203.77 | 15.72% | 30.49% | 5.3170 | 0.262 | +27/+31/+7 |
| D: 0,5/1/1 | B | 0.01/0.01/0.01 | 203.77 | 15.72% | 30.49% | 5.3170 | 0.262 | +27/+31/+7 |
| E: 0,25/0,75/1 | A | 0.01/0.01/0.01 | 205.46 | 15.66% | 30.08% | 5.3252 | 0.207 | +22/+24/+10 |
| E: 0,25/0,75/1 | B | 0.01/0.01/0.01 | 205.46 | 15.66% | 30.08% | 5.3252 | 0.207 | +22/+24/+10 |

η=0.01 ızgara kenarıdır; B/C/D/E maskeleri için val CE η=0,01'de hâlâ düşüyordu (bölüm 5'e bakın). [Kesin: log]

## 3. Doğrulama taraması (H=1024, tohum 1)

| maske | η | val CE | (α,T) | val Top-1 | val Top-5 |
|---|---|---|---|---|---|
| A_V20 | 0 | 5.4179 | a=0.2 T=0.01 | 0.1492 | 0.2787 |
| A_V20 | 0.003 | 5.3776 | a=0.2 T=0.01 | 0.1520 | 0.2839 |
| A_V20 | 0.005 | 5.3673 | a=0.25 T=0.01 | 0.1486 | 0.2892 |
| A_V20 | 0.0075 | 5.3664 | a=0.25 T=0.01 | 0.1432 | 0.2890 |
| A_V20 | 0.01 | 5.3705 | a=0.25 T=0.01 | 0.1426 | 0.2843 |
| B_sik0 | 0 | 5.4179 | a=0.2 T=0.01 | 0.1492 | 0.2787 |
| B_sik0 | 0.003 | 5.4055 | a=0.2 T=0.01 | 0.1502 | 0.2809 |
| B_sik0 | 0.005 | 5.4007 | a=0.2 T=0.01 | 0.1504 | 0.2811 |
| B_sik0 | 0.0075 | 5.3979 | a=0.2 T=0.01 | 0.1490 | 0.2839 |
| B_sik0 | 0.01 | 5.3976 | a=0.2 T=0.01 | 0.1496 | 0.2825 |
| C_sik.25 | 0 | 5.4179 | a=0.2 T=0.01 | 0.1492 | 0.2787 |
| C_sik.25 | 0.003 | 5.3963 | a=0.2 T=0.01 | 0.1514 | 0.2833 |
| C_sik.25 | 0.005 | 5.3874 | a=0.2 T=0.01 | 0.1508 | 0.2843 |
| C_sik.25 | 0.0075 | 5.3811 | a=0.2 T=0.01 | 0.1496 | 0.2859 |
| C_sik.25 | 0.01 | 5.3783 | a=0.2 T=0.01 | 0.1512 | 0.2837 |
| D_sik.5 | 0 | 5.4179 | a=0.2 T=0.01 | 0.1492 | 0.2787 |
| D_sik.5 | 0.003 | 5.3886 | a=0.2 T=0.01 | 0.1504 | 0.2837 |
| D_sik.5 | 0.005 | 5.3777 | a=0.2 T=0.01 | 0.1512 | 0.2843 |
| D_sik.5 | 0.0075 | 5.3714 | a=0.2 T=0.01 | 0.1496 | 0.2869 |
| D_sik.5 | 0.01 | 5.3700 | a=0.2 T=0.01 | 0.1496 | 0.2847 |
| E_.25/.75/1 | 0 | 5.4179 | a=0.2 T=0.01 | 0.1492 | 0.2787 |
| E_.25/.75/1 | 0.003 | 5.3979 | a=0.2 T=0.01 | 0.1516 | 0.2833 |
| E_.25/.75/1 | 0.005 | 5.3884 | a=0.2 T=0.01 | 0.1508 | 0.2833 |
| E_.25/.75/1 | 0.0075 | 5.3812 | a=0.2 T=0.01 | 0.1512 | 0.2839 |
| E_.25/.75/1 | 0.01 | 5.3775 | a=0.2 T=0.01 | 0.1502 | 0.2849 |

## 4. Sıklık gruplarına göre etki (test, H=1024, kural A'nın maske-başına seçimi, tohum 1)

| maske | η | grup | ΔCE (yeni−Ridge) | Top-1 Ridge → yeni | Top-5 Ridge → yeni |
|---|---|---|---|---|---|
| A: 1/1/1 (V20) | 0.0075 | sik | +0.0412 | 27.3% → 24.1% | 51.6% → 51.6% |
| A: 1/1/1 (V20) | 0.0075 | orta | -0.1260 | 5.1% → 7.3% | 7.2% → 9.9% |
| A: 1/1/1 (V20) | 0.0075 | nadir | -0.1066 | 0.0% → 1.0% | 0.3% → 1.1% |
| B: 0/1/1 | 0.01 | sik | +0.0077 | 27.3% → 26.9% | 51.6% → 51.3% |
| B: 0/1/1 | 0.01 | orta | -0.0839 | 5.1% → 6.6% | 7.2% → 8.7% |
| B: 0/1/1 | 0.01 | nadir | -0.0432 | 0.0% → 0.8% | 0.3% → 1.1% |
| C: 0,25/1/1 | 0.01 | sik | +0.0226 | 27.3% → 25.6% | 51.6% → 51.7% |
| C: 0,25/1/1 | 0.01 | orta | -0.1165 | 5.1% → 7.2% | 7.2% → 8.9% |
| C: 0,25/1/1 | 0.01 | nadir | -0.0802 | 0.0% → 1.0% | 0.3% → 1.1% |
| D: 0,5/1/1 | 0.01 | sik | +0.0438 | 27.3% → 25.2% | 51.6% → 51.4% |
| D: 0,5/1/1 | 0.01 | orta | -0.1386 | 5.1% → 7.4% | 7.2% → 9.2% |
| D: 0,5/1/1 | 0.01 | nadir | -0.1063 | 0.0% → 1.0% | 0.3% → 1.1% |
| E: 0,25/0,75/1 | 0.01 | sik | +0.0181 | 27.3% → 25.8% | 51.6% → 52.0% |
| E: 0,25/0,75/1 | 0.01 | orta | -0.1017 | 5.1% → 7.0% | 7.2% → 8.6% |
| E: 0,25/0,75/1 | 0.01 | nadir | -0.0883 | 0.0% → 1.1% | 0.3% → 1.2% |

Doğru→yanlış / yanlış→doğru geçişleri (kalibre Top-1, test, H=1024, kural A seçimi):

| maske | tohum 1 | tohum 2 | tohum 3 |
|---|---|---|---|
| A: 1/1/1 (V20) | 191→yanlış / 142→doğru (net -49) | 163→yanlış / 123→doğru (net -40) | 120→yanlış / 97→doğru (net -23) |
| B: 0/1/1 | 36→yanlış / 50→doğru (net +14) | 38→yanlış / 37→doğru (net -1) | 34→yanlış / 46→doğru (net +12) |
| C: 0,25/1/1 | 83→yanlış / 75→doğru (net -8) | 81→yanlış / 77→doğru (net -4) | 77→yanlış / 74→doğru (net -3) |
| D: 0,5/1/1 | 139→yanlış / 123→doğru (net -16) | 129→yanlış / 106→doğru (net -23) | 101→yanlış / 85→doğru (net -16) |
| E: 0,25/0,75/1 | 74→yanlış / 69→doğru (net -5) | 74→yanlış / 68→doğru (net -6) | 70→yanlış / 70→doğru (net +0) |

## 5. EK (görev dışı): genişletilmiş η ızgarası {…, 0,015, 0,02, 0,03} — H=1024, 3 tohum

Görevin η ızgarası 0,01'de bitiyor ve maskeli varyantlar orada kenarda seçiliyor; maskeli düzeltmenin daha büyük η isteyip istemediğini görmek için aynı kodla `-DEXT` ile yeniden çalıştırıldı. Seçim yine yalnız doğrulamadan. Bu bölüm görev ızgarasının sonucunu **değiştirmez**; ayrı raporlanır.

| maske | kural | η (s1/s2/s3) | PPL ort. | Top-1 ort. | Top-5 ort. | test CE | train–val farkı |
|---|---|---|---|---|---|---|---|
| A: 1/1/1 (V20) | A | 0.0075/0.0075/0.005 | 198.89 | 15.43% | 30.80% | 5.2928 | 0.666 |
| A: 1/1/1 (V20) | B | 0.005/0.003/0.003 | 199.83 | 15.90% | 30.85% | 5.2975 | 0.542 |
| B: 0/1/1 | A | 0.01/0.015/0.01 | 201.99 | 16.35% | 30.22% | 5.3082 | 0.494 |
| B: 0/1/1 | B | 0.01/0.015/0.01 | 201.99 | 16.35% | 30.22% | 5.3082 | 0.494 |
| C: 0,25/1/1 | A | 0.015/0.015/0.01 | 199.81 | 15.97% | 30.48% | 5.2974 | 0.630 |
| C: 0,25/1/1 | B | 0.015/0.015/0.0075 | 199.97 | 16.08% | 30.54% | 5.2982 | 0.611 |
| D: 0,5/1/1 | A | 0.01/0.01/0.0075 | 198.88 | 15.81% | 30.71% | 5.2927 | 0.637 |
| D: 0,5/1/1 | B | 0.01/0.0075/0.005 | 199.32 | 15.96% | 30.70% | 5.2949 | 0.588 |
| E: 0,25/0,75/1 | A | 0.015/0.015/0.01 | 199.19 | 16.03% | 30.74% | 5.2942 | 0.644 |
| E: 0,25/0,75/1 | B | 0.015/0.01/0.0075 | 199.70 | 16.14% | 30.70% | 5.2968 | 0.592 |

- GLOBAL kural A (genişletilmiş): A_V20 η=0.0075 / D_sik.5 η=0.01 / A_V20 η=0.005 → PPL 199.84 / 197.73 / 199.27 (ort. **198.95**), Top-1 ort. 15.54%, Top-5 ort. 30.82%
- GLOBAL kural B (genişletilmiş): A_V20 η=0.005 / D_sik.5 η=0.0075 / D_sik.5 η=0.005 → PPL 199.33 / 198.37 / 200.16 (ort. **199.29**), Top-1 ort. 15.91%, Top-5 ort. 30.79%

## 6. Eski–yeni karşılaştırma (H=1024, test, 3 tohum ort.)

| Referans | PPL | Top-1 |
|---|---|---|
| V17 Ridge (bu çalışmada yeniden ölçüm) | 206.8 | 16.2% |
| V19 (rapor) | 199,9 | %14,9 |
| V20 A (yeniden üretildi) | 198.9 | 15.4% |
| V20 B (yeniden üretildi) | 199.8 | 15.9% |
| **V21 GLOBAL kural A** (doğrulama seçimi) | 198.9 | 15.5% |
| **V21 GLOBAL kural B** (doğrulama seçimi) | 199.3 | 15.9% |

## 7. Maliyet

- H=1024: Ridge çözümü ort. 1.35 s; CE düzeltmesi (gradyan + D, eğitim) ort. 6.02 s; son modelde (eğitim+doğrulama) 6.67 s. Maske uygulaması ek Cholesky/gradyan gerektirmez; her (maske, η) için yalnız `Z0 − η m Zd` eleman-bazlı işlemi (+ val kalibrasyon taraması) yapılır.
- Not: V20'ye göre toplam süre/RAM yüksek (48 s, 1,5 GB), çünkü bu koşu 21 (maske,η) × α,T taraması + eğitim logitleri (train–val farkı için) + 10 test raporu içeriyor; dağıtım maliyeti değil, deney maliyeti. Dağıtım maliyeti V20 ile aynı: 1 Ridge + 1 gradyan + 1 D.

| komut | wall s | user CPU s | RAM MB |
|---|---|---|---|
| `./tek_hamle_v21 2 1024 1 4 10` | 47.91 | 128.37 | 1494 |
| `./tek_hamle_v21 2 1024 2 4 100` | 44.23 | 125.81 | 1494 |
| `./tek_hamle_v21 2 1024 3 2 1` | 44.25 | 128.01 | 1494 |
| `./tek_hamle_v21 2 512 1 1 1` | 35.02 | 93.15 | 1306 |
| `./tek_hamle_v21 2 512 2 2 1` | 35.66 | 94.60 | 1306 |
| `./tek_hamle_v21 2 512 3 1 1` | 34.40 | 92.63 | 1306 |
| `./tek_hamle_v21_ext 2 1024 1 4 10` | 54.45 | 159.32 | 1494 |
| `./tek_hamle_v21_ext 2 1024 2 4 100` | 53.09 | 146.69 | 1494 |
| `./tek_hamle_v21_ext 2 1024 3 2 1` | 50.17 | 148.55 | 1494 |


## 8. Sınırlar ve dürüstlük notları

- Tek metin (Pride and Prejudice), tek veri bölünmesi, test n=4 980 → Top-1 standart hatası ≈ %0,5; 0,2–0,4 pp farklar gürültü sınırında. PPL'de ±1 puan tohum gürültüsü.
- Doğrulama Top-1 kısıtı (kural B, ±0,2 pp = ≈10 örnek) çok dar; global B kuralı bu yüzden çoğunlukla maskesiz V20 / D seçti. Kuralın test Top-1'ini garanti etmediği C ve E satırlarında da görülüyor.
- η ızgarası görevde 0,01'de bitiyor; B–E maskeleri çoğu zaman kenarda (0,01) seçildi → kenar seçimi. Bölüm 5 ek taramasında büyük η'nın test'te yardım etmediği görüldü, ama bu tarama yalnız H=1024.
- Maske katsayıları ve grup sınırları görevden alınan sabit değerler; ince ayar yapılmadı (ör. 0,75 sık katsayısı veya orta grup maskeleri denenmedi). [Tahmin] 0,5–0,75 aralığı Pareto cephesinde daha iyi bir nokta olabilir; bu denenmedi ve test üzerinden seçilmedi.
- "Maske ≈ sütun-bazlı η" yaklaşımı softmax bağlaşımı nedeniyle sık sütunları dolaylı etkiler: B maskesinde sık sütunların ağırlığı değişmez ama logit sıralaması orta/nadir sütunların yükselmesiyle yine kayar (sık Top-1 %27,3 → %26,9). 
- Süre: kodlama + 6 koşu + ek 3 koşu 20 dk sınırı içinde; H=512 için ek η taraması yapılmadı.

## Dosyalar

`cpp/tek_hamle_v21.cpp` (ana: `g++ -O3 -march=native -fopenmp -std=c++17`; ek tarama: `-DEXT`), `v21_ham/v21log/` (ana ham loglar + manifest), `v21_ham/v21ext/` (ek η taraması), `v21_ham/rapor_uretici.py` (tablo üretici). Komut: `./tek_hamle_v21 2 H TOHUM GAMMA LAMBDA` ile (γ,λ): H=1024 (4,10),(4,100),(2,1); H=512 (1,1),(2,1),(1,1).
