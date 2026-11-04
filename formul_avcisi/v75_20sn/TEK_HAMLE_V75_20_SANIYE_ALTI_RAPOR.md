# V75: 10.001 kelime, bütün program — sonuç: MEDYAN 19,0 s (5 koşu: 17,1–20,4 s); hedef KISMEN sağlandı (3/5 koşu < 20 s)

Kod: `cpp/tek_hamle_v75.cpp` (V73 + `V75_STREAM=1`; varsayılan KAPALI), ham çıktılar `v75_20sn/ham/`. V65–V74 değişmedi. Aynı veri (10.001, V59 ortak, aynı bölmeler), aynı parametreler, 4 CPU iş parçacığı. Tüm V73 hızlı yol bayrakları açık ve çıktıda doğrulandı: `V63_ARROW=2` (ok-başı + seyrek U), `V64_IMPL=1` (örtük W_C), `V64_SPQ=1` (seyrek Q_havuz), `V64_COMPACT=1`, `V72_UCSC=1`, `V73_RHS=1`, `V73_SYRK=1`, seyrek K-graf/Q/Cn (V60/V61 varsayılan açık). Bağımsız duvar saati: `calistir.py` rusage WALL (başlatma→bitiş).

## 1) 51 saniye nerede harcanıyordu? (V74 yolu, ölçülen; toplam 51,3 s)
| Aşama | Süre |
|---|---|
| Veri okuma + hazırlık | ≈ 0,5 s |
| Model hazırlama (`MODEL_HAZIR`: özellik, K/kapı, Gram/RHS, ok-başı, U, W_F, Q) | 9,2 s |
| Validation logit + okuma seçimi (e1a) | 10,8 s |
| α_Q seçimi (e1b) | 4,8 s |
| V65 tanı değerlendirmesi (test akış + val + eğitim alt kümesi + 4 kontrol + tek-girdi) | 26,3 s |
| Toplam | 51,1 ≈ 51,3 s (duvar) |
Aynı bayraklarla tanı değerlendirmesi (`V65_EVAL`) kapalıyken (V73 yolu) 36,3 s: model 6,3 s + val logit **11,2 s** + pickLin 1,0 s + α_Q 4,2 s + test **11,7 s**; MAXRSS 4,1 GB, SYS süresi 21–30 s. Asıl neden: n×V logit matrislerini (36 852×10 001×4 B = 1,5 GB) **sıfırdan tahsis etmek** (VM'de sayfa hatası maliyeti: ≈ 0,37 M sayfa/matris, MINFLT 1,4 M) ve aynı logit/softmax geçişlerinin yinelenmesi.

## 2) Yapılan (hesap aynı, gereksiz hesap/hareket kaldırıldı)
- Val ve test logitleri **2048 satırlık tek yeniden kullanılan tampona** blok-akışla üretiliyor (1,5 GB ×2 tahsis yok; tepe RSS 4,1 → **1,56 GB**; MINFLT 1,41 M → 0,72 M).
- α_Q seçimi: 8 aday α için aynı örneğin pm vektörü ve q satırı **bir kez** kuruluyor, 8 alfa tek örnek içinde ardışık hesaplanıyor (eski: 9 ayrı h_fuse çağrısı, her çağrıda S₂/pm/q yeniden); tam softmax paydası, hedef olasılık ve **tüm V sınıfı üzerinden rank** korunuyor (Top-1/Top-5 tam).
- Test, seçilen (α, ε, α_Q) ile bir geçişte; validation/test örnek sayısı aynı (36 852/36 852), sözlük 10.001, eğitim verisi aynı. Test atlanmadı; hiçbir kısaltma yok.
- V65 tanı blokları (eğitim alt kümesi, karıştırma, tek-girdi) bu yolda çalışmıyor — bunlar sonuç için zorunlu değil (V74'te yalnız raporlama içindi).

## 3) Gerçek sonuç (aynı oturum, bağımsız duvar saati)
| Koşu | Program (duvar) | `MODEL_HAZIR` | Val+test akışı |
|---|---|---|---|
| V75 s1 | 20,41 s | 9,50 s | 3,43+4,28+3,07 |
| V75 s2 | 18,06 s | 7,72 s | 3,35+3,98+2,91 |
| V75 s3 | 20,11 s | 9,30 s | – |
| V75 s4 | 19,02 s | 8,15 s | – |
| V75 s5 | **17,07 s** | 6,22 s | – |
| **Medyan V75** | **19,02 s** | 8,15 s | ≈ 10,4 s |
| V74 yolu (aynı oturum, `V65_EVAL`, aynı ikili) | 51,36 s | 10,96 s | – |
| V73 yolu (EVAL yok, dense val/test) | 36,30 s | 6,27 s | 11,2+1,0+4,2+11,7 s |
Model hazırlama farkı VM gürültüsü (SYS süresi 1,8–9,8 s): 6,2–9,5 s; V73'ün temiz ölçümü 5,8–6,1 s. Aşama toplamları duvarla uzlaşıyor: s2 → 7,72 + 10,2 (val+test) + ≈0,2 yükleme ≈ 18,1 s.
**Doğruluk:** Val CE **5,441941**, test CE 5,571887, **test PPL 262,930**, Top-1 0,1370, Top-5 0,3112 (V73/V74 ile aynı, 5/5 koşu), α_Q=0,3, okuma α=0,85, ε=1e-5 aynı. Model-hazırlama karmaları V73 ile **aynı**: AFF `26401df5ea0b1dea`, RF `18b4555f8bd71339`, ACF `8ea5f941b27af912`, X `e2858fa1ad353f51`, U `bc55ec1d32364d8d`, W_F `d54c462f219c825b`. Logit (Zval) karması akış yolunda üretilmiyor (Z saklanmıyor); eşdeğerlik CE/PPL/Top-k'ın altı hanesi ve seçilen hiperparametrelerle doğrulandı. Tek fark riski: birleşik pm/q kod yapısında FMA kontraksiyonu; ölçülen CE/PPL 6 hanede aynı.

## 4) Net cevaplar
1. **Bütün program:** medyan **19,0 s** (17,1–20,4).
2. **Ağırlık hazırlama:** 6,2–9,5 s (medyan 8,2 s bu oturumda; V73 temiz 5,8–6,1 s).
3. **Validation + test:** ≈ 10,4 s (val aşama-1 3,4 + aşama-2 4,0 + test 3,0).
4. **Kaldırılan:** V65 tanı değerlendirmesi (26 s, sonuçta zorunlu değil), 2× 1,5 GB logit tahsisi (+ sayfa hataları), 9× tekrar eden pm/q/softmax paydası kurulumu; test/val logit **gemm ikişer kez değil birer (val için iki: seçim + α taraması) kez**.
5. **İşlem/bellek:** tepe RSS −2,5 GB (4,1 → 1,56), sayfa hatası −0,7 M; val'de gemm iki kez (aşama-1 ve -2) çalışıyor (1,9e11 madd fazladan) — Z saklamamak için bilinçli takas; e1b'de h_fuse 9 geçiş → 1 birleşik.
6. **PPL/CE:** tamamen korundu (altı hane).
7. **Hızlı yollar aktif mi:** Evet (bayraklar yukarıda; karmalar V73 ile aynı).
8. **20 s hedefi:** **KISMEN — medyan 19,0 s ile EVET; 5 koşudan 3'ü <20 s, 2'si 20,1–20,4 s.** VM dalgalanması nedeniyle her koşuda garanti edemiyorum.
Kalan: model hazırlama 6–9,5 s (VM'ye duyarlı), val'de ikinci logit gemm'i (≈1,5 s), 8-α rank geçişleri. `V75_STREAM` varsayılan KAPALI (logit karması akışta üretilemediği için bit-düzeyi doğrulama CE/PPL ile sınırlı). 50k–100k: akış tamponu 2048×V (100k'da 0,8 GB) — V ile doğrusal; yoğun n×V tahsisi geri gelmiyor.
