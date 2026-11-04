# TEK HAMLE V37 — 133,2 uyuşmazlığı ve F öğrenme testi

Kod: `cpp/tek_hamle_v37.cpp` (V36 üzerine; V34–V36 yolları duruyor). Ham çıktılar: `v37_ham/` (`tablo_bias.txt` özet; V36 ham dosyaları `Anb_ref_v36_real.txt`, `Bnb_ref_v36_shuf.txt` olarak kopyalandı). Tek tohum (1), tek karıştırma: kesin genelleme değildir. Hiperparametreler (γ, λ_C, λ_F) yalnız doğrulama CE'siyle seçildi; test seçimde kullanılmadı.

## 1) 133,2 → 140,05: kesin neden [Kesin]
**V31/V36 "yalnız kimlik" sisteminde sabit (bias/intercept) sütunu yok; V26c `C0` sisteminde var.**
- V26c `E` modu: `f0.mz=1`, yani yoğun blokta tek bir sabit-1 sütunu + 4001 seyrek kimlik; Ridge her sütuna λ (λ=20) uyguluyor. 1'ler sütununun Gram girdisi ≈ n, λ=20 ihmal edilebilir → pratikte cezasız intercept.
- V31/V36 `C` bloğu yalnız seyrek kimlik; F kapatıldığında (`λ_F=10⁹`) F1'in sabit sütunu da yok edilince intercept sıfırlanıyor. Tablo "yalnız kimlik 140,05 / 144,1" bu yüzden intercept'siz.
- Kanıt (aynı bölünme, aynı özellik: ID(w_{t−1}) + en sık 2000 (w_{t−2},w_{t−1}) çifti, aynı doğrusal okuma): V37'de sabit sütuna yalnız λ_C uygulanınca (`V37_BIAS=1`, F kapalı):
  | λ | V26c doğr. CE | V37 doğr. CE |
  |---|---|---|
  | 10 | 4.9544 | 4.9544 |
  | 20 | 4.9356 | 4.9356 |
  | 100 | 4.9625 | 4.9625 |
  λ=20 test: **V26c 133.20 (CE 4.8918, Top-1 0.1655, Top-5 0.3641)** = **V37 133.20 (CE 4.8918, 0.1655, 0.3641)**; eğitim PPL 32.5 de aynı. Yani birebir aynı sonuç.
- Kontrol edilen ve fark yaratmayanlar: kimlik/çift oluşturma (en sık 2000 çift, aynı tanım), örnek sırası, ridge (A=S+λI), doğrusal olasılık (α,ε doğrulamada), unigram karışımı. Tek fark intercept.

**Sonuç:** V31–V36 raporlarındaki "yalnız kimlik" referansları (144,1; 140,05; 144,49) intercept'siz; adil taban **133,20**. V36'daki "F'nin ek kazancı −10,4 PPL" iddiası bu yüzden **yanlış ölçülmüş**: F1'in sabit sütunu kayıp intercept'i geri getiriyordu. V31'in "kimlik+4F 140,3 vs kimlik 144,1" kazancı da büyük ölçüde aynı nedenle şişkin olmalı [Muhtemel; V31 yeniden ölçülmedi].

## 2–3) Adil F karşılaştırması (aynı C0+bias tabanı, F₁–F₄, H=512, tek Ridge çözümü)
Seçim: her yöntem için γ ∈ {0.1, 0.5, 1}, λ_C ∈ {20,100}, λ_F ∈ {3·10³…10⁵, 10⁹} doğrulamayla.
| Yöntem | γ | doğr. CE (PPL) | test PPL | Top-1 | Top-5 | F üretim süresi* | efektif rank F1/F2/F3/F4 |
|---|---|---|---|---|---|---|---|
| V26c kimlik+çift (F yok, intercept var) | – | 4.9356 (139.2) | **133.20** | 0.1655 | 0.3641 | Cholesky/Schur ≈1.5–2 s | – |
| V36 kimlik+çift (intercept YOK; hatalı taban) | – | 4.9840 (146.1) | 140.05 | 0.1610 | 0.3566 | – | – |
| A. gerçek residual-F | 0.1 | 4.9221 (137.3) | 131.42 | 0.1667 | 0.3645 | ≈9.2 s (OOF 2.2 s dahil) | 76/83/96/104 |
| B. karışık residual-F | 0.1 | 4.9216 (137.2) | 131.43 | 0.1671 | 0.3651 | ≈9.7 s | 102/83/91/110 |
| C. hedefsiz PCA-F (rank 128) | 0.1 | 4.9254 (137.7) | 132.00 | 0.1669 | 0.3639 | ≈8.9 s** | 128/128/128/128 |
| D. V34 rastgele-çift F (hedef etiketi yalnız çift seçiminde) | 1/0.5 | 4.9253 (137.7) | 131.25 | 0.1677 | 0.3665 | ≈1.2 s | 4.9/2.0/1.4/1.2 |
(*) tek-hamle F zinciri+Gram hazırlığı, 4 çekirdek, sıcak; (**) PCA yolu yine R/M matrisini kuruyor (kodda çıkarılmadı), saf PCA süresi daha küçük olur; ölçülmedi.
Diğer γ'larda sonuçlar `tablo_bias.txt` içinde (γ=0.5: A 131.58, B 131.35, C 132.46; γ=1: A 133.17, B 133.10, C = F kapalı 133.20 seçildi).

V36 ayarı (sabit sütun λ_F ile cezalı, `V37_BIAS` kapalı), aynı C0 tabanı: A 129.67 (doğr. 4.9128), B 129.55 (4.9109), C PCA 130.29 (4.9156). Sıralama aynı: gerçek ≈ karışık; hedefsiz PCA 0.003–0.005 CE geride. Bu ayarda tüm F'ler intercept'i farklı biçimde ele aldığından (F'nin tanh sütunları ortalaması ≠0) mutlak sayılar daha iyi; ama bunun kaynağını (intercept cezası vs F'nin ortalama taşıması) ayrıştırmadım.

## Cevaplar
1. **133,2 neden 140,05'e dönüştü?** V31/V36 hattının kimlik bloğunda sabit (intercept) sütunu yok; V26c'de var (λ=20 ile cezalı ama n büyük olduğundan fiilen cezasız). Aynı özellik+λ ile V37'de bit düzeyinde 133.20 yeniden üretildi (doğr. CE ve test CE aynı). [Kesin]
2. **Gerçek residual karıştırılmışdan iyi mi?** Hayır: 131.42 vs 131.43 (doğr. 4.9221 vs 4.9216); V36 ayarında 129.67 vs 129.55. Fark gürültü içinde ve karışık hafifçe iyi. Tek tohum/tek karıştırma. [Kesin ölçüm, genelleme sınırlı]
3. **F'nin kazancı hedef bilgisi mi, özellik çeşitliliği mi?** Hedef bilgisi değil. Doğru tabanda F'nin toplam kazancı yalnız ≈ −1.8 PPL (133.20 → 131.4; V36'daki −10.4 iddiası intercept hatasıydı). Bu kazancı etiket bilgisi içermeyen yönler de veriyor: hedefsiz PCA-F 132.00, rastgele-çift F 131.25 (en iyi, ve rank'ı çok düşük, PR≈1–5), karışık residual 131.43. Kazanan sıralaması rank'a da uymuyor (PR 128 olan PCA, PR 1–5 olan rastgele F'den kötü); yani "yüksek rank = iyi" de desteklenmiyor. En iyi açıklama: X üzerinde herhangi bir makul doğrusal-olmayan genişleme ~1–2 PPL veriyor [Muhtemel]; residual hedefi bunu ölçülebilir biçimde artırmıyor.

## Dürüst durum ve sınırlar
- Performans kazanılmadı: en iyi doğru-taban sonucu 131.25–131.43, taban 133.20'ye göre −1.8…−1.95 PPL; yöntemler arası farklar (≤0.8 PPL) tek tohumda gürültü seviyesinde.
- V36 raporundaki "F gerçek kazanç (−10.4 PPL)" ve V31 "kimlik+4F" kazancı yeniden yorumlanmalı: V31/V36 yalnız-kimlik referansları intercept'siz.
- Rank-eşleme kusurlu: hedefsiz PCA-F rank'ı (PR 128) gerçek residual-F'den (76–104) yüksek, rastgele F'den çok yüksek; "benzer efektif rank" koşulunu tam sağlayan hedefsiz F üretilmedi.
- Hız: A/B/C ≈ 9–10 s, rastgele F ≈ 1.2 s; hiçbir kalite avantajı hız farkını ödemiyor.
- λ'ya duyarlılık: V36 ayarı (cezalı sabit sütun) doğrulamada 0.009 CE daha iyi çıktı; nedeni bu görevde incelenmedi.
