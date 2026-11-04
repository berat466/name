# TEK HAMLE V33 — C++ hız optimizasyonu (aynı matematik, optimize doğrusal cebir)

Dal `claude/formul-avcisi` · kod `cpp/tek_hamle_v33.cpp` (V31+zamanlayıcı kopyası; V31/V31_sure dosyaları değişmedi) · ham çıktılar `v33_ham/`. Makine: 4 çekirdek Xeon 2,8 GHz. **Tek tohum (1), H=512, sabit hiperparametre: λ_C=100, λ_F=3000, 3 tekrar.** Formül, F zinciri, γ, λ, bölünme, okuma (V26 doğrusal) değişmedi.

**Ne değişti:** `V33_BLAS=1` ile elle yazılmış çekirdekler yerine sunucuda zaten bulunan **OpenBLAS** (scipy wheel'i ile gelen `libscipy_openblas-5f890258.so`; indirme yok; sistemin `libblas.so.3`'ü referans BLAS olduğundan kullanılmadı) kullanılır: `sgemm` (tüm float matris çarpımları, `gemm_f`), `dpotrf` (Cholesky, 4002² ve 513²), `dtrsm` ×2 (çoklu sağ taraf çözümü; `dpotrs` yerine satır-ana düzende doğrudan), `dgemm` (Schur parçaları T, U). Ek olarak tek-λ_C koşusunda `A_CC` kopyası taşıma ile atlanır, Y₀/X çözümleri çift duyarlıkta tutulup T,U'da kullanılır. **Eski çekirdekler silinmedi**: `V33_BLAS=0` (varsayılan) eski yolu çalıştırır.

## 1. Süre kapsamı

"Tek-hamle toplam" = veri okuma + `expand_s` + F₁–F₄ üretimi + F Gram/FᵀY + C bloğu (A_CC, A_CF, R_C) kurulumu + [A_CC kopya, Cholesky 4002², Y₀ çözümü, X çözümü, Schur parçaları, Schur sistemi]. **Dışarıda:** hiperparametre taraması, yalnız-kimlik λ_C adayları, doğrulama okuma seçimi (E1/E2), eğitim logitleri ve rapor tanıları (E3), test tahmini (P2/P5). V32 denetiminde 14,6–14,8 s’lik "eski" süre aynı kapsamdaydı (hazırlık 2,7–3,8 + matematik 11,1–12,0); bu raporda eski süre aynı program, aynı koşulda yeniden ölçüldü.

## 2. Hız testi (3 tekrar, ort. ± std, saniye)

| Ölçüm | Eski C++ | Optimize C++ (OpenBLAS, 4 iplik) | Hızlanma |
|---|---|---|---|
| Veri okuma + `expand_s` | 0.09 ± 0.02 | 0.07 ± 0.00 | 1.3× |
| F₁–F₄ üretimi + F Gram/FᵀY (tek kalem; ayrı ölçülmedi) | 1.36 ± 0.06 | 1.36 ± 0.11 | 1.0× |
| Matris oluşturma (A_CC, A_CF, R_C; seyrek sayım) | 0.91 ± 0.33 | 0.65 ± 0.32 | 1.4× |
| A_CC kopya + λ_C (M1) | 0.55 ± 0.33 | 0.00 ± 0.00 | kopya atlandı |
| Cholesky 4002² (M2) | 4.00 ± 0.07 | 0.20 ± 0.02 | 19.7× |
| Çoklu sağ taraf çözümü: Y₀ (2001 sağ taraf, M3) + X (513, M4) | 4.89 ± 0.09 | 0.88 ± 0.10 | 5.5× |
| Schur işlemleri: T,U (M5) + 513² sistem ve W_C (M6) | 1.17 ± 0.12 | 0.58 ± 0.03 | 2.0× |
| **Tek-hamle toplam** | 12.97 ± 0.37 | 3.75 ± 0.19 | 3.5× |
| Matematiksel çözüm (M1–M6) | 10.61 ± 0.10 | 1.67 ± 0.12 | 6.4× |
| (kapsam dışı) program toplam wall süresi: tarama yok ama tanılar dahil | 23.23 ± 0.37 | 9.70 ± 0.08 | 2.4× |
| Test PPL (tohum 1) | 139.78 | 139.78 | – |

Eski 14,6 s ile yeni sonuç aynı kapsamda: **evet** (aynı satırlar toplanıyor; bu raporun eski koşuları da ≈13–15 s verdi, V32 denetimiyle tutarlı). Tohum 1 için tek konfigürasyon; 3 tekrar donanım gürültüsünü gösterir, tohum varyansını değil.

## 3. Sayısal eşdeğerlik (aynı girdi, eski vs yeni)

| Büyüklük | W_C (4002×2001, kimlik) | W_F (513×2001, F) |
|---|---|---|
| maks. mutlak fark | 4.47e-08 (maks \|W\|=0.328) | 9.41e-08 (maks \|W\|=0.0108) |
| göreli Frobenius fark | 2.34e-07 | 8.17e-06 |
| denklem rezidüsü ‖Zᵀ(ZW−Y)+ΛW‖/‖ZᵀY‖ | eski 2.72e-07 → yeni 6.05e-08 | (aynı ortak sistem) |
| Test CE / PPL / Top-1 / Top-5 | eski 4.9401 / 139.78 / 15.60% / 35.68% | yeni 4.9401 / 139.78 / 15.60% / 35.68% |

Olasılık normalizasyonu: her iki yolda `|Σp−1|` ≤ 1e-13 (logda `olasilik_toplam_sapma`). Fark kaynağı: eski yol Y₀/X çözümlerini float’a yuvarlayıp T,U’yu float Y₀/X ile hesaplıyordu; yeni yol çift duyarlık kullanır → yeni rezidü daha küçük (**yeni çözüm eskiden daha doğru**), ağırlık farkı ~1e-7…1e-5 göreli, PPL ve Top-k **özdeş** (iki tohum-1 koşusunda aynı 139,78).

## 4. Çekirdek sayısı etkisi (tek tekrar)

| yapılandırma | tek-hamle toplam (s) | matematik M1–M6 (s) |
|---|---|---|
| Eski, 1 iplik (OMP=1) | 51.20 | 47.58 |
| Eski, 4 iplik (ort.) | 12.97 | 10.61 |
| OpenBLAS, 1 iplik | 5.86 | 3.28 |
| OpenBLAS, 2 iplik | 4.35 | 2.29 |
| OpenBLAS, 4 iplik (ort.) | 3.75 | 1.67 |

## 5. Sorular

1. **Eski süre → yeni süre.** Aynı kapsamda, bu raporun 3 tekrarında: **12,97 ± 0,37 s → 3,75 ± 0,19 s (3,5×)**. V32 denetiminde raporlanan ≈14,6 s'ye göre (o ölçüm ısınma/dalgalanma payı taşıyordu; bu oturumun eski koşuları 12,6–13,4 s verdi) hızlanma ≈ **3,9×**. Yalnız matematiksel çözüm (M1–M6): **10,61 → 1,67 s (6,4×)**. Tek tohum (1), H=512, tek konfigürasyon.
2. **En büyük darboğaz:** elle yazılmış Cholesky (4002², 4,0 s) ve çoklu sağ taraf çözümü (Y₀, 2001 sağ taraf; 4,9 s) — ikisi toplam matematiksel sürenin ≈%84'ü. OpenBLAS ile Cholesky 4,0→0,20 s (19,7×), sağ taraf çözümleri 4,9→0,88 s (5,5×).
3. **Ağırlık farkı:** W_C göreli Frobenius 2,3e-7 (maks mutlak 4,5e-8), W_F göreli 8,2e-6 (maks mutlak 9,4e-8). Denklem rezidüsü eskiden yeniye 2,7e-7 → 6,1e-8 (yeni daha doğru: çift duyarlık Y₀/X). Farklar float duyarlığı mertebesinde ve eski yolun yuvarlama kaynaklı.
4. **PPL aynı mı?** Evet: test CE 4,9401, PPL **139,78 (fark 0,00)**, Top-1 %15,60, Top-5 %35,68 — eski ve yeni özdeş (tohum 1; 3 tohum ortalaması 140,3 daha önceki V31 raporunda; bu denetimde tohum 2/3 yeniden koşturulmadı).
5. **Klasik MLP'nin 7 s eşiği:** sabit-hiperparametreli ağırlık üretimimiz artık **≈3,75 s** (+ α,ε okuma seçimi ≈0,45 s, test tahmini ≈0,43 s) → klasik MLP'nin doğrulama PPL ≈147'ye ulaştığı ≈7 s'nin **altında**, tek-koşu süresinin (21,5 s) çok altında. **Kalite farkı korunuyor ve hız kazancı bunu kapatmıyor:** MLP 3 epok sonra devam edip test PPL **122,9**'a iniyor (en iyi doğrulama ≈12 s'de), bizim yöntem 140,3'te sabit. Yani artık "aynı kalite düzeyinde daha hızlı" ama "daha iyi kalite" değil. Uyarı: klasik süre `import torch` (13,4 s) ve hiperparametre seçimi (≈216 s) hariç; bizim tarafta hiperparametre taraması (V31: ≈45–50 s hesap) hariç — OpenBLAS ile λ_C adayı başına maliyet ≈10,6 → ≈1,2 s (M1–M5 bileşenlerinden hesap, doğrudan ölçülmedi) olacağından seçim maliyeti de benzer oranda düşmeli [tahmin].
6. **Sonraki gerçek darboğaz:** F₁–F₄ üretimi + F Gram/FᵀY **1,36 s (toplamın %36'sı)** — elle yazılmış `syrk`/çapraz Gram ve yön üretimi, hâlâ OpenBLAS dışı; ardından Y₀+X çözümü 0,88 s, Schur 0,58 s, C bloğu kurulumu 0,65 s (seri sayım döngüleri). OpenBLAS ile iplik ölçeklemesi: 1 iplik 5,86 s, 2 iplik 4,35 s, 4 iplik 3,75 s (verimsiz ölçekleme: seri kısımlar ve küçük işler baskın).

## 6. Ek kontrol: V32 süre betiği (checkpoint hatası)

`v32_sure_ham/mlp_sure_olcum.py` test öncesi **en iyi doğrulama checkpoint'ini geri yüklemiyordu** (132,5 PPL buradan). Düzeltme eklendi (`best_state` saklanır, test öncesi `load_state_dict(best_state)`; V32 `klasik_baz_v32.py`'deki davranışla aynı). **Düzeltilmiş betik YENİDEN ÇALIŞTIRILMADI**: görev klasik MLP'yi yeniden eğitmeyi yasakladığı için yalnız kodu düzelttim, sözdizimini doğruladım; bu yüzden düzeltmenin PPL'i (beklenti: V32'nin 122,9'una yakın) bu raporda **ölçülmüş değil**. Resmî 122,9 PPL (V32, `klasik_sonuclar.jsonl`) değişmedi.

## 7. Sınırlamalar

- Tek tohum (1) ve tek konfigürasyon (λ_C=100, λ_F=3000); H=512. H=1024 ve diğer tohumlar yeniden zamanlanmadı.
- OpenBLAS, scipy wheel'inin gömülü kütüphanesi (`/usr/local/lib/python3.13/dist-packages/scipy.libs/libscipy_openblas-5f890258.so`); başka makinede yol değişir (rpath). Sistemin `libblas.so.3`'ü referans BLAS olduğu için tercih edilmedi.
- F zinciri + F Gram kalemi ayrı ayrı zamanlanmadı (tek kalem).
- Hızlanma yalnız lineer cebir çekirdekleri ve kopya atlama ile; formül, λ, F boyutları, veri ve okuma değişmedi. Yeni model/mimari/hiperparametre araması yok.
- V31 ile yeni sürüm arasındaki 14,6 s ↔ 12,97 s fark: farklı koşular arası dalgalanma (V32 denetiminde özellik üretimi 1,3–2,2 s oynadı).

## 8. Komutlar ve dosyalar

Derleme: `g++ -O3 -march=native -fopenmp -std=c++17 cpp/tek_hamle_v33.cpp -o tek_hamle_v33 -L<scipy.libs> -l:libscipy_openblas-5f890258.so -Wl,-rpath,<scipy.libs>`. Eski/yeni: `V33_BLAS=0|1 ./tek_hamle_v33 2 512 1 1 1 0.5 Z 1 Y - 100 100 3000` (`V33_DUMP=önek` ağırlıkları yazar; `V33_BLAS_THREADS` OpenBLAS iplik sayısı). Ham loglar: `v33_ham/*.txt`, `manifest.tsv`.
