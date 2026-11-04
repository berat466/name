# V31/V32 HIZ ÇELİŞKİSİ DENETİMİ

Dal `claude/formul-avcisi` · ölçüm kodu `cpp/tek_hamle_v31_sure.cpp` (V31 ile **aynı hesap**, yalnız zamanlayıcılar eklendi; PPL değişmedi: aynı koşuda test PPL 139,78 / 144,49) ve `v32_sure_ham/mlp_sure_olcum.py` (V32'nin seçtiği MLP'nin tek koşusunun zaman dökümü) · ham çıktılar `v32_sure_ham/`. Makine: 4 çekirdek Xeon 2,8 GHz (tek iş parçacığı/çekirdek), ölçümler **ardışık, başka iş çalışmadan** yapıldı.

Her satır etiketli: **[ölçüldü]** bu denetimde, **[önceki rapor]** loglardan alındı, **[bileşenlerden hesap]** ölçülmüş parçalardan toplanan (doğrudan ölçülmedi), **[bilinmiyor]**.

## 1. Bulgu özeti

- **"40–60 s" tek bir ağırlık üretimi değil**, tam V31 hattıdır: veri/özellik hazırlığı + yalnız-kimlik için λ_C adayları + ortak çözüm + eğitim logitleri ve rapor tanıları dahil. Sabit hiperparametreyle **tek ağırlık üretimi ≈ 14,6–14,8 s** [ölçüldü, 2 ölçüm]; bunun 11–12 s'si matematiksel çözüm.
- **Klasik MLP'nin "13–25 s"'si** erken durdurmalı **tek eğitim koşusudur** (12 epok, 1872 güncelleme, her epokta doğrulama dahil). Hiperparametre seçimi dahil toplam V32'de **≈216 s** (id+X: 6 aday + 3 tohum) [V32 logundan hesap].
- İki süre **aynı işi ölçmüyordu**: ben V32'de bunları yan yana "40–60 s vs 13–25 s" diye yazdım; bu, taramalı/tanılı bir hattı tek koşuyla karşılaştırmaktı.
- Aynı kalitede (doğrulama PPL ≈147) klasik MLP **≈7 s**, bizim sabit-hiperparametreli V31 **≈15 s** [ikisi ölçüldü, tek tohum] → bu kalite düzeyinde klasik yöntem şu an daha hızlı.

## 2. Zorunlu karşılaştırma tablosu

| Ölçüm | Bizim C++ (V31, H=512, tohum 1, λ_C=100, λ_F=3000) | Klasik MLP (id+X, V32'nin seçtiği yapı, tohum 0) |
|---|---|---|
| Veri/özellik hazırlığı | **2,7–3,8 s** [ölçüldü, 2 koşu]: dosya okuma 0,02–0,04 + `expand_s` 0,02 + F zinciri+Gram+RHS 1,3–2,2 + C sayımları/Gram (ACC, ACF, R_C) 1,3–1,5 | **0,58 s** veri yükleme [ölçüldü] (+ `import torch` 13,4 s ve ilk model kurma 4,3 s — soğuk başlatma/kütüphane yükü, yazılım ortamı; MLP'nin hesabı değil) |
| Tek ağırlık üretimi / tek eğitim koşusu | **11,1–12,0 s** matematik [ölçüldü]: Cholesky(A_CC+λ_C I, 4002²) 3,9–4,2 + Y₀ çözümü (2001 sağ taraf) 4,3–4,8 + A_CC kopyası 0,8 + X çözümü 0,85–0,9 + Schur parçaları 0,7–0,8 + Schur sistemi 0,46. **Hazırlıkla birlikte ≈ 14,6–14,8 s** | **21,5 s** [ölçüldü]: 12 epok × 156 mini-batch = **1872 AdamW güncellemesi**, eğitim adımları 21,1 s + epok başı doğrulama 0,36 s; epok 1 = 3,7 s (ısınma), sonraki ≈1,55 s/epok; **en iyi doğrulama 6. epokta** (≈12 s), kalan ≈9,5 s early-stopping sabrı (patience 6) |
| Hiperparametre seçimi dahil toplam | **Doğrudan ölçülmedi** [bilinmiyor]. Ölçülenler: tohum 1 taraması 64,0 + 63,5 = **127,5 s** (2 koşu, 7 λ_C Cholesky'si, 13 λ_F, ID-only tanıları) ve tohum 2/3 koşuları 42,2 / 40,0 s [önceki rapor; V31 manifestosu]. Bileşenlerden hesap (3 λ_C × 5 λ_F): ≈ 3×(M1–M5 ≈ 9,5–10,6) + 15×(0,45+0,45) + 3 ≈ **45–50 s** [bileşenlerden hesap] | **163,8 s** (6 aday) **+ 51,9 s** (3 tohum) = **215,7 s** [V32 logu `klasik_sonuclar.jsonl`'dan hesap]; tek aday süreleri 17–41 s |
| Test/tahmin süresi | **0,43 s** [ölçüldü]: test logitleri 0,36 + PPL/Top-k 0,07 (dogrulamada α,ε seçimi: ayrıca 0,45 s) | **0,03 s** [ölçüldü] |
| Test PPL | **140,3** (3 tohum, V31) | **122,9** (3 tohum, V32) |

Not: `mlp_sure_olcum.py`'nin kendi test PPL'i 132,5 çıktı, çünkü bu süre-ölçüm betiği **en iyi checkpoint'i geri yüklemiyor** (son epok ağırlıkları); V32'nin 122,9'u en iyi doğrulama checkpoint'iyle. Zaman sonuçları bundan etkilenmez; PPL tablosu V32 değerleridir ve değişmedi.

## 3. 40–60 saniye neleri kapsıyor? (tam döküm, tohum 1 sabit hiperparametre koşusu, toplam wall 24,5–25,0 s)

| Kalem | Süre | Zorunlu mu? |
|---|---|---|
| Veri okuma + `expand_s` | 0,04–0,07 s | evet |
| F zinciri (X'ten 4 aşama) + F Gram blokları + FᵀY | 1,3–2,2 s | evet |
| C sayımları, A_CC/A_CF/R_C kurulumu (seyrek, 4002²'lik Gram 128 MB) | 1,3–1,5 s | evet |
| M1–M5: A_CC kopya, Cholesky 4002², Y₀ çözümü, X çözümü, Schur parçaları | 10,6–11,5 s | **evet (ortak çözüm)** |
| M6: Schur 513 sistemi + W_C yeniden yapımı (λ_F başına) | 0,46 s | evet |
| α,ε seçimi + doğrulama değerlendirmesi (E1+E2) | ≈0,95 s | okuma kalibrasyonu için gerekli |
| Test logit + PPL | 0,43 s | tahmin |
| **Eğitim logitleri (5,3 s) + eğitim PPL (0,6 s) + kategori/parça okumaları + rezidü (≈ 8,7 s toplam "E3")** | **≈ 8,7 s** | **hayır — yalnız rapor/tanı** |

Tam V31 koşularında (42/40 s ve 64 s) ek olarak: yalnız-kimlik referansı için **her λ_C adayı ≈ 9–11 s** (Cholesky + Y₀ çözümü + doğrulama), λ_F ızgarası (≈0,9 s/değer) ve parça okumaları var. **40–60 s'nin yaklaşık 15 s'si zorunlu hesap, ≈ 9 s'si rapor tanıları, geri kalanı (≈ 15–35 s) hiperparametre taraması/referans çözümleridir** [bileşenlerden hesap; tohum 2/3 koşusu: 3 λ_C ≈ 28 s].

## 4. Klasik MLP'nin 13–25 s'si neleri kapsıyor?

V32 logundaki `egitim_suresi_s`, `train_mlp` fonksiyonunun süresi: model kurma + **epok döngüsü (mini-batch 256, AdamW) + her epokta tüm doğrulama kümesi değerlendirmesi + early stopping** (patience 6, en fazla 80 epok) + en iyi durumun geri yüklenmesi. Kapsamadıkları: veri yükleme, `import torch`, hiperparametre ızgarası (diğer 5 aday), test değerlendirmesi, 3 tohum çoğaltması. Seçilen id+X yapısı: 12 epok (11–12), 627 697 parametre. "13–25 s" **seçilmiş tek koşunun** aralığıdır (MLP-X 13–16 s, MLP-id 22–25 s, id+X 17–18 s); ızgaradaki büyük adaylar 41–53 s sürdü.

## 5. Koşullar aynı mı?

| | C++ (V31) | PyTorch MLP |
|---|---|---|
| Donanım | aynı 4 çekirdek makine | aynı |
| İş parçacığı | OpenMP, `omp_get_max_threads()=4` | `torch.set_num_threads(4)` |
| Lineer cebir | **elle yazılmış çekirdekler** (`gemm_f`, paralel Cholesky) — BLAS/LAPACK **yok** | PyTorch CPU (oneDNN/MKL türü optimize çekirdekler) |
| Veri | aynı 39 840 eğitim örneği, aynı bölünme | aynı |
| Ölçüm kapsamı | tam hat (hazırlık+çözüm+tarama+tanı) farklı koşularda | yalnız eğitim fonksiyonu (+ayrı veri yükleme/import) |
| Çözümün doğası | doğrudan, ama **4002 boyutlu yoğun** sistem × 2001 sağ taraf | 1872 küçük SGD adımı (627 bin parametre) |
| Yazılım başlangıcı | yok (derlenmiş) | `import torch` 13,4 s (bu raporda dışarıda bırakıldı) |

Not: Cholesky 4002² ≈ 2,1·10¹⁰ flop / 4,0 s ≈ **5 GFLOP/s** — 4 çekirdek için çok düşük. Optimize BLAS ile bu kalemin ve Y₀ çözümünün belirgin küçülmesi **olası ama ölçülmedi** [bilinmiyor/tahmin].

## 6. Esas sorular

**1. Tek hamle kullanmamıza rağmen klasik MLP neden daha hızlı görünüyor?** Üç ayrı sebep, ölçümlerle: (a) *"tek hamle" ucuz hamle demek değil*: V31'in tek hamlesi 4002 boyutlu yoğun normal denklemin Cholesky'si ve 2001 sağ tarafın çözümü (≈ 11 s); küçük bir MLP bu veride ≈ 1900 küçük adımda ≈ 12–21 s'de bitiyor ve 3 epokta (≈ 7 s) bizim kalite düzeyine geliyor. (b) *Karşılaştırdığım süreler farklı kapsamlıydı*: taramalı/tanılı 40–60 s'lik hat ile tek eğitim koşusu. (c) bizim çekirdekler optimize BLAS değil.

**2. Bizim gerçek tek-hamle ağırlık üretimimiz kaç saniye?** Sabit hiperparametreyle **≈ 14,6–14,8 s** (hazırlık 2,7–3,8 + matematik 11,1–12,0), tohum 1, H=512, ardışık iki ölçümde tutarlı. Üstüne α,ε okuma seçimi +0,45 s ve test tahmini +0,43 s.

**3. 40–60 s'nin ne kadarı zorunlu hesap?** ≈ 15 s zorunlu hesap; ≈ 9 s rapor tanısı; kalan ≈ 15–35 s tarama (yalnız-kimlik λ_C adayları ve λ_F ızgarası) [bileşenlerden hesap; tohum 1 taramasında 127,5 s].

**4. Önceki "bizim yöntem daha hızlı" iddiam yanlış mıydı, yoksa farklı süreleri mi karşılaştırdım?** İkisi de, ve ağırlığı şu: **iddia ölçüme dayanmıyordu.** Klasik baz çizgisi V32'ye kadar hiç koşturulmamıştı. "Ridge + Cholesky ile saniyeler" cümlesi (ve verdiğim verimlilik puanı ≈75) gerçek ama **farklı bir şeye** dayanıyordu: tek F Ridge çözümü (H=512'de 0,3–0,4 s, H=1024'te 1,7 s [önceki rapor, V20/V22 logları]) — fakat o modeller 182–192 PPL veriyordu. 122,9 PPL'e ulaşan klasik MLP ile aynı kalitede karşılaştırılmış bir süre yoktu. V32'de zaten "hız avantajı yok" demiştim (puanı 45'e indirmiştim); asıl hata, V32'de bizim tarafı taramalı toplam süreyle (40–60 s) klasik tarafı tek koşuyla (13–25 s) yazmamdı.

**5. Hangisi hızlı, ne kanıtlayabiliyorum?**
- Kanıtlı [ölçüldü, tek tohum]: bizim V31 sabit hiperparametre ≈ 14,6 s; klasik MLP tek koşu ≈ 21,5 s, ama doğrulama PPL ≈147'ye klasik **≈ 7 s'de** ulaşıyor (bizim 15 s); test tahmini klasikte daha hızlı (0,03 s vs 0,43 s).
- Kanıtlı [ölçüldü]: bizim 182,5 PPL'lik tek F Ridge (0,3–0,4 s, önceki log) klasik MLP'nin ilk epokundan (3,7 s, doğrulama PPL 181) belirgin hızlı — **ama bu düşük kalite noktasında**; daha yüksek kalitede klasik yöntem öne geçiyor.
- **Kanıtlayamıyorum**: hiperparametre seçimi dahil adil toplam (bizim tarafta minimal seçim süresi doğrudan ölçülmedi; klasikte 216 s); optimize BLAS ile bizim sürenin ne olacağı; büyük veri/sözlükte ölçekleme; 3 tohumda süre dağılımı (burada tek tohum, ve süre tekrarları arasında özellik üretimi 1,3 vs 2,2 s gibi ~%40'a varan oynama gözlendi).

## 7. Sınırlamalar

Tek makine, tek tohum, 2 C++ ölçümü + 1 klasik ölçüm. Klasik ölçümde ilk epok ısınması (3,7 s) ve torch başlatma yükleri ayrı belirtildi. Eksik ölçümler tahminle doldurulmadı; hesap satırları "bileşenlerden hesap" diye işaretli. Hiçbir yeni model/yöntem eğitilmedi; PPL sonuçları değişmedi.

## 8. Dosyalar

`cpp/tek_hamle_v31_sure.cpp`, `v32_sure_ham/v31_sure_sabit_hiper.txt` (zaman dökümü: `ZAMAN|`, `TT|`, `V31_*`), `v32_sure_ham/manifest.tsv`, `v32_sure_ham/mlp_sure_olcum.py` + `mlp_sure_olcum.json`. Komutlar: `./tek_hamle_v31_sure 2 512 1 1 1 0.5 Z 1 Y - 100 100 3000`; `python3 v32_sure_ham/mlp_sure_olcum.py`.
