# V69: çift-koşullu eğim birikimi — sonuç: İPTAL TESTİ GEÇTİ (R² 0,955); Digits ve Diabetes hedefleri GEÇİLMEDİ

Kod: `v69_dogrudan_agirlik/cift_kosullu.py` (mekanizma + iptal testi), `genelleme.py` (8 tohum, Digits, Diabetes), ham çıktılar `sonuc.json`, `sonuc_genelleme.json`. V65–V68 değişmedi.

## Mekanizma (tek kural, açık tanım)
Toplamsal ikinci-mertebe momentler (artımlı): n, Σx, Σxxᵀ (d×d), Σy, Σxyᵀ. Her hedef t ve her özellik çifti (j,k) için **aynı kapalı-form 2×2 koşullu eğim**:
B_t[j,k] = (c_j V_k − c_k C_jk) / (V_j V_k − C_jk² + ε V_j V_k)
(c=çapraz kovaryans, V=varyans, C=özellik kovaryansı). Ağırlık: w_j = Σ_k A_jk B_t[j,k] / Σ_k A_jk, A_jk = ρ_jk^{2q} (korelasyonu yüksek partner baskın, q=0 düz ortalama). Tahmin: ŷ = ȳ + s (x−x̄)w, s = (w·c)/(wᵀCw) (momentlerden tek skaler). ε ve q **yalnız validation** ile seçildi; test hiç kullanılmadı.
**Dürüst denetim:** (i) d×d tam ters/Cholesky/ridge/SGD/iterasyon yok; (ii) ama her çift için 2×2 sistemin kapalı-form çözümü vardır — yani **yerel (ikinci mertebe) bir ters** içerir; bunu "klasik çözücüsüz" diye sunmuyorum, "tam d×d çözüm yok, çiftler düzeyinde yerel çözüm var" diye sunuyorum; (iii) Gram'ın tamamı (Σxxᵀ) birikir (çözülmez ama okunur: maliyet n·d²); (iv) A_jk ağırlığı veriye uyarlanmış bir seçim (korelasyon), genel bir kanıt değil; (v) bu bir **yaklaşımdır** (üçüncü mertebe iptalde kayıp: aşağıda). Eğitim örnekleri saklanmaz; parametre sayısı n'den bağımsız (d×C); yeni örnek = momentlere ekleme (üç parça artımlı fark 2e-14).

## Sonuçlar
| Ölçüm | V68 en iyi | V69 |
|---|---|---|
| İptal testi test R² (V68 verisi, bölme, tohum 11; ε=1e-2, q=4 val ile) | 0,116 | **0,9546** (val 0,947, eğitim 0,960) |
| İptal, 8 bağımsız tohum, test R² | – | 0,946–0,968, ort. **0,955**, min 0,945 (8'den 2'si 0,95 altında) |
| x₁/x₂ ağırlık oranı (r=0,965) | – | w₁=0,251, w₂=−0,287 → oran −0,87 (iptal yakalandı; V66'da pozitif/küçük) |
| 3'lü iptal (y=x₁+x₂−2x₃, sınır testi) | – | 0,857 (üçüncü mertebe kısmen kayıp) |
| Digits test doğruluğu (hedef ≥%97,2) | ölçülmedi | **%89,7** (val 89,97; V66 87,8; V67 97,2) — hedef **geçilmedi** |
| Diabetes test R² (hedef ≥0,359) | ölçülmedi | **0,333** (V67 0,292, V66 0,359) — hedef **geçilmedi** |
| Ağırlık üretme süresi | – | iptal 0,7 ms, Diabetes 28 ms, Digits 95 ms |
| Üretilen ağırlık sayısı | – | 40 / 512 / 5120 |
| Aritmetik işlem (madd) | – | iptal 1,16 M; Diabetes 71,4 M; Digits 306 M |
| Tepe RAM | – | iptal 0,11 MB; Digits 19 MB |
| Klasik çözücü | Yok | Tam d×d yok; çift başına 2×2 kapalı-form çözüm var (üstte) |
| Eğitim verisi olmadan tahmin | – | Evet (yalnız W,s,x̄,ȳ; sonuç özdeş) |
Etiket karıştırılmış kontrol: Digits %11,7 (şans), Diabetes R² −0,364 → öğrenme gerçek. Digits'te 306 M madd, aynı özelliklerde ridge zinciri (334 M) ile **aynı mertebe**: hesap kazancı yok.

## Sorular
1. **Tek hesaplama mekanizmasıyla gerçek ağırlık ürettin mi?** Evet (40/512/5120 ağırlık, örnek saklamadan) — ama mekanizma çift başına yerel 2×2 çözüm içeriyor.
2. **Özellikler arası bağımlılığı öğrendi mi?** Evet, ikili düzeyde (iptal çiftini yakaladı); üçlü düzeyde kısmen (0,857).
3. **İptal R² 0,95?** Evet: 0,9546 (tohum 11); 8 tohumda ort. 0,955, ama 2/8 tohum 0,95'in biraz altı.
4. **Klasik çözücü/optimizer?** Optimizer yok, iterasyon yok, tam çözücü yok; çift başına 2×2 kapalı-form çözüm (yerel ters) var.
5. **Matematiksel tanım:** üstteki B_t[j,k] ve ρ-ağırlıklı ortalama.
6. **Digits/Diabetes:** aynı mekanizma çalıştı, ancak **hedefler tutmadı** (%89,7 < %97,2; 0,333 < 0,359).
7. **Ölçeklenebilirlik kanıtlanmadı:** maliyet O(n·d² + C·d²) (d=512'de Gram'a eşdeğer), ε/q val ile ayarlı, üçlü iptale genelleme zayıf; büyük AI ağırlıkları için iddia yok.
