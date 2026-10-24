# TEK HAMLE V45 — Ortak havuzda akıllı bilgi aktarımı

Kod: `cpp/tek_hamle_v45.cpp` (V43/V44 kopyası; yalnız havuz bölümüne seçilebilir ağırlık ve kelimeye özel β eklendi, varsayılan davranış V44 ile aynı). Ham çıktılar: `v45_ham/` (`tablo45.txt`). Sistem: tek F×512, C0 kelime+çift kimliği, intercept, kNN=10, gruplu Ridge, tek geçiş, **λ_C=85, λ_F=1000** sabit (V44), tohum 1, aynı bölünme. Tüm aktarım katsayıları yalnız eğitim sayım/F ortalamalarından; seçim yalnız doğrulama CE (test sütunu seçimden sonra okundu).

## Yöntemler
- **A (V44):** β=0.9, a_wv ∝ kosinüs benzerliği (normalize, ≥0).
- **B (bağlantı ağırlıkları):** a_wv ∝ exp((S_wv − S_max)/τ), Σ_v a_wv=1, a≥0 (τ→0 en güçlü komşuya, τ→∞ tekdüze; V44'teki doğrusal kosinüs ağırlığı ara bir durum). τ ∈ {0.05, 0.1, 0.2, 0.4, 1.0}.
- **C (kelimeye özel β):** β_w = β_max·(f+(1−f)·κ/(κ+n_w))·ρ_w/ρ̄ (n_w: eğitim görülme sayısı, ρ_w: komşu benzerliklerinin ağırlıklı ortalaması = bağlantı güvenilirliği, ρ̄ ortalaması), [0,1]'e kırpılır; ρ olmadan da denendi (D). (κ,f) ∈ {100,1000}×{0.5,0.8}.
- **Birleşik:** B(τ=0.2) + C (ρ'suz, κ=1000, f=0.8).
- β=0: havuz kapalı; karıştırılmış bağlantı: komşu kimlikleri permüte (ağırlıklar aynı).

## Sonuçlar
| Yöntem | doğr. CE (PPL) | **test PPL** | Top-1 / Top-5 |
|---|---|---|---|
| β=0 (havuzsuz, bu λ'larda) | 4.9758 (144.9) | 138.59 | 0.1657 / 0.3647 |
| **A: V44 sabit β=0.9 (referans)** | 4.8675 (130.0) | **124.27** | 0.1637 / 0.3691 |
| B τ=0.05 | 4.8808 | 125.46 | 0.1610 / 0.3659 |
| B τ=0.1 | 4.8690 | 124.43 | 0.1639 / 0.3673 |
| **B τ=0.2 (doğrulamada en iyi)** | **4.8662 (129.8)** | **124.16** | 0.1637 / 0.3683 |
| B τ=0.4 | 4.8663 | 124.07 | 0.1631 / 0.3697 |
| B τ=1.0 | 4.8668 | 124.07 | 0.1643 / 0.3697 |
| C (ρ'lu) κ=100/1000, f=0.5/0.8 | 4.9188 / 4.9035 / 4.9040 / 4.8996 | 131.11 / 128.54 / 128.23 / 127.63 | – |
| D C ρ'suz κ=100,f=0.5 / κ=100,f=0.8 / κ=1000,f=0.5 / κ=1000,f=0.8 | 4.9076 / 4.8819 / 4.8833 / 4.8728 | 130.20 / 126.73 / 127.09 / 125.27 | ~0.165 / ~0.368 |
| Birleşik B(0.2)+C(κ=1000,f=0.8) | 4.8718 | 125.15 | 0.1647 / 0.3687 |
| Kontrol: karışık bağlantı, A | 4.9279 | 131.70 | 0.1596 / 0.3635 |
| Kontrol: karışık bağlantı, B τ=0.2 | 4.9283 | 131.62 | 0.1602 / 0.3631 |

## Aktarım katsayılarının dağılımı
- A: β_w=0.9 hepsinde. B(τ=0.1): komşu ağırlığı a_wv min 0.0004, medyan 0.081, maks 0.974; kelime başına en büyük ağırlık ortalaması 0.255 (tekdüze 0.100) — sivrilmiş. τ=0.2–1.0'da ağırlıklar tekdüzeye yaklaşır (V44'ün doğrusal kosinüs ağırlıkları zaten neredeyse tekdüzeydi).
- C (ρ'lu, κ=100,f=0.5): β_w min 0.00, q25 0.70, medyan 0.86, q75 1.00 (çok sayıda kelime 1.0'a kırpıldı, bazıları 0'a düştü). C ρ'suz: β_w ≈0.47–1.0 (κ=100,f=0.5) veya 0.78–0.90 (κ=1000,f=0.8).

## Değerlendirme (dürüst)
- **B:** doğrulamada τ=0.2 ile 4.8662 vs A 4.8675 (−0.0013 CE), test 124.16 vs 124.27 (−0.11 PPL). τ=0.2–1.0 arası fark 0.0001–0.0006 CE: düz. Bu bir iyileşme sayılamaz; V44 seçiminden bile küçük, gürültü sınırında. Kesin ağırlıklandırma (τ ≤ 0.05) kötüleştiriyor.
- **C kötü:** her yapılandırma A'dan kötü (doğr. CE +0.005…+0.05, test +1…+7 PPL). Daha çok kelime-özgü β azaldıkça kötüleşme artıyor: **sık kelimeler için β'yı düşürmek (kendi F'ine güvenmek) zarar veriyor**; en iyi C, tekdüze β'ya en yakın olandı (κ=1000,f=0.8 ρ'suz). Yani "sık kelime = kendi bilgisine güven" varsayımı bu veride desteklenmedi; havuz karışımı sık kelimelerde de yararlı.
- **Birleşik** B+C, B'den kötü (125.15).
- **Kontrol:** karışık bağlantılarla hem A hem B ~131.6–131.7 → kazanç (hem A hem B için) gerçek bağlantılardan geliyor, B'nin ağırlıklandırmasından değil (A: 124.27 vs 131.70; B: 124.16 vs 131.62). β=0: 138.59 (λ_C=85/λ_F=1000'de havuzsuz; havuzsuz için ayrı ayarlanmış λ ile V40'ta 131.2 idi — farklı koşul, aynı deney değil).
- **Süre:** havuz hesabı ve karışım A ile aynı maliyette (kNN ağırlığı veya β_w O(V·k)); tek-hamle toplamı tipik ≈1.4–1.8 s (gürültülü: 1.4–4.6 s aralığında ölçüm var; aynı yapının tekrarları arasında). Aktarım yönteminin kendisinden kaynaklanan ölçülebilir ek süre yok.

## Sonuç
Matematiksel olarak hesaplanan aktarım miktarı sabit β=0.9'dan **anlamlı biçimde iyi değil**: B ≈ A (−0.11 PPL, gürültü seviyesi), C belirgin kötü. Başarı iddia etmiyorum. En iyi: B τ=0.2 (124.16, aynı koşulda A 124.27), ancak aradaki fark tek tohumda güvenilir değil; yeni varsayılan yapmadım.

## Sınırlar
Tek tohum; τ ve (κ,f) küçük ızgaralar; C yalnız bir tür formül (rarity×reliability) denendi; ters yönlü β (sık kelimelerde daha yüksek) denenmedi; seçim 15 yapılandırma arasında doğrulama en iyisi (hafif iyimserlik).
