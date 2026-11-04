# TEK HAMLE V50 — Havuz bilgisini son tahminde kullanmak (olasılık karışımı)

**Teyit:** V50, V48'in tekrarı değildir: V48 benzer *bağlamların* komşuluğunu (kNN) ağırlık hedefine karıştırıyordu; V50, V49'un (120,43 PPL) hesapladığı kelime olasılıklarına bağlantılı kelime havuzunun dağılımını **en son tahmin aşamasında** `P_son=(1−α)·P_model+α·Q_havuz` olarak karıştırır ve bunu ağırlık içindeki ρ sinyaliyle karşılaştırır. Bağlam-kNN yok, yeni embedding/model yok.

Kod: `cpp/tek_hamle_v50.cpp` (V49 kopyası; `V50=1`; V49 ve öncesi dokunulmadı). Ham çıktılar: `v50_ham/` (`v50_main.txt` tohum 1, `v50_seed2/3.txt`, `seed_tablo50.txt`). Sistem V49'un seçilmiş ayarlarıyla: tek F×512, havuz β=0.9, **V49 B kuvvet dönüşümü (A, (A−μ)², (A−μ)³)**, C0 kelime+çift kimliği, intercept, gruplu Ridge, λ_C=85, λ_F=3000, ρ=−0.25 (yalnız ρ ve α değişir), doğrusal okuma parametreleri (α_okuma, ε) her ağırlık kümesi için doğrulamada α_son=0'da seçilip sabit tutuldu.

## Yöntem
- **P_model:** ağırlıklardan gelen doğrusal okuma `(1−a)π + a·max(z,ε)/Σmax(z,ε)` (V49 ile aynı).
- **Q_havuz:** her kelime w için tek bir dağılım: `Q(w)=(1−b)·P_w + b·Σ_v a_wv P_v`, b=0.9, a_wv havuzun kNN=10 ağırlıkları, P_v komşunun eğitim sayımlarından sonraki-kelime dağılımı. Yalnız **önceki kelime (giriş)** ve **eğitim** sayımları/bağlantıları kullanılır; sıfır toplamlı (komşusuz/görülmemiş) satırlar eğitim unigramına döner; doğrulama/test etiketi hiçbir yerde kullanılmaz. Olasılıklar aynı sözlükte, normalize.
- **Karıştırılmış kontrol:** Q tablosunda komşu kimlikleri rastgele permüte edilir (ağırlıklar aynı).
- α ∈ {0, 0.025, 0.05, 0.10, 0.20, 0.30, 0.40, 0.60}, **yalnız doğrulama CE** ile seçildi. Her sistemde α=0 kontrolü dahil.

## Sonuçlar (tohum 1; doğrulama CE ile seçilen α)
| Deney | ρ | α (seçilen) | doğr. CE (PPL) | **test PPL** | Top-1 / Top-5 |
|---|---|---|---|---|---|
| D — sinyalsiz kontrol | 0 | 0 | 4.8655 (129.7) | 123.85 | 0.1633 / 0.3701 |
| **A — V49 referansı** | −0.25 | 0 | 4.8400 (126.5) | **120.43** (yeniden elde edildi) | 0.1604 / 0.3689 |
| **B — yalnız son aşama (gerçek Q)** | 0 | **0.2** | **4.8316 (125.4)** | **119.30** | 0.1629 / 0.3703 |
| C — her iki aşama (gerçek Q) | −0.25 | 0.1 | 4.8352 (125.9) | 119.71 | 0.1590 / 0.3695 |
| B — karıştırılmış Q | 0 | 0 (seçildi) | 4.8655 | 123.85 | 0.1633 / 0.3701 |
| C — karıştırılmış Q | −0.25 | 0 (seçildi) | 4.8400 | 120.43 | 0.1604 / 0.3689 |
α taraması (doğr. CE), B gerçek: 0→4.8655, 0.025→4.8561, 0.05→4.8491, 0.1→4.8394, **0.2→4.8316**, 0.3→4.8352, 0.4→4.8488, 0.6→4.9090 (iç optimum). B karışık Q: α arttıkça tek yönde kötüleşiyor (0.2'de 4.9570, 0.6'da 5.33). C gerçek: 0→4.8400, 0.1→**4.8352**, 0.2→4.8391, 0.6→4.9363; C karışık: 0.025'te bile 4.8453 (kötü).

## Tohum tekrarı (V49 ile aynı tohumlar; yalnız α doğrulamayla seçildi)
| tohum | D | A (V49) | B (α=0.2) | C (α=0.1) |
|---|---|---|---|---|
| 1 | 4.8655 / 123.85 | 4.8400 / 120.43 | **4.8316 / 119.30** | 4.8352 / 119.71 |
| 2 | 4.8686 / 124.59 | 4.8427 / 120.94 | **4.8345 / 119.84** | 4.8379 / 120.19 |
| 3 | 4.8636 / 124.34 | 4.8380 / 120.87 | **4.8302 / 119.68** | 4.8334 / 120.13 |
(doğr. CE / test PPL). B, her üç tohumda A'dan doğrulama CE'de 0.0078–0.0084 ve testte 1.10–1.19 PPL daha iyi; C, A'dan 0.003–0.005 CE / 0.7 PPL iyi ama B'den kötü. Karışık Q'da her tohumda α=0 seçildi (kazanç yok) — kazanç gerçek bağlantılardan geliyor.

## Süre ve bellek (CPU, 4 vCPU)
- **Ağırlık çözme (eğitim):** V49 ile aynı: F+havuz+dönüşüm+Gram ≈1.2–2.0 s, C bloğu Cholesky/Schur ≈0.4–1.6 s, ρ çözümü 0.2–0.3 s. **B için ρ sinyali hesaplanmaz** (ρ=0): V46/V49 sinyalinin 0.4–1.4 s maliyeti ortadan kalkar; ek maliyet yalnız Q tablosu **≈0.06 s** (gerçek+karışık birlikte 0.12 s).
- **Tahmin:** val+test logitleri 0.11–0.25 s (aynı), olasılık karışımı+PPL hesabı val+test (9 960 satır) **0.02–0.03 s** (α=0'da 0.02 s): tahmin maliyeti ihmal edilebilir artış. Tepe RSS 1.22 GB.

## Cevap
**Evet, bu denemede bağlantılı havuz bilgisini son kelime tahmininde doğrudan kullanmak, aynı bilgiyi ağırlık hesabında (ρ sinyali) kullanmaktan daha etkili:** B 119.30 (3 tohum ort. 119.60), A 120.43 (120.75), ikisi birlikte (C) 119.71 — yani aynı bilgiyi iki yerde kullanmak B'ye ek kazanç sağlamıyor, hafifçe kötüleştiriyor (çift sayım). Tek hamlede ağırlıklara sinyal eklemeden, yalnız son olasılık karışımıyla hem daha ucuz hem daha iyi. Kazanç gerçek bağlantılara özgü (karışık Q'da yok).

## Sınırlar
- Tek bölünme, 3 tohum; Q_havuz yalnız önceki kelimeye bağlı (tek kelimelik giriş), kimlik+çift Ridge'in zaten kullandığı kelime bilgisiyle örtüşen bir kısmı var; α ızgarası kaba (iç optimum 0.2); α_okuma/ε karışımda yeniden ayarlanmadı (α=0'da seçilen sabit).
- Top-1 B'de A'dan hafif yüksek (0.1629 vs 0.1604), C'de düşük (0.1590).
- V49'un B dönüşümü ve ρ=−0.25 kaynaklı olası dolaylı eğitim-hedefi bağımlılığı bu deneyde giderildiği varsayılmadı; doğrulama/test ölçümleri hold-out'tur.
