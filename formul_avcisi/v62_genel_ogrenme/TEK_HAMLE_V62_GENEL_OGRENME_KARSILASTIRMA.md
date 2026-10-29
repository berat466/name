# V62: Tek Hamle AI — metin dışı görevler ve klasik eğitimle karşılaştırma

Motor: **değiştirilmemiş `cpp/tek_hamle_v61`** (V61 commit fdee138; ikili aynı, kaynak aynı). Eklenen tek şey `adapter.py` (veri adaptörü). Kodlar: `adapter.py`, `grid.py` (hiperparametre seçimi, val'e göre), `kontrol.py` (etiket karıştırma + tekrar süreleri), `klasik.py` (klasik referans; Tek Hamle'ye bağlı değil). Ham çıktılar: `grid_*.txt/json`, `klasik_sonuc.json`, `tekhamle_kontrol.json`.
Donanım: bu VM (4 çekirdek, gürültülü; süreler tekrarlarla verildi). Bölme (hepsi aynı, stratified, random_state=0): %60 eğitim / %20 val / %20 test; standardizasyon eğitimden.

## TEST B — sayısal sınıflandırma: Tek Hamle motoru GERÇEKTEN çalıştı
Adaptör: sayısal özellikler motorun 64'lük girdi bloğuna (kalan 0) konuldu; **kelime bağlamı sabit 0** (C bloğu tek boş slota çöküyor); hedef = sınıf kimliği (V=sınıf sayısı). Yani öğrenme yalnızca F=tanh(γ(XW+b)) rastgele-özellik bloğu + tek analitik (Schur/Cholesky) çözümle; sayılar kelimeye çevrilmedi. Hiperparametre (γ, λ_F; 40 kombinasyon) **yalnız val doğruluğuyla** seçildi, test hiç seçimde kullanılmadı.

| Veri (eğit/val/test) | Tek Hamle test doğr. (val) | model hazır süresi (3 tekrar medyan) | klasik MLP test (val) | klasik lojistik test | eğitimsiz referans (çoğunluk) | etiketi karışık Tek Hamle test / klasik MLP test |
|---|---|---|---|---|---|---|
| Digits 64 özellik, 10 sınıf (1078/359/360) | **0,9889** (0,9861) | 0,25 s | 0,9833 (0,9778) | 0,9639 | 0,1028 | 0,1222 / 0,1083 |
| Breast Cancer 30 özellik, 2 sınıf (341/114/114) | 0,9474 (0,9649) | 0,39 s | 0,9737 (0,9561) | 0,9825 | 0,6228 | 0,6228 / 0,5965 |
| Wine 13 özellik, 3 sınıf (106/36/36) | 1,0000 (1,0000) | 0,28 s | 1,0000 (0,9722) | 0,9722 | 0,4167 | 0,4722 / 0,3056 |

Klasik: `MLPClassifier`, 1 gizli katman 512 nöron, Adam (lr 1e-3), L2 α∈{1e-4,1e-2,1} val'e göre; epoch: digits 92, bc 179, wine 161 (erken durdurma yok, max 500); parametre: digits 38 410, bc 16 898, wine 8 707. Eğitim doğruluğu 1,00. Klasik fit süresi (seçilen): digits 2,51 s, bc 0,87 s, wine 0,20 s (3 α'lık grid toplamı 9,5 / 2,7 / 0,9 s). Tek Hamle parametre: rastgele sabit projeksiyon (d·512, **eğitilmiyor**) + analitik çözülen 513·V ağırlık (digits 5 130, bc 1 026, wine 1 539). Süre karşılaştırması: Tek Hamle tek çözüm 0,25–0,39 s (sürecin tamamı 0,27–0,51 s duvar) — digits'te klasik MLP'den ≈10× hızlı; ama benim 40'lık (γ,λ) taramam digits'te ≈73 s duvar sürdü (süreç-başına arama, klasik 3'lük grid 9,5 s) → ayar maliyeti dahil toplam süre bu iş akışında Tek Hamle lehine değil. Küçük veri kümeleri (bc/wine test 114/36 örnek) genelleme kanıtı **değildir**; farklar bc'de 3 örnek, wine'da 0.

Sızıntı/ezberleme kontrolü: etiketleri karıştırılmış eğitimle Tek Hamle test doğruluğu şansa/çoğunluğa düştü (digits 0,12; bc 0,62 = çoğunluk; wine 0,47, n=36 gürültüsü). Yani doğruluk etiket–özellik ilişkisinden geliyor.

## TEST C — sürekli regresyon: motor DESTEKLEMİYOR (başarısız), yalnız denklem düzeyinde test
V61 motoru sürekli hedefi **çalıştıramadı**; denenmedi çünkü kısıt kodda yapısal: hedef `Data.y` tam sayı sınıf kimliği (`load`, `cpp/tek_hamle_v61.cpp:143`); sağ taraf `rhs_add(F, y, n, w, V, R)` yalnız one-hot sayım ekliyor (satır ~1267, `RY/RQ`); değerlendirme CE/Top1/PPL; Q_havuz (geç-birleştirme), K-grafı ve C bloğu sınıf=kelime kimliği varsayıyor. Genelleştirilmesi gereken: (1) y'nin float hedef olması, (2) R_F = Fᵀt ve R_C = Σ t (slot başına) sağ tarafı, (3) MSE/MAE/R² değerlendirmesi, (4) Q_havuz/Q-karışımın regresyon için kapatılması. Sınıf etiketlerine kutulama (binning) yapılmadı (yasak).

Bunun yerine **aynı çözüm denklemi** (sabit rastgele tanh özellik + tek Cholesky ridge çözümü, F=[1,tanh(γ(XW+b))], w=(FᵀF+λI)⁻¹Fᵀy) **motor dışı bir numpy gerçeklemesiyle** Diabetes'te (442×10, hedef sürekli; 265/88/89) çalıştırıldı. Bu **V61 motorunun sonucu DEĞİLDİR**, yalnızca denklemin sürekli hedefe uygun olup olmadığını gösterir.

| Diabetes test (89 örnek) | MAE | RMSE | R² | süre | parametre |
|---|---|---|---|---|---|
| Tek Hamle denklemi (motor dışı; γ=1, λ=1000, val'e göre) | 42,56 | 53,97 | 0,341 | tek çözüm 0,015 s (grid 0,29 s) | 513 çözülen |
| Klasik MLP (512, Adam, α=10, 156 epoch) | 41,45 | 53,22 | 0,359 | 0,48 s (grid 5,5 s) | 6 145 |
| Doğrusal ridge (referans) | 42,77 | 55,06 | 0,314 | 0,007 s | 11 |
| Ortalama tahmin (eğitimsiz) | 56,03 | 66,67 | −0,006 | – | – |
| Etiketi karışık (denklem) | 62,41 | 74,50 | −0,256 | – | – |

Diabetes gürültülü ve küçük; üç yöntem birbirinden istatistiksel olarak ayırt edilemez (89 test örneği). Sayısal hassasiyet: tek Cholesky çözümü, çift duyarlık; ayrıca kontrol edilmedi.

## TEST A — metin (kontrol; yeniden çalıştırılmadı, V61 ölçümü)
2.001 kelime, ~297 k eğitim tokenı, V59 ortak veri, V61 motoru: val CE 4,503665 / PPL 90,35 / Top1 0,1947 (aynı oturumda V60/V61 aynı), model hazır 8,8–21 s (VM gürültüsü). Test PPL V61'de ölçülmedi. **Klasik metin eğitimi bu görevde koşulmadı → metinde klasik karşılaştırma YOK.**

## Özet tablo
| Görev | Tek Hamle AI | Klasik AI |
|---|---|---|
| Metin tahmini doğruluğu | Val Top1 0,1947, CE 4,5037 (V61) | ölçülmedi |
| Sayısal sınıflandırma doğruluğu (test) | digits 0,989 / bc 0,947 / wine 1,000 | 0,983 / 0,974 / 1,000 (MLP) |
| Sayısal regresyon hatası | motor desteklemiyor; denklem (motor dışı): RMSE 53,97, R² 0,341 | RMSE 53,22, R² 0,359 (MLP) |
| Eğitim süresi | 0,25–0,39 s (tek çözüm) | 0,20–2,51 s (Adam, 92–179 epoch) |
| Parametre sayısı | 1 026–5 130 analitik (+ eğitilmeyen rastgele projeksiyon) | 8 707–38 410 |

## Sorular
1. **Metin dışı sayısal girdiden gerçekten öğrendi mi?** Evet, sınıflandırmada (motor değişmeden; etiket karıştırma kontrolü şansa düşüyor). Veri küçük; genelleme kanıtı değil.
2. **Sınıflandırma:** Evet — digits'te test %98,9, klasik MLP'den ≥ bir sınıf iyi/eşit; bc'de 3 örnek geride, wine'da eşit.
3. **Sürekli çıktı:** **Motorda hayır** (desteklenmiyor, çalıştırılmadı). Yalnızca aynı denklemin motor dışı gerçeklemesi MLP ile benzer sonuç verdi.
4. **Üç görevde aynı motor?** Metin ve sınıflandırma: aynı ikili (`tek_hamle_v61`). Regresyon: hayır — motor dışı denklem, başarı olarak sayılmıyor.
5. **Klasik AI'den hızlı mı?** Tek çözüm süresi evet (digits ≈10×, bc ≈2×, wine ≈1× — süreler küçük ve gürültülü). Ayar taraması dahil toplam süre bu iş akışında değil.
6. **Aynı veya daha iyi doğruluk?** Sınıflandırmada aynı bölgede (digits +0,6 puan, bc −2,6 puan [3 örnek], wine eşit); regresyonda (motor dışı) MLP'den hafif geride (R² 0,341 vs 0,359), farklar anlamlı değil.
7. **Kısıtlar:** (a) hedef tam sayı sınıf kimliği + one-hot RHS; (b) sınıf=kelime varsayımı (K-grafı, Q_havuz, C bloğu); (c) girdi 2×64 sabit düzen (`expand_s`, K=128); (d) hiperparametre (γ, λ) metin için ayarlı, tablo verisinde taranması gerekti; (e) özellik bloğu rastgele sabit projeksiyon — özellik öğrenimi yok (derin temsil öğrenmez).
8. **Eksik somut unsur:** sürekli hedefli sağ taraf ve değerlendirme (regresyon desteği), sınıf-kelime varsayımından bağımsız bir "tablo kipi" (C/Q/K bloklarının kapatılabilmesi), ölçekli ve çok daha büyük gerçek tablo/görü verilerinde (ör. MNIST, 70 k) doğrulama; birden fazla katmanlı temsil gerektiren görevlerde klasik eğitimin yerini alabileceği henüz gösterilmedi.

## Sonuç sınıfı
Sınıflandırma: **KISMİ BAŞARI** (motor gerçekten öğrendi, klasikle başa baş; küçük veri). Regresyon: **MOTORDA BAŞARISIZ / desteklenmiyor** (denklem düzeyinde olumlu sinyal). Metin: V61 kontrolü korundu. "Klasik eğitimin yerine geçti" iddiası bu deneylerle **kanıtlanmadı**.
