# V72: U hesabında gereksiz sayı/bellek hareketinin kaldırılması — sonuç: BAŞARILI (U bit-eşit, ~30× hızlı; varsayılan KAPALI bayrak)

Kod: `cpp/tek_hamle_v72.cpp` (V65 tabanı; `V72_UCSC=1` U yolu, `V72_SINGLE=1` tek-girdi ölçümü; V71'in T/dsyrk'ı **dahil değil**). V65/V71 değişmedi. Formül, çekirdek ve havuz aynı; yeni öğrenme algoritması yok.

## 1) U'daki gereksiz iş (ölçüm)
U = XᵀR_C. Eski yol (`h_u_sparse`) R_C'yi **kompakt grup blokları** olarak taradı: her slot satırı, grubun birleşim y-sütunları (ns_g) kadar giriş taşıyor ve bunların çoğu **yapısal sıfır** (bir çift slotu yalnız kendi 1–3 hedefini görür). Girdi sayıları (10.001): kompakt **16 599 671** giriş, gerçek sıfır-olmayan **279 500** (2.001: 8 460 919 → 217 705) = **59× / 39× az**. Eski yol her girişte 513'lük FMA yapıyor (1,66e7×513 = 8,5e9 madd) ve ACF/X satırını her iş parçacığı kendi y aralığı için baştan okuyor (4 iş parçacığı × tüm X + 128'lik j-blokları → X birden çok kez okunuyor, CPU/duvar ≈ 1,8).
**Yapılan:** gerçek sıfır-olmayanlardan y'ye göre sıralı (k artan) CSC kurulup her y sütunu **tek iş parçacığında, 513'lük tek vektörde** birikiyor (acc+C 8 KB L1'de), X satırı yalnız gerçekten gereken (k,y) için okunuyor, U tek yazma. Toplama sırası eski yolla aynı (k artan, aynı K-blok, `fma(1,acc,C)` boşaltması) ve sıfır terimler FMA'yı değiştirmediği için sonuç **bit-eşit**: 2.001'de U karması `8702ee7932712763` iki yolda aynı, WF ve val logit (Zval) karmaları da aynı; test PPL 88,893 aynı.

## 2) Ölçümler (hafif ayar, ortak veri; aynı oturumda A/B/A/B, A=V65, B=V72)
| Ölçüm | V65 (a,a) | V72 (b,b) |
|---|---|---|
| 10.001 U (g9) | 4,37; 3,56 s | **0,127; 0,138 s** (CSC kurulum 0,07 s dahil) |
| 10.001 model hazırlama | 14,3; 10,1 s | **7,15; 6,99 s** |
| 10.001 tepe RSS | 2,667; 2,666 GB | 2,658; 2,658 GB (≈ aynı; CSC ≈ 3 MB) |
| 10.001 val CE | 5,441941 | 5,441941 (aynı) |
| 10.001 test PPL (V72, ayrı eval koşusu) | 262,930 | 262,930 |
| 2.001 U (g9) / hazırlama | 0,927 s / 10,98 s | 0,091 s / 7,59 s |
| U aritmetiği (10k) | 8,5e9 madd | **1,43e8 madd** (−98,3 %) |
| U giriş taraması (10k) | 1,66e7 giriş | 2,8e5 giriş |
Not: model hazırlama farkının tamamı U değildir (V65 yolunun ilk koşusu 14 s VM gürültüsü içeriyor); U bileşeni kendi zamanlayıcısıyla 3,6–4,4 → 0,13 s (≈ 28–33×), toplam ≈ 3–7 s düşüş, tutarlı iki tekrarda. Gürültü payı için tekrar sayısı küçük (2 çift).

## 3) Tek yeni girdi (hazır ağırlıklarla; eğitim hesabı yok) — `V72_SINGLE=1`
z(x) = (F−ΣX_slot)·W_F + ΣY0_slot, tek iş parçacığı, 513'lük F̃ + sgemv-benzeri akış (n=1 toplu yol 16 ms idi: 2 MB tampon tahsisi + iş parçacığı maliyeti — bu yüzden ayrı tek-girdi yolu yazıldı).
| | 2.001 | 10.001 |
|---|---|---|
| Tam logit (tüm V sınıf) medyan (200 tekrar; minimum, maksimum) | **139 µs** (134 / 8 487 µs) | **1,74 ms** (1,37 / 3,74 ms) |
| Top-1 (argmax) ek maliyeti | 2,4 µs | 11,9 µs |
| Logit + softmax paydası + hedef olasılık | 139 µs | 2,37 ms |
| Aritmetik | F̃ çıkarma 1 026 + W_F 1,03e6 madd + Y0 ≈14 ekleme | 1 026 + 5,13e6 madd + ≈18 |
| Ara sayı | 513 (F̃) + 2 001 (logit) | 513 + 10 001 |
| Sözlük tarandı mı | Evet — tam softmax için W_F (4,1 / 20,5 MB) okunmak zorunda; Top-1 de aynı logitleri ister | |
| Doğruluk | tek-girdi logiti toplu yoldan maks |fark| 6e-8 (20 örnek) | aynı |
Maksimum değerlerin büyük olması ilk/önbellek etkisi ve VM gürültüsü; "ilk çağrı ayrı" ölçülmedi (rapordaki min/maks bunu temsil ediyor). Tek girdi için eğitim hesabı tekrar çalıştırılmadı (model hazır).

## 4) Diğer aşamalar (yalnız inceleme, uygulanmadı)
K-tablo+kapı 2,7 s (s(K) standardizasyon k4 1,3 s, atanh/tanh k8 1,1 s), V49 0,84 s, FᵀF 0,67 s (3,9e10 madd, ssyrk adayı), FᵀY 0,50 s. Aynı F satırı FᵀF, FᵀY, ACF/RC saçılımı için farklı aşamalarda okunuyor (n×513×4 = 600 MB okuma ×3): tek geçişte birleştirmek bellek hareketini kaldırır ama toplama sırasını değiştirir → bit-eşitlik riski; **bu görevde yapılmadı**. U'nun hesabı başka aşamaya taşınmadı (CSC kurulumu 0,07 s U içinde).

## Sorular
1. **Kaldırılan:** kompakt R_C'nin yapısal sıfırları üzerindeki ~1,6e7 giriş ×513 FMA, ve X'in iş parçacığı × j-bloğu başına yinelenen okunması.
2. **U süresi:** 3,6–4,4 s → 0,13 s (10.001); 0,93 → 0,09 s (2.001).
3. **Tam model hazırlama:** 7,0–7,1 s (10.001), V65 aynı oturumda 10,1–14,3 s.
4. **Tek yeni girdi:** 0,14 ms (2.001), 1,7 ms (10.001) tam logit; Top-1 ek maliyeti 2–12 µs.
5. **RAM:** değişmedi (2,67 → 2,66 GB); bu görevde bellek değil bellek *hareketi* ve madd hedeflendi.
6. **Kaldırılan işlem:** U'da 8,5e9 → 1,43e8 madd (−8,36e9); giriş taraması 16,6 M → 0,28 M.
7. **PPL/ağırlık:** korundu; U, WF ve Zval karması bit-eşit (2.001), test PPL 88,893 / 262,930, val CE 5,441941.
8. **Tek formül/çekirdek mi:** Evet, aynı formül, aynı XᵀR_C; yalnız yürütme düzeni (hangi girişlerin toplanacağı) değişti.
9. **50k–100k:** U maliyeti artık nnz(R_C)·513 ile orantılı (nnz ≤ 2n, V'den bağımsız); yoğun matris geri gelmiyor. Gerçek 50k+ model koşusu yapılmadı.
Bayrak `V72_UCSC` varsayılan KAPALI; bit-eşit olduğu 2.001'de karma ile, 10.001'de CE/PPL ile doğrulandı (10k U karması ayrıca karşılaştırılmadı).
