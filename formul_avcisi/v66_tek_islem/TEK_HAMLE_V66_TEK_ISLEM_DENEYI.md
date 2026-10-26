# V66: tek işlemle doğrudan ağırlık üretimi — sonuç: KISMİ (mekanizma çalışıyor, ağırlık üretiyor; doğruluk çözücülü zincirin altında)

Kod: `v66_tek_islem/tek_islem.py` (tek dosya, V65 ve öncesine dokunulmadı), ham çıktı `sonuc.json`. Her ölçüm gerçekten çalıştırıldı (≈ 20 s toplam).

## 1) Tek işlem — tanım
Girdi–hedef çiftlerinden **tek geçişli çapraz-kovaryans birikimi**:
- Birikim (artımlı): S = (n, Σφ, Σφ², Σy, φᵀy); yeni örnek gelince S += (φᵢ, yᵢ) — eski hesap tekrarlanmaz.
- Ağırlık: **W = ( φᵀy/n − φ̄ȳᵀ ) ⊘ ( var(φ)+ε )** (öğe-bazlı bölme; matris tersi/çözüm yok).
- Tahmin: ŷ = ȳ + s·(φ−φ̄)W. Sınıflamada s önemsiz (argmax), regresyonda s = tek skaler (eğitim istatistiğinden ⟨h,y−ȳ⟩/⟨h,h⟩).
- φ: motordakiyle aynı sabit rastgele tanh(γ(xW₁+b)) (H=512, eğitilmez); işlem-belirleme testinde φ = ilkel işlem kütüphanesi {a+b, a−b, a·b, a/b}. Aynı birikim, yalnız φ/Y biçimi değişiyor; işlem seçimi = |korelasyon| argmax'ı (korelasyon aynı S'den).
**Ortadan kalkanlar:** Gram matrisi (FᵀF), Cholesky, Schur, ridge çözümü, grup çözümleri, SGD/backprop. Kalan tek şey Fᵀ Y (O(n·H·C)) + öğe-bazlı bölme. Bu bir "yeni formül": Gram'ın yalnız köşegenini kullanıyor → öznitelikler arası korelasyonu yok sayıyor, bu yüzden daha zayıf (aşağıda ölçüldü).

## 2) Deneyler (aynı mekanizma)
**A — işlem belirleme + sayısal sonuç.** 4 ayrı veri kümesi (400 örnek, a,b∈[1,20]; hedef a+b, a−b, a·b, a/b); sistem işlemi doğru seçti **4/4** (kendi korelasyonu 1,0000; diğerleri ≤0,95), ağırlık (eğim) ≈ 0,99999999. **Görülmeyen aralıkta** (a,b∈[50,500]) en büyük göreli hata 1,6e-10 (+), 1,9e-10 (−), 1,6e-12 (×), 3,9e-8 (/). **Uyarı:** doğruluk, elle verilen ilkel işlem kütüphanesinden gelir; aritmetik öğrenme değil, kütüphaneden seçim. AI öğrenmesi diye sunulmuyor (ağırlık üretimi var: 4 eğim, ama ifade gücü kütüphaneye ait).
**B — sınıflandırma (Digits 64 özellik, 10 sınıf; 1078 eğit / 359 val / 360 test; V62 ile aynı bölme).** γ yalnız val ile seçildi (γ=1,0). Ağırlık: 5120 sayı, **6,6 M madd, 5,2 ms, 8,9 MB** (tracemalloc). Eğitim %88,2, **görülmeyen test %87,8**, val %85,8. Etiketi karıştırılmış kontrol %10,8 (şans). Artımlı: veri 3 parçada gelince ağırlık farkı 7e-16 (aynı sonuç, tekrar hesap yok).
**C — sürekli regresyon (Diabetes, 442×10; 265/88/89).** Görülmeyen test: MAE 43,24, RMSE 53,22, **R² 0,359** (ortalama tahmini R² −0,006; etiket karışık R² −0,317). 512 ağırlık + 1 skaler, 0,6 ms.

## 3) Karşılaştırma
| Ölçüm | Yeni tek işlem (V66) | V65/V62 motoru (referans) |
|---|---|---|
| Gerçek ağırlık üretimi | Evet (5120 / 512 sayı, A'da 4 eğim) | Evet |
| Hesaplama türleri | Fᵀy birikimi + öğe bölme | Gram, grup/ok-başı çözüm, Cholesky 513³, Schur, U, sgemm, Q-karışımı |
| Süre (Digits) | 5 ms | 0,25–0,4 s (V62, süreç içi) |
| İşlem (Digits, aynı φ) | 6,6 M madd | 334 M madd (Gram+Cholesky+RHS, tek λ; aynı φ'de numpy ridge) |
| RAM | 8,9 MB | 27 MB (V62 süreci) |
| Görülmeyen veri başarısı, Digits | **%87,8** | %98,9 (V62 motoru), ridge zinciri aynı φ'de %97,2 |
| Görülmeyen veri, Diabetes | R² 0,359 | motor desteklemiyor (V62); MLP 0,359, ridge aynı φ 0,305 |
| Klasik çözücü gerekiyor mu? | **Hayır** | Evet (Cholesky/Schur) |

## Sorular
1. **Tek hesaplama işlemini buldun mu?** Evet, tek bir birikim+öğe-bölme işlemi; üç göreve de uygulandı.
2. **Tanım:** W=(φᵀy/n − φ̄ȳᵀ)⊘(var φ+ε), birikim S artımlı.
3. **Klasik zincir olmadan ağırlık ürettin mi?** Evet; Gram/Cholesky/Schur/ridge/SGD yok (karşılaştırma ridge'i yalnız referans).
4. **Gerçek işlem sayısı:** Digits 6,6 M madd (+ φ üretimi n·64·512=35 M); ridge zinciri aynı φ'de 334 M. Diabetes 0,14 M madd.
5. **Görülmeyen veri başarısı:** Digits %87,8 (ridge zincirinden ~9 puan düşük), Diabetes R² 0,359 (zincirle eşit/üstü, küçük veri), A: aralık dışı hata ≤4e-8.
6. **V65'ten az hesapla sonuç?** Evet işlem sayısı ~50× az, ama sınıflamada doğruluk belirgin düşük; V65 metin LM'ine bu işlem **uygulanmadı** (metin testi yok).
7. **Nerede başarısız:** (i) öznitelikler korele olduğundan Gram köşegeni yetersiz → sınıflamada ~9–11 puan kayıp; (ii) metin (V×V sınıf, C slotları) denenmedi; (iii) A'nın doğruluğu elle verilen kütüphaneye bağlı, çarpma gibi işlemi örneklerden **keşfetmiyor**; (iv) regresyon için bir skaler (s) hâlâ eğitim verisinden hesaplanıyor.
