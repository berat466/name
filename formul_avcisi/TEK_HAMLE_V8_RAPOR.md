# Tek hamle V8 raporu — gerçek metin üzerinde sonraki-kelime tahmini

Branch: `claude/formul-avcisi`. Eski kod/raporlar korundu. Kod: `metin_hazirla.py` (veri), `cpp/tek_hamle_v8.cpp` (tek hamle, C++/OpenMP), `metin_ngram.py` (n-gram referansı). Ham loglar: `v8_ham/`.
Kural: gradyan yok, Adam yok, backprop yok, epok yok, RLS yok. Yalnız sunucu CPU/RAM (4 çekirdek, 16 GB).

## Ana sonuç (olumsuz/karma)
- [Kesin] **Tek hamle bu metinde işe yarıyor ama zayıf ve olasılık modeli olarak kötü.** Doğruluk (top-1) %13,8–14,6 ile unigram tabanını (%9,0) geçiyor, ama trigramın (%16,0) altında; top-5 %32–33 ile bigramın (%34,8) altında. Cross-entropy/perplexity ise **unigramdan bile kötü**: perplexity ≈ 530 (tek hamle) vs 281 (unigram) vs 137 (trigram).
- [Kesin] **W1'i veri çiftlerinden üretmek rastgele ELM'e göre doğruluğu artırıyor**: top-1 %13,8–14,6 vs %11,2–12,8; top-5 %32–33 vs %27–31; olasılık-MSE'si de daha iyi. Perplexity farkı küçük ve yön karışık (H=256'da rastgele daha iyi: 489–515 vs 533–536; H=512'de çift daha iyi: 530–537 vs 549–562).
- [Muhtemel] Perplexity'nin kötü olmasının nedeni model değil çıktı kalibrasyonu: ridge ile one-hot hedefe regresyon skorları olasılık değil (2001 sınıf, çoğu hedef sıfır), tek parametreli `softmax(z/T)` bunu düzeltmiyor. Bunu aşan bir kalibrasyon (ör. unigram ile karışım) denemedim.

## Veri ve ayrım (sızıntı önlemi)
- Kaynak: **Pride and Prejudice** (Jane Austen, 1813), Project Gutenberg #1342, ABD'de kamu malı. İlk **50 000 token** ("It is a truth universally acknowledged..." ile başlayıp), küçük harf, `[a-z]+('[a-z]+)?` sözcük belirteci (noktalama atılır). Ham metin repoya konmadı (`curl https://www.gutenberg.org/cache/epub/1342/pg1342.txt`).
- 500 tokenlık 100 **blok**, rastgele (tohum 0) 80 eğitim / 10 doğrulama / 10 test bloğu → 40 000 / 5 000 / 5 000 token. Bağlam pencereleri blok dışına taşmaz (blok başında ilk c konum atlanır), bloklar arası örtüşme yok. Hedef dışı bilgi sızmaz.
- Sözlük (en sık 2000 + `<unk>`, V=2001), PPMI-SVD kelime gömmeleri (64 boyut) ve n-gram sayımları **yalnız eğitim bloklarından**. `<unk>` oranı: eğitim %4,9, doğrulama %8,1, test %9,0. Meta: `v8_ham/meta.json`.
- Örnek sayıları: c=2 → 39 840 / 4 980 / 4 980; c=4 → 39 680 / 4 960 / 4 960 (eğitim/doğrulama/test).
- Hiperparametre (γ, λ ve çıktı sıcaklığı T) yalnız doğrulama kümesinde seçildi (doğrulama cross-entropy); son model eğitim+doğrulama istatistikleriyle (toplanabilir) çözüldü; test yalnız sonda BİR kez.

## Yöntemler
- **Giriş (sırayı koruyan bağlam)**: önceki c kelimenin 64 boyutlu gömmesinin **konuma göre birleştirilmesi** (K = 64·c; c=2 → 128, c=4 → 256 boyut). Gömme verisi eğitimden hesaplanan PPMI-SVD (kapalı form; geri yayılım yok).
- **Tek hamle (çift)**: H özellik; her özellik için **hedef kelimeleri farklı** iki eğitim satırının girdi farkı (birim norm) = `w_j`; bias `N(0, σ_b²)`, σ_b o çiftlerin orta-nokta bias'larının std'si (V3 kazanan yöntemi); `F=[1, tanh(γ(XW1+b1))]`; `W2=(FᵀF+λI)⁻¹FᵀT` (T: 2001 sınıflı one-hot), Cholesky, 256 satırlık FP64 birikim (V7).
- **Rastgele ELM**: aynı kod, W1~N(0,1/K), b1~N(0,0,5²).
- **Olasılık**: ham skorlara `softmax(z/T)`; T tek skaler, doğrulamada seçilir (ağırlık ögrenmesi değil).
- **n-gram**: lineer interpolasyon (ML olasılıkları + add-1 unigram + uniform), karışım ağırlıkları doğrulamada EM; son model eğitim+doğrulama sayımlarıyla. Unigram, bigram, trigram (c=2) ve 5-gram (c=4) ayrı ayrı.
- Metrikler (test): **MSE (ham skor)** = `mean((z − onehot)²)`; **MSE (olasılık)** = `mean((p − onehot)²)` (n-gram için de bu); **CE** = `−log p(doğru)` ortalaması (nat); **perplexity** = `exp(CE)`; top-1/top-5 doğruluk. Tüm modeller aynı test konumlarında değerlendirildi. Tabanlar: hep-sıfır tahmin ham-MSE'si = 1/V = 0,000500; düzgün dağılım CE = ln 2001 = 7,60.

## Tek hamle sonuçları (3 tohum ort. ± std; c=2, ve c=4 tek tohum)
| c (bağlam) | H | yöntem | tohum sayısı | test MSE (ham skor) | test MSE (olasılık) | test CE (nat) | perplexity | doğruluk top-1 | top-5 | tek model eğitim sn | ayar aramasıyla toplam sn | tepe RAM MB | seçilen γ / λ / T |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 2 | 256 | tek hamle, W1 veri çiftlerinden | 3 | 0.000484 ± 0.000000 | 0.000494 ± 0.000000 | 6.280 ± 0.002 | 534 ± 1 | 0.141 ± 0.001 | 0.319 ± 0.001 | 0.23 | 20.8 | 140 | 1/100/0.02, 1/100/0.02, 1/100/0.02 |
| 2 | 256 | tek hamle, rastgele ELM | 3 | 0.000487 ± 0.000000 | 0.000517 ± 0.000002 | 6.219 ± 0.021 | 502 ± 10 | 0.119 ± 0.003 | 0.292 ± 0.002 | 0.26 | 20.5 | 140 | 0.5/1000/0.01, 0.5/1000/0.01, 0.5/1000/0.01 |
| 2 | 512 | tek hamle, W1 veri çiftlerinden | 3 | 0.000483 ± 0.000000 | 0.000494 ± 0.000001 | 6.280 ± 0.006 | 534 ± 3 | 0.143 ± 0.001 | 0.324 ± 0.003 | 0.53 | 31.9 | 217 | 0.5/100/0.02, 2/1000/0.02, 2/1000/0.02 |
| 2 | 512 | tek hamle, rastgele ELM | 3 | 0.000486 ± 0.000000 | 0.000543 ± 0.000002 | 6.319 ± 0.010 | 555 ± 6 | 0.128 ± 0.000 | 0.310 ± 0.002 | 0.61 | 32.3 | 217 | 0.5/1000/0.01, 0.5/1000/0.01, 0.5/1000/0.01 |
| 4 | 256 | tek hamle, W1 veri çiftlerinden | 1 | 0.000485 | 0.000494 | 6.277 | 532 | 0.138 | 0.320 | 0.34 | 19.8 | 164 | 0.5/10/0.02 |
| 4 | 256 | tek hamle, rastgele ELM | 1 | 0.000489 | 0.000498 | 6.208 | 497 | 0.112 | 0.268 | 0.31 | 20.5 | 163 | 0.5/1000/0.01 |
| 4 | 512 | tek hamle, W1 veri çiftlerinden | 1 | 0.000484 | 0.000495 | 6.265 | 526 | 0.146 | 0.322 | 0.67 | 33.2 | 241 | 1/100/0.02 |
| 4 | 512 | tek hamle, rastgele ELM | 1 | 0.000487 | 0.000518 | 6.220 | 502 | 0.122 | 0.292 | 0.68 | 32.7 | 241 | 0.5/1000/0.01 |

(Süre: "tek model eğitim" = W1/b1 + eğitim+doğrulama istatistiği + 1 Cholesky/çözüm, ayar araması hariç. "Ayar aramasıyla toplam" = 16 (γ,λ) noktası × 9 sıcaklık + son çözüm + test.)

## n-gram referansı (aynı test konumları, tek koşu)
| c (konum kümesi) | model | MSE (olasılık) | CE (nat) | perplexity | top-1 | top-5 | eğitim sn | tepe RAM MB |
|---|---|---|---|---|---|---|---|---|
| 2 | unigram | 0.000492 | 5.639 | 281 | 0.0904 | 0.2255 | 0.28 | 29 |
| 2 | bigram | 0.000480 | 4.951 | 141 | 0.1562 | 0.3480 | 0.43 | 32 |
| 2 | 3-gram (trigram) | 0.000478 | 4.923 | 137 | 0.1600 | 0.3616 | 0.71 | 48 |
| 4 | unigram | 0.000492 | 5.637 | 281 | 0.0901 | 0.2254 | 0.26 | 48 |
| 4 | bigram | 0.000480 | 4.951 | 141 | 0.1560 | 0.3480 | 0.43 | 48 |
| 4 | 5-gram | 0.000479 | 4.923 | 137 | 0.1597 | 0.3615 | 1.19 | 118 |

(n-gram Python/numpy ile; tek hamle C++ ile. Süreler farklı uygulamalar; kıyas dikkatle yapılmalı.)

## Okuma
1. [Kesin] CE/perplexity: tek hamle (≈ 6,2–6,3 nat, PPL 490–560) **unigramdan (5,64 nat, PPL 281) ve n-gramdan (trigram 4,92 nat, PPL 137) belirgin kötü**. Düzgün dağılımdan (7,60 nat) iyi.
2. [Kesin] Doğruluk: tek hamle (çift) top-1 %13,8–14,6 > unigram %9,0, < bigram %15,6, < trigram %16,0; top-5 %31,8–32,7 < bigram %34,8 < trigram %36,2.
3. [Kesin] MSE: ham-skor MSE'si 0,000483–0,000489, hep-sıfır tabanının (0,000500) yalnız %2–3 altında; olasılık-MSE'si çift: 0,000493–0,000495 (unigram 0,000492, trigram 0,000478 daha iyi), rastgele: 0,000498–0,000546 (unigramdan kötü).
4. [Kesin] Çift ve rastgele farkı (3 tohum): top-1 H=256: 0,1410 vs 0,1192 (ort.), H=512: 0,1429 vs 0,1278; fark tohum std'sinden büyük. Etiketli (hedefleri farklı) çiftlerin kazancı bu metinde de görüldü.
5. [Kesin] Daha büyük H (512) doğruluğu çok az artırıyor (çift top-1 %14,1 → %14,3); c=4 (daha uzun bağlam) H=256'da hafif düşürüyor (%14,1 → %13,8), H=512'de hafif artırıyor (%14,3 → %14,6, tek tohum): net bir bağlam kazancı yok.
6. [Kesin] Süre/RAM: tek model eğitimi H=256'da ≈ 0,2 sn, H=512'de ≈ 0,5 sn; ayar aramasıyla 20 / 32 sn; tepe RAM 140 / 217 MB (c=4: 164 / 241). n-gram eğitimi 0,3–1,2 sn, 29–118 MB.
7. [Muhtemel] Neden zayıf: (a) 50 000 token çok az (3 848 tür, 2001'lik sözlük, hedef başına ortalama 20 örnek); (b) PPMI-SVD gömmeleri 64 boyutlu ve yalnız 40 000 tokenden; (c) 256–512 özellik, 2001 sınıfı ayırmak için az; (d) one-hot ridge çıktıları olasılık değil. Bunların hangisinin ne kadar etkili olduğunu ayrıca ölçmedim.

## Sınırlar
- Tek metin (tek yazar/dönem), 50 000 token; sonuçlar başka metinlere genellenmeyebilir. Test seti 5 000 token → doğruluk standart hatası ≈ ±0,5 puan.
- n-gram baseline'ı basit interpolasyon (Kneser–Ney değil); daha iyi bir n-gram daha da iyi olurdu.
- Gömmeler ve sözlük eğitimden kapalı formda üretildi; "tek hamle" yalnız W1 ve W2 için geçerli, gömme hesabı (SVD) ayrı bir adım (süresi tek model süresine dahil değil, `metin_hazirla.py` birkaç saniye).
- Çıktı kalibrasyonu `softmax(z/T)` ile sınırlı; CE/perplexity bu seçime bağlı.
- 20 dakikalık süre sınırı: tüm planlanan koşular (c=2: 3 tohum × 2 H × 2 yöntem; c=4: 1 tohum) tamamlandı, `atlanan.txt` yok. Ana deney 424 sn.
- 50 000'den fazla token (örn. 120 000+) denenmedi; veri arttıkça tek hamle ve n-gram kıyasının nasıl değişeceği bilinmiyor.

## Komutlar (gerçek)
Veri: `curl https://www.gutenberg.org/cache/epub/1342/pg1342.txt -o /tmp/pg1342.txt; python3 metin_hazirla.py`. Derleme: `g++ -O3 -march=native -fopenmp -o tek_hamle_v8 tek_hamle_v8.cpp`. n-gram: `python3 metin_ngram.py`. Tek hamle (`v8_ham/v8log/manifest.tsv`):

| komut | duvar sn | kullanıcı CPU sn | tepe RAM MB |
|---|---|---|---|
| `./tek_hamle_v8 cift 2 256 1` | 20.64 | 69.38 | 140 |
| `./tek_hamle_v8 rastgele 2 256 1` | 20.94 | 70.42 | 140 |
| `./tek_hamle_v8 cift 2 512 1` | 32.08 | 113.76 | 217 |
| `./tek_hamle_v8 rastgele 2 512 1` | 31.60 | 112.18 | 217 |
| `./tek_hamle_v8 cift 4 256 1` | 19.84 | 67.98 | 164 |
| `./tek_hamle_v8 rastgele 4 256 1` | 20.57 | 69.63 | 163 |
| `./tek_hamle_v8 cift 4 512 1` | 33.27 | 117.22 | 241 |
| `./tek_hamle_v8 rastgele 4 512 1` | 32.77 | 116.01 | 241 |
| `./tek_hamle_v8 cift 2 256 2` | 20.80 | 70.17 | 140 |
| `./tek_hamle_v8 rastgele 2 256 2` | 20.44 | 69.22 | 140 |
| `./tek_hamle_v8 cift 2 512 2` | 32.37 | 115.14 | 217 |
| `./tek_hamle_v8 rastgele 2 512 2` | 32.65 | 116.13 | 217 |
| `./tek_hamle_v8 cift 2 256 3` | 21.13 | 71.23 | 140 |
| `./tek_hamle_v8 rastgele 2 256 3` | 20.14 | 68.75 | 140 |
| `./tek_hamle_v8 cift 2 512 3` | 31.42 | 112.32 | 217 |
| `./tek_hamle_v8 rastgele 2 512 3` | 32.85 | 116.11 | 217 |

## Ham terminal çıktısı örneği (`./tek_hamle_v8 cift 2 256 1`)
```
V8 mod=cift c=2 H=256 tohum=1 V=2001 K=128 egitim=39840 dogrulama=4980 test=4980 (4 cekirdek)
  gamma=0.5 lambda=1 en_iyi_T=0.02 val_CE=6.4541
  gamma=0.5 lambda=10 en_iyi_T=0.02 val_CE=6.3795
  gamma=0.5 lambda=100 en_iyi_T=0.02 val_CE=6.4101
  gamma=0.5 lambda=1000 en_iyi_T=0.01 val_CE=6.5184
  gamma=1 lambda=1 en_iyi_T=0.02 val_CE=6.4887
  gamma=1 lambda=10 en_iyi_T=0.02 val_CE=6.4310
  gamma=1 lambda=100 en_iyi_T=0.02 val_CE=6.3606
  gamma=1 lambda=1000 en_iyi_T=0.02 val_CE=6.5056
  gamma=2 lambda=1 en_iyi_T=0.02 val_CE=6.5098
  gamma=2 lambda=10 en_iyi_T=0.02 val_CE=6.4734
  gamma=2 lambda=100 en_iyi_T=0.02 val_CE=6.3895
  gamma=2 lambda=1000 en_iyi_T=0.02 val_CE=6.4038
  gamma=4 lambda=1 en_iyi_T=0.02 val_CE=6.4545
  gamma=4 lambda=10 en_iyi_T=0.02 val_CE=6.4486
  gamma=4 lambda=100 en_iyi_T=0.02 val_CE=6.4112
  gamma=4 lambda=1000 en_iyi_T=0.02 val_CE=6.3654
SECILEN gamma=1 lambda=100 T=0.02 val_CE=6.3606
TEST mse_ham=0.000484 mse_olasilik=0.000494 CE=6.2788 PPL=533.13 top1=0.1400 top5=0.3199
SURE W1_uretim=0.001s tek_model_egitim(W1+istatistik(egitim+dogrulama)+1 Cholesky/cozum)=0.25s ayar_aramasi_dahil_toplam=20.60s
OZET|cift|c=2|H=256|tohum=1|gam=1|lam=100|T=0.02|valCE=6.3606|mse_ham=0.000484|mse_olas=0.000494|CE=6.2788|PPL=533.13|top1=0.1400|top5=0.3199|egitim_sn=0.25|toplam_sn=20.60|RAM_MB=140
```
