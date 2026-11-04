# Tek Hamle AI — V77: gereksiz matematiksel işlemlerin kaldırılması

Dal: `claude/formul-avcisi` · kaynak: `cpp/tek_hamle_v77.cpp` (V76'dan türetildi; V65–V76 kaynaklarına dokunulmadı) · betikler ve ham çıktılar: `v77_matematiksel_islem_azaltimi/`

Etiketler: **[Kesin]** = bu oturumda ölçülen/koddan okunan, **[Muhtemel]** = güçlü çıkarım, **[Tahmin]** = boşluk doldurma. Yanıtın büyük kısmı [Kesin]; tahminler ayrıca işaretli.

---

## 0. Sonuç (tek bakışta)

| | V76 | V77 | fark |
|---|---|---|---|
| tüm program, medyan (7 dönüşümlü çift, 10.001, 4 iş parçacığı) | **11.43 s** [11.18–12.21] | **9.41 s** [9.20–10.36] | **−2.03 s (−17.7 %)** |
| model hazırlama | 5.54 s | 5.14 s | −0.41 s |
| doğrulama (val) | 3.91 s | 2.42 s | −1.49 s |
| test | 1.89 s | 1.78 s | −0.11 s |
| kullanıcı CPU | 37.9 s | 31.5 s | −6.4 s |
| tepe RSS | 1526 MB | 1530 MB | +3.6 MB |
| eval-GEMM madd (3×36 852×513×10 001 = 5.672e11'in karşılığı) | 5.672e11 | 3.860e11 | **−1.81e11 (−31.9 %)** |
| val CE / test CE / test PPL / Top-1 / Top-5 | 5.441941 / 5.571887 / 262.930 / %13.70 / %31.12 | **aynı** | 0 |

* 7 çiftin 7'sinde V77 daha hızlı (V77/V76 duvar oranı 0.77–0.91, medyan 0.83). [Kesin] `ham/A76_*`, `ham/B77_*`, `ab_ozet.json`.
* **Matematik değişmedi.** Model ara büyüklüklerinin hepsinin (Fc, AFF, RF, ACF, RC, Y0f, X, T, U, W_F) özet-karmaları V76 ile **bit-bit aynı**; val/test logit karmaları, 8 α için val CE/Top-k (15 hane), test CE/Top-k (15 hane), seçilen (α, ε, α_Q) aynı. 2.001, 5.001, 10.001 ve 20.001 üzerinde doğrulandı (ayrıntı §6). [Kesin]
* Tek bit farkı: ε×α **seçim tablosunun** (`lin_table`) toplamları ≤1.1e-14 mutlak (≤1.8e-15 bağıl) farklı — örneklerin karıştırılmış sırayla toplanmasından. Bu tablo yalnız `argmin` için kullanılır; en iyi ile ikinci en iyi arasındaki boşluk 1.3e-4 … 1.0e-3 (≥10 mertebe büyük), seçim aynı. Öğrenmeye girmez. [Kesin]
* **Dürüst sınırlar:** (i) kazanç yalnız eval ve model hazırlamadaki kodsal tekrarlardan geliyor; modelin matematiksel çekirdeğinde (F1, K-grafı, FᵀF, Schur) bit-özdeş kalarak kaldırılabilecek işlem **bulamadım** — bunların listesi ve nedeni §3 ve §9'da. (ii) Tekil bileşenlerin çoğu (S2 yeniden kullanımı, bölmesiz rank, RHS, sütun kısıtlama) tek başına gürültü bandında (±0.4 s); onları bileşen zamanlayıcılarıyla gösteriyorum, duvar saatiyle değil (§7). Duvar saatinde ölçülebilir tek kalem spekülatif tek geçiş (+1.06 s ablasyonda).

---

## 1. Matematiksel bağımlılık grafı

Her düğüm: değer/boyut/dtype → bağımlı olduğu düğümler → tüketenler → madd (ya da işlem) → bellek trafiği → ölçülen duvar → gerekli kesir → daha kısa eşdeğer var mı. Süreler 10.001 verisinde V77 (V76) tipik koşusu, `ham/B77_4.txt` / `ham/A76_4.txt`. n_tr = 294 816, n_val = n_test = 36 852, V = 10 001, m_f = 513, K_girdi = 192, mc = 42 651 (C bloğu: kelime + çift slotları).

| # | düğüm | değer / dtype | bağımlı | tüketen | madd / işlem | trafik | süre V77 (V76) | gerekli kesir | daha kısa eşdeğer? |
|---|---|---|---|---|---|---|---|---|---|
| N1 | X ham girdi (+fark bloğu `expand_s`) | n×192 f32 (tr 226 MB) | veri | N3 | kopya | 0.3 GB | 0.15 + 0.12 s | %100 | yok |
| N2 | W1, b | 192×512 f32 | tohum | N3 | — | — | 0.02 s | %100 | — |
| N3 | Z1 = X·W1 (+seyrek satır) | n×512 f32 | N1,N2 | N4 | tr 2.90e10; val/test: V76 7.25e9 → **V77 2.43e9** | 0.6 GB yaz | 0.20 s (0.25) | tr %100*, val/test **%33.6** | val/test için yalnız 172 sütun (kaldırıldı) |
| N4 | A1 = tanh(γ(Z1+b)) | n×512 f32 | N3 | N6 | 1.9e8 tanh (val/test'te −2.5e7) | 0.6 GB | 0.23 s (0.25) | tr %100*, val/test %33.6 | sütun kısıtlama (kaldırıldı) |
| N5 | K tabloları (5 OOF + 1 tam), seyrek kNN grafı, s(K) 64-D, standartlama, Pm projeksiyonu, T_K = tanh(g_F·U) | altı V×V seyrek graf; nu = 37 787 × 512 | eğitim sayımları | N6 | seyrek kNN 0.54 Gmadd; s(K) ~0.4 s | küçük | **1.52 s** (1.52) | %100 | V60/V61 zaten seyrek-kesin; değişmedi |
| N6 | kapı: A = (f+t)/(1+f·t) | n×512 f32 | N4,N5 | N7 | 1.9e8 bölme | 0.6 GB | 0.16 s (0.19) | tr %100*, val/test %33.6 | tanh toplama formülü (V54'ten beri); val/test kısıtlandı |
| N7 | V39 komşu ortalaması/karışımı | V×513 ort. f32; F karışımı | N6, kNN | N8 | ort. 1.5e8 ekleme; karışım 1.9e9 madd (val/test −2.5e8) | 0.6 GB | 0.23 s (0.28) | tr %100*, val/test %33.7 | ortalama paralelleştirildi (bit-özdeş) |
| N8 | V49: μ, σ, cm, cs, apply → F = [1, A_j, d², d³ (170 taban ×3) + 2 kopya] | n×513 f32 (605 MB) | N7 | N9–N11, eval | 4 geçiş: ~0.6e9 işlem | 4×0.6 GB | **0.53 s (0.86)** | 512 sütundan **172'si F'e girer**; kalan 340 yalnız `rmsA` skalerine (§3-C) | tanı döngüsü paralelleştirildi (−0.3 s) |
| N9 | AFF = FᵀF | 513² f64 | N8 | N15 | 3.89e10 madd (syrk, yarım) | 0.6 GB oku | 0.40 s (0.42) | %100 | tam sgemm 7.8e10 → V73'ten beri yarım; BLAS ssyrk bit-uyumsuz |
| N10 | RF = FᵀY (one-hot) | 513×V f64 | N8,y | N15 | 1.5e8 ekleme | 0.6 GB oku + yaz 41 MB | **0.13 s (0.17–0.48)** | %100 | sınıf-sıralı toplama (bit-özdeş) |
| N11 | slot sayımları: ACC (okbaşı), ACF = Σ F satırları, RC | ACF 42 651×513 f64 (175 MB); RC kompakt 1.66e7 | N8, slot id | N12–N14 | ACF: 3.0e8 ekleme | 0.6 GB | 0.32 s (0.27) | %100 | kelime slotu = Σ çift slotları + kalan (§3-D) — kazanç ≤0.05 s |
| N12 | Y0, X kapalı form (okbaşı) | Y0 kompakt f64 1.66e7; X 42 651×513 f64 + f32 | N11 | N13,N14, eval | ~3.8e7 | 0.4 GB | 0.10 s (0.08) | %100 | zaten kapalı form (V63); yoğun grup-Cholesky 1.22e10 |
| N13 | T = A_CFᵀX | 513² f64 | N11,N12 | N15 | 1.12e10 madd | 0.35 GB | 0.175 s | üst üçgen %67 (3 bant) | `V77_TSYM`: üst üçgen blokları (madd −3.7e9) — **varsayılan kapalı** (§3-D) |
| N14 | U = XᵀR_C (CSC, yalnız gerçek sıfır-olmayanlar) | 513×V f64 | N12, R_C | N15 | 279 500 nz × 513 = 1.43e8 | 41 MB | 0.13 s (0.115) | %100 | zaten V72 |
| N15 | Schur: (A_FF − T + λI) W_F = R_F − U | W_F 513×V f32 | N9,N10,N13,N14 | eval | Chol 4.5e7 + 2 trsm 2.6e9 | 0.1 GB | 0.14 s (0.13) | %100 | — |
| N16 | Q_havuz satırları (seyrek + π) | seyrek | sayımlar | eval | O(nnz) | küçük | 0.115 s | %100 | — |
| N17 | **val logit** z = (F−ΣX)W_F + ΣY0 | blok (≤8000×V) f32, saklanmaz | N8,N15,N12 | seçim (aşama 1), 8-α birleşik (aşama 2) | V76: **2×1.8907e11**; V77: 1.8907e11 + 7.88e9 | 0 (blok) | 1.12 s sgemm (V76'da ayrı zamanlayıcı yok; 2 geçiş ≈2×1.1 s [Muhtemel]) | 1 geçiş yeter | **spekülatif tek geçiş (§4)** |
| N18 | seçim: 8 ε × 20 α → (α₀, ε₀) | 160 skaler | z_y, S_e | N19 | 36 852×(8 toplam V + 160 log) | — | ~0.35 s [Tahmin: ayrı zamanlayıcı yok] | %100 | z_y, S_e skaler; V77 S_e'yi saklar (§4) |
| N19 | 8-α birleşik karışım, rank | 36 852×8 CE+rank | N17,N18,Q | CE, Top-k | 3 geçiş V; bölmesiz rank | — | 0.64 s | %100 | sm tam softmax paydası korunur (kullanıcı kuralı) |
| N20 | test logit + tek-α karışım | 36 852 | N17, α_Q | CE | 1.8907e11 + 1 α | — | 1.78 s | %100 | — |

\* tr: 340 fazlalık sütun yalnız `rmsA` içindir; bit-özdeşlik için hesaplanmaları gerekir (§3-C).

Bu tablodaki "gerekli kesir" sütunu üç gözlemden geliyor (kod okumasıyla [Kesin]): (1) V49 `mod=2` yalnız ilk 170 A sütununu ×3 genişletir ve sondaki 2 sütunu kopyalar; (2) val/test satırları `μ, σ, cm, cs` hesabına girmez; (3) F1'in 512 sütunu eğitim satırlarında yalnız `rmsA = mean_j σ_j` için gerekir.

---

## 2. V76'da gereksiz çıkan işlemler (soru 1) — kod kanıtı

1. **Validasyon logitlerinin ikinci hesabı** (1.8907e11 madd): aşama 1 `(α,ε)` seçimi için tüm val logitlerini hesaplar; aşama 2 `(α₀,ε₀)` bilindikten sonra aynı logitleri **yeniden** hesaplar (`blockLogits` ikinci kez). Neden: `(α₀,ε₀)` yalnız tüm val örneklerinin toplamı bitince bilinir ve aşama 2'nin `prepRow`'u ona bağlıdır; n×V logit saklamak 1.47 GB'tır ve ölçümde yavaş (V76 raporu). Kanıt: `V77_SPEK=0` yolunda iki geçişin satır-karması bit-özdeş (`54dbb30ae95dea9d` = `54dbb30ae95dea9d`, `ham/z77b.txt`) — ikinci geçiş yeni bilgi üretmiyor.
2. **S2 = Σ_v max(z_v, ε₀) yeniden toplama**: aşama 1 her örnek için 8 ε'nin S_e toplamını zaten hesaplıyor (AVX-512 şeritleri; şerit ε₀ skalerle aynı sıralı toplam), aşama 2 `prepRow` aynı toplamı V adımla tekrar hesaplıyor. (Her val örneği için 10 001 max+ekleme.)
3. **Rank karşılaştırmasında V×8 bölme**: `p/sm > p_y/sm` her v ve her α için zmm `vdivpd`. Bölme tekdüzedir; p ≤ p_y' ise `fl(p/sm) ≤ fl(p_y'/sm)`, p > p_y'(1+2⁻⁵⁰) ise kesin büyüktür; yalnız dar bantta eski bölmeli karşılaştırma kullanılır (§5 kanıt).
4. **FᵀY için 4×41 MB sıfırlanan iş parçacığı dizisi + birleştirme** (`rhs_add`): rastgele satır-RMW ve nt×V×513 ekleme; sınıf-sıralı toplama aynı toplama sırasını verir.
5. **V49 tanı döngüsü**: yalnız yazdırılan `sutun RMS araligi` için tek iş parçacığında, sütun-dış, adım 2 KB erişim (0.3–0.35 s).
6. **val/test için kullanılmayan F1 sütunları**: 512 sütunun 340'ı için gemm, tanh, kapı ve V39 karışımı (val/test satırlarında hiçbir tüketen yok).
7. **V39 ortalama toplama** tek iş parçacığında (0.10–0.14 s).

---

## 3. Model hazırlama soruları A–E (koddan kanıtlı)

**A. FᵀF ve FᵀY doğrudan nihai katsayı olarak oluşturulabilir mi?** Hayır. İkisi de Schur sistemine **girdi**: `(A_FF − T + λI) W_F = R_F − U`; W_F bir lineer çözüm (§1 N15). Katsayı ancak çözümden sonra vardır. Her biri tek geçişte bir kez oluşturuluyor ve tek tüketene gidiyor; "oluşturulup sonra başka forma dönüştürülüp iptal edilen" kopya yok. İki dönüşüm var ve ikisi de bit-uyumu için gerekli: F f32 → 256 satırlık f32 birikim → f64 boşaltma (V73'te sgemm'in 256-satır bloklu toplama sırası taklit edildi); RF f64.
**B. Oluşturulup sonra dönüştürülen/iptal edilen ara büyüklük:** V63–V72 zinciri zaten kaldırdı: yoğun W_C (42 651×V f32 = 1.7 GB; V64), yoğun Y0 (4.3e8 → 1.66e7 kompakt; V53h), gerçek R_C sıfır-olmayanları (V72). V77'de kaldırılanlar: ikinci val logit akışı (%95.8'i), nt×(V×513) f64 kısmi diziler, val/test için kullanılmayan 340 sütunun değerleri.
**C. Büyük matris yalnız kısmen kullanılıyor mu?** Evet: F1'in 512 sütunundan F'e **172** girer (`mod=2`: 170 taban + 2 kopya). val/test satırlarında kalan 340 sütun hiç okunmaz → V77 hesaplamıyor (F1 gemm −4.8e9 madd, tanh/kapı −2.5e7'şer, V39 karışımı −2.5e8 madd). Eğitim satırlarında 340 sütun yalnız `rmsA = mean_j σ_j` (eğitim sütun std ortalaması; F'nin A², A³ sütunlarının ölçeği) için kullanılır. Bu skaler bit-özdeşlik için tam hesaba bağlı; **kaldıramadım** (§9-1: alt-örnekleme `rmsA`'yı yaklaşık 1e-4 bağıl değiştirir [Tahmin, ölçülmedi] → farklı model, varsayılan olamaz). Tahmini potansiyeli ~0.4–0.5 s (toplam sürenin %4–5'i) [Tahmin].
**D. Tekrarlanan katkılar:**
* val logitleri (kaldırıldı).
* **T simetrik**: T = A_CFᵀ A_CC⁻¹ A_CF; Cholesky yalnız satır-ana üst üçgeni okur (alt üçgen NaN ile zehirlenince `W_F` karması `d54c462f219c825b` aynı kaldı: `ham/tkanit_zehir.txt`). Üst üçgen blokları (`V77_TSYM=1`, 3 bant) madd'i 1.122e10 → 7.4e9'a düşürür; **fakat** alt-matris boyutlu dgemm çağrıları üst üçgenin bazı elemanlarını 1 ulp değiştiriyor (`T_ust_ucgen` karması tam dgemm'de `414cf095da6aab5a`, blokta `9e20a1136ce3c564`; `W_F` f32'ye yuvarlanınca ikisinde de aynı: `ham/tkanit_tam.txt`, `ham/tkanit_blok.txt`). Tek koşuda W_F aynı olması garanti değil → **varsayılan kapalı**. Duvar kazancı ölçülemeyecek kadar küçük (0.175 → 0.13–0.16 s).
* V49: `σ_j²` toplamı, `cm` kolonlarının (p=2) toplamıyla bit-özdeş (aynı d·d dizisi) — 170 sütun için `sd` geçişi tekrar; kazanç ≤0.03 s, uygulanmadı [Muhtemel: kod okuması, ayrıca test edilmedi].
* Kelime slotu ACF'si = Σ (aynı kelimenin çift slotları ACF'si) + (çift slotu olmayan örneklerin toplamı): ok-başı yapısından (A[w,p] = A[p,p]) çıkan geçerli özdeşlik; ancak toplama sırası değişir (bit farkı) ve kazanç ≈6.5e7 ekleme ≈0.05 s → uygulanmadı [Tahmin: çift-slotu kapsama oranı ölçülmedi].
**E. V60/V63/V64/V72 cebiri başka yere uygulanabilir mi?** Aday aradım: (i) K-grafı: V60 zaten uygulanıyor. (ii) Okbaşı kapalı form (V63): yalnız A_CC için geçerli (A[w,p]=A[p,p]); başka okbaşı yapı yok. (iii) Örtük W_C (V64): eval'de uygulanıyor; eğitim logiti rapor için bile hesaplanmıyor (varsayılan). (iv) CSC (V72): U için. Yeni uygulama alanı **bulamadım**; kazanç bu zincirin kendi içinde bitmiş.

---

## 4. Validasyon tekrarı: neden, üçüncü yol, kanıt

**Neden [Kesin]:** `h_finish` içinde aşama 1 döngüsü `blockLogits` + `lin_table` toplamlarını biriktirir; ancak tüm bloklar bitince `argmin → (a0,e0)`; aşama 2 döngüsü `prepRow` içinde `a0,e0`'ı kullandığından logitleri baştan üretir. `ZCACHE` (n×V = 1.47 GB) V76'da yavaş çıktı (RSS 2 GB üstü SYS artışı).

**Üçüncü yol — sınırlı bellekli spekülatif tek geçiş + kesin yedek:**
1. Val örnekleri sabit tohumlu (`mt19937_64(77)`) bir permütasyonla işlenir (val metni sıralı ve stasyoner değil: sıralı işlemde ilk iki 8000'lik blokta kümülatif argmin yanlış (ε=1e-6) kaldı, karıştırınca düzeldi).
2. Bloklar küçükten büyüğe: 512, 512, 1024, 2048, 4096, 8000… Her blokta logitler üretilir, `lin_table` toplamlarına eklenir, **kümülatif** argmin'e göre geçici `(a0,e0)` seçilir ve aynı blokta, logitler hâlâ bellekteyken `fuseRow8` ile örnek başına sonuç (CE×8 α, rank) saklanır.
3. Sonda gerçek `(a0,e0)` belli olur; geçici etiketi farklı çıkan bloklar **yeniden** hesaplanır (gemm + fuse). Eşit olanlar aynen kullanılır.
4. 8 α için CE toplamı ve Top-k, V76'daki gibi **örnek sırasıyla** (orijinal indeks) toplanır → V76 ile bit-özdeş toplam.

**Doğruluk argümanı [Kesin]:** `fuseRow8`, bir örnek için `(z_satır, a0, e0, bağlam)` ile belirlenen saf bir fonksiyondur; bir örneğin saklanan sonucu ya nihai `(a0,e0)` ile hesaplanmıştır ya da yeniden hesaplanır. Satır sırası/blok boyutu sgemm eleman bitlerini değiştirmez (K=513 sıralı FMA); doğrulandı: val logit satır-karması V76 yolunda ve V77 yolunda aynı (`54dbb30ae95dea9d`), test aynı (`ab76d69b092a3d11`).

**Sonuçlar (yeniden hesaplanan örnek payı):**

| veri | blok | yeniden hesaplanan | val geçişi madd (V76 → V77) | not |
|---|---|---|---|---|
| 2.001 | 6 | **0 / 36 852** | 7.57e10 → 3.78e10 | tam başarı |
| 5.001 | 7 | 7 680 / 36 852 (%20.8) | 1.89e11 → 1.14e11 | ilk dört blok yanlış etiketli |
| 10.001 | 9 | **1 536 / 36 852 (%4.2)** | 3.78e11 → 1.97e11 | 7.9e9 yeniden |
| 20.001 | 42 | 57 496 / 159 858 (%36) | 3.28e12 → 2.23e12 | ikinci-iyi boşluğu 1.3e-4: seçim geç oturuyor |

Spekülasyon her zaman kesin yedekle bitiyor; başarısız olması yalnız hız kaybı, yanlış sonuç değil. 5.001 ve 20.001'de oran düşüyor (boşluk küçük); bunu iyileştirecek bir sınır kanıtı **yok** (§9-3). V77 ek maliyetleri: kısmi bloklarda boşa çalışan `fuseRow8` (yeniden hesaplanan %4.2 örnek × 0.64 s ≈ 0.03 s), `S_e` (36 852×8 f64 = 2.4 MB), `ceAll` (2.4 MB), `rkAll` (1.2 MB), 4 fazladan küçük sgemm çağrısında B paketleme (≈2e7 eleman), blok başına 160 karşılaştırma.

---

## 5. Bölmesiz rank — kanıt

p/sm ve p_y'/sm aynı pozitif sm ile bölünür; IEEE bölme tekdüzedir: x ≤ y ⇒ fl(x/sm) ≤ fl(y/sm). Böylece `p ≤ p_y'` ise `fl(p/sm) ≤ fl(p_y'/sm) = p_y` → sayılmaz. u = 2⁻⁵³ olmak üzere `p > p_y'(1 + 3u)` ise `fl(p/sm) ≥ (p/sm)(1−u) > (p_y'/sm)(1+u) ≥ p_y` → kesin sayılır. V77 eşik olarak `hi = p_y'·(1+2⁻⁵⁰)` kullanır (3u ≈ 3.3e-16 < 8.9e-16); `p_y' < p ≤ hi` bandındaki (nadir) elemanlarda **eski** `fl(p/sm) > p_y` karşılaştırması çalışır. Sonuç: rank sayıları V76 ile birebir aynı; AVX-512 yolunda v başına 1 zmm bölme (ve skaler test yolunda 1 bölme) kaldırıldı. val: 36 852×10 001 = 3.69e8 zmm bölme (2.95e9 şerit bölmesi), test: 3.69e8 skaler bölme. Doğrulama: 8 α için Top-1/Top-5 sayıları ve test Top-k V76 ile aynı. [Kesin]

S2 yeniden kullanımı: aşama 1 AVX-512 şeridi `ε₀` için `S += max((double)z[v], ε₀)` sıralı toplamdır; aşama 2 skaler döngü aynı işlem dizisi → aynı bit. [Kesin: kod + karma eşitliği]

---

## 6. Doğruluk karşılaştırması V76 ↔ V77

`dogrula.sh`, `olcek.sh`: aynı veri, aynı bayraklar, `V76_DEBUG=1`, `V53H_HASH=1`.

| büyüklük | 2.001 | 5.001 | 10.001 | 20.001 (karmasız) |
|---|---|---|---|---|
| Fc, AFF, RF, ACF, RC, Y0f, X, U, W_F karmaları | aynı | aynı | aynı | — (RAM) |
| T (tam) | aynı | aynı | aynı | — |
| 8 α val CE (15 hane), Top-1/Top-5 | aynı | aynı | aynı | aynı |
| test CE (15 hane), Top-1/Top-5 | aynı | aynı | aynı | aynı |
| seçilen (α₀, ε₀, α_Q) | (0.9, 1e-5, 0.2) aynı | (0.85, 1e-5, 0.2) aynı | (0.85, 1e-5, 0.3) aynı | (0.85, 1e-6, 0.2) aynı |
| val/test logit karması | — | — | aynı (V77 eski yol ile) | — |
| NaN / Inf | yok | yok | yok | yok |
| `lin e= a=` tablosu (seçim) | ≤1e-14 fark | ≤1e-14 | ≤1.07e-14 (9/32 satır, ≤12 ulp) | ≤1e-14 |

OMP_NUM_THREADS=3 ve 2'de de (2.001, `ham/th3_*`, `ham/th2_*`) tüm karmalar aynı. [Kesin]
`lin e= a=` ilk farklı ara büyüklük: **seçim tablosu** (toplama sırası). Öğrenmeyi etkilemediğinin kanıtı: argmin boşluğu 4.5e-4 (2.001) … 1.0e-3 (5.001), 1.3e-4 (20.001) ≫ 1e-14 → seçilen (α₀, ε₀) aynı. İlk farklı **model** ara büyüklüğü: yok. Not: V76'nın kendisi de iş parçacığı kısmi toplamlarını `critical` bölümünde sırası belirsiz ekler (koddan); yani bu tablonun ulp-seviyesi tekrarlanabilirliği V76'da da garantili değildi.

---

## 7. Süre (aynı veri/ayar, 4 iş parçacığı, dönüşümlü çiftler)

`ab.sh` N=7, `ab_ozet.py`; ham: `ham/A76_*`, `ham/B77_*`.

| çift | V76 duvar | V77 duvar | fark | oran |
|---|---|---|---|---|
| 1 | 11.96 | 9.20 | 2.76 | 0.77 |
| 2 | 11.36 | 10.35 | 1.02 | 0.91 |
| 3 | 11.70 | 9.83 | 1.88 | 0.84 |
| 4 | 11.43 | 9.34 | 2.10 | 0.82 |
| 5 | 11.18 | 9.41 | 1.77 | 0.84 |
| 6 | 12.21 | 10.36 | 1.85 | 0.85 |
| 7 | 11.26 | 9.34 | 1.92 | 0.83 |
| **medyan** | **11.43** | **9.41** | **1.88** | **0.83** |

Medyanlar [min–maks] — model: 5.54 [5.38–5.97] → 5.14 [5.05–5.43]; val: 3.91 [3.76–4.46] → 2.42 [2.23–2.93]; test: 1.89 [1.74–1.95] → 1.78 [1.77–2.00]; kullanıcı CPU 37.9 → 31.5 s; sistem 2.32 → 2.51 s. Bu oturumun makine durumu önceki oturumdan farklı (V76 medyanı 12.85 s idi, bugün 11.43 s); karşılaştırma yalnız aynı oturumdaki dönüşümlü çiftler arasında anlamlı.

**Ablasyon** (aynı V77 ikilisi, ortam değişkeniyle bir özellik kapalı, 3 tur dönüşümlü, medyan): taban 9.92 s; `V77_SPEK=0` **10.98 s (+1.06)**; `V77_DIV=0` 10.01 (+0.09); `V77_S2=0` 9.86 (−0.06); `V77_RHS2=0` 9.90 (−0.02); `V77_COLS=0` 9.85 (−0.07); `V77_TSYM=1` 9.77 (−0.15). Yani yalnız spekülatif tek geçiş duvar saatinde bağımsız ölçülebilir; diğerleri ±0.5 s gürültü bandı içinde. Bileşen zamanlayıcıları (`ham/B77_4.txt` ↔ `ham/A76_4.txt`): V49 0.86 → 0.53 s, FᵀY 0.17–0.48 → 0.13 s, V39 ortalama 0.10 → 0.08 s, kapı 0.19 → 0.16 s, F1 gemm 0.25 → 0.20 s.

---

## 8. Ölçekleme

| V | V76 duvar | V77 duvar | RSS | test PPL | eval-GEMM madd V76 → V77 |
|---|---|---|---|---|---|
| 2.001 | 5.78 [5.72–5.94] | 4.85 [4.70–4.97] | 1.29 → 1.26 GB | 88.893 | 1.13e11 → 7.57e10 |
| 5.001 | 7.99 [7.67–8.04] | 6.75 [6.64–6.77] | 1.44 → 1.38 GB | 173.827 | 2.84e11 → 2.09e11 |
| 10.001 | 11.43 | 9.41 | 1.57 GB | 262.930 | 5.67e11 → 3.86e11 |
| 20.001 | 84.6 | 64.6 | 6.44 GB | 252.531 | 4.92e12 → 3.87e12 |

2.001 ve 5.001: 3 dönüşümlü çiftin medyanı [min–maks] (`ham/sure*`, `olcek_sure.sh`); 10.001: 7 çift; 20.001 tek çiftli koşu (iki koşu da aynı makine durumunda ardışık; önceki V76 oturumunda 85–93 s, bu oturumda ilk koşuda 110 s görüldü → makine gürültüsü büyük; tek çift kesin hız iddiası değil). 2.001/5.001'de V77 RSS'i V76'dan düşük (FᵀY'nin 4×41 MB kısmi dizileri kalktığı için); 10.001'de tepe RSS başka bir aşamada (eval tamponları) oluştuğundan +3.6 MB.

**50 000 ve 100 000 kelime — yalnız yapısal fizibilite (gerçek veri yok, eğitim sonucu yok) [Tahmin: koddan okuma, çalıştırılmadı]:** varsayılan yolda V×V yoğun yapı kalmadı (yoğun kNN `S(V·V)` yalnız `V60_SPK=0` yolunda; `posy(V·V)` yalnız `!V64_COMPACT` yolunda). O(m_f·V) yapılar: W_F f32 103/205 MB, RF f64 205/410 MB, U f64 205/410 MB, Schur sağ taraf f64 205/410 MB. Logit tamponu `CH = 320 MB/(4V)` ile sınırlı: 1 600/800 satır. Eval sgemm maliyeti 513·V·(n_val+n_test): n_val=160k için V=50k'da ≈4.1e12 madd/geçiş. RSS ağırlıkla F (n×513×4 B) tarafından belirlenir; gerçek 50k/100k kelime korpusu mevcut değil, bu yüzden hiçbir süre iddia edilmiyor.

---

## 9. Yapılamayanlar ve uygulanmayanlar (dürüst liste)

1. **`rmsA` için fazladan 340 F1 sütunu (eğitim)**: 1.9e10 madd gemm + 1e8 tanh + 1e8 bölme + μ/σ geçişleri ≈ 0.4–0.5 s [Tahmin]. Bit-özdeş kaldırma yok. Eğitim satırlarının alt-örneğinden `rmsA` kestirmek ölçeği ~1e-4 değiştirir → farklı model → varsayılan olamaz; uygulanmadı.
2. **FᵀF / F1 / K-grafı / Schur**: bit-özdeşlik kısıtı altında daha az işlemle aynı sonuç **bulamadım**. BLAS ssyrk, V49 momentleri tek geçiş, F1 K 192→128 birleştirmesi, kNN blok-taklidi kaldırma V76'da toplama sırası/bit nedeniyle reddedilmişti; V77'de yeniden ölçmedim.
3. **Spekülasyon oranı**: 10.001'de %4.2, 20.001'de %36 yeniden hesap. Seçimin erken kesinleşmesini kanıtlayan bir sınır yok (CE farkı örnek başına sınırsız); iki aday için fuse yapmak yeniden hesabın gemm+fuse maliyetiyle aynı büyüklükte, uygulanmadı.
4. **T üst üçgen**: madd −3.7e9 ama bit-olarak üst üçgen farklı, kazanç gürültü altında → varsayılan kapalı (`V77_TSYM=1`).
5. **ACF kelime-slotu = Σ çift slotları**: geçerli özdeşlik, ≈0.05 s, bit değişimi → uygulanmadı.
6. **Test yolunda fuseRow8 (tek α için)**: bit-özdeş ama hızlanmadı → kapalı (`V77_T8=1`).
7. **F tahsis + sıfırlama** (`vector::resize` seri sıfır-doldurma, 0.2 s): veri tipi değişikliği gerektirir; dokunulmadı.

---

## 10. Değişen / eklenen madd ve işlem muhasebesi (soru 5–6)

| kalem | V76 | V77 | net |
|---|---|---|---|
| val logit sgemm | 3.781e11 | 1.969e11 | **−1.812e11** |
| test logit sgemm | 1.891e11 | 1.891e11 | 0 |
| **eval sgemm toplam** | **5.672e11** | **3.860e11** | **−1.812e11 (−%31.9)** |
| F1 gemm val/test satırları | 7.25e9 | 2.43e9 | −4.8e9 |
| V39 karışımı val/test | 3.8e8 | 1.3e8 | −2.5e8 |
| val S2 toplamı (max+ekleme) | 3.7e8 | 0 | −3.7e8 |
| rank bölmesi (zmm) | 3.69e8 | ~0 | −3.69e8 zmm (+3.69e8 skaler test) |
| FᵀY: sıfırlanan kısmi dizi + birleştirme | 4×41 MB + 2e7 ekleme | 41 MB tek dizi | −164 MB yazma |
| **toplam kaldırılan madd (sgemm sınıfı)** | | | **≈ −1.86e11** |
| T (varsayılan kapalı) | 1.122e10 | (7.5e9 açıkken) | (−3.7e9) |

**Eklenen:** yeniden hesaplanan val blokları 7.9e9 madd (yukarıdaki 1.969e11'e dahil), boşa spekülatif fuse (~%4.2 örnek), küçük saklama dizileri (6 MB), blok başına 160 karşılaştırma. **Net:** −1.81e11 (eval) − 5.1e9 (F1/V39 val/test) ≈ **−1.86e11 madd**; V76 toplam madd'inin ≈%27'si [Muhtemel: toplam ≈6.9e11 tahmini]. Hiçbir kaldırılan işlem başka yerde geri eklenmedi (eval sayacı `V77_MADD`, V76 eşdeğer yol `V77_SPEK=0` ile 5.6721e11 yazdırır = kullanıcının 5.67e11 doğrulaması).

---

## 11. Soruların cevabı

1. **V76'da gereksiz çıkan işlemler:** §2 (val logit tekrarı, S2 tekrarı, bölmeli rank, RHS kısmi diziler, V49 tanı döngüsü, val/test kullanılmayan F1 sütunları, V39 ortalama seri toplama).
2. **Tamamen kaldırılan ara büyüklükler:** ikinci val logit akışı (%95.8), `S2` yeniden toplamı, 4×41 MB FᵀY kısmi dizileri, val/test için 340 kullanılmayan F1 sütununun değerleri.
3. **Kaldırılan madd:** ≈1.86e11 (sgemm 1.81e11 + 5e9).
4. **Eklenen:** 7.9e9 yeniden hesap + ~%4 boşa fuse + 6 MB saklama.
5. **Net kazanç:** −1.81e11 eval-GEMM madd (−%31.9); toplamda ≈−1.86e11.
6. **Model hazırlama süresi:** 5.54 → 5.14 s (medyan, 7 çift).
7. **Validasyon ve test süresi:** val 3.91 → 2.42 s; test 1.89 → 1.78 s.
8. **Tüm program medyanı:** 11.43 → 9.41 s (−%17.7).
9. **V76 ile eşdeğerlik:** model ara büyüklükleri ve logitler bit-özdeş; CE/PPL/Top-k aynı; yalnız seçim tablosunda ≤1.1e-14 toplama-sırası farkı, seçim aynı.
10. **20.001 hız kazancı korunuyor mu:** evet, V76'nın 20.001 kazanımı (yoğun V×V yok, seyrek kNN, örtük W_C) aynen duruyor; ek olarak bu koşuda 84.6 → 64.6 s (tek çift; gürültü büyük).
11. **Kaldırılamayan işlemlerin matematiksel gerekçesi:** (a) test logiti: her test örneği için tam softmax paydası V sınıfın tümüne bağlı, `z = (F−ΣX)W_F + ΣY0` m_f·V madd gerektirir, düşük rank yok; (b) val aşama 1: seçim tüm örneklerin toplamına bağlı → en az bir tam geçiş; (c) FᵀF: ridge normal denklemi tam Gram'a bağlı; syrk simetrik yarısını zaten kullanıyor; (d) T, U, Schur: W_F çözümü için gerekli; (e) F1 eğitim sütunları: `rmsA` skaleri tüm 512 sütunun std'sine bağlı; (f) K-grafı: altı tablo ayrı kNN gerektirir.

---

## 12. Yeniden üretim

```
cd cpp && g++ -O3 -march=native -fopenmp -std=c++17 -o tek_hamle_v77 tek_hamle_v77.cpp -L$SL -l:libscipy_openblas*.so -Wl,-rpath,$SL
cd v77_matematiksel_islem_azaltimi
./dogrula.sh          # V76 <-> V77 karma + 15 haneli debug
./ab.sh               # N=7 ./ab.sh ; ab_ozet.py N
./olcek.sh etiket veri_dizini [HASH]
./ablasyon.sh         # her V77 özelliğini kapatma
```
V77 bayrakları: `V77_SPEK`, `V77_S2`, `V77_DIV`, `V77_RHS2`, `V77_COLS` (varsayılan 1), `V77_TSYM` (varsayılan 0), `V77_T8` (varsayılan 0), `V77_ZHASH=1` (logit karması), `V77_DEBUG=1` (spekülasyon etiketleri). `V77_SPEK=0 V77_S2=0 V77_DIV=0 V77_RHS2=0 V77_COLS=0` V76 koduna eşdeğer yolu çalıştırır.
