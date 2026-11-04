# TEK HAMLE V39 — Bağlantılı ortak bilgi havuzu

Kod: `cpp/tek_hamle_v39.cpp` (V38 üstüne, `V39=1`; V34–V38 yolları korunuyor). Ham çıktılar: `v39_ham/` (`tablo39.txt`). Taban ve ayarlar V37 hızlı F ile aynı: ID(w_{t−1}) + en sık 2000 çift + intercept (`V36_C0=1 V37_BIAS=1`), rastgele-çift F₁–F₄ (γ=1/0.5), H=4×128, tek ortak Ridge/Schur çözümü. Tek tohum, tek karıştırma. β, λ_C, λ_F doğrulama CE'siyle seçildi.

## Havuz ve bağlantılar
- **Düğümler:** 2001 kelime (önceki kelime w_{t−1}). Her düğümün özelliği = o kelimeyle biten **eğitim** satırlarının ortalama F₁–F₄ vektörü (513 sütun).
- **Bağlantılar (yalnız eğitim etiketleri):** iki kelime arasında "ortak bağlam" benzerliği = sonraki-kelime dağılımlarının Hellinger-kosinüsü (eğitim sayımlarından, ≥5 örnekli komşular, kNN=10, ağırlık ∝ benzerlik, normalize).
- **Etkisi:** her satırın F vektörü, bağlı kelimelerin düğüm özellikleriyle ağırlıklı karıştırılarak **yeniden hesaplanır**: `F'_i = (1−β)·F_i + β·Σ_k a_{w(i),k}·F̄_k` (eğitim, doğrulama, test aynı kuralla; doğrulama/test etiketi hiçbir yerde kullanılmaz). Özellik sayısı (4×128+1) değişmez. Bu F sütunlarının *değerlerini* değiştirir; ZᵀZ'nin yeniden ifadesi değildir. β=0 → V37 hızlı F (bire bir: 131.25).
- **Karıştırılmış kontrol:** aynı ağırlıklar, komşu kimlikleri rastgele permütasyonla değiştirilir (bağlantı hedefleri bozulur).
- Kapsam notu: havuzda yalnız kelime düğümleri var; kelime-çifti düğümleri ve F↔F özellik-benzerliği bağlantıları bu 10 dakikalık denemede **yapılmadı**.

## Sonuçlar (aynı koşul; β=0 aynı ikili kodda yeniden ölçüldü)
| β | gerçek: doğr. CE (PPL) | gerçek test PPL | Top-1 / Top-5 | karışık: doğr. CE | karışık test PPL |
|---|---|---|---|---|---|
| 0 (V37 hızlı F) | 4.9253 (137.7) | **131.25** | 0.1677 / 0.3665 | – | – |
| 0.3 | 4.9221 | 130.83 | 0.1685 / 0.3667 | 4.9254 | 131.33 |
| 0.6 | 4.9106 | 128.99 | 0.1610 / 0.3645 | 4.9253 | 131.52 |
| **0.9 (doğrulamayla seçilen)** | **4.8930 (133.4)** | **126.87** | 0.1586 / 0.3639 | 4.9301 | 132.51 |
| 1.0 | 4.8931 | 126.93 | 0.1568 / 0.3622 | 4.9332 | 132.84 |
Seçilen: β=0.9, λ_C=100, λ_F=3000. Karışık bağlantıda aynı β için seçim: λ_C=20, λ_F=3000.
Referans: kimlik+çift, F kapalı 133.20 (V37).

## Süre
F zinciri+Gram hazırlığı ≈1.3–1.4 s (V37 hızlı F ile aynı); bağlantı grafı + karışım ≈0.2–0.3 s ek. Yani ağırlık üretim süresi ≈+0.25 s. (Raporlanan `V31_TOPLAM` değerleri doğrulama taramaları ve değerlendirmeyi içerir, süre ölçümü olarak kullanılmadı.)

## Yorum
- **Gerçek−karışık bağlantı farkı:** test 126.87 vs 132.51 (β=0.9) = **5.6 PPL**; doğrulama CE 4.8930 vs 4.9301. Karışık bağlantı V37'nin (131.25) bile gerisinde: yani kazanç özellik karıştırma yapmaktan değil, **doğru** kelimelerin birbirine bağlanmasından geliyor. Sonuç tek tohum/tek karıştırmada ama fark çok büyük (0.037 CE) ve β ile tutarlı biçimde artıyor (0.3→0.9: 130.83→128.99→126.87), düzlük β≈0.9–1.0.
- **Ne olabilir:** bu, eğitimde benzer sonraki-kelime dağılımına sahip kelimeler arasında F özelliklerini paylaşan bir sınıf-tabanlı yumuşatma [Muhtemel]; seyrek kelimelerin F vektörleri benzer kelimelerin ortalamasından güç alıyor. Sızıntı kontrolü: bağlantılar ve düğüm özellikleri yalnız eğitim satırlarından; eğitim satırı kendi kelimesini komşu olarak kullanamaz (kendisi hariç); doğrulama/test satırları yalnız önceki kelimeleri ve eğitim düğümlerini kullanır. Doğrulama/test etiketi yok.
- **Dikkat edilmesi gerekenler:** Top-1 düşüyor (0.1677→0.1586) ve Top-5 hafif düşüyor, yani kazanç PPL/CE'de (olasılık kalitesinde), en olası kelimeyi bulmada değil. Seçilen λ_C da 20'den 100'e kaydı (daha güçlü kimlik cezası): kazancın ne kadarı F'nin kendisinden, ne kadarı F ile kimlik bloğunun etkileşiminden geliyor ayrıştırılmadı. Eğer β=1'de F satırı yalnız önceki kelimeye bağlı bir fonksiyon oluyorsa (X'in öbür bağlamı kayboluyor), F fiilen kelime-kümesi ID özelliklerine yaklaşıyor: bu, "bağlantılı havuz" yerine bir kelime-sınıfı yumuşatması olabilir.
- **Hâlâ yapılmamış:** farklı kNN, τ, çift düğümleri, F↔F bağlantıları; çoklu tohum; karışık kontrolün birden fazla permütasyonu; bu kazancın klasik bir kelime-sınıfı (ör. benzer kelime ortalaması) taban çizgisiyle karşılaştırılması.

## Ana soru
Evet — bu denemede, bilgilerin eğitimden hesaplanmış bağlantılarla ortak havuzda karıştırılması ayrı F üretiminden belirgin biçimde daha iyi CE/PPL verdi: **131.25 → 126.87 test PPL** (−4.4), karışık bağlantıyla 132.51 (gerçek lehine 5.6 PPL fark). Top-1/Top-5 ise biraz geriledi. Tek tohum olduğundan kesin genelleme sayılmaz; kazanç büyük ve karışık-bağlantı kontrolünden net ayrıştığı için V35–V38'in tersine anlamlı bir sinyal.
