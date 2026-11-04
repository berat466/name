# TEK HAMLE V44 — Tek hamlede en iyi düzenlileştirmeyi bul

Kod: V43 ikilisi (`cpp/tek_hamle_v43`, `V43_PASSES=1` tek geçiş; kaynak değişmedi, yeni kod yok). Sistem: tek F×512, bağlantılı havuz β=0.9, C0 kelime+çift kimliği, intercept (sabit sütun λ_C ile cezalı), gruplu Ridge; tohum 1, bölünme aynı. Ham çıktılar: `v44_ham/` (`tarama1.txt`, `tarama2.txt`, `sec_r*.txt`, `ref100_r*.txt`, `sure44.txt`). **Seçim yalnız doğrulama CE ile**; test PPL yalnız programın seçtiği ayar için basıldı.

## Verimlilik
Program λ_C listesinde her λ_C için C-bloğu grup çözümünü bir kez, her λ_F için yalnız 513'lük Schur çözümünü yapar; F, havuz, Gram, ACC/ACF tek sefer üretilir (λ kombinasyonu başına baştan hesap yok). Bu yüzden istenen "önce λ_C=100'de λ_F, sonra λ_C" sıralı taraması yerine tam ızgara tek koşuda yapıldı (32 kombinasyon; sıralı taramanın üst kümesi, ek maliyet küçük).

## Tarama 1 (λ_C ∈ {20,50,100,200} × λ_F ∈ {30,100,300,500,700,1000,1500,3000}), doğrulama CE
| λ_F → | 30 | 100 | 300 | 500 | 700 | 1000 | 1500 | 3000 |
|---|---|---|---|---|---|---|---|---|
| λ_C=20 | 4.9415 | 4.9195 | 4.9025 | 4.8982 | 4.8971 | 4.8972 | 4.8985 | 4.9029 |
| λ_C=50 | 4.9373 | 4.9085 | 4.8825 | 4.8739 | 4.8713 | **4.8705** | 4.8720 | 4.8781 |
| λ_C=100 | 4.9465 | 4.9161 | 4.8856 | 4.8747 | 4.8705 | **4.8687** | 4.8693 | 4.8753 |
| λ_C=200 | 4.9665 | 4.9339 | 4.9027 | 4.8912 | 4.8870 | 4.8849 | 4.8844 | 4.8894 |
En iyi: λ_C=100, λ_F=1000 (4.8687; V43 ile aynı). Optimum ızgara içinde (kenarda değil): λ_F için 700–1500 düz bölge, λ_C için 50–100.

## Tarama 2 (en iyi çevresinde λ_C ∈ {70,85,120,150} × λ_F ∈ {700,1000,1500})
| λ_C \ λ_F | 700 | 1000 | 1500 |
|---|---|---|---|
| 70 | 4.8692 | 4.8676 | 4.8682 |
| **85** | 4.8693 | **4.8675** | 4.8680 |
| 120 | 4.8730 | 4.8713 | 4.8720 |
| 150 | 4.8780 | 4.8764 | 4.8763 |
**Seçilen: λ_C=85, λ_F=1000 (doğr. CE 4.8675).** 70–100 arası fark 0.0001–0.0012 CE: düz bir plato; seçim 44 kombinasyon arasında doğrulama en iyisi olduğundan hafif doğrulama-seçimi iyimserliği taşır [Tahmin].

## Sonuç (seçilen ayar; tek geçiş)
| Ayar | doğr. CE (PPL) | **test PPL** | Top-1 / Top-5 | rezidü |
|---|---|---|---|---|
| V43 referansı: λ_C=100, λ_F=1000 (aynı koşulda yeniden koşuldu) | 4.8687 (130.1) | **124.43** | 0.1624 / 0.3679 | 7.4e-08 |
| **V44: λ_C=85, λ_F=1000** | **4.8675 (130.0)** | **124.27** | 0.1637 / 0.3691 | 7.5e-08 |
| (V42: λ_C=100, λ_F=3000) | 4.8753 | 124.92 | 0.1610 / 0.3673 | – |
Kazanç V43'e göre −0.16 PPL, doğrulamada 0.0012 CE: **ölçülebilir ama gürültü sınırında**; asıl kazanç V42→V43'te (λ_F 3000→1000) gelmişti. Sayısal çözüm rezidüsü ‖ZᵀZW+ΛW−ZᵀY‖/‖ZᵀY‖ = 7.5e-8 (doğru Λ ile; float gemm sınırı).

## Süre
- **Tek-hamle ağırlık üretimi** (F+havuz+Gram + C-blok grup çözümü/Schur hazırlığı + λ_F çözümü; 3 tekrar): λ_C=85: **1.30 / 1.40 / 3.87 s** (ilk tekrar gürültülü), medyan **1.40 s**; V43 ayarı (λ_C=100): 1.44 / 1.48 / 1.50 s, medyan 1.48 s. Süre değişmedi; V42'deki "≈2.2 s" ölçümü farklı kapsamdaydı (sunucu yük farkı ve ilk-koşu gürültüsü dahil), bugün aynı iş ≈1.4–1.5 s.
- **Toplam hiperparametre arama süresi:** tarama 1 (32 kombinasyon) 29.7 s, tarama 2 (12 kombinasyon) 19.7 s duvar saati = **≈49 s** (veri yükleme, F/havuz üretimi, her kombinasyon için doğrulama logit/okuma seçimi ve seçilen ayarın tam değerlendirmesi dahil; her λ_F noktası ≈0.3 s, her yeni λ_C ≈0.3–1 s). Seçilen tek ağırlık üretimi aramadan ayrı ≈1.4 s.

## Cevap
Ek geçiş olmadan yalnız düzenlileştirmeyle daha düşük PPL elde edilebiliyor, ama kazanç büyük ölçüde V43'te alındı: 124.92 → 124.43 (λ_F) → 124.27 (λ_C=85 ile). İnce ayar etkisi küçük (−0.16 PPL, doğrulama CE −0.0012). Optimum bölge düz (λ_C 70–100, λ_F 700–1500); bunun dışına çıkmak PPL'yi kötüleştiriyor. 122.90 hedefi bu yolla erişilemez görünüyor [Muhtemel].

## Sınırlar
Tek tohum, tek bölünme; α,ε doğrulamada kombinasyon başına seçiliyor (protokol gereği); λ_C'nin sabit sütun cezasını da belirlediğini unutmamak gerekir (ayrı sabit-sütun λ'sı denenmedi); gürültülü sunucu nedeniyle süre ±0.5 s.
