# V67: tek temel işlem — softmax-ağırlıklı dış-çarpım yazma/okuma (yoğun birleşmeli bellek)

Kod: `v67_tek_temel_islem/tek_temel_islem.py`, ham çıktı `sonuc.json`, `cikti.txt`. V65/V66 ve öncesi değişmedi.
**Bilinen yöntem — yeni keşif değil:** Nadaraya–Watson çekirdek düzgünleştirici (1964), yoğun birleşmeli bellek / modern Hopfield (Krotov–Hopfield 2016; Ramsauer vd. 2020), dikkat (attention) okuması.

## Tek işlem (tanım)
- **Yazma (ağırlık üretimi):** her örnek için anahtar kᵢ = (φ(xᵢ)−m)/‖φ(xᵢ)−m‖, değer vᵢ = yᵢ; bellek (K,V)'ye **satır eklenir**. Tek aritmetik: çıkar, normalize et, ekle (n·H madd). Yeni örnek = tek ekleme; eski hesap tekrarlanmaz (artımlı tahmin farkı **0,0**, bit-eşit).
- **Okuma:** ŷ(x) = Σᵢ softmax_i(β·k(x)·kᵢ) vᵢ. Tek işlem: benzerlik (iç çarpım) → softmax → ağırlıklı hedef toplamı. Özellikler arası ilişki iç çarpımda doğrudan kullanılır.
- Sınıflama (V=one-hot değer) ve regresyon (skaler değer) **aynı işlem**; Gram/Cholesky/Schur/ridge/SGD/optimizer/kovaryans–varyans–bölme **yok**. Tuttuğu ara bilgi: yalnızca (K,V) ve m. φ: sabit rastgele tanh(γ(xW₁+b)), H=512 (V66/motorla aynı, eğitilmez). γ, β **yalnız val** ile seçildi.
- **Kısıt (açık):** "ağırlık" sabit boyutlu parametre değil, O(n·(H+C)) bellek; tahmin maliyeti O(n·H) / örnek (V66: O(H·C)).

## Tablo
| Ölçüm | V66 | V67 |
|---|---|---|
| Temel işlem | çapraz-kovaryans / varyans bölme | softmax-ağırlıklı dış-çarpım yazma/okuma |
| Ağırlık üretim süresi (Digits) | 5 ms | 19,6 ms (yazma); okuma 360 örnek 8,5 ms |
| Digits test doğruluğu (görülmeyen, n=360) | %87,8 | **%97,2** (val %97,5; etiket karışık %13,1; V62 motoru %98,9; ridge zinciri aynı φ %97,2) |
| Diabetes R² (test, n=89) | 0,359 | **0,292** (MAE 41,35, RMSE 55,93) — hedefin altında; etiket karışık −0,32 |
| İşlem (Digits) | 6,6 M madd | yazma 0,55 M + okuma 202,6 M madd |
| RAM | 8,9 MB | 13,3 MB tepe; bellek 4,5 MB |
| Korele özellikler (sentetik, 300 özellik, 5 gizli değişken, ort. |r|=0,37, doğrusal hedef) | – | V66 R² 0,994, **V67 0,912**, ridge 0,9988 |
| İptal testi (y=x₁−x₂, çiftler r=0,965) | R² −0,015 | R² **0,009**, ridge 0,99998, ortalama tahmin −0,00002 |

## Sorular
1. **Gerçekten tek temel hesaplama türü buldun mu?** Bir tane tanımlandı ve üç göreve uygulandı, ama **yeni değil** (çekirdek düzgünleştirici/birleşmeli bellek).
2. **İşlem:** (K,V) ← (K,V) ∪ {(k̂ᵢ, yᵢ)}; ŷ = softmax(βKk̂(x))ᵀV.
3. **Hangi adımlar üretiyor:** çıkar+normalize+ekle (yazma); iç çarpım+softmax+ağırlıklı toplam (okuma). Bunun dışında hiçbir şey yok.
4. **Klasik zincir gizlice kullanılıyor mu?** Hayır (karşılaştırma ridge'i yalnızca referans). Ancak **parametrik ağırlık değil, örnek belleği** üretiyor — ağırlık = veri kopyası. Bu, "ağırlık üretimi" tanımını zorluyor; sıkıştırma yok.
5. **V66'nın sorununu çözdü mü?** Kısmen: Digits'te evet (%87,8 → %97,2). Diabetes'te hayır (0,359 → 0,292). Doğrusal korele özellikte **kötüleşti** (0,994 → 0,912). İptal testinde **ikisi de başarısız** (R²≈0; ridge 0,99998). Kaybolan bilgi: sinyal küçük-varyanslı bir yönde (e₁−e₂) olduğunda benzerlik/korelasyon tabanlı işlemler baskın ortak bileşeni (u) görür; yönü ayıran **ters-kovaryans (beyazlatma)** bilgisi bu işlemlerde yok. Ridge'in Gram çözümü tam bunu yapıyor → "çözücüsüz" tek işlem bu sınıfta eşdeğer değil.
6. **Ölçeklenir mi?** Kanıtlanmadı. Bellek ve okuma maliyeti n ile doğrusal/örnek-başı artar (milyar parametreli ağırlıklara karşılık gelen sabit boyutlu bir temsil yok). Doğrusal-bağımlı özellik/ince yön problemleri çözülmedi. Kanıtlanan: küçük tabular veride (Digits) çözücüsüz %97,2.
**Sonuç sınıfı:** KISMİ — Digits'te güçlü, Diabetes'te zayıf, iptal yapısında başarısız; yöntem bilinen bir yöntem; "klasik eğitim zincirine gerek yok" iddiası **genel olarak doğrulanmadı**.
