# V73: hesaplar arası doğrudan aktarım — sonuç: BAŞARILI (2 bit-eşit kaldırma; F-geçişlerinin birleştirilmesi bilinçli olarak yapılmadı)

Kod: `cpp/tek_hamle_v73.cpp` (V72 + `V73_RHS=1`, `V73_SYRK=1`; varsayılan KAPALI), ham çıktılar `v73_dogrudan_aktarim/ham/`. V65/V71/V72 değişmedi. Formül, çekirdek, havuz aynı; yeni algoritma yok.

## Denetim: F ile ilgili bağlantılar (kaynak kodundan; F=n×513 float, 10.001'de ≈ 0,6 GB)
F üretimi (sgemm → tanh yerinde) → kapı (yerinde) → V49 (yerinde) → **FᵀF** (`cross`) → **FᵀY** (`rhs_add`) → **slot toplamları** (ACF/RC). Aynı F üç ayrı tüketici tarafından üç ayrı geçişte okunuyor (≈1,8 GB okuma).
Denetlenen bağlantılar ve kararlar:
| İşlem bağlantısı | Doğrudan aktarılabilir mi? | Kaldırılan işlem / veri hareketi | Süre kazancı (10.001, ardışık A/B/A/B) | Bit eşitliği |
|---|---|---|---|---|
| F → FᵀY (`rhs_add`) | Evet (üretici/tüketici düzeni uyumsuzdu: parça toplamı [j][c], adım V·8 bayt) | satır başına 513 dağınık 8-bayt yazma (≈1,5e8 önbellek satırı, ≈9,7 GB hat trafiği) → 513 bitişik ekleme (satır başına 4 KB). Hesap sayısı aynı | f7: 0,46–1,75 s → **0,20–0,24 s** (2.001: 0,196 → 0,105) | **Bit-eşit** (RF karması aynı, 2.001 ve 10.001) |
| F → FᵀF (`cross`) | Evet (A≡B simetrik, yalnız ssyrk) | üst üçgen: 7,8e10 → **3,9e10 madd** (n·513²/2); alt üçgen kopyalanır | f6: 0,69–0,72 → **0,46–0,47 s** | **Bit-eşit** (AFF karması aynı) |
| F → ACF/RC (slot toplamı) ile FᵀY/FᵀF'in tek geçişte birleştirilmesi | **Uygulanmadı**: toplama sırası değişir (RF için y-sırası ↔ grup-sırası), bit-eşitlik bozulur | (potansiyel ≈1,2 GB okuma) | – | bozulur → etkinleştirilmedi |
| RC → U (V72) | Evet (V72, gerçek sıfır-olmayan CSC) | 1,66e7 giriş × 513 → 2,8e5 × 513 | U 3,6–4,4 → 0,13 s (V72) | Bit-eşit; 10.001'de de doğrulandı (aşağıda) |
| K-graf → kapı (s(K) → standardizasyon → gemm → atanh/tanh) | Zaten anahtar tablosuyla (kat,kelime) bir kez (V54/V61); n×512 dizisi oluşmuyor | – | – | – |
| Q_havuz → projeksiyon | Zaten seyrek satır + π (V64); V×V yok | – | – | – |
| X/Y0 → örtük çıktı | Zaten doğrudan (V64): W_C oluşmuyor | – | – | – |
| X (double) → Xf (float) | Gerekli dönüşüm: U çift duyarlık (bit-eşitlik), örtük okuma float | (Xf yazımı arrow döngüsünde aynı geçişte) | – | – |

## Doğrulama (V72 ile aynı veri/ayar; `V53H_HASH=1`)
- 2.001: AFF `e361bcb1efb2548e`, RF `b0b28f8c4c793346`, ACF, U `8702ee79…`, W_F `3630d647…`, Zval — **hepsi V72=V73(RHS)=V73(RHS+SYRK)**; test PPL 88,893.
- 10.001: AFF `26401df5ea0b1dea`, RF `18b4555f8bd71339`, ACF `8ea5f941b27af912`, X `e2858fa1ad353f51`, **U `bc55ec1d32364d8d`**, W_F `d54c462f219c825b`, Zval `b853e845e74f2c8b` — V72 ve V73 aynı. **U karması V65 (V72_UCSC=0) ile de aynı** (`bc55ec1d32364d8d`): V72'nin bit-eşitliği 10.001'de doğrulandı. Val CE 5,441941, **test PPL 262,930** (Top1 0,1370).
- Model hazırlama (10.001, HASH'siz): V72 6,39; 13,34 s (ikinci koşu VM gürültüsü: f7 1,75 s) → V73 5,79; 6,10 s. Tepe RSS 2,658 GB iki yolda da aynı. Süre farkının yalnız f6+f7 bileşenleri güvenilir: toplam ≈ 0,5–0,9 s; toplam süre kazancı 2 çiftle gürültüden ayrılamadı.

## Sorular
1. **Kaç ara işlem tamamen kaldırıldı?** Yeni: 2 (FᵀF'in alt üçgeni — 3,9e10 madd; FᵀY'de sınıf-ana olmayan dağınık yazma düzeni). V72'den: U'nun yapısal-sıfır taraması (1,6e7 giriş). 
2. **Önlenen gereksiz bellek hareketi:** FᵀY'de ≈ 9,7 GB önbellek-hattı trafiği → ≈ 1,2 GB bitişik (tahmin, sayım tabanlı; donanım sayacı ölçülmedi); FᵀF'te yarı hesap (okunan F aynı). V72 U'da ≈ 8,5e9 → 1,4e8 madd.
3. **Model hazırlama:** 10.001'de 5,8–6,1 s (V72: 6,4 s temiz koşu; 13,3 s gürültülü).
4. **Dönüşümsüz bağlananlar:** XY0→örtük çıktı; Q satırı→h_fuse; RC→CSC→U (aynı çift duyarlık sayılar, kopya yok); F→FᵀF/FᵀY (aynı float tampon, ek kopya yok).
5. **Zorunlu dönüşümler:** X çift→float (Xf, örtük okuma için), partial-toplam→R (tek bir mf×V transpoze/indirgeme, ≈20 MB × iş parçacığı), kapı atanh/tanh (anahtar başına bir kez), Schur Cholesky 513³/3, FᵀF'in üst üçgeni (n·mf²/2). Ek: F'nin üç geçişte okunmasını birleştirmek toplama sırasını değiştirdiğinden bit-eşitliği bozar → uygulanmadı.
Bayraklar varsayılan KAPALI; bit-eşitlik hash'le kanıtlandığı için ana sürüme bağlamak güvenli, ama bu görevde bağlanmadı. 50k–100k: her iki değişiklik V'den bağımsız yoğun matris getirmez; FᵀY parça tamponu nt·V·513·8 B (100k kelimede ≈ 1,6 GB/iş parçacığı → bu değişiklik büyük V'de **bellek maliyetini** eski yolla aynı tutar, azaltmaz).
