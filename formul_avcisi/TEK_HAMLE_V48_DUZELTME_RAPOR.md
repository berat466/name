# TEK HAMLE V48 DÜZELTME — Gerçek bağlam-kNN (SONUÇ: İYİLEŞME YOK)

**Hata kabulü:** İlk V48'de her geçmiş kelime için ayrı `P(y|w_{t−k})` hesaplayıp topladım; istenen bu değildi. Bu düzeltmede **bütün bağlam vektörünün** benzerliğiyle kNN uygulandı. İlk V48 kodu ve raporu silinmedi (`cpp/tek_hamle_v48.cpp`, `TEK_HAMLE_V48_RAPOR.md`).

Kod: `cpp/tek_hamle_v48d.cpp` (V46/V48 üstüne; `V48=1`). Ham çıktılar: `v48_duzeltme_ham/`. Sistem V46 ile aynı: tek F×512, havuz β=0.9, C0 kelime+çift kimliği, intercept, gruplu Ridge, λ_C=85, λ_F=1000, tek geçiş; tohum 1.

## Yöntem (gerçek bağlam-kNN)
- **Bağlam vektörü (sıra korunur):** `u_i = [ρ_1·E(w_{t−1}) ; ρ_2·E(w_{t−2}) ; … ; ρ_L·E(w_{t−L})]`, ρ_k=k^{−1/2}, bütünüyle L2-normalize. E = mevcut kelime gömme tablosu (X'in 64-boyutlu kelime bloğu; c2 verisinde konumdan bağımsız aynı tablo olduğu doğrulandı). Boyut 64·L (L=2: 128, L=4: 256, L=8: 512). Diziler token dizisinden (`ids_train.npy`) kuruldu; satır eşleşmesi doğrulandı.
- **kNN:** bütün bağlam vektörü arasındaki kosinüs benzerliği (kaba kuvvet, 39 840×39 840, sgemm), K=32 komşu. Komşu seçimi yalnız giriş bağlamına dayanır.
- **Q_bağlam:** `Q_i = (1−ε)·Σ_j w_ij·onehot(y_j) + ε·π`, `w_ij ∝ exp((s_ij − s_max)/τ)` (Σw=1), ε=0.2, τ=0.05 (τ ∈ {0.02, 0.2} L=4'te de denendi). Önceki kelimelerin ayrı dağılımlarının toplamı **kullanılmadı**.
- **Out-of-fold:** eğitim satırları 5 kata dizi (belge) bazında bölündü; bir satır yalnız **başka katlardaki** satırları komşu olarak görür (kendisi ve aynı belgedeki bitişik/örtüşen bağlamlar dışarıda). Doğrulama/test etiketleri belleğe girmez (Q yalnız eğitim hedefi için).
- **Formül:** `Y* = (1−a−b)Y + a·Q_havuz + b·Q_bağlam`; Zᵀ Y* satır parçalarıyla biriktirilir (N×V hedef matrisi oluşturulmadı); ağırlıklar `W=(1−a−b)W_Y + a·W_Qhavuz + b·W_Qbağlam`. Izgara: a ∈ {0, 0.25}, b ∈ {0, 0.05, 0.1, 0.2, 0.3, 0.4}; seçim doğrulama CE.
- **Karıştırılmış kontrol:** sorgu bağlam vektörü başka rastgele bir satırdan alınır (komşu dışlama o satırın katına göre), yani bağlam–hedef ilişkisi koparılır.

## Sonuçlar
| Sistem | seçilen (a,b) | doğr. CE (PPL) | **test PPL** | Top-1 / Top-5 |
|---|---|---|---|---|
| Referans a=b=0 (V44) | 0, 0 | 4.8675 (130.0) | 124.27 | 0.1637 / 0.3691 |
| **V46 (b=0; aynı koşuda yeniden ölçüldü)** | 0.25, 0 | **4.8474 (127.4)** | **121.32** | 0.1606 / 0.3681 |
| Yalnız bağlam-kNN L=2 | 0, 0.05 | 4.8678 | 124.32 | 0.1637 / 0.3691 |
| Yalnız bağlam-kNN L=4 | 0, 0.05 | 4.8672 | 124.23 | 0.1635 / 0.3695 |
| Yalnız bağlam-kNN L=8 | 0, 0.1 | 4.8670 | 124.18 | 0.1635 / 0.3689 |
| Yalnız bağlam-kNN L=4, τ=0.2 | 0, 0.1 | 4.8669 | 124.14 | 0.1627 / 0.3689 |
| Havuz + bağlam-kNN L=2 (b>0 zorunlu) | 0.25, 0.05 | 4.8481 | 121.42 | 0.1606 / 0.3683 |
| Havuz + bağlam-kNN L=4 | 0.25, 0.05 | 4.8477 | 121.33 | 0.1604 / 0.3681 |
| Havuz + bağlam-kNN L=8 | 0.25, 0.05 | 4.8476 | 121.31 | 0.1608 / 0.3681 |
| Havuz + bağlam-kNN L=4, τ=0.2 | 0.25, 0.05 | 4.8475 | 121.29 | 0.1610 / 0.3681 |
| Havuz + bağlam-kNN L=4, τ=0.02 | 0.25, 0.05 | 4.8478 | 121.37 | 0.1600 / 0.3681 |
| **Karıştırılmış bağlam** L=2, yalnız bağlam / havuz+bağlam | 0,0.05 / 0.25,0.05 | 4.8695 / 4.8508 | 124.57 / 121.80 | – |
| **Karıştırılmış bağlam** L=4, yalnız bağlam / havuz+bağlam | 0,0.05 / 0.25,0.05 | 4.8700 / 4.8512 | 124.60 / 121.86 | – |

**Doğrulama CE'sinde b=0 en iyi** (V46 4.8474; havuz+bağlam hiçbir L/τ'da bunu geçemedi: 4.8475–4.8481). Bu yüzden **bağlam yöntemi seçilmiş değildir**; tablodaki "havuz+bağlam" satırları b>0 zorlanarak raporlandı. b=0'da V46'nın 121.32 PPL'i yeniden elde edildi (doğrulama CE 4.8474, test 121.32, Top-1 0.1606, Top-5 0.3681).

## Komşu/Q kalitesi
- Ortalama benzerlik (en yakın / 32. komşu): L=2: 0.927 / 0.727; L=4: 0.798 / 0.610; L=8: 0.673 / 0.520 (bağlam uzadıkça komşular uzaklaşıyor). Karışık bağlamda en yakın benzerlik 0.96–0.99 görünüyor (karışık sorgu eğitim bellek dağılımına benzeyen rastgele bağlam) ama hedefle ilişkisiz.
- **OOF Q_bağlam eğitim CE'si** (satırın gerçek etiketine verdiği olasılıktan): L=2: 5.698, L=4: 5.671 (τ=0.2: 5.573; τ=0.02: 6.108), L=8: 5.645; unigram: 5.850; karışık bağlam: 7.20–7.26. Yani gerçek bağlam-kNN, unigramdan iyi ve karışıktan çok iyi (gerçek bilgi taşıyor) ama kelime havuzu + kimlik+çift Ridge'in zaten verdiği bilginin çok altında/ona fazlalık.

## Süre ve bellek (CPU 4 vCPU; gürültülü)
- Komşu arama (kaba kuvvet): **5.3 s (L=2), 6.4–7.0 s (L=4), 9.6 s (L=8)**; Q satırları + FᵀQ ≈0.7–1.0 s; Y0/U ≈0.2 s. Bağlam sinyali toplamı ≈ **6–11 s** (V46 sinyali ≈1.6–2 s ayrı; V46 toplamı ≈2.3–2.6 s, yani bağlam-kNN toplam süreyi ≈4–5× büyütür).
- Bellek: tepe RSS 1.44 GB (L=2) – 1.50 GB (L=8) (bağlam matrisi 40k×d float: 20–80 MB; benzerlik parçası 1024×39 840×4 B = 163 MB; hafıza tabloları 32–128 MB). Brüt kuvvet O(n²·d); n büyüdükçe pahalı (yaklaşık kare).

## Değerlendirme
- **Soru: bütün bağlamın ilişkisini hesaplamak PPL'i düşürdü mü? Hayır.** V46'nın üstüne ek kazanç yok (doğrulama CE'de b=0 en iyi; test farkları ±0.1 PPL, gürültü). Yalnız bağlam, a=0 referansına göre çok küçük kazanç gösteriyor (doğr. CE −0.0006…−0.0009, test −0.05…−0.13 PPL; L=8/τ=0.2'de en büyük) — gerçek bağlam karışıktan tutarlı iyi (124.14–124.32 vs 124.57–124.60), ama etki gürültü sınırında ve havuz sinyaliyle birleşince kayboluyor.
- Gerçek-karışık farkı belirgin: L=4 havuz+bağlam 121.33 vs 121.86 (karışık, +0.53 PPL zarar), yalnız bağlam 124.23 vs 124.60. Bağlam bilgisi gerçek ama V46'ya katkısı yok.
- **Başarı iddia edilmiyor.** V46 (121.32) en iyi sistem olarak kalıyor.
- **Yapılamayanlar / sınırlar:** ρ_k, K, ε sabit (yalnız τ ve b/a ızgarası taranmış); gömme olarak yalnız mevcut 64-boyutlu X gömmesi kullanıldı; kNN'de F/X benzerliği karışımı ve ANN (yaklaşık arama) denenmedi; tek tohum, tek karıştırma; tahmin sırasında kNN olasılığı eklemenin etkisi bu görevde ölçülmedi; L=8 tek ayarla (τ=0.05).
