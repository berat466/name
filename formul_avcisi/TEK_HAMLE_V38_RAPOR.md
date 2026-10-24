# TEK HAMLE V38 — Bilginin hangi F ağırlığına yerleşeceğini hesapla

Kod: `cpp/tek_hamle_v38.cpp` (V37 üstüne, `V38=1`; V34–V37 yolları korunuyor). Ham çıktılar: `v38_ham/` (`tablo38.txt`). Taban: V37'nin doğruladığı kimlik+çift (ID(w_{t−1}) + en sık 2000 çift) **intercept'li** (`V36_C0=1 V37_BIAS=1`), F₁–F₄ yapısı, H=4×128, tek ortak Ridge/Schur çözümü, aynı doğrusal okuma. Tek tohum, tek karıştırma. Hiperparametreler yalnız doğrulama CE'siyle seçildi.

## Adresleme kuralı (yalnız C++ matematik, yinelemeli öğrenme yok)
Her aşamada (F_s girdisi `[X, F_1..F_{s-1}]`, V28 ölçekleriyle) 128 özellik **tek tek** yerleştirilir:
1. **Giriş ilişkisi seçimi:** girdi, ridge-beyazlatılır (`Σ→(Σ+ε·tr(Σ)/d·I)^{-1/2}`, ε doğrulamada) → giriş ilişkileri birbirinden bağımsız hâle gelir.
2. **Hangi giriş ilişkisi → hangi çıkış ilişkisi:** kalan artık `R` (Y − OOF kimlik+çift tahmini − önceki aşamaların tahmini) ile beyaz girdi arasındaki çapraz-kovaryansın `M=ZᵀR` en büyük tekil çifti `(u_j, v_j)` hesaplanır. `u_j` o özelliğin giriş yönüdür (W, bias doğrudan `Wh·u_j/σ_j`).
3. **Tekrarın azaltılması (iki mekanizma):** (a) `u_j` önceki yerleştirilmiş yönlere dik tutulur (aynı giriş ilişkisi iki özelliğe gitmez; ölçülen en büyük |cos| = 0.000); (b) özellik `f_j=tanh(γ·Zu_j/σ_j)` kurulduktan sonra artık, `f_j`'nin **kendi doğrusal-olmayan katkısı çıkarılarak** güncellenir (`R←R−f_j a_jᵀ`, `M←M−(Zᵀf_j)a_jᵀ`; `R`'nin `M` satır uzayına izdüşümü üzerinde, tam eşdeğer). Böylece her özellik kalan bilginin farklı bir parçasını taşır.
Bu, yalnız sütun sıralaması değildir: W ve bias değerleri bu hesapla belirlenir. V35–V37'den farkı: beyazlatma + tek tek deflation (V35/V36 tek `M`'nin ilk 128 tekil vektörünü aynı anda alıyordu). Toplam özellik sayısı (4×128), Ridge, λ'lar aynı.

## Sonuçlar (aynı koşul; seçilen ayar doğrulamayla)
| Sistem | γ / ε | doğr. CE (PPL) | test PPL | Top-1 | Top-5 | F üretim süresi |
|---|---|---|---|---|---|---|
| Kimlik+çift, F kapalı (V37 ölçümü) | – | 4.9356 (139.2) | **133.20** | 0.1655 | 0.3641 | – |
| V37 hızlı (rastgele-çift) F (V37 ölçümü) | 1 / – | 4.9253 (137.7) | **131.25** | 0.1677 | 0.3665 | ≈1.2 s |
| **V38 gerçek hedef adresleme** (doğrulamada en iyi) | 0.5 / 0.5 | **4.9234 (137.5)** | **131.71** | 0.1681 | 0.3667 | ≈24 s |
| V38 karışık hedef (aynı ayar) | 0.5 / 0.5 | 4.9265 (137.9) | 132.18 | 0.1677 | 0.3639 | ≈28 s |
Referans satırları V37 koşularından alınmıştır (yeni ölçüm değil; aynı kod tabanı, aynı veri). Diğer ayarlar:
| V38 gerçek / karışık | doğr. CE gerçek | test gerçek | doğr. CE karışık | test karışık |
|---|---|---|---|---|
| γ=0.1, ε=0.05 | 4.9252 | 131.89 | 4.9252 | 132.01 |
| γ=0.5, ε=0.05 | 4.9262 | 131.81 | 4.9283 | 132.25 |
| γ=1, ε=0.05 | 4.9356 (F kapalı seçildi) | 133.20 | 4.9356 (F kapalı) | 133.20 |
| γ=0.5, ε=0.5 | 4.9234 | 131.71 | 4.9265 | 132.18 |

## "Bilgi-adres eşleşmeleri gerçekten kullanıldı mı?" kontrolü
- **Mekanik kullanım: evet.** Artık sinyalden özelliklere aktarılan enerji (F1/F2/F3/F4, γ=0.5, ε=0.05): gerçek hedef 0.037/0.012/0.003/0.003, karışık hedef 0.007/0.004/0.001/0.002. İlk tekil değerler gerçekte çok büyük (F1: 2581 vs karışıkta ~417): gerçek residual'ın girdiyle gerçek bir çapraz-kovaryansı var, karışıkta yok. Yönlerin tekrarı yok (maks |cos| 0.000, her aşamada tam 128 yön; sayısal rank M: 128/256/384/≈510 ≥ 128, rastgele doldurma yok).
- **Tahmin kalitesine etkisi: çok küçük.** Gerçek−karışık: doğrulama CE farkı 0.000–0.003 (γ=0.1'de 0.000), test farkı 0.12–0.47 PPL. Gerçek taraf her ayarda ya eşit ya biraz iyi, ama fark tek tohumda gürültü düzeyine yakın; ε=0.5 satırı en büyük fark (0.003 CE, 0.47 PPL) ve bu ayar doğrulamada seçilen ayarla aynı.

## Dürüst sonuç
- Sorulan şey: hesaplanmış adresleme mevcut F yönteminden daha iyi öğrenme sağladı mı? **Hayır (kanıtlanamadı).** Test: V38 131.71 vs V37 hızlı F 131.25 (V38 0.46 PPL **kötü**); doğrulamada V38 0.002 CE iyi (4.9234 vs 4.9253). Yani yön tutarsız ve fark gürültü seviyesinde. Kimlik+çift 133.20'ye göre kazanç −1.5 PPL; V37'de bulunan "herhangi bir iyi doğrusal-olmayan genişleme ≈ −1.8 PPL" bandının içinde.
- Karışık hedefle fark küçük ama tutarlı biçimde gerçek lehine (γ=0.5: 0.44; ε=0.5: 0.47 PPL; γ=0.1: 0.12). Bu, ilk kez gerçek residual'ın karışık kontrolden ölçülebilir biçimde (ama küçük) iyi çıkmasıdır [Muhtemel]; tek tohum ve tek karıştırmayla "öğrenme başarısı" olarak sunmuyorum: kazanç hem hızlı rastgele F'nin hem de karışık adreslemenin gürültü payı içinde.
- Hız: V38 F üretimi ≈24–29 s (m=512 art arda 128 güç yinelemesi ve n×r deflation) vs hızlı F ≈1.2 s: ≈20× yavaş; optimize edilmedi.
- Sınırlar: tek tohum; ε yalnız {0.05, 0.5}, γ {0.1, 0.5, 1}; ε=0.5 sınır noktası (daha büyüğü denenmedi); aşama Ridge λ=1000 sabit; OOF 2-kat; "hangi ilişkiler aynı özellikte tutulsun" kararı bu çalışmada yalnız dik-yerleştirme kuralıyla verildi (aynı özellikte birden fazla ilişkiyi birleştirme denenmedi).
