# V70: hiyerarşik toplam/fark (Haar-benzeri) dekorelasyon + Hebb kuralı — sonuç: BAŞARISIZ (yalnız ikili iptal geçti; üçlü ve çoklu testler geçilmedi)

Kod: `v70_tek_hesaplama/haar_agaci.py` (mekanizma + A/B/C testleri; `python3 haar_agaci.py [full]`), `teshis.py`, ham çıktılar `sonuc.json`, `sonuc_full.json`. V65–V69 değişmedi. Talimat gereği A/B/C'nin hepsi geçilmediği için **Digits ve Diabetes çalıştırılmadı**.

## Aday (tek hesaplama türü, bilinen yöntem: Haar/Walsh–Hadamard dönüşümü + hiyerarşik özellik birleştirme; yeni keşif değil)
- Birikim (artımlı): n, Σx, Σxxᵀ, Σy, Σxyᵀ.
- Tek işlem: iki standartlaştırılmış özellik a,b (işaret s=sgn ρ) → **toplam (a+s·b)/√2 ve fark (a−s·b)/√2** (yalnız ±, ÷√2; bölme-ile-çözüm, determinant, ters yok). Eşit varyansta ikisi tam dekoreledir. Eşleşme korelasyon sırasına göre (|ρ|>τ, açgözlü); farklar son özellik, toplamlar aynı kuralla tekrar eşleşir (varyant "full": farklar da yeniden eşleşir).
- Son özellikler z_f = T_f x üzerinde Hebb kuralı W_f = Cov(z_f,y)/Var(z_f); nihai ağırlık w = Tᵀ W (girdi uzayında, d×C); tek skaler s. τ **yalnız val** ile seçildi. Örnek saklanmaz, parametre sayısı n'den bağımsız.
- **Denetim:** ters/Cholesky/ridge/SGD/optimizer yok; yerel 2×2 *çözüm* yok (sabit ±45° dönüş), ancak **eşleşme (korelasyon sıralaması) + çok seviyeli tekrar** var; "full" varyantı Jacobi-benzeri bir tekrarlı dekorelasyondur (bunu kural ihlali sınırında sayıyorum). V69'dan farkı: koşullu eğim çözümü yok, sabit dik dönüşler var.

## Sonuçlar (V69 ile aynı veri ve bölmeler: 700/250/250; tohum 11, 5)
| Test (hedef R² ≥ 0,95) | V69 | V70 (toplam yalnız) | V70 full |
|---|---|---|---|
| A ikili (y=x₁−x₂, r=0,965) | 0,9546 | **0,958** (τ=0,3) | 0,958 |
| B üçlü (y=x₁+x₂−2x₃) | 0,857 | **0,349** | 0,359 |
| C1 çoklu, 12 özellik, eşit yükleme | – | 0,879 | 0,879 |
| C2 çoklu, 12 özellik, heterojen yükleme | – | 0,923 | 0,929 |
| C3 iki faktör, 16 özellik | – | 0,644 | 0,757 |
C'de katsayılar algoritmaya verilmedi (hedef = Σ aⱼ eⱼ, aⱼ rastgele). **A geçti; B ve C geçmedi → V70 başarısız**, V69'un üçlü sonucunun (0,857) da gerisinde. Digits/Diabetes: ölçülmedi.
Maliyet (A): 8 ms, 1,15 M madd, 40 ağırlık, < 1 MB.

## Neden başarısız (ölçülen)
Üçlü iptal verisinde Gram koşul sayısı **183**; 60 özdeğerden 40'ı < 0,2; en küçük özdeğerler 0,021–0,026 (neredeyse **dejenere** 40 boyutlu alt uzay); sinyalin yaklaşık %46'sı en küçük 20 yönde. İki sorun: (i) y=x₁+x₂−2x₃ yönü, birim-varyanslı özelliklerin ±45° toplam/farkıyla ifade edilemez ((1,1,−√2) gibi katsayılar çıkar, (1,1,−2) değil); doğru açı = 2×2 özdeğer problemi çözmek (yerel çözücü, yasak); (ii) dejenere alt uzayda doğru taban, n=700 örnekte gürültülü korelasyonlardan sabit dönüşlerle bulunamıyor — taban seçimi tam özayrışım gerektiriyor. Küçük-özdeğerli yönlerin yükseltilmesi (kazanç ≈ 1/λ ≈ 40–180) ancak ters/özayrışım ya da kondisyon sayısı kadar adımla elde ediliyor. V66–V69'da görülen aynı engel; bu aday onu aşmadı.

## Kesin cevaplar
1. **Tek temel işlem bulundu mu?** Tek işlem (dik toplam/fark) tanımlandı; **doğru ağırlık üretemedi** (genel olarak başarısız).
2. **Nasıl çalışıyor?** Yukarıda: korelasyonla eşleştir → toplam/fark → Hebb → w=TᵀW.
3. **Klasik çözücü/optimizer?** Hayır; ancak "full" varyantı tekrarlı dekorelasyon içerir (ihlal sınırında) ve eşleşme adaptiftir.
4. **Gerçek nihai ağırlıklar üretildi mi?** Evet (d×C, örneksiz tahmin), ama B/C'de yanlış.
5. **İkili/üçlü/çoklu öğrenildi mi?** İkili evet (0,958); üçlü hayır (0,35); çoklu kısmen (0,64–0,92).
6. **Klasik referansa göre maliyet azaldı mı?** Evet küçük boyutta (ms, ~1 M madd) ama doğruluk hedefi tutmadı; karşılaştırma anlamsız.
7. **Büyük AI ağırlıklarına uygulanabilirlik:** kanıtlanmadı (B/C başarısız; Digits/Diabetes ölçülmedi).
