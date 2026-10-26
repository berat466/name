# V57 düzeltme: hızı koru, ağırlık farklarını gider — sonuç: KISMİ (bit eşitliği XᵀR_C ile mümkün değil)

Ölçüm: tam yapılandırma (mc=39 946), V53H_LEAN=1 FAST=1 HASH=1, 3 tur, dense / `V57_UX=1` / `V53H_U=2` aynı turda ardışık (aynı VM yükü). VM bu oturumda çok gürültülü (aynı ikili 8,7–13,9 s; HASH modu da süre ekler) — mutlak süreler eski 3,63 s ile kıyaslanamaz.

## 7 soru

1. **İlk sayısal fark nerede?** [Kesin] Doğrudan U'da. Fc, AFF, RF, ACF, RC, Y0f, X, T karma değerleri dense ve `V57_UX=1` arasında **aynı**; ilk ayrışan karma U (`1f56…` vs `a50c…`), sonra WC/WF/Zval.
2. **Neden?** [Kesin] Fark toplama sırasından değil formülden geliyor. U=ACFᵀY0 ile XᵀR_C matematiksel olarak eşit, ama X ve Y0 ayrı çözümlerin ayrı yuvarlanmış çıktıları. `V57_DIAG` (long double, 29 241 eleman): iki formül birbirinden en çok **8,7e-11 bağıl** farklı; her formülün kendi sıralı-FMA toplamı kendi long double değerine 1,3e-12 / 8,6e-13 yakın. Yani XᵀR_C'yi hangi sırayla toplarsam toplayayım ACFᵀY0'ın bitlerini üretemez; bu, FMA/BLAS sırası/f32-f64 dönüşümü sorunu değil. Eleme: sparse toplama sırası (dgemm K-blok emülasyonu zaten ayrı yolla kuruldu), FMA, BLAS bölme, Schur'a yayılma (yalnızca U farkının sonucu).
3. **Bit eşitliği düzeltildi mi?** XᵀR_C ile **hayır** ve genel olarak yapılamaz (yukarıdaki gerekçe). Yasak olan yollarla (referans kopyası, fark listesi, gizli yoğun hesap) kapatmadım.
   Ancak: `V53H_U=2` (ACFᵀY0'ı dgemm K-blok sırasıyla, yalnız sıfır olmayan Y0 konumlarında toplayan emülasyon) 3/3 turda **WC ve WF karmaları dense ile aynı** (`ddab7b8c…`, `a9dc7b3f…`). U çift-duyarlık karması farklı (`3226…` vs `1f56…`), ama float ağırlıklar yuvarlamada aynı kalıyor. Emülasyon yalnızca ilk 32 sütun panelini çekirdeğe karşı doğruluyor; çok iş parçacıklı dgemm'in bölüm-kuyruk sütunları taklit edilemiyor ([Muhtemel] U'nun bazı bitleri dense'ten ayrışıyor). Dolayısıyla "WC/WF aynı" bu girdi için ölçülmüş, her girdi için garanti değil.
4. **U hâlâ ~5,3 GF mi?** `V57_UX=1` evet (5,1 M sıfır-olmayan konum, GF hesabı değişmedi; U süresi 0,52–0,64 s). `V53H_U=2` de aynı FLOP düzeyinde (0,62–0,67 s, doğrulama 0,1 s dahil). Yoğun: 82 GF (0,69–0,88 s; 1. tur 2,0 s VM gürültüsü).
5. **Gerçek model-hazır süresi (aynı VM yükü, HASH açık):**

| tur | dense | UX | hu2 |
|---|---|---|---|
| 1 | 13,16 | 9,00 | 8,83 |
| 2 | 8,73 | 9,08 | 9,33 |
| 3 | 9,53 | 9,28 | 8,81 |

   Medyan: dense 9,53 / UX 9,08 / hu2 8,83 s. Fark gürültü içinde; U kazancı (~0,1–0,3 s) toplam süreyi güvenilir biçimde ayırt edemiyor. [Muhtemel] kazanç gerçek ama küçük; yoğun dgemm 82 GF'i çok iş parçacığıyla ~0,5 s'de bitirdiği için FLOP azalması duvar süresine orantılı yansımıyor.
6. **RAM ve PPL:** tepe RSS 2160–2169 MB hepsinde aynı. Val CE 4,609033 (PPL 100,387, Top1 0,1663) üç yolda da aynı; **test PPL 98,092 bu kısa koşularda ölçülmedi** (V53H_TEST=0) — onu doğrulamadım.
7. **Hızlı mod güvenle açılabilir mi?** Hayır, varsayılan kapalı kalıyor. `V57_UX=1` WC/WF'yi bit düzeyinde değiştirir (val CE aynı ama karmalar farklı). `V53H_U=2` bu girdide WC/WF-eşit ama kazancı ölçülebilir değil ve tail-sütun genelliği kanıtsız; açık-isteğe-bağlı kalmalı.

## Sonuç sınıfı: KISMİ
Matematik korundu, FLOP düştü, ama XᵀR_C ile WC/WF bit-eşit olmasının önünde formül kaynaklı (X/Y0 bağımsız yuvarlama) bir engel var; hız kazancı bu VM'de doğrulanamadı. Kod: `cpp/tek_hamle_v57b.cpp` (V57_DIAG teşhisi), V54/V56/V57 kaynakları değişmedi. Ham çıktılar: `v54_baglantili_hesaplama/ham/v57d/`.
