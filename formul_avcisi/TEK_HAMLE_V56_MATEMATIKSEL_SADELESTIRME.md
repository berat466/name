# V56 — matematiksel sadeleştirme (15 dk)

Referans: V54 `1cf9b09` (değişmedi). Deney kopyası: `cpp/tek_hamle_v56.cpp`; mikro test: `v56_sadelestirme/syrk_test.cpp`.

## Seçilen aday: simetrik Gram tipi çarpımlar (SYRK özdeşliği)
* **Eski ifade:** S = N·Nᵀ (K-tablosu grafı, V×V×V, 6 tablo) ve G = FᵀF (parça başına 513×513×4096) tam `sgemm` ile.
* **Yeni ifade:** N·Nᵀ ve FᵀF simetriktir → yalnız üst üçgen `ssyrk`, alt üçgen = yansıtma (S_ab = S_ba).
* **Eşdeğerlik:** sonuç matrisi tanım gereği simetrik; yeni yol alt üçgeni hesaplamadan kopyalar. Ek olarak **ölçüldü**: gerçek veride `sgemm` çıktısı zaten tam simetrik (0 asimetrik eleman) ve `ssyrk` üst üçgeni `sgemm`'le **bit-özdeş** (0/2.003.001 farklı eleman graf için; 0/1.054.728 F parçası için).
* **FLOP (teorik tahmin):** K-grafı 6×2·V³ = 96.2 GF → 48.1 GF (−48.1); FᵀF 59.5 → ~29.7 GF (−29.7). Toplam −77.8 GF / 392 GF (−%20). Gerçek aritmetik azalıyor (üçgen yarım), "küçültüp büyütme" yok.

## Ölçümler
* Mikro (tek çağrı): graf S 36.6 → 21.6 ms (−%41); F parçası 9.8 → 5.7 ms (−%42).
* **Tam hat (V54 gerçek veri, tam konfigürasyon): hash'ler aynı** — Fc, AFF, RF, WC, WF, val logitleri (Zval) V54 ile **bit-özdeş** (`V53H_HASH`).
* Hat içi süre: K-grafı sgemm kısmı ≈ 0.32 → 0.28 s medyan (−%10; mikro testteki −%41 değil), toplam "model hazır" fark **gürültü içinde** (VM bu sırada 4.4–5.5 s ile dalgalı). `FᵀF` için `ssyrk` hat içinde **daha yavaş** çıktı (0.24 → 0.40 s; 4096×513 parçalar + OpenBLAS iş parçacığı etkileşimi, nedeni araştırılmadı) → o kullanım varsayılan **kapalı** (`V56_XSYRK=1` ile açılır); K-grafı kullanımı varsayılan açık (`V56_SYRK=0` ile kapanır).

## Rapor
1. **Sadeleştirilen ifade:** N·Nᵀ ve FᵀF simetrik çarpımları (SYRK).
2. **İşlem sayısı:** teorik −77.8 GF (−%20); varsayılan uygulanan: K-grafı −48.1 GF (−%12); FᵀF −29.7 GF uygulandı ama yavaş olduğu için kapalı.
3. **Süre:** mikro −%41; hat içi K-grafı sgemm ≈ −%10; tam model hazır: ölçülebilir fark yok (gürültü ±0.3 s).
4. **Eşdeğerlik:** matematiksel olarak ve sayısal olarak (bit düzeyinde) evet.
5. **Hash'ler:** WC, WF, Zval, Fc, AFF, RF aynı.
6. **Tam V54 süresi:** değişmedi (gürültü içinde).
7. **Sonraki en büyük fırsat:** `U = A_CFᵀY0` (82 GF yoğun dgemm) yerine **U = Xᵀ·R_C** (R_C sıfır olmayan 5.1 M eleman → 5.3 GF, −%94 FLOP; Y0'a gerek kalmaz); bit-özdeşliği bozar (farklı toplama düzeni) ve V54'te seyrek U zaten bellek-sınırlı çıkmıştı, bu yüzden kazanç FLOP kadar süreye yansımayabilir. İkinci: T=A_CFᵀX yerine grup başına L⁻¹A_CF'nin SYRK'ı (−10.5 GF) — yine bit-özdeşlik bozulur.

Sınırlar: 15 dk; tek A/B turu (5 çift), VM gürültülüydü; `F ᵀF` yavaşlığının nedeni incelenmedi.
