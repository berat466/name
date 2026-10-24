# TEK HAMLE V40 — Dört F yerine tek F + ortak havuz

Kod: `cpp/tek_hamle_v40.cpp` (V39 kopyası; yalnız F aşama sayısı/genişliği `V40_STAGES`, `V40_W` ile seçilir; V39 ve öncesi dokunulmadı). Havuz, bağlantılar, Ridge çözümü, intercept (`V37_BIAS=1`), taban (ID(w_{t−1}) + en sık 2000 çift) ve veri bölünmesi V39 ile aynı. **Tek F gerçek tek dönüşümdür**: doğrudan X'ten (192 boyut) tanh(γ·XA+b), A rastgele-çift yönleri (V34/V37 yöntemi), kopya yok, zincir yok. Havuz yalnız kelime düğümleri (V39 ile aynı), bağlantılar yalnız eğitim etiketlerinden. β, λ_C, λ_F doğrulama CE'siyle seçildi; test seçimde kullanılmadı. Tek tohum.

## Sonuçlar (β=0.9; doğrulamayla seçilen λ_C=100, λ_F=3000)
| Yapı | doğr. CE (PPL) | **test PPL** | Top-1 / Top-5 |
|---|---|---|---|
| 4F × 128 = 512 (V39 referansı, yeniden ölçüldü) | 4.8930 (133.4) | **126.87** | 0.1586 / 0.3639 |
| **Tek F × 512** | **4.8753 (131.0)** | **124.92** | 0.1610 / 0.3673 |
| Tek F × 128 | 4.8990 (134.2) | 127.65 | 0.1586 / 0.3643 |

β taraması (yalnız doğrulamada değerlendirildi; seçim β=0.9):
| β | 4F×128 doğr.CE / test | Tek F×512 doğr.CE / test | Tek F×128 doğr.CE / test |
|---|---|---|---|
| 0 (havuz kapalı) | 4.9253 / 131.25 | 4.9232 / 131.16 | 4.9249 / 131.24 |
| 0.6 | (V39) 4.9106 / 128.99 | 4.8992 / 127.92 | 4.9118 / 129.03 |
| 0.9 | 4.8930 / 126.87 | 4.8753 / 124.92 | 4.8990 / 127.65 |
| 0.9, karışık bağlantı | (V39) 4.9301 / 132.51 | 4.9269 / 132.02 | – |

## Süre (eşit kapsam: tek λ_C=100, tek λ_F koşusu; F üretimi + havuz + Gram + RHS ("hazırlık") ve C-bloğu Cholesky + X çözümü + Schur; 4 çekirdek, 2 tekrar)
| Yapı | F+havuz+Gram hazırlığı | Cholesky+Schur | **Toplam ağırlık üretimi** |
|---|---|---|---|
| 4F × 128 | 1.29 / 1.42 s | 2.52 / 2.43 s | **≈3.8–3.9 s** |
| Tek F × 512 | 0.90 / 0.86 s | 2.25 / 2.67 s | **≈3.2–3.5 s** (≈1.1–1.2× hızlı) |
| Tek F × 128 | 0.40 / 0.43 s | 1.07 / 1.13 s | **≈1.5 s** (≈2.5× hızlı) |
(Havuz hesabı her birinde 0.16–0.22 s, hazırlığın içinde. Ölçümler gürültülü: aynı yapının iki tekrarı arasında ±0.4 s; Cholesky+Schur süresi, boyutu F genişliğine bağlı Schur sistemi nedeniyle F'ye göre değişiyor. Tablodaki farklar yalnız büyüklük sırası kadar güvenilir.)

## Kontroller
- **Havuz kapalıyken (β=0) tek F:** test 131.16 (×512) / 131.24 (×128) vs 4F 131.25. Yani havuzsuz ayrı F'ler birbirine denk; tek F'nin "gerçek katkısı" 4F'ninkiyle aynı (kimlik+çift F kapalı 133.20'ye göre ≈ −2 PPL).
- **Havuzun etkisi, tek F×512:** 131.16 → 124.92 (−6.2 PPL). Karışık bağlantıyla 132.02: gerçek−karışık = **7.1 PPL**. Kazanç doğru bağlantıdan geliyor.
- Bağlantılar yalnız eğitim sayımlarından; doğrulama/test yalnız önceki kelimeyle düğüm özelliklerine bakar; etiket kullanılmaz. Eğitim satırı kendi kelimesini komşu olarak kullanmaz.
- "Aynı 512 özellik aynı bilgi" varsayımı yapılmadı: tek F×512 doğrudan 192-boyutlu X'ten 512 yön üretir, 4F zinciri ise F₂–F₄'e önceki F'leri ve X'i de girdi verir; ikisi farklı bilgi taşır ve sonuçlar (124.92 vs 126.87) bunu gösteriyor.

## Yorum ve sınırlar
- Top-1 havuzla her yapıda 0.1677'den ~0.159–0.161'e düşüyor (V39'daki gibi); kazanç olasılık kalitesinde (CE/PPL), Top-5 ise 4F ve tek F×512'de aynı/biraz yüksek (0.3639–0.3673).
- Tek tohum, tek karıştırma, tek kNN; tek F×512'nin 4F'den 1.95 PPL iyi çıkması (doğrulamada 0.018 CE) tek tohumda bir ayar farkı da olabilir [Tahmin], ama yön kararlı: β=0.6 ve 0.9'da tutarlı.
- Neden tek F daha iyi olabilir: 4F zincirinde F₂–F₄ girdileri küçük γ=0.5 ve RMS ölçekli; kelime-düğüm ortalamaları alınırken zincir sonrası özellikler daha çok yumuşatılıyor (daha az kelimeye özgü bilgi). Sınanmadı [Tahmin].
- Hız: zaman kazancı F kısmında büyük (1.3→0.9→0.4 s) ama toplamın çoğunu paylaşılan C-bloğu Cholesky'si oluşturuyor; bu yüzden toplam hızlanma modest.

## Kesin cevap
Evet, tek F yeterli: bağlantılı havuzla tek F×512 (124.92) dört F'den (126.87) hem daha iyi hem biraz hızlı (≈3.3 s vs ≈3.85 s toplam), tek F×128 ise 127.65 ile 4F'ye yakın (+0.8 PPL) ve ≈2.5× hızlı (≈1.5 s). Dört F bu kurulumda üstünlük göstermedi. Tek tohumla, kesin genelleme değil.
