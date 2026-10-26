# V55 — tek örnekten çoğaltılmış hesap etkisi (fizibilite, 15 dk)

V54 (`1cf9b09`) değiştirilmedi; V54 dökümündeki gerçek F satırları (n=113,046×513) ve gerçek Gram/RHS formülleri (AFF=FᵀF, RF=FᵀY, A_CC/A_CF/R_C slot sayımı) küçük bağımsız numpy testinde kullanıldı. Kod: `v55_cogaltma/test_cogaltma.py`, ham sonuç `v55_cogaltma/sonuc.json`. Entegrasyon yok.

## Sonuçlar

1. **1.000 örneklik hesap:** float32 sgemm yolu (V54 `cross` mantığı) **3.06 ms**; V54 sayım mantığıyla sıralı çift duyarlık toplama **502 ms** (numpy döngüsü, şişkin).
2. **Tek tetikleyicili hesap (k·f fᵀ, k·f, sayaç k):** **0.85 ms** (maliyetin tamamı 513² dış çarpım; RHS/ACF/sayaç O(513)). Adil kıyas (sgemm) için ≈3.6×; gerçek kazanç büyüklüğü satır sayısıyla orantılı: k satır O(k·m²) yerine O(m²) — daha iyisi Gram'a rank-1 güncelleme olarak hiç oluşturulmadan eklenmesi.
3. **Matematiksel eşitlik:** evet. RHS, A_CF, sayaçlar **bit-özdeş**; Gram sıralı toplamdan **2.8e-14**, sgemm(float32) yolundan **5.0e-6** bağıl farklı (fark yuvarlama; tetikleyici yüksek duyarlıklı referansa **0.0** fark, sıralı toplama 2.8e-14 — yani tetikleyici daha doğru). k=1024 için sıralı toplama çarpanla bit-özdeş **değil**: bit-özdeşlik beklenmemeli, yalnız sayısal eşitlik.
4. **Farklı örnekler üretmeden etki:**
   * *Dogrusal olcek donusumleri s_t·f:* **tam** (3.7e-15) — yeter istatistik Σs, Σs².
   * *Ozellik uzayinda toplamsal gurultu:* yalnız **beklenen değer** T(ffᵀ+σ²I) (rastgele örnekle bağıl Frobenius hata %1.8); yani etkisi **diyagonal ekleme = ridge artışı**.
   * *Girdi uzayinda kucuk bozulma (gerçek tanh haritası):* birinci mertebe formül sıfırıncı mertebeden az iyi (σ=0.05: %0.74 vs %0.94 hata; σ=0.01: %0.154 vs %0.157) — kazanç önemsiz, **tam değil, yalnız yaklaşık**.
   * **Ayrım:** aynı örneği k kez kullanmak = o örneğe ağırlık k vermek (yeni bilgi **yok**); bozulmuş/dönüştürülmüş kopyalar da yeni etiket bilgisi getirmez, yalnız (i) ağırlık, (ii) ridge benzeri düzgünleştirme ekler. Gerçek yeni bilgi yalnız yeni (bağlam, hedef) çiftinden gelir.
5. **V54'e uygulanabilir somut fırsat:** zayıf. Gerçek veride **aynı F satırı 53.3 % benzersiz** (60,265 / 113,046; ayrıca (F,y) çifti 88.7 %): aynı (w₂,w₁,kat) → aynı F. Gram'ı benzersiz satırlar × çokluk ile hesaplamak float32'de **2.2× hızlı** (67.8 vs 149 ms numpy) → V54'te FᵀF 0.17 s olduğundan **≈ −0.08 s**; ama sonuç bit-özdeş değil (maks bağıl 4.3e-5, mutlak 8.7e-11) ve hash eşitliği kurallarımıza göre otomatik kabul edilemez → **aday** olarak not edildi, uygulanmadı. RHS ve slot sayımında benzer sıkıştırma da toplama sırasını bozar.

Sınır: tek örnek (i0=1234) ve tek tohum; zamanlar numpy; C3'te F haritası V54 `make_w1x` yapısının yaklaşığı (gerçek W,b değil).
