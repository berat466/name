# TEK HAMLE V49 — Tek değerden matematiksel ilişkiler (kuvvet ve etkileşim)

Kod: `cpp/tek_hamle_v49.cpp` (V46 kopyası; `V49=0/1/2/3` dönüşüm seçer, `V46_ONLY0=1` yalnız havuz-sinyal modunu çalıştırır; V46 ve öncesi dokunulmadı). Ham çıktılar: `v49_ham/` (`A_kontrol`, `B0_olcek2`, `B_kuvvet(_ext)`, `C_etkilesim(_ext)`, `seed*_*`, `tablo49.txt`, `seed_tablo.txt`). Sistem V46: tek F×512, havuz β=0.9, C0 kelime+çift kimliği, intercept, gruplu Ridge, V46 öğrenme sinyali (`ρ`), tek geçiş, λ_C=85; tohum 1 + 2 ek tohum. λ_F ve ρ **yalnız doğrulama CE ile** seçildi.

## Dönüşümler (havuzdan sonra, aynı 512 özellik kapasitesi)
A = havuzlanmış özellik sütunları h_j (512 adet; sabit-1 sütunu ayrı kalır). Dönüşüm havuz sonrası, Gram/sinyal öncesi yapıldı; böylece ZᵀZ, ACF, V46 sinyali hep dönüşmüş özelliklerle hesaplanır.
- **A (kontrol):** değişmeden.
- **2A (ölçek kontrolü):** h_j → 2h_j (aynı 512 sütun).
- **B (kuvvetler):** 170 taban özellik × {A, (A−μ)², (A−μ)³} = 510 + 2 ek A = 512. (A−μ) merkezi eğitim ortalamasıyla; işaret bilgisi A ve (A−μ)³ ile korunur.
- **C (etkileşimler):** 128 A + 128 (A−μ)² + 256 çarpım (A_i−μ_i)(A_j−μ_j) (i≠j, sabit rastgele çiftler) = 512.
Türetilmiş sütunlar eğitim momentleriyle merkezlenip A sütunlarının ortalama std'sine ölçeklendi (yalnız eğitim istatistikleri; doğrulama/test aynı sabitleri kullanır); değerler sınırlı (tanh ∈ (−1,1) karışımı, p≤3) → taşma yok (sütun RMS aralığı kontrol çıktısında). Ortak hesap: μ ve std tek geçişte; ZᵀZ, ACF, sayımlar yeni sütunlar üzerinde tek kez.

## Doğrusal ölçeğin yeni bilgi üretmediğinin kanıtı (2A kontrolü)
Z' = Z·D (D=diag(1; 2,…,2)). Ridge W' = (D ZᵀZ D + Λ)⁻¹ D ZᵀY = D⁻¹(ZᵀZ + D⁻¹ΛD⁻¹)⁻¹ZᵀY: tahminler Z'W' = Z (ZᵀZ+Λ')⁻¹ZᵀY, Λ' = Λ/4 (F sütunları). Yani 2A aynı model, λ_F/4. **Ölçüm bunu doğruladı:** 2A λ_F=1000 ≡ A λ_F=250: ρ=0 doğrulama CE 4.8878 = 4.8878; V46 sinyaliyle 4.8670 / test 123.91 = 4.8670 / 123.91 (birebir). 2A λ_F=3000 ≡ A λ_F=750 (ızgara dışı). Dolayısıyla "daha büyük/küçük ölçek" yalnız düzenlileştirmeyi oynatır; hiçbir yeni bilgi içermez.

## Sonuçlar (doğrulamayla seçilen λ_F, ρ)
| Sistem | λ_F, ρ | doğr. CE (PPL) | **test PPL** | Top-1 / Top-5 | hazırlık* |
|---|---|---|---|---|---|
| **V46 / A kontrol** | 1000, −0.25 | 4.8474 (127.4) | **121.32** | 0.1606 / 0.3681 | 1.2 s |
| 2A ölçek (≡A, λ_F=3000 seçilir) | 3000, −0.25 | 4.8491 | 121.59 | 0.1604 / 0.3669 | 1.6 s |
| **B kuvvetler (A,A²,A³)** | **3000, −0.25** | **4.8400 (126.5)** | **120.43** | 0.1604 / 0.3689 | 1.4 s |
| C etkileşimler (A,A²,AB) | 3000, −0.25 | 4.8521 | 121.61 | 0.1602 / 0.3695 | 1.3 s |
(*) F+havuz+dönüşüm+Gram hazırlığı; dönüşüm kendisi 0.5–0.7 s. Chol/Schur 0.3–0.5 s. V46 sinyali 0.4–0.7 s. Toplam ≈ V46 +0.5–0.7 s (dönüşüm). λ_F ızgarası: A: {100,250,300,1000,3000}; B,C: ayrıca {10⁴,3·10⁴,10⁵} — optimum iç noktada (B: 3000 → 4.8400; 10⁴ → 4.8505). Tam tablo `tablo49.txt`.
ρ=0 (V44, sinyalsiz) doğrulama CE: A 4.8675 (λ_F=1000), B 4.8655 (3000), C 4.8779 (3000): sinyalsiz B ≈ A, yani B'nin kazancı V46 sinyaliyle birlikte beliriyor.

## Tohum tekrarı (tohum 2, 3; yalnız A/B/C, λ_F∈{1000,3000}, ρ doğrulamayla)
| tohum | A (doğr.CE / test) | B | C |
|---|---|---|---|
| 1 | 4.8474 / 121.32 | 4.8400 / 120.43 | 4.8521 / 121.61 |
| 2 | 4.8484 / 120.98 | 4.8427 / 120.94 | 4.8487 / 122.09 |
| 3 | 4.8466 / 121.19 | 4.8380 / 120.87 | 4.8502 / 122.47 |
B doğrulama CE'de her tohumda A'dan iyi (−0.0057…−0.0086); test farkı −0.89 / −0.04 / −0.32 (ortalama ≈ −0.42 PPL; A'nın tohumlar arası yayılımı ≈ ±0.17). C testte A'dan kötü (+0.3…+1.3).

## ZᵀZ ile farkı (zorunlu açıklama)
ZᵀZ lineer özelliklerin **ikinci mertebe** momentlerini (Σ_n h_i h_j) içerir; model yine h'nin **doğrusal** fonksiyonudur: ŷ = Σ w_j h_j. Çapraz çarpım h_i h_j (veya h²) yeni bir **sütun** olduğunda model h_i h_j terimini de öğrenebilir: ŷ'ye w·h_i h_j eklenir, bu satır-bazında doğrusal-olmayan bir fonksiyondur ve doğrusal sütunların kombinasyonuyla ifade edilemez. Yeni Gram matrisi, eski ZᵀZ'de bulunmayan **üçüncü ve dördüncü mertebe** momentleri (Σ h_i h_j h_k, Σ h_i² h_j², …) içerir. Yani ZᵀZ özellikler arası ilişkiyi doğrusal (korelasyon) düzeyde zaten kullanır; B ve C ise ilişkiyi fonksiyon uzayına ekler. Ölçek dönüşümü ise (2A) aynı kolon uzayı olduğundan hiçbir fonksiyon eklemez (yukarıdaki özdeşlik).

## Değerlendirme (dürüst)
- **B (kuvvetler)** küçük ama tutarlı bir doğrulama kazancı veriyor (CE −0.006…−0.009 üç tohumda), testte ortalama ≈ −0.4 PPL; tohum 1'de −0.89. Bu, sınırlı kapasitede (170 taban yön, her biri A,A²,A³) 512 farklı doğrusal yönden iyi olması anlamına geliyor [Muhtemel]; hız maliyeti ≈ +0.5–0.7 s (dönüşüm). Etki küçük; kesin genelleme değil (3 tohum, tek bölünme).
- **C (etkileşimler)** kazanç vermedi (test A'dan kötü): rastgele çiftlerin AB çarpımları çoğunlukla gürültü; 128 tabana indirmek zarar.
- Kazancın kaynağı "daha fazla işlem" değil: kapasite 512'de sabit, B daha az taban yönle daha iyi; ama B'nin λ_F'si de farklı (3000) ve ρ-sinyaliyle birlikte etkili (sinyalsiz B≈A). Sebep (tanh sonrası kuvvet katkısı mı, λ-ölçek etkisi mi) tam ayrıştırılmadı: B'nin türetilmiş sütunları farklı etkin düzenlileştirme alıyor.
- Sızıntı: dönüşüm sabitleri yalnız eğitimden; doğrulama/test etiketi kullanılmadı. V46'nın eğitim hedefindeki olası ikinci-dereceden bağımlılığı (graf benzerliğinin satırın etiketine bağımlılığı) giderilmiş varsayılmadı; held-out ölçümler bundan etkilenmez.
- Sınırlar: tek F×512 seçimi; B'de taban sayısı/üs seçimi (170×3) ayarlanmadı; C'de çift seçimi rastgele; ölçek normalizasyonu tek biçim.
