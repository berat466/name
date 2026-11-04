# Tek hamle V10 raporu — ilişki özellikleri (kelime farkı, etkileşim) ile dil öğrenimi

Branch: `claude/formul-avcisi`. Referans: `TEK_HAMLE_V9_RAPOR.md`. Eski kodlar değişmedi (V10 yeni dosyalarda). Kural: gradyan/Adam/backprop/epok/RLS yok; tek çözüm `W2=(FᵀF+λI)⁻¹FᵀT` (Cholesky, 256 satırlık FP64 birikim).
Kod: `cpp/tek_hamle_v10.cpp` (V9 kopyası + özellik genişletme), `metin_kalibrasyon_v10.py` (V9 kalibrasyonu, V10 skor dizini). Veri/bölme: V8 ile aynı (Pride and Prejudice ilk 50 000 token; 80/10/10 blok). Ham loglar: `v10_ham/`.

## Ana soru ve kısa cevap
**Kelimeler arası ilişkileri (fark, etkileşim) özellik yapmak tek hamle ağırlıklarının kapasitesini artırıyor mu?** — Çok az, yalnız büyük H'de; hedefe ulaşılamadı.
- [Kesin] H=512'de **kelime farkı** perplexity'yi V9'un ≈ 226,1'inden ≈ 217,0'a indiriyor (3 tohum; tohum std'si ≈ 0,2; her tohumda aynı yön; %4 düşüş). H=256'da fark yok (226,9 → 226,6).
- [Kesin] **Etkileşim (I) ek kazanç vermiyor**: fark+etkileşim 217,1 (fark tek başına 217,0).
- [Kesin] 4 kelimelik bağlam (c=4, fark+etkileşim, H=512, tek tohum) **iyileştirmedi**: PPL 226,9.
- [Kesin] **Trigramın perplexity'sine (137) inilemedi**: en iyi ≈ 217. Unigramın (281) altında; bu V9'daki aynı kalibrasyon sayesinde.
- Kalibrasyon V9'daki gibi `P=(1−α)P_unigram+α softmax(z/T)`: α ve T yalnız doğrulama CE ile seçildi, test sonda. Şartname ızgarası yine α=0 (PPL 281,15) seçti, genişletilmiş ızgara (T 0,005–4, α ince) kullanıldı.

## Yöntem
Giriş: önceki 2 (veya 4) kelimenin 64 boyutlu PPMI-SVD gömmesi (V8, yalnız eğitimden), konuma göre birleştirilmiş: `X=[e_{t−2}, e_{t−1}]`. Genişletme (`oz`):
- `taban`: `X` (V9 referansı).
- `fark`: `X' = [X, D]`, `D = e_{t−1} − e_{t−2}` (c=4'te komşu çiftlerin üç farkı).
- `fark_etk`: `X' = [X, D, I]`, `I = e_{t−1} ⊙ e_{t−2}` (c=4'te üç komşu çarpım).
`W1 = PairDirections(X')` (V3'ün kazanan yöntemi: hedefleri farklı iki eğitim satırının **X' farkı**, birim norm; bias `N(0,σ_b²)`); `F=[1, tanh(γ(X'W1+b))]`; `W2=(FᵀF+λI)⁻¹FᵀT` (T: 2001 sınıflı one-hot). (γ,λ) V8 ızgarasında doğrulama CE ile seçilir. Bloklar ayrıca ölçeklenmedi (I elemanları D'ye göre küçük, ≈ 0,015).

## Tohum 1 — tüm yöntemler
| bağlam c | H | özellik | α=1 (V8-tarzı) PPL | şartname ızgarası PPL | **genişletilmiş ızgara** test CE | **PPL** | top-1 | top-5 | doğr. CE | ağırlık üretimi sn | toplam sn (ayar dahil) | tepe RAM MB |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 2 | 256 | taban | 533.1 | 281.1 | 5.425 | **226.9** | 0.1422 | 0.2817 | 5.5122 | 0.26 | 20.4 | 199 |
| 2 | 256 | fark | 537.9 | 281.1 | 5.423 | **226.6** | 0.1406 | 0.2791 | 5.5096 | 0.27 | 20.6 | 213 |
| 2 | 256 | fark_etk | 537.7 | 281.1 | 5.423 | **226.5** | 0.1402 | 0.2791 | 5.5096 | 0.25 | 20.2 | 226 |
| 2 | 512 | taban | 537.5 | 281.1 | 5.428 | **227.7** | 0.1442 | 0.2783 | 5.5187 | 0.61 | 30.4 | 303 |
| 2 | 512 | fark | 521.1 | 281.1 | 5.379 | **216.8** | 0.1492 | 0.2900 | 5.4671 | 0.68 | 31.8 | 317 |
| 2 | 512 | fark_etk | 521.6 | 281.1 | 5.379 | **216.9** | 0.1484 | 0.2900 | 5.4674 | 0.63 | 32.1 | 329 |
| 4 | 512 | fark_etk | 542.6 | 280.6 | 5.425 | **226.9** | 0.1440 | 0.2780 | 5.5057 | 0.91 | 33.1 | 400 |

(Referanslar: unigram PPL 281,1; trigram 137,4. "α=1 V8-tarzı" = sıcaklık seçilmiş saf softmax; "genişletilmiş ızgara" = V9 kalibrasyonu.)

## Tohum doğrulaması (c=2, H=512, 3 tohum)
| özellik (c=2, H=512) | PPL tohum 1 / 2 / 3 | PPL ort. ± std | test CE ort. | top-1 ort. | top-5 ort. | doğr. CE ort. | α / T (seçilen) |
|---|---|---|---|---|---|---|---|
| taban | 227.7 / 225.2 / 225.5 | 226.1 ± 1.1 | 5.421 | 0.1448 | 0.2792 | 5.5124 | 0.25/0.01, 0.25/0.01, 0.25/0.01 |
| fark | 216.8 / 216.9 / 217.3 | 217.0 ± 0.2 | 5.380 | 0.1497 | 0.2893 | 5.4732 | 0.25/0.01, 0.2/0.01, 0.2/0.01 |
| fark_etk | 216.9 / 217.0 / 217.4 | 217.1 ± 0.2 | 5.380 | 0.1493 | 0.2897 | 5.4733 | 0.25/0.01, 0.2/0.01, 0.2/0.01 |

Tohum başına PPL farkı (taban − fark): 10.9 / 8.3 / 8.2.

## Okuma
1. [Kesin] İlişki özellikleri H=512'de küçük ama tutarlı kazanç sağladı (PPL ≈ −9, %4; doğrulama CE 5,512 → 5,473). H=256'da kazanç yok. [Muhtemel] özellik sayısı artışa yetmiyor; H=1024 denenmedi.
2. [Kesin] Etkileşim `I` katkısız: aynı PPL (217,1 vs 217,0), aynı doğrulama CE. [Muhtemel] I elemanları küçük olduğundan çift-yön farkında neredeyse etkisiz; ölçekleme denenmedi.
3. [Kesin] c=4 daha uzun bağlam yardım etmedi (226,9). [Muhtemel] aynı veride bağlam seyreliyor.
4. [Kesin] Hedef 137 ile fark çok büyük (≈ 217 vs 137): tek hamle modeli, bu veri/gömme/özellik düzeninde trigramın bilgisini yakalamıyor. V10 yalnız küçük bir kapasite artışı gösterdi.
5. Süre/RAM: ağırlık üretimi (W1+istatistik+1 çözüm) H=256'da ≈ 0,25 sn, H=512'de ≈ 0,6–0,7 sn (c=4: 0,9 sn); özellik genişletmenin maliyeti ihmal edilebilir. Ayar dahil toplam 20 / 32 sn; tepe RAM 199–329 MB (c=4: 400 MB; skor dökümü dahil, V8'den yüksek). Kalibrasyon ≈ 2.9 sn CPU/koşu, ≈ 340 MB (numpy).

## Sınırlar
- Tek metin, 50 000 token; yalnız H=256/512; H=1024 ve ölçeklenmiş özellik denenmedi.
- 3 tohum yalnız c=2, H=512 için; H=256 ve c=4 tek tohum.
- Kazanç küçük (%4) ve tek bir (H) kurulumda; genelleme iddiası yok.
- Test yalnız sonda; α, T, γ, λ yalnız doğrulamadan.
- Süre sınırı (20 dk) nedeniyle H=256 için tohum doğrulaması yapılmadı.

## Komutlar (gerçek)
Derleme: `g++ -O3 -march=native -fopenmp -o tek_hamle_v10 tek_hamle_v10.cpp`. Çalıştırma: `./tek_hamle_v10 cift c H TOHUM OZELLIK` (OZELLIK ∈ taban|fark|fark_etk); kalibrasyon: `python3 -I metin_kalibrasyon_v10.py cift_OZELLIK c H TOHUM`. Ölçülen süreler/RAM (`v10_ham/v10log/manifest.tsv`):

| komut | duvar sn | kullanıcı CPU sn | tepe RAM MB |
|---|---|---|---|
| `./tek_hamle_v10 cift 2 256 1 taban` | 20.50 | 66.30 | 199 |
| `./tek_hamle_v10 cift 2 512 1 taban` | 30.40 | 103.56 | 303 |
| `./tek_hamle_v10 cift 2 256 1 fark` | 20.67 | 66.05 | 213 |
| `./tek_hamle_v10 cift 2 512 1 fark` | 31.88 | 108.80 | 317 |
| `./tek_hamle_v10 cift 2 256 1 fark_etk` | 20.24 | 65.13 | 226 |
| `./tek_hamle_v10 cift 2 512 1 fark_etk` | 32.21 | 110.06 | 329 |
| `./tek_hamle_v10 cift 4 512 1 fark_etk` | 34.84 | 112.43 | 400 |
| `./tek_hamle_v10 cift 2 512 2 fark` | 30.38 | 103.58 | 317 |
| `./tek_hamle_v10 cift 2 512 3 fark` | 29.46 | 101.20 | 317 |
| `./tek_hamle_v10 cift 2 512 2 taban` | 28.74 | 99.10 | 303 |
| `./tek_hamle_v10 cift 2 512 3 taban` | 29.37 | 100.77 | 303 |
| `./tek_hamle_v10 cift 2 512 2 fark_etk` | 30.15 | 103.48 | 330 |
| `./tek_hamle_v10 cift 2 512 3 fark_etk` | 32.09 | 110.11 | 329 |

## Ham terminal çıktısı örneği (`metin_kalibrasyon_v10.py cift_fark 2 512 1`)
```
OZET_V9|unigram|mod=cift_fark|c=2|H=512|tohum=1|test_CE=5.6389|PPL=281.15|top1=0.0904|top5=0.2255|mse_olasilik=0.000492
OZET_V9|V8_alpha1_V8_T_izgarasi|mod=cift_fark|c=2|H=512|tohum=1|alpha=1.0|T=0.02|val_CE=6.3596|test_CE=6.2559|PPL=521.09|top1=0.1502|top5=0.3371|mse_olasilik=0.000501|olasilik_toplami=1.000000
OZET_V9|V9_sartname_izgarasi|mod=cift_fark|c=2|H=512|tohum=1|alpha=0|T=0.25|val_CE=5.6947|test_CE=5.6389|PPL=281.15|top1=0.0904|top5=0.2255|mse_olasilik=0.000492|olasilik_toplami=1.000000
OZET_V9|V9_genisletilmis_izgara|mod=cift_fark|c=2|H=512|tohum=1|alpha=0.25|T=0.01|val_CE=5.4671|test_CE=5.3791|PPL=216.82|top1=0.1492|top5=0.2900|mse_olasilik=0.000483|olasilik_toplami=1.000000
CPU_SURE|cift_fark_c2_H512_s1|kalibrasyon_cpu_sn=2.61|duvar_sn=2.41|RAM_MB=341
```
