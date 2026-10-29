# TEK HAMLE V20 — CE düzeltmesini geliştirme (η ince tarama, Top-1 analizi, hız ayrıştırması)

Dal: `claude/formul-avcisi`. Kod: `cpp/tek_hamle_v20.cpp` (V19 koduna dokunulmadı). Ham loglar/komutlar: `v20_ham/v20log/`. Görev: tek Ridge + tek CE düzeltmesi `W_η = W0 − η·D`, `D=(FᵀF+λI)⁻¹FᵀG`, D bir kez hesaplanır (aynı Cholesky); G yalnız eğitim verisinden. Adam/epok/çoklu adım/RLS yok. Test yalnız nihai seçimden sonra.

Veri: Pride and Prejudice, ilk 50k token, V=2001, c=2, 39 840 / 4 980 / 4 980 örnek. Referans: V17 PPL 206,8 · V19 PPL 199,9 (Top-1 %14,9). Trigram PPL 137.

## Ana sonuç (test, nihai seçimden sonra)

| H | yöntem | PPL (3 tohum: s1/s2/s3) | PPL ort. | CE ort. | Top-1 ort. | Top-5 ort. | η seçimleri |
|---|---|---|---|---|---|---|---|
| 1024 | Ridge (η=0, kalibre) | 206.37 / 207.82 / 206.28 | **206.82** | 5.3318 | 16.18% | 29.99% | 0/0/0 |
| 1024 | V20 kural A (en düşük CE) | 199.84 / 197.56 / 199.27 | **198.89** | 5.2928 | 15.43% | 30.80% | 0.0075/0.0075/0.005 |
| 1024 | V20 kural B (CE, val Top-1 ≥ ridge−0,2 pp) | 199.33 / 200.16 / 200.01 | **199.83** | 5.2975 | 15.90% | 30.85% | 0.005/0.003/0.003 |
| 512 | Ridge (η=0, kalibre) | 213.10 / 211.43 / 212.48 | **212.34** | 5.3582 | 15.29% | 29.48% | 0/0/0 |
| 512 | V20 kural A (en düşük CE) | 202.31 / 203.42 / 202.52 | **202.75** | 5.3120 | 15.40% | 30.41% | 0.01/0.01/0.01 |
| 512 | V20 kural B (CE, val Top-1 ≥ ridge−0,2 pp) | 204.72 / 203.28 / 202.95 | **203.65** | 5.3164 | 15.48% | 30.52% | 0.005/0.0075/0.0075 |

Not: (γ,λ) V17 seçimleri korundu: H=1024 → (4,10),(4,100),(2,1); H=512 → (1,1),(2,1),(1,1). α,T her η için doğrulama CE ile seçildi (α0=0,2, T0=0,01 başlangıç kalibrasyonu ile gradyan). [Kesin: sayılar loglardan]

## Deney A — η ince taraması (doğrulama, kalibre CE; sayılar tohum 1 / H=1024)

**H=1024, tohum 1** — SECIM KURAL_A (en dusuk dogr. CE): eta=0.0075 (CE 5.3664, dogr. top1 0.1432) ; KURAL_B (CE ama dogr. top1 >= ridge-0.002): eta=0.005 (CE 5.3673, dogr. top1 0.1486)

| η | val CE | (α,T) | val Top-1 | val Top-5 | ham Top-1 | ΔW/W0 |
|---|---|---|---|---|---|---|
| 0 | 5.4179 | a=0.2 T=0.01 | 0.1492 | 0.2787 | 0.1476 | 0.000 |
| 0.001 | 5.4005 | a=0.2 T=0.01 | 0.1494 | 0.2831 | 0.1470 | 0.039 |
| 0.003 | 5.3776 | a=0.2 T=0.01 | 0.1520 | 0.2839 | 0.1490 | 0.118 |
| 0.005 | 5.3673 | a=0.25 T=0.01 | 0.1486 | 0.2892 | 0.1436 | 0.196 |
| 0.0075 | 5.3664 | a=0.25 T=0.01 | 0.1432 | 0.2890 | 0.1392 | 0.294 |
| 0.01 | 5.3705 | a=0.25 T=0.01 | 0.1426 | 0.2843 | 0.1347 | 0.392 |
| 0.015 | 5.3842 | a=0.2 T=0.01 | 0.1337 | 0.2785 | 0.1249 | 0.588 |
| 0.02 | 5.4054 | a=0.2 T=0.01 | 0.1213 | 0.2757 | 0.1151 | 0.783 |
| 0.03 | 5.4512 | a=0.15 T=0.01 | 0.1082 | 0.2673 | 0.0988 | 1.175 |

**H=1024, tohum 2** — SECIM KURAL_A (en dusuk dogr. CE): eta=0.0075 (CE 5.3658, dogr. top1 0.1434) ; KURAL_B (CE ama dogr. top1 >= ridge-0.002): eta=0.003 (CE 5.3825, dogr. top1 0.1482)

| η | val CE | (α,T) | val Top-1 | val Top-5 | ham Top-1 | ΔW/W0 |
|---|---|---|---|---|---|---|
| 0 | 5.4203 | a=0.2 T=0.01 | 0.1490 | 0.2777 | 0.1478 | 0.000 |
| 0.001 | 5.4053 | a=0.2 T=0.01 | 0.1498 | 0.2799 | 0.1484 | 0.037 |
| 0.003 | 5.3825 | a=0.25 T=0.01 | 0.1482 | 0.2871 | 0.1448 | 0.112 |
| 0.005 | 5.3699 | a=0.25 T=0.01 | 0.1462 | 0.2882 | 0.1396 | 0.187 |
| 0.0075 | 5.3658 | a=0.25 T=0.01 | 0.1434 | 0.2869 | 0.1365 | 0.280 |
| 0.01 | 5.3692 | a=0.25 T=0.01 | 0.1394 | 0.2867 | 0.1325 | 0.374 |
| 0.015 | 5.3867 | a=0.2 T=0.01 | 0.1313 | 0.2773 | 0.1223 | 0.561 |
| 0.02 | 5.4086 | a=0.2 T=0.01 | 0.1209 | 0.2721 | 0.1122 | 0.748 |
| 0.03 | 5.4478 | a=0.15 T=0.01 | 0.1090 | 0.2675 | 0.1006 | 1.122 |

**H=1024, tohum 3** — SECIM KURAL_A (en dusuk dogr. CE): eta=0.005 (CE 5.3706, dogr. top1 0.1458) ; KURAL_B (CE ama dogr. top1 >= ridge-0.002): eta=0.003 (CE 5.3782, dogr. top1 0.1498)

| η | val CE | (α,T) | val Top-1 | val Top-5 | ham Top-1 | ΔW/W0 |
|---|---|---|---|---|---|---|
| 0 | 5.4123 | a=0.2 T=0.01 | 0.1504 | 0.2821 | 0.1490 | 0.000 |
| 0.001 | 5.3977 | a=0.2 T=0.01 | 0.1516 | 0.2841 | 0.1494 | 0.039 |
| 0.003 | 5.3782 | a=0.2 T=0.01 | 0.1498 | 0.2882 | 0.1440 | 0.117 |
| 0.005 | 5.3706 | a=0.2 T=0.01 | 0.1458 | 0.2916 | 0.1394 | 0.194 |
| 0.0075 | 5.3713 | a=0.2 T=0.01 | 0.1404 | 0.2884 | 0.1355 | 0.291 |
| 0.01 | 5.3768 | a=0.2 T=0.01 | 0.1402 | 0.2851 | 0.1325 | 0.388 |
| 0.015 | 5.3928 | a=0.2 T=0.01 | 0.1305 | 0.2773 | 0.1239 | 0.583 |
| 0.02 | 5.4161 | a=0.2 T=0.01 | 0.1213 | 0.2735 | 0.1135 | 0.777 |
| 0.03 | 5.4492 | a=0.25 T=0.02 | 0.1251 | 0.2622 | 0.1028 | 1.165 |

**H=512, tohum 1** — SECIM KURAL_A (en dusuk dogr. CE): eta=0.01 (CE 5.4027, dogr. top1 0.1402) ; KURAL_B (CE ama dogr. top1 >= ridge-0.002): eta=0.005 (CE 5.4094, dogr. top1 0.1468)

| η | val CE | (α,T) | val Top-1 | val Top-5 | ham Top-1 | ΔW/W0 |
|---|---|---|---|---|---|---|
| 0 | 5.4459 | a=0.2 T=0.01 | 0.1460 | 0.2715 | 0.1440 | 0.000 |
| 0.001 | 5.4336 | a=0.2 T=0.01 | 0.1464 | 0.2757 | 0.1452 | 0.032 |
| 0.003 | 5.4177 | a=0.2 T=0.01 | 0.1482 | 0.2783 | 0.1458 | 0.096 |
| 0.005 | 5.4094 | a=0.25 T=0.01 | 0.1468 | 0.2849 | 0.1422 | 0.161 |
| 0.0075 | 5.4036 | a=0.25 T=0.01 | 0.1410 | 0.2845 | 0.1376 | 0.241 |
| 0.01 | 5.4027 | a=0.25 T=0.01 | 0.1402 | 0.2831 | 0.1349 | 0.321 |
| 0.015 | 5.4091 | a=0.25 T=0.01 | 0.1311 | 0.2869 | 0.1237 | 0.482 |
| 0.02 | 5.4182 | a=0.2 T=0.01 | 0.1237 | 0.2795 | 0.1147 | 0.642 |
| 0.03 | 5.4453 | a=0.2 T=0.01 | 0.1141 | 0.2733 | 0.1044 | 0.963 |

H=1024 seçilen η değerleri tarama içinde (0,003–0,0075, kenar değil); H=512 kural A üç tohumda η=0,01 seçti (grid'in 6. noktası, kenar değil). Tam tablolar için `v20_ham/v20log/`.

## Deney B — Top-1 neden düşüyor? (H=1024)

Geçişler (test, kalibre argmax / ham logit argmax; tohum başına):

| kural | tohum | kalibre doğru→yanlış | yanlış→doğru | net | ham doğru→yanlış | yanlış→doğru | net |
|---|---|---|---|---|---|---|---|
| A | 1 | 191 | 142 | -49 | 213 | 130 | -83 |
| A | 2 | 163 | 123 | -40 | 186 | 126 | -60 |
| A | 3 | 120 | 97 | -23 | 137 | 92 | -45 |
| B | 1 | 132 | 108 | -24 | 155 | 102 | -53 |
| B | 2 | 71 | 64 | -7 | 76 | 64 | -12 |
| B | 3 | 73 | 63 | -10 | 81 | 62 | -19 |

Sıklık grupları (yalnız eğitim hedef kitlesi: sık = kümülatif kütle <%50 → 44 tür; orta <%80 → 284 tür; nadir = kalan 1673 tür). Test, H=1024, tohum 1:

**V20_KURAL_A**

| grup | n | CE ridge | CE yeni | ΔCE | Top-1 ridge | Top-1 yeni | Top-5 ridge | Top-5 yeni |
|---|---|---|---|---|---|---|---|---|
| sik | 2693 | 3.6087 | 3.6500 | +0.0412 | 0.2733 | 0.2406 | 0.5162 | 0.5162 |
| orta | 1397 | 6.4425 | 6.3165 | -0.1260 | 0.0515 | 0.0730 | 0.0716 | 0.0988 |
| nadir | 890 | 8.7901 | 8.6835 | -0.1066 | 0.0000 | 0.0101 | 0.0034 | 0.0112 |

**V20_KURAL_B**

| grup | n | CE ridge | CE yeni | ΔCE | Top-1 ridge | Top-1 yeni | Top-5 ridge | Top-5 yeni |
|---|---|---|---|---|---|---|---|---|
| sik | 2693 | 3.6087 | 3.6199 | +0.0111 | 0.2733 | 0.2518 | 0.5162 | 0.5191 |
| orta | 1397 | 6.4425 | 6.3472 | -0.0953 | 0.0515 | 0.0702 | 0.0716 | 0.0909 |
| nadir | 890 | 8.7901 | 8.7120 | -0.0781 | 0.0000 | 0.0090 | 0.0034 | 0.0112 |

Bulgu: CE kazancı orta ve nadir kelimelerde (−0,08…−0,13 nat), sık kelimelerde CE neredeyse sabit/hafif kötü (+0,01…+0,04) ve sık kelimelerde Top-1 düşüyor (%27,3 → %24,1–25,2). Top-1 kaybı sık kelimelerden geliyor; düzeltme olasılık kütlesini orta/nadir kelimelere kaydırıyor. Ham softmax argmax da düşüyor (ham net negatif), yani kayıp kalibrasyondan değil, düzeltilmiş logitlerin kendisinden geliyor. [Muhtemel: mekanizma yorumu; sayılar Kesin]

Seçim kuralları H=1024 (3 tohum ort.): A → PPL 198.89, Top-1 15.43% ; B → PPL 199.83, Top-1 15.90% ; Ridge → PPL 206.82, Top-1 16.18%.

## Deney C — Hız ayrıştırması (tek geçiş, eğitim 39 840 satır, H=1024, saniye)

| tohum | sürüm | ileri gemm (F·W0) | softmax+G | transpoz | geri gemm (FᵀG) | birikim | Cholesky çözüm | toplam |
|---|---|---|---|---|---|---|---|---|
| 1 | orijinal | 2.364 | 0.454 | 0.424 | 3.070 | 0.026 | 0.204 | 6.542 |
| 1 | optimize | 2.337 | 0.235 | 0.322 | 2.990 | 0.023 | 0.178 | 6.085 |
| 2 | orijinal | 2.126 | 0.443 | 0.411 | 2.762 | 0.027 | 0.179 | 5.948 |
| 2 | optimize | 2.272 | 0.226 | 0.326 | 2.925 | 0.024 | 0.185 | 5.958 |
| 3 | orijinal | 2.174 | 0.436 | 0.411 | 2.941 | 0.025 | 0.206 | 6.192 |
| 3 | optimize | 2.212 | 0.227 | 0.362 | 2.933 | 0.024 | 0.186 | 5.944 |
| ort. | orijinal | 2.221 | 0.444 | 0.415 | 2.924 | 0.026 | 0.196 | 6.227 |
| ort. | optimize | 2.274 | 0.229 | 0.337 | 2.949 | 0.024 | 0.183 | 5.996 |

En pahalı iki kalem iki büyük matris çarpımı: ileri gemm + geri gemm ≈ 5.15 s / 6.23 s (%83). Softmax+G %7, transpoz %7, Cholesky çözümü (önceden faktörlenmiş) %3.

Optimizasyon (satır başına exp tek kez, iş parçacığı yerel tampon) yalnız softmax+G kalemini 0.444 → 0.229 s düşürdü; toplam 6.23 → 6.00 s (%3.7 kazanç). Büyük gemm'ler değişmedi; gerçek bir hızlanma **sağlanamadı** (kazanç ölçüm gürültüsü sınırında). Sayısal fark:
- tohum 1: optimize vs orijinal: ||G0-G1||/||G0||=2.505e-08 ; ||dW0-dW1||/||dW0||=1.747e-06 ; egitim CE toplam fark=0.000e+00
- tohum 2: optimize vs orijinal: ||G0-G1||/||G0||=2.407e-08 ; ||dW0-dW1||/||dW0||=1.202e-06 ; egitim CE toplam fark=0.000e+00
- tohum 3: optimize vs orijinal: ||G0-G1||/||G0||=2.330e-08 ; ||dW0-dW1||/||dW0||=3.333e-06 ; egitim CE toplam fark=0.000e+00

Not: ileri gemm, W0 ile logit hesabı; Ridge eğitiminde F·W0 zaten bir kez hesaplandıysa atlanabilir (matematik aynı) — bu denenmedi, ek tasarruf potansiyeli ≈2,3 s [Tahmin]. Geri gemm FᵀG (≈3 s) yarı-düşük-rank G veya float32 ile hızlanabilir ama matematik değişir; denenmedi.

## Süre ve bellek

| komut | wall s | user CPU s | RAM MB |
|---|---|---|---|
| `./tek_hamle_v20 2 1024 1 4 10` | 30.00 | 104.55 | 491 |
| `./tek_hamle_v20 2 1024 2 4 100` | 28.78 | 100.00 | 491 |
| `./tek_hamle_v20 2 1024 3 2 1` | 29.24 | 105.00 | 491 |
| `./tek_hamle_v20 2 512 1 1 1` | 15.91 | 57.23 | 329 |
| `./tek_hamle_v20 2 512 2 2 1` | 16.32 | 58.99 | 329 |
| `./tek_hamle_v20 2 512 3 1 1` | 16.57 | 59.08 | 329 |

## Ana soruya cevap

- **PPL**: H=1024 ort. Ridge 206.8 → kural A 198.9, kural B 199.8. V19 (tek η, 199,9) ile fark tohum gürültüsü (±1) içinde; ince η taraması PPL'yi belirgin biçimde 199,9'un altına indirmedi. [Kesin: ölçüm; Muhtemel: tohum gürültüsü yorumu]
- **Top-1**: kural B ort. 15.9% (Ridge 16.2%, V19 14,9%, kural A 15.4%). Kayıp yarıdan fazla azaldı ama kapanmadı; bedeli PPL'de ~+0.9. Doğrulama Top-1 kısıtı (4 980 örnek, SE≈%0,5) test Top-1'ini garanti etmez.
- **Hız**: düzeltme maliyetinin ~%85'i iki büyük gemm; softmax tek-exp optimizasyonu önemsiz kazandırdı. Maliyet azaltılamadı.
- **Sınırlar**: tek metin, tek bölme, n=4 980 test; PPL farkları 1–2 puan tohum gürültüsü ile aynı mertebede; H=512 kazancı benzer (ort. 212,3 → 202,8 / 203,7; ≈%4–4,5). Trigram (137) hâlâ çok uzakta; bu yaklaşım n-gram sayımını yakalamıyor. 20 dk sınırında 3 tohum A/B/C tamamlandı; H=512 için yalnız η taraması + iki kural.
