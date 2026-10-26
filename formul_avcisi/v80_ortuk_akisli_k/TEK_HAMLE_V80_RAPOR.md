# Tek Hamle AI — V80: K-grafı / s(K) — doğrudan ve akışlı hesap

Dal `claude/formul-avcisi` · taban V78 `83df01d` (+V79 σ düzeltmesi; V65–V79 dosyalarına dokunulmadı) · kod `cpp/tek_hamle_v80.cpp`, `cpp/knn_sparse_v80.h` (V60 başlığının kopyası) · betik/ham çıktı `v80_ortuk_akisli_k/` · süre ≈21 dk / 30 dk.
Etiketler: **[Kesin]** bu oturumda ölçülen/koddan okunan, **[Muhtemel]** çıkarım, **[Tahmin]**.

## 0. Sonuç
* **K-grafı/s(K) bloğu (`f2`) 1.55 → 0.83 s; model hazırlama 5.34 → 4.83 s; tam program 8.89 → 8.45 s** (5 dönüşümlü çift medyan, 10.001; 5/5 çiftte V80 hızlı; model farkı 0.34–1.02 s). [Kesin]
* **Bit-özdeş:** Fc, AFF, RF, ACF, RC, Y0f, X, T, U, W_F karmaları, 6 s(K) tablosunun karması, 6 grafın (komşu indeks+sıra `nb`, benzerlik ağırlığı `wt`, `bw`) karması, 8 α val CE (15 hane), test CE, Top-1/Top-5, α/ε/α_Q — 2.001, 5.001, 10.001'de V78 ile aynı; 20.001'de Top-k/seçimler aynı. [Kesin]
* Matematik değişmedi (aynı benzerlik, aynı top-k eşitlik bozma `(−tot, w)`); **doğrusallık yalnız s(K)'da** (q satırı sayımların doğrusal fonksiyonu) kullanıldı. K benzerliğinin √sayım yapısına dokunulmadı.

## 1. K-grafı ve s(K) matematiği (koddan) — soru 3
1. **Adaylar:** u kelimesinin sayım satırıyla en az bir ortak sütunu (sonraki kelime y) olan tüm w'ler (seyrek Gram'dan dokunulanlar); dokunulmayan w'nin benzerliği tam 0 → top-k'ya yalnız k dolmazsa küçük indeksle girer.
2. **Benzerlik:** S(u,w) = Σ_y N_uy N_wy, N = √C / (‖√C‖ + 1e−9) (√sayım, satır normu); toplama sgemm K-blok (Q=384) taklidiyle `fmaf` zinciri (bit uyumu için).
3. **√/sayım/normalizasyon:** yalnız satır başına bir kez (CSR kurulumunda). Sıralama: top-10, çift `(−S, w)`; `cnt[w] < 5` olanlar aday değil.
4. **İncelenmek zorunda olan adaylar:** u'nun satırıyla ortak sütunlu tüm w (hepsi); eşik süzmesi yalnız k dolduktan sonra kesin elemeler.
5. **Tekrar kullanılan:** 6 grafın `nb/wt/bw` çıktısı (V×10) üç tüketicide: s(K) (her tablo), V39 karışımı (295k eğitim satırı, fold'a göre OOF tablo), Q. Tablonun kendisi küçük (6×0.8 MB).
6. **Hiç kullanılmayan:** yok (her kelimenin komşusu bir yerde gerekli); V×V tablo yok.
7. **s(K) tekrar mı üretiyor? Evet:** her w için komşu satırlarının desteğinin birleşimi `sup` (ortalama ≈ 4 000 girdi) yeniden kuruluyor, her girdi için 64-boyutlu E(y) çarpımı; aynı komşu nb, onu komşu sayan her w için `C_nb·E`'yi baştan hesaplıyor.
8. **Zorunlu parçalar:** seyrek Gram (2.7e8 fma, bit-uyum için sıralı), top-10 seçimi, komşu ağırlıkları, sayım CSR'ı.

## 2. Üç yöntem
| | A seyrek (V78) | B tamamen örtük | C akışlı/doğrudan (uygulanan) |
|---|---|---|---|
| tanım | graf tablosu + s(K) destek birleşimi | graf yok; komşu her tüketici isteyince hesaplanır | graf (küçük) + s(K) doğrudan sayım türevlerinden |
| benzerlik hesabı | 2.7e8 fma | **≥** A: her komşu sorgusu bir Gram satırı = A'nın her satırı kadar iş; V39 karışımı 295k satır için (fold,u) başına yeniden gerekir → önbellek = graf | 2.7e8 fma (aynı, zorunlu) |
| √ işlemi | CSR'da bir kez | aynı veya tekrar | aynı |
| s(K) madd | **1.47e9** (Σ\|sup\|·64, 6 tablo; ölçüldü) | — | **8.4e7** (R_u: 6×nnz·64 = 4.2e7 + komşu birleşimi 6×V×11×64 = 4.2e7) |
| ölçüm | ölçüldü | **prototiplenmedi** (30 dk; yukarıdaki sayımla A'dan ucuz olamayacağı gösterildi) [Muhtemel] | ölçüldü |
Karar: **B reddedildi** (tüketici sayısı ≥ 2 ve komşu başına maliyet graf satırı kadar; yeniden hesap sistemi yavaşlatır). **C uygulandı.** Gizli yoğun V×V/V³ oluşmuyor.

## 3. Uygulanan C parçaları (hepsi bit-özdeş)
**C1 — s(K) doğrusal türetme (`V80_STAB`).** q_w = (1−b)·C_w/n_w + b·Σ_nb (a_nb/ws)·C_nb/n_nb ⇒ s_w = Σ_y q_y E(y) = [(1−b)/n_w · R_w + (b/ws)·Σ a_nb/n_nb · R_nb] / toplam, **R_u = Σ_y C_u[y] E(y)** (64-boyut, tablo başına bir kez). Bu doğrusallık **sayımlarda**; √ yalnız grafın içinde kaldı. toplam = (1−b)+b = 1. Eşdeğerlik: q olasılık (satır toplamı 1), π terimi ve λ birleşimi aynen korunur. Kanıt: 6 tablonun s(K) karması V78 yolu ile aynı (`V80_STAB=0` ↔ `1`: `734c4478eb4aabd3`, `dd995748…`, `dd37148d…`, `4880fe3a…`, `836cac92…`, `95f5a3bf…`).
**C2 — OOF sayım = tam − fold (`V80_CNT`).** Tamsayı sayımlar ⇒ tam; 6×(~295k anahtar sıralama) yerine 1 tam + 5×(59k) sıralama + birleştirme.
**C3 — seyrek Gram döngüsü sadeleştirme.** Değerler > 0 olduğundan `tot/bacc != 0` "dokunuldu" işareti; iki damga dizisi (`stampT`, `long stampB`) ve dallı `push_back` kalktı (dalsız sayaç); top-k'da k dolunca eşik (`tot < en kötü`) süzmesi. Kanıt: eski başlıkla (`knn_sparse.h`) derlenen ikili ile 6 grafın `nb/wt/bw` karması **aynı**.
**Denenen, reddedilen (C4):** `um/us` (eğitim satırlarının nu=37 787 farklı anahtarı için ağırlıklı toplam, 0.14→0.03 s): gerçek aritmetik azaltımı (295k yerine 37.8k satır) ama toplama sırası değişir ve **Fc/W_F karması değişti** (test CE farkı 2.8e-12) ⇒ bit-özdeş değil ⇒ `V80_UMW=1` isteğe bağlı, **varsayılan kapalı**.

### İşlem muhasebesi
| ölçüm | eski | yeni |
|---|---|---|
| s(K) çarpma-toplama (6 tablo) | 1.47e9 madd (+ N/qv kurulum, destek birleşimi, sıralama) | 8.4e7 madd |
| **net kaldırılan** | | **≈ −1.39e9 madd** (−%94) |
| eklenen aritmetik | — | R_u 4.2e7 (yukarıdaki 8.4e7'ye dahil) |
| benzerlik (Gram) fma | 2.7e8 | 2.7e8 (**kaldırılmadı**: bit-uyum için zorunlu) |
| sayım sıralama karşılaştırması | 6×≈5e6 | ≈1e7 (−%64) [Muhtemel: n log n] |
| Gram iç döngü: damga okuma/yazma + dal | 2 damga/iç iterasyon | 0 (aritmetik değil, **kontrol/bellek**) |
| s(K) süresi | 0.42 s | **0.027 s** |
| graf süresi (Gram+top-k) | 0.57 s | **0.28 s** (Gram 4.13→1.71 Gcycle) — matematiksel işlem azaltımı **değil**, dal/bellek sadeleştirmesi |
| tepe RSS | 1530 MB | 1531 MB (bellek tasarrufu yok: P 5 MB, R 5 MB) |

Gram kazancı (0.29 s) ve sayım kazancı **aritmetik azaltım değildir**; yalnız s(K) kazancı (−1.39e9 madd) gerçek matematiksel işlem azaltımıdır.

## 4. A/B (aynı oturum, 5 dönüşümlü çift, 10.001, 4 iş parçacığı; `ham/A78_*`, `ham/B80_*`)
| | V78 medyan | V80 medyan |
|---|---|---|
| f2 (K tabloları+kapı) | 1.55 | **0.83** |
| k0 (6 tablo CSR+graf+s(K)) | 1.13 | **0.38** |
| s(K) | 0.42 | **0.027** |
| graf (Gram+top-k) | 0.57 | **0.28** |
| model hazırlama | 5.34 | **4.83** |
| tam program | 8.89 [8.84–9.08] | **8.45** [7.98–8.80] |
| test PPL | 262.930 | 262.930 |
| val CE / test CE | 5.441941 / 5.571887 | aynı |
Çift farkları (V78−V80): duvar 0.36, 0.39, 1.10, 0.49, 0.09 s; model 0.51, 0.34, 1.02, 0.46, 0.51 s.

## 5. Ölçekleme
* 2.001 / 5.001 / 10.001: karmaların hepsi aynı (`ham/f2k_*`, `f5k_*`, `f10k_*`, `z10k_*`).
* **20.001** (tek çift, gürültülü, `ham/s20k_*`): K bloğu `f2` 7.72 → **4.50 s**, graf 3.42 → 1.48 s, s(K) 1.14 → 0.15 s; Top-k/seçimler aynı, test PPL 252.531 her ikisinde. (Model hazırlama 58.7 → 21.9 s farkının büyük kısmı VM gürültüsü: V78'in önceki ölçümlerinde 24–30 s.) Yoğun V×V yok; komşu arama maliyeti hâlâ Σ_k nnz(sütun k)² (seyrek), sözlük büyüdükçe nnz ile büyür, V³ değil. 50k/100k: gerçek veri yok, iddia yok.

## 6. Soruların cevabı
1. K-grafı tamamen oluşturmadan çalışabildik mi? **Hayır, tamamen değil:** graf (V×10, 0.8 MB/tablo) üç tüketici için zorunlu ve ucuz; kaldırılan graf-kaynaklı büyük iş s(K) tarafındaki destek yeniden kurulumu.
2. Gerçekten gerekli komşuluk bilgisi: her kelimenin top-10 `(nb, wt, bw)`'si (tüm kelimeler).
3. Örtük mi akışlı mı hızlı? **Akışlı/doğrudan** (B analizle elendi, prototiplenmedi).
4. Kaldırılan işlem: **−1.39e9 madd** (s(K)); Gram fma kaldırılmadı.
5. Engellenen tekrar: komşu başına `C_nb·E` her kelime için yeniden (6 tablo × ~4 000 destek girdisi × 64 → 8.4e7'ye); 5 sıralama.
6. Bellek: tasarruf yok (RSS 1530→1531 MB); ara dizi yazma/okuma azaldı (per-thread N, Cw, sup).
7. K-grafı/s(K): **1.55 → 0.83 s** (graf 0.28, s(K) 0.03, sayım+CSR+diğer 0.5).
8. Model ağırlıkları: **4.83 s** (V78 5.34 s, aynı oturum).
9. Tam program: **8.45 s** (V78 8.89 s).
10. Ağırlıklar/PPL: **bit-özdeş, PPL 262.930**.
11. 20.001: K bloğu 7.7 → 4.5 s (tek çift).
12. Neden başarılı: s(K) q'nun *sayımlarda doğrusal* olması sayesinde komşu katkısı bir kez (R_u) hesaplanıp 11 vektörle birleşiyor; √'li benzerlik ise zorunlu kalıp yalnız kontrol akışı sadeleştirildi. Hedef <4 s model hazırlama **tutmadı** (4.83 s).

## 7. Yeniden üretim
`cd v80_ortuk_akisli_k && ./dogrula.sh z10k /tmp/v59_10001` (V78↔V80 karma), `kos.sh`, `olcek20k.sh`. Bayraklar: `V80_STAB`, `V80_CNT` (1), `V80_UMW` (0), `V80_SHASH=1`, `V80_GHASH=1`, `V80_COUNT=1`.
