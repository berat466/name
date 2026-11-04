# TEK HAMLE V46 — Öğrenilmiş ağırlığın sinyalini matematiksel olarak üret

Kod: `cpp/tek_hamle_v46.cpp` (V45 kopyası; `V46=1` ile açılır, V44/V45 yolları korunuyor). Ham çıktılar: `v46_ham/` (`v46_main.txt` kaba ρ ızgarası + 4 mod, `fine_bq*.txt` ince ρ ızgarası). Sistem: tek F×512 + bağlantılı havuz (β=0.9, kNN=10), C0 kelime+çift kimliği, intercept, gruplu Ridge, tek geçiş, λ_C=85, λ_F=1000, mevcut doğrusal okuma; tohum 1, aynı bölünme. **Yalnız çıkış ağırlıkları** etkileniyor; iç F ağırlıklarının öğrenildiği iddia edilmiyor.

## Formül
`W = (ZᵀZ+Λ)⁻¹(ZᵀY + ρ·S)`, `S = Zᵀ(Y−Q)` ⇒ RHS = Zᵀ((1+ρ)Y − ρQ): Ridge'in hedefi Y'den `(1+ρ)Y−ρQ`'ya kayıyor (ρ<0: Y ile Q'nun karışımı). Çözüm tek hamlede, doğrusallık sayesinde `W = (1+ρ)W_Y − ρW_Q`: Q için gruplu Cholesky çözümü bir kez (V sağ taraf), Schur sağ tarafı ρ'ya göre doğrusal birleşir; ekstra döngü yok.
**Q (yalnız eğitim, LOO):** `Q_i = (1−b)·(cnt_w − e_{y_i})/(n_w−1) + b·N_w`, `N_w = Σ_v a_wv P_v` (havuzdaki aynı kNN bağlantıları, P_v = komşu kelimenin eğitimdeki sonraki-kelime dağılımı; kendi kelimesi komşu değildir, satırın kendi etiketi çıkarılmıştır, n_w<2 ise yalnız N_w). b=0.9 (sabit; {0.7, 0.9, 1.0} doğrulamada karşılaştırıldı). ZᵀQ, ZᵀY'den gelen toplamlardan **kapalı formla** hesaplanıyor (C kısmı sayımlardan, F kısmı Fsumᵀ·M ve tek bir LOO düzeltme geçişi); n×V boyutlu Q matrisi hiç oluşturulmuyor.
**Kontroller (aynı formül, farklı Q):** mod1 = komşu kimlikleri karıştırılmış bağlantılar; mod2 = yalnız kendi-kelime LOO dağılımı (komşu yok); mod3 = basit etiket yumuşatma (Q=unigram). ρ=0'da dört modda da referans ağırlıklar geri geliyor (doğr. CE 4.8675, ağırlık değişimi 0.000, test 124.27).

## Sonuçlar (ρ yalnız doğrulama CE ile seçildi; test seçimden sonra okundu)
| Sistem | seçilen ρ | doğr. CE (PPL) | **test PPL** | Top-1 / Top-5 |
|---|---|---|---|---|
| V44 mevcut formül (ρ=0) | 0 | 4.8675 (130.0) | **124.27** | 0.1637 / 0.3691 |
| V45 ağırlıklı bağlantı (referans, ayrı deney) | – | 4.8662 | 124.16 | 0.1637 / 0.3683 |
| **V46 gerçek öğrenme sinyali (pool-Q, b=0.9)** | **−0.25** | **4.8474 (127.4)** | **121.32** | 0.1606 / 0.3681 |
| V46 karıştırılmış bağlantı sinyali | 0 (kaba ızgarada; −0.1'de 126.80) | 4.8675 | 124.27 | 0.1637 / 0.3691 |
| Yalnız kendi-kelime LOO (komşusuz) | 0 (−0.1'de 124.20) | 4.8675 | 124.27 | – |
| Basit etiket yumuşatma (unigram) | 0 | 4.8675 | 124.27 | 0.1637 / 0.3691 |
ρ taraması (gerçek sinyal, b=0.9; doğr. CE): ρ=−1: 5.0589, −0.5: 4.8636, −0.4: 4.8531, −0.3: 4.8480, **−0.25: 4.8474**, −0.2: 4.8483, −0.15: 4.8507, −0.1: 4.8541, 0: 4.8675, +0.1: 4.8859, +0.25: 4.9142. Optimum ızgara içinde. b: 0.7 → en iyi 4.8482 (ρ=−0.3, test 121.36); 0.9 ve 1.0 → 4.8474 (ρ=−0.25; test 121.32). Pozitif ρ (Q'dan uzaklaştırma) her modda zarar veriyor. Karışık bağlantılarda ρ<0 doğrulamayı belirgin kötüleştiriyor (ρ=−0.25: CE 4.9438, PPL 140.3).

## Ağırlıkların değişimi ve sinyalin dağılımı
- ρ=−0.25: bağıl değişim ‖ΔW_C‖/‖W_C‖ = 0.215, ‖ΔW_F‖/‖W_F‖ = 0.106 (ρ ile doğrusal: ρ=−0.5 → 0.43/0.21).
- Sinyal normu ‖S‖/‖ZᵀY‖ = 0.42 (gerçek), 0.74 (karışık), 0.016 (kendi-kelime), 0.44 (unigram). Sinyalin **%99.2'si F satırlarında**, %0.8'i C satırlarında (gerçek mod). Çıkış-sütun bazında (kare norm) q10=13, medyan=69, q90=1.3e3, maks=1.2e7: çok eğik, sinyal sık/ortak çıkış kelimelerinde yoğunlaşıyor.
- ρ=−0.25 ağırlık değişiminin W_C'de büyüğü bağıl fakat ZᵀY'de C'nin payı küçük; bu nedenle etkiyi F satırlarındaki kayma taşıyor [Muhtemel].

## Süre
- Sinyal hesabı (gerçek mod; Pn, M, 513×2001×2001 dgemm, LOO geçişi, Q için gruplu çözüm, ACFᵀY0q): **1.4–1.8 s** (3 koşuda 1.62 / 1.52 / 1.37 s). Diğer modlar 0.2–0.55 s.
- ρ başına çözüm (Schur çözümü + X·W_F gemm + doğrulama değerlendirmesi): ayrı ölçülmedi (tarama toplamı ≈ 7 ρ × birkaç yüz ms).
- Tek-hamle toplamı ≈ V44'ün 1.4–1.8 s'si + sinyal 1.4–1.8 s + tek ρ çözümü ≈ **3–4 s** (temiz, tek-ρ tek koşu olarak ayrıca ölçülmedi; tahmini). Sinyal hesabı optimize edilmedi (Pn/M yoğun V×V, dgemm 2 GFLOP; LOO geçişi satır başına sütun-sıralı).

## Değerlendirme
- Gerçek sinyal, tek geçişle ve ek eğitim olmadan **doğrulamada −0.020 CE (130.0→127.4 PPL), testte 124.27→121.32 (−2.95 PPL)** verdi. Karışık bağlantılı sinyal, kendi-kelime sinyali ve unigram yumusatma hiçbir iyileşme vermedi (hepsi ρ=0'a döndü); yani kazanç (i) yumuşatmanın kendisinden değil, (ii) komşu kelime dağılımlarının **gerçek** bağlantılar üzerinden aktarılmasından geliyor. Bu, V35–V45'in aksine gerçek-karışık ayrımı net olan ilk sonuç (fark: 121.32 vs 124.27; doğr. CE 4.8474 vs 4.8675).
- Mekanizma: ρ<0 ile Ridge hedefi Y'den komşu-kelime dağılımına doğru çekiliyor (≈ 0.75·Y + 0.25·Q) → bilgi aktarımı distilasyon benzeri bir yumuşak hedef; yeni bir öğretmen modeli yok (Q, eğitim sayımlarından kapalı formda).
- Top-1 hafif düştü (0.1637→0.1606), Top-5 aynı; kazanç olasılık kalitesinde.

## Sınırlar ve dikkat
- **Sızıntı:** Q'nun kendi satırının etiketi LOO ile çıkarıldı; komşu seti kendi kelimesini içermez. Ancak bağlantı benzerliği S_wv ve kNN seçimi eğitim sayımlarından (satırın kendi etiketi dahil) hesaplanıyor — küçük ikinci-dereceden bağımlılık, özellikle seyrek kelimelerde; doğrulama/test satırlarının etiketi hiçbir yerde kullanılmadı, doğrulama/test kazançları gerçek hold-out ölçümüdür.
- Tek tohum, tek bölünme; ρ ve b seçimi 7+3 değer üzerinden doğrulamada; 4 mod karşılaştırması tek karıştırmayla.
- Pozitif ρ ve ρ=−1 zararlı; ρ ve b'nin etkileşimi ince taranmadı.
- Süre toplamı tahmini; V44/V45 korunuyor, V46 varsayılan olarak kapalı (`V46=1`).
