# TEK HAMLE V36 — Eksik öğrenme sinyalinden (residual) iç ağırlık üretimi

Kod: `cpp/tek_hamle_v36.cpp` (V35 kodunun üstüne; V34/V35 dosyaları duruyor, hızlı OpenBLAS yolu korundu). Ham çıktılar: `v36_ham/`. Bayraklar: `V36_C0=1` (taban = ID(w_{t−1}) + en sık 2000 sıralı çift), `V36=1` (residual-F), `V36_LOOF` (OOF ridge λ), `V35_YSHUF=1` (V36'da: residual SATIRLARI karıştırılır), `V35_RANK=1`. Aynı veri, c=2, H=4×128, tohum 1. Test hiçbir seçimde kullanılmadı.

## Yöntem
1. Taban `C` = ID(w_{t−1}) (2001) + eğitimde en sık 2000 tam (w_{t−2},w_{t−1}) çifti (4001 seyrek sütun). Son ağırlıklar mevcut ortak Ridge/Schur çözümüyle tek hamle: `Z=[C,F1..F4]`, `W=(ZᵀZ+Λ)⁻¹ZᵀY`.
2. Residual: yalnız eğitim etiketleriyle **2-kat out-of-fold** kimlik+çift ridge (λ=100; her yarı diğer yarıyla eğitilen modelden tahmin alır) → `R = Y − Ŷ_oof`.
3. F1 yönleri: `(X−μ)ᵀR` matrisinin ilk 128 tekil vektörü (randomize alt-uzay SVD, sketch 138, 3 güç adımı), beyazlatma, `tanh(γ·)`. F2–F4: girdi `[X, önceki F'ler]`, R = kalan sinyal (Y − OOF kimlik − önceki aşamaların kümülatif ridge tahmini). Gradyan/Adam/backprop yok. Sketch'teki rastgele matris yalnız SVD'yi hesaplamak için; yönler tekil vektörlerdir, rastgele doldurma yok. SVD rankı F genişliğini kısıtlamadı (D=192/320/448/576 ≥ 128).
4. Seçim yalnız doğrulamada: γ ∈ {0.1, 0.5, 1}, λ_C ∈ {20,100}, λ_F ∈ {10⁴ … 3·10⁵}.

## Önemli düzeltme (V35 raporu için)
V35 raporunda "girdi boyutu 8290 (yoğun+seyrek)" yazdım; o satır bir **yazdırma hatasıydı**: F zincirinin girdisinde kimlik yoktu (idm=0), gerçek girdi boyutu 192/320/448/576'dır. Sonuçlar etkilenmedi, yalnız raporlanan boyut yanlıştı.

## Referans 133,2 yeniden üretilemedi
V26c'nin 133.2'si bu hattın kopyası değil: aynı özellik tanımıyla (ID(w_{t−1}) + 2000 çift, V31 hattı, ridge+doğrusal okuma) ölçümüm **doğr. 146.1 / test 140.05** çıktı (V26c: doğr. 139.2 / test 133.2). Farkın nedenini (V26c'nin ek ayrıntısı, ölçekleme vs.) **bulmadım**. Ayrıca "kelime kimliği + hash çift (4096 kova)" tabanı denendi: yalnız-kimlik test 144.74. Bu yüzden karşılaştırmaları **aynı hat içinde** yapıyorum; 133.2 sadece ek referans.

## Sonuçlar (aynı hat, aynı bölünme; seçilen ayarlar doğrulamayla)
| Sistem | doğr. CE (PPL) | test PPL | Top-1 | Top-5 |
|---|---|---|---|---|
| (V26c raporu) kelime+çift, farklı hat | 4.9356 (139.2) | 133.2 | 0.166 | 0.364 |
| Kimlik+çift C, F kapalı (bu hat; λ_F=10⁹ kontrolü aynı sayıyı verdi) | 4.9840 (146.1) | 140.05 | 0.1610 | 0.3566 |
| C + V34 rastgele-çift F (γ=1/0.5, λ_C=20, λ_F=10⁴) | 4.9251 (137.7) | 131.39 | 0.1679 | 0.3661 |
| V35 hedef-bilgili F (başka taban, referans) | – | 140.91 | 0.1580 | 0.3606 |
| **V36 C + residual-F (γ=0.1, λ_C=100, λ_F=3·10⁴)** | **4.9128 (136.0)** | **129.67** | 0.1667 | 0.3659 |
| Kontrol: aynı ama residual SATIRLARI karışık | 4.9109 (135.8) | **129.55** | 0.1657 | 0.3649 |

- F'nin ek kazancı (aynı C tabanında): 140.05 → 129.67, yani **−10.4 PPL**. Rastgele V34 F'si de −8.7 PPL veriyor (131.39); residual-F rastgele-F'den 1.7 PPL iyi (doğr. CE farkı 0.0123).
- **Gerçek vs karışık residual: 129.67 vs 129.55 (doğr. 4.9128 vs 4.9109); fark yok, karışık hafifçe iyi.** Yani kazancın kaynağı residual bilgisi değil.
- F kapatılınca (λ_F=10⁹) yalnız kimlik+çift çözümü bire bir geri geldi (doğr. CE 4.9840 ve 4.9902, aynı λ_C'lerdeki F'siz sayılarla özdeş; test 140.05).
- γ taraması (doğr., λ_F seçilmiş): γ=0.1: 4.9128, γ=0.5: 4.9495, γ=1: 4.9739. En iyi γ grid kenarında (daha küçük denenmedi). λ_F=3·10⁴ iç noktada.

## Özellik (F) efektif rankı (PR) / sayısal rank
| | F1 | F2 | F3 | F4 |
|---|---|---|---|---|
| V34 rastgele F (V35 koşusundan) | 4.9 | 2.0 | 1.4 | 1.2 |
| V36 gerçek residual, γ=0.1 | 76.2 | 82.6 | 95.9 | 104.2 |
| V36 karışık residual, γ=0.1 | 101.6 | 82.7 | 91.2 | 109.9 |
Sayısal rank 128 (hepsi). Yönler çeşitli; ama karışık residual de aynı çeşitlilik ve aynı doğruluğu veriyor.

## Süre (tek hamle ağırlık üretimi; 4 çekirdek)
- V36 F üretimi + Gram: **≈10.1–11.1 s** (bunun içinde 2-kat OOF kimlik ridge ≈2.9–5.5 s; SVD/aşama ridge'leri geri kalanı). Cholesky/Schur ≈1.3–1.5 s (λ_C=100).
- V34 rastgele F: ≈1.2 s + aynı Cholesky/Schur. Yani V36 ≈ 5–6× yavaş. Optimizasyon yapılmadı.

## Cevap: F₁–F₄ kimliğin öğrenemediği yeni bilgiyi hesapladı mı?
**Hayır, ölçümle gösterilemedi.** F gerçek kazanç sağlıyor (−10.4 PPL, F kapatınca yok), ama aynı kazancı residual satırları karıştırılmış (X ile ilişkisi kopuk) yönler de veriyor (129.55). Dolayısıyla kazanç, "kimliğin açıklayamadığı sinyali yakalama"dan değil, X üzerinde genel bir doğrusal-olmayan genişlemeden (düşük γ ile hafif doğrusal-olmayan, yüksek ranklı özellikler) geliyor [Muhtemel]. Residual-F'nin rastgele V34 F'ye göre +1.7 PPL üstünlüğü var ama bu, karışık-residual kontrolüyle açıklanamayan bir şey değil: kontrol de aynı seviyede; üstünlük yönlerin hedefe duyarlılığından değil rank/çeşitlilik farkından gelmiş olabilir [Tahmin].

## Sınırlar
- Tek tohum, tek karıştırma, 2-kat OOF (yarı veriyle kurulan residual zayıf olabilir; 5-kat denenmedi).
- 133.2 yeniden üretilemedi; karşılaştırma aynı hat içinde yapıldı.
- γ grid kenarında; aşama ridge λ=1000 ayarlanmadı; F girdisi X-only (kimlik yok).
