# Tek hamle V9 raporu — olasılık kalibrasyonuyla dil kaybını düşürme

Branch: `claude/formul-avcisi`. Referans: `TEK_HAMLE_V8_RAPOR.md`. V7/V8 dosyaları değişmedi (V9 yeni dosyalarda). Kural: gradyan/Adam/backprop/epok/RLS yok.
Kod: `cpp/tek_hamle_v9.cpp` (V8 kodunun kopyası + skor dökümü), `metin_kalibrasyon.py` (kalibrasyon). Ham loglar: `v9_ham/`.

## Ana sonuç
**Bu iyileşme yeni dil öğrenimi değil, yalnızca olasılık kalibrasyonudur.** W1 ve W2 aynı, tek hamle modelinin sıralama yeteneği (top-1 ≈ %14) aynı; değişen yalnızca skorların olasılığa çevrilmesi.
- [Kesin] **Şartnamedeki ızgarayla (T ∈ {0,25 … 4}) kayıp düşmedi**: doğrulama seçimi her tohumda α=0 (yalnız unigram) verdi → perplexity 281,15, model katkısı sıfır.
- [Kesin] **Genişletilmiş ızgarayla (T=0,005–4, α ince) perplexity ≈ 226–227 (H=256 ve H=512 ortalaması)**, V8'in ≈ 533'ünden %57 düşük, unigramın (281) %19 altında. α doğrulamada 0,20–0,25, T=0,01 seçildi (ızgara içinde, kenarda değil).
- [Kesin] **Trigramın perplexity'sine (137) inilemedi**: 226 vs 137.
- Test yalnız sonda kullanıldı; α,T yalnız doğrulama CE'siyle seçildi; olasılık toplamları 1,000000.

## Yöntem
`P = (1−α)·P_unigram + α·softmax(z/T)`; `z` = V8'in tek hamle skorları (`W2`'den). V8 skorları kaydetmediği için **aynı tohum ve aynı veriyle yeniden üretildi**; V8 sonucu birebir tekrarlandı (örn. H=256 tohum 1 test CE 6,2788 / PPL 533,13). Ağırlıklar değiştirilmedi; `tek_hamle_v9.cpp` yalnız skor dökümü ekler.
- Doğrulama skorları: seçilen (γ,λ)'nın **eğitim-yalnız** modelinden; test skorları **eğitim+doğrulama** modelinden (V8 protokolü). Unigram: doğrulama için eğitim sayımlarından, test için eğitim+doğrulama sayımlarından, add-1 yumuşatmalı.
- Seçim: α,T doğrulama CE'sinin en küçüğü. İki ızgara: (a) şartname: α ∈ {0,0.25,0.5,0.75,1}, T ∈ {0.25,0.5,1,2,4}; (b) genişletilmiş: α ∈ {0,.05,.1,.15,.2,.25,.35,.5,.75,1}, T ∈ {.005,.01,.02,.03,.05,.1,.25,.5,1,2,4}. Genişletme nedeni aşağıda.
- Veri ve bölme V8 ile aynı (Pride and Prejudice ilk 50 000 token; 80/10/10 blok bölme; c=2 → 39 840 / 4 980 / 4 980 örnek). 3 tohum (1,2,3).

## Sonuçlar (test; 3 tohum ort. ± std; unigram/n-gram tek koşu)
| H | model | α | T | doğrulama CE | test CE (nat) | **perplexity** | top-1 | top-5 | olasılık-MSE |
|---|---|---|---|---|---|---|---|---|---|
| 256 | unigram | - | - | - | 5.639 | 281.1 | 0.0904 | 0.2255 | 0.000492 |
| 256 | bigram (interpolasyonlu) | - | - | 5.013 | 4.951 | 141.3 | 0.1562 | 0.3480 | 0.000480 |
| 256 | **trigram** (interpolasyonlu) | - | - | 4.995 | 4.923 | 137.4 | 0.1600 | 0.3616 | 0.000478 |
| 256 | V8 (α=1, V8 sıcaklık ızgarası) | 1, 1, 1 | 0.02, 0.02, 0.02 | 6.361 ± 0.002 | 6.280 ± 0.002 | 533.9 ± 1.2 | 0.1410 ± 0.0010 | 0.3194 ± 0.0009 | 0.000494 ± 0.000000 |
| 256 | **V9 şartname ızgarası** (α∈{0,.25,.5,.75,1}, T∈{.25,.5,1,2,4}) | 0, 0, 0 | 0.25, 0.25, 0.25 | 5.695 ± 0.000 | 5.639 ± 0.000 | 281.1 ± 0.0 | 0.0904 ± 0.0000 | 0.2255 ± 0.0000 | 0.000492 ± 0.000000 |
| 256 | **V9 genişletilmiş ızgara** (T 0,005–4, α ince) | 0.25, 0.2, 0.2 | 0.01, 0.01, 0.01 | 5.514 ± 0.002 | 5.427 ± 0.002 | 227.4 ± 0.5 | 0.1431 ± 0.0009 | 0.2774 ± 0.0031 | 0.000484 ± 0.000000 |
| 512 | unigram | - | - | - | 5.639 | 281.1 | 0.0904 | 0.2255 | 0.000492 |
| 512 | bigram (interpolasyonlu) | - | - | 5.013 | 4.951 | 141.3 | 0.1562 | 0.3480 | 0.000480 |
| 512 | **trigram** (interpolasyonlu) | - | - | 4.995 | 4.923 | 137.4 | 0.1600 | 0.3616 | 0.000478 |
| 512 | V8 (α=1, V8 sıcaklık ızgarası) | 1, 1, 1 | 0.02, 0.02, 0.02 | 6.360 ± 0.005 | 6.280 ± 0.006 | 534.0 ± 3.0 | 0.1429 ± 0.0010 | 0.3236 ± 0.0031 | 0.000494 ± 0.000001 |
| 512 | **V9 şartname ızgarası** (α∈{0,.25,.5,.75,1}, T∈{.25,.5,1,2,4}) | 0, 0, 0 | 0.25, 0.25, 0.25 | 5.695 ± 0.000 | 5.639 ± 0.000 | 281.1 ± 0.0 | 0.0904 ± 0.0000 | 0.2255 ± 0.0000 | 0.000492 ± 0.000000 |
| 512 | **V9 genişletilmiş ızgara** (T 0,005–4, α ince) | 0.25, 0.25, 0.25 | 0.01, 0.01, 0.01 | 5.512 ± 0.004 | 5.421 ± 0.005 | 226.1 ± 1.1 | 0.1448 ± 0.0011 | 0.2792 ± 0.0007 | 0.000485 ± 0.000000 |

(Aynı test konumları. Unigram ve n-gram satırları H'den bağımsız; iki kez yazıldı. n-gram: V8 raporundaki interpolasyonlu modeller.)

## Neden şartname ızgarası işe yaramadı
- [Kesin] V8 skorlarının ölçeği çok küçük: doğrulama satırlarında standart sapma ≈ 0,004, bir satırdaki en yüksek skor ile ortalama arası ≈ 0,09, tüm skorlar yaklaşık [−0,10 ; 0,35] aralığında (tohum 1, H=256 ve 512). T ≥ 0,25 olduğunda `z/T` farkları en çok ≈ 1,4 → softmax neredeyse düzgün, doğru sınıfa anlamlı olasılık veremiyor; α>0 yalnız unigramı bozuyor, doğrulama α=0'ı seçiyor.
- [Kesin] Keskin softmax (T ≈ 0,01) skor farklarını anlamlı olasılığa çevirebiliyor, ama tek başına (α=1) kalibre değil (PPL 533); unigramla küçük pay (α≈0,25) karıştırınca kayıp düşüyor.

## Okuma
1. [Kesin] V8 → V9 genişletilmiş: test CE ≈ 6,28 → ≈ 5,42 nat, perplexity ≈ 533 → ≈ 226. Hedef 1 (281'in altı): yalnız genişletilmiş ızgarada sağlandı. Hedef 2 (137'nin altı): sağlanmadı.
2. [Kesin] top-1 korunuyor (3 tohum ort.: V8 %14,1–14,3 → V9 %14,3–14,5), ama **top-5 düşüyor** (V8 ≈ %32 → V9 ≈ %28, tablo): unigram önceliği, nadir ama doğru adayları geri itiyor. Kalibrasyon her metrikte bedavaya iyileşme sağlamıyor.
3. [Kesin] Olasılık-MSE'si: V9 genişletilmiş ≈ 0,000485 (V8: 0,000494, unigram: 0,000492, trigram: 0,000478) — unigramı geçiyor, trigramı geçmiyor.
4. [Muhtemel] Kalan fark (226 vs 137) model kapasitesinden: tek hamle modelinin bağlam bilgisi (top-1 %14) trigramınkinden (%16) zayıf ve doğru sınıfa verdiği skor ayrımı küçük. Kalibrasyonla kapatılabilir görünmüyor; model iyileştirmeleri (daha çok veri, daha iyi gömme) ayrı iş. Ölçülmedi.
5. [Kesin] Ağırlık hesabı değişmedi; kalibrasyon ek, ucuz bir sayısal adım.

## Süre ve RAM
| H | tek hamle eğitim sn (V8 ağırlıkları, ayar aramasız) | skor üretimi dahil C++ koşusu duvar sn | C++ tepe RAM MB | kalibrasyon (python/numpy) CPU sn | kalibrasyon tepe RAM MB |
|---|---|---|---|---|---|
| 256 | 0.24 | 25.2 | 183 | 3.5 | 341 |
| 512 | 0.62 | 40.9 | 303 | 3.3 | 341 |

(C++ koşusu V8 ile aynı (γ,λ) arama ızgarasını yeniden çalıştırır + iki ek model çözümü (eğitim-yalnız doğrulama skorları) + skor dökümü, bu yüzden duvar süresi V8'den biraz uzun. Kalibrasyon CPU süresi `time.process_time` ile, üç ızgara (V8 sıcaklık ızgarası, şartname, genişletilmiş) ve test değerlendirmesi dahil.)

## Sınırlar ve dürüstlük notları
- Tek metin, 50 000 token; test 4 980 örnek. Sonuçlar tek yazar/dönem içindir.
- Seçim ızgarasını şartnamenin ötesine genişlettim; bu, şartname ızgarasında doğrulamanın kenar durumu α=0'ı seçmesi yüzünden gerekti. Genişletilmiş sonuç şartname dışı, ayrı raporlandı; ikisi de gösterildi.
- Kalibrasyon yalnız unigramla karışım; n-gram ile karışım **denenmedi** (n-gramın bilgisi karışır, tek hamle katkısını ayırmak zorlaşırdı).
- Tek veri bölünmesi, 3 tohum (W1 tohumları).
- Test yalnız sonda; hiçbir test sonucu α,T seçimini etkilemedi.

## Komutlar (gerçek)
Çalıştırma: `./tek_hamle_v9 cift 2 H TOHUM` (H=256,512; tohum=1,2,3), ardından `python3 -I metin_kalibrasyon.py cift 2 H TOHUM`. Derleme: `g++ -O3 -march=native -fopenmp -o tek_hamle_v9 tek_hamle_v9.cpp`. Veri: V8'in `metin_hazirla.py` çıktısı (`/tmp/v8data`). C++ koşuları (`v9_ham/v9log/manifest.tsv`):

| komut | duvar sn | kullanıcı CPU sn | tepe RAM MB |
|---|---|---|---|
| `./tek_hamle_v9 cift 2 256 1` | 27.36 | 83.12 | 183 |
| `./tek_hamle_v9 cift 2 512 1` | 44.46 | 148.20 | 303 |
| `./tek_hamle_v9 cift 2 256 2` | 24.04 | 79.58 | 183 |
| `./tek_hamle_v9 cift 2 512 2` | 39.41 | 136.41 | 303 |
| `./tek_hamle_v9 cift 2 256 3` | 24.25 | 80.00 | 183 |
| `./tek_hamle_v9 cift 2 512 3` | 38.91 | 135.28 | 303 |

## Ham terminal çıktısı örneği (`python3 -I metin_kalibrasyon.py cift 2 256 1`; tüm koşular `v9_ham/v9log/kalibrasyon.txt`)
```
OZET_V9|unigram|mod=cift|c=2|H=256|tohum=1|test_CE=5.6389|PPL=281.15|top1=0.0904|top5=0.2255|mse_olasilik=0.000492
OZET_V9|V8_alpha1_V8_T_izgarasi|mod=cift|c=2|H=256|tohum=1|alpha=1.0|T=0.02|val_CE=6.3606|test_CE=6.2788|PPL=533.13|top1=0.1400|top5=0.3199|mse_olasilik=0.000494|olasilik_toplami=1.000000
OZET_V9|V9_sartname_izgarasi|mod=cift|c=2|H=256|tohum=1|alpha=0|T=0.25|val_CE=5.6947|test_CE=5.6389|PPL=281.15|top1=0.0904|top5=0.2255|mse_olasilik=0.000492|olasilik_toplami=1.000000
OZET_V9|V9_genisletilmis_izgara|mod=cift|c=2|H=256|tohum=1|alpha=0.25|T=0.01|val_CE=5.5122|test_CE=5.4246|PPL=226.91|top1=0.1422|top5=0.2817|mse_olasilik=0.000485|olasilik_toplami=1.000000
CPU_SURE|cift_c2_H256_s1|kalibrasyon_cpu_sn=4.15|duvar_sn=3.97|RAM_MB=341

```
