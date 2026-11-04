# TEK HAMLE V54 — bağlantılı hesaplama ve CPU verimliliği

Dal: `claude/formul-avcisi` · Başlangıç: `897727b` (V53 hızlandırma) · Kaynak: `cpp/tek_hamle_v54.cpp` + `cpp/tanhf_vec.h` (V53h kopyası; **`tek_hamle_v53.cpp`, `tek_hamle_v53h.cpp`, `tek_hamle_v51b.cpp` değişmedi**) · Deney klasörü: `v54_baglantili_hesaplama/` (betikler + ham çıktılar `ham/`)

Matematik, veri, hiperparametre ve V53 ağırlıkları değişmedi; V54 yalnızca aynı hesabın daha verimli yürütülmesidir. Etiketler: **[Kesin]** ölçüldü/doğrulandı, **[Muhtemel]**, **[Tahmin]**.

## 0. Sonuç

| Ölçüm | V53 tam (897727b) | **V54 tam** | V53 hafif | **V54 hafif** |
|---|---|---|---|---|
| Model hazır (medyan, 5 tur ara ara) | 5.54 s (5.45–5.90) | **3.63 s** (3.50–3.85) | 4.49 s (4.35–4.55) | **2.58 s** (2.35–2.71) |
| Tepe RAM | 2,124 MB | **1,550 MB** (düşük-RAM modu: 1,250 MB) | 1,347 MB | **775 MB** |
| CPU süresi (user+sys) | 21.5 s | 14.8 s | 17.5 s | 10.5 s |
| Sayfa hatası (≈ dokunulan bellek) | 744 k (3.05 GB) | 602 k (2.47 GB) | 500 k | 344 k |
| WC / WF / val logit hash'i | referans | **bit-özdeş** | referans | **bit-özdeş** |
| Validation CE | 4.609033 | 4.609033 | 4.612900 | 4.612900 |
| Test PPL (yalnız nihai doğrulama) | 98.092 | **98.092** | 98.212 | **98.212** |

* **Hafif < 3 s hedefi ✓ (2.58 s).** **Tam < 3 s hedefi ✗:** 3.63 s medyan (en iyi turlar 3.3–3.5 s). 1 saniye "ileri hedefi" bu algoritma + bu makinede **fiziksel olarak ulaşılamaz** (bölüm 6): yalnızca yoğun matris çarpımları ≈ 390 GF = 0.7 s tepe, geri kalan işlerin tabanı ≈ 1.5–2 s.
* Ölçümler aynı VM, aynı veri, aynı CPU ayarlarıyla, 1 ısınma + 5 tur ara ara (V53, V54, V53, …) alındı; tam model hazır ile saf ağırlık üretimi ayrı raporlanır (bölüm 7).

## 1. Bağımlılık grafiği (V53 kaynak kodundan çıkarıldı)

Tam konfigürasyon (n = 113,046 satır, F = n×513 float = 232 MB, V = 2001, mc = 39,946 C-slotu). "‖" = bağımsız, "→" = zorunlu bağımlılık.

| # | İşlem | Girdi → Çıktı | Kim kullanıyor | Yeniden hesaplanıyor mu? | Bağımsızlık |
|---|---|---|---|---|---|
| 1 | Veri + `expand_s` | X (n×192) | 2,5 | — | ‖ 4 ile bağımsız değil (X gerekir) |
| 2 | Yön üretimi `make_w1x` | X,y → W (192×512), b | 3 | — | seri (512 yön, 8 ms) |
| 3 | F1 = tanh(γ(XW+b)) | X,W → F (232 MB) | 6,… | hayır | **‖ 4/5** |
| 4 | 6 K-tablosu (5 OOF + tam): sayım Cf → graf (norm, `sgemm` S=NnNnᵀ, top-10) → Q_f (V×V) | y,ctx → Q_f, Pool_f | 5, 7 (OOF havuz), 15 (tam graf) | **V39 grafı = tablo 5'in grafı → yeniden hesaplanıyordu** | tablolar birbirinden **‖** |
| 5 | s(K)=Q_f·E → U satırı (standardize, 64→512), tanh(gU) | Q_f → U (anahtar: kat×kelime) | 6 | **U her satır için ayrı (n×512) üretiliyordu; yalnız 12 k anahtara bağlı** | 3 ile ‖ |
| 6 | Kapı: F ← tanh(atanh(F)+gU) | 3,5 → F | 7 | hayır | satır bağımsız |
| 7 | V39: kelime ortalaması (**global indirgeme**) → karışım F←(1−β)F+βΣaMean | 6,4(Pool) → F | 8 | hayır | ortalama→karışım arası **bariyer** |
| 8 | V49: sütun μ,σ (**global**) → A,(A−μ)²,(A−μ)³ standardize | 7 → Fc | 9,10 | hayır | μ→dönüşüm **bariyeri** |
| 9 | Gram: AFF=FᵀF, RF=FᵀY (iş parçacığı-yerel kısmi toplamlar) | Fc,y | 13 | hayır | 10 ile ‖ |
| 10 | Slot sayımı: A_CC blokları, R_C (kompakt), A_CF | Fc,ctx,y | 11 | **yoğun RC/ACC (640 MB) açılıyordu** | grup bağımsız |
| 11 | Grup çözümü: Cholesky(A_g+Λ), Y0_g=A_g⁻¹R_g, X_g=A_g⁻¹A_CF,g | 10 → Y0,X | 12,14 | Y0'ın %93.6'sı sıfır sütun çözülüyordu (V53'te giderildi) | **2001 grup ‖** |
| 12 | T=A_CFᵀX, U=A_CFᵀY0 (dgemm) | 11 | 13 | — | T ‖ U |
| 13 | Schur: (AFF−T+λI)W_F=RF−U | 9,12 → W_F | 14 | — | **seri zincir** (12→13→14) |
| 14 | W_C=Y0−X·W_F (sgemm) | 11,13 | model | hayır | satır bloğu bağımsız |
| 15 | Q_havuz tablosu (geç karışım) | 10(sayımlar),4(graf) | değerlendirme | — | **11–14 ile ‖** |

Bağımlı zincirler (üretici→tüketici, birleştirilebilir): 3→6; 5→6; 11→(12,14). Bariyerli olanlar (birleştirilemez): 7 (ortalama→karışım), 8 (istatistik→dönüşüm), 12→13→14.

## 2. Birleştirilen / kaldırılan işlemler ve ara bellek hareketleri [Tahmin: bayt sayıları analitik, sayaç değil]

| Değişiklik | Kaldırılan RAM yazma/okuma | Etki |
|---|---|---|
| **U-tablosu** (anahtar başına bir kez: gemm 64→512, standardize, tanh) — `g_Utr` (n×512, 232 MB) hiç oluşmuyor | proj gemm yazma + ortalama/σ 2 geçiş + ustd r/w + kapı okuma ≈ 6 geçiş ≈ **1.4 GB** trafik, −232 MB RAM | proj 0.13→0.01 s, ustd 0.05→0.01 s |
| **Kapı özdeşliği** tanh(atanh f+gU)=(f+t)/(1+ft), t tablodan | 59 M `atanh`+59 M `tanh` (libm) → 6.4 M `tanh` + 59 M temel işlem | kapı özdeşliği+U-tablosu birlikte **−0.99 s**, −3.7 CPU-s (ablasyon; U-tablosu payı −0.34 s) |
| **Vektör `tanhf`** (libm ile 2³² girdide bit-özdeş) | 59 M libm çağrısı (21 ns) → AVX-512 (2.4 ns) | −0.30 s, −1.1 CPU-s |
| V49 `apply` yerinde (A/A²/A³ için d bir kez, satır tamponu) | n×513 `out` tahsisi + swap (232 MB, r+w) | V49 1.07→0.22 s |
| `cat` → `move` (tek aşama) | 232 MB kopya (r+w) | −0.13 s |
| V39 grafı = K-tablo 5 grafı (yeniden kullanım) | bir tam `sgemm`+sıralama | gürültü altı (−0.06) |
| top-k seçimi (partial_sort yerine tek geçiş, aynı tam sıra) | 2001×2001 pair vektörü ×6 | gürültü altı (−0.06) |
| Bellek serbest bırakma: `Fc`, `g_Qf` (192 MB), `g_Pf51`, `tr/va/te.X`, `g_U*` | — | **tepe RAM −27 %** (2124→1550 MB), sayfa hatası −19 % |
| 4 K-tablo döngüsü `omp parallel`, `Cn` kopyası yok, `Nn/S` ilklenmemiş tampon, V49 istatistikleri sütun-bloklarıyla | seri sıfır-doldurma + kopyalar | OOF/Q 2.27→1.02 s (kapı/U-tablo/graf birlikte) |
| Lean yol: Teach/EM/tcat atlanır (rapor amaçlı) | — | −0.03 s |

Birleştirilemeyenler ve nedenleri: tanh→kapı→V39 ortalaması arasında tek geçiş mümkün değil çünkü kapı K-tablolarını, ortalama tüm satırları bekler; V49 μ/σ global; Schur zinciri seri. `F`'nin aynı geçişte hem `FᵀY` hem slot toplamına gitmesi (üretici-tüketici) bit-özdeşliği bozar: `rhs_add` iş-parçacığı sayısına bağlı kısmi toplamlar kullanır (bkz. bölüm 5).

## 3. Optimizasyon aşamaları (A→E) ve geri alınanlar

| Aşama | Yapılan | Sonuç | Karar |
|---|---|---|---|
| A tekrar hesap | V39 grafı yeniden kullanımı; U'nun satır başına değil anahtar başına hesabı; kapıda t=tanh(gU) tablosu | −0.99 s (kapı özdeşliği ≈−0.65 + U-tablosu ≈−0.34) | **kabul** |
| B bellek akışı | yerinde V49; `move`; serbest bırakma; ilklenmemiş tamponlar; `F` sayfalarının paralel önceden dokunulması | RAM −27 %, sayfa hatası −19 % | **kabul** |
| C bağımlı işlemler | tanhf+kapı vektörleştirildi (üretici-tüketici ayrı geçiş; bariyerler korunur) | −0.3 s (tanhf) | **kabul** |
| D çekirdek dengesi | 1–16 iş parçacığı, OpenBLAS/OpenMP iç içelik, bağlama, bekleme politikası ölçüldü | 4 iş parçacığı seçildi (bölüm 5) | **kabul (4)** |
| E kalan darboğaz | K-tablosu yoğun kısmı (sgemm+kapı); seyrek-tam-eşdeğer U | aşağıda | karma |

Geri alınan / kabul edilmeyen denemeler (hepsi ölçüldü):
* `GLIBC_TUNABLES=glibc.malloc.hugetlb=1` (THP): sayfa hatası ↓13× ama **wall +%12** (2 MB sayfa sıfırlama) → red.
* `mallopt(M_MMAP_THRESHOLD/TRIM)` (serbest bırakılan tampon yeniden kullanımı): hata ↓22 % ama **RSS +600 MB**, wall aynı → red.
* `operator new` ile ≥4 MB tahsislerin paralel ön-dokunuşu: sys −0.2 CPU-s ama user +0.9 → net 0 → red (yalnız `F` için ön-dokunuş tutuldu, −0.05 s).
* PGO (`-fprofile-use`): fark gürültü içinde → red.
* **dgemm K-blok emülasyonuyla seyrek-tam-eşdeğer U** (`V53H_U=2`): OpenBLAS SkylakeX `dgemm`'in toplama düzeni çözüldü (K blok Q=384, kalan <2Q ise yarısı; blok içi k artan **tek FMA birikimi**, blok sonu C+=acc; çalışma zamanında üretim şekliyle doğrulanır). U'yu 5.3 GF ile hesaplar, 640 MB `Y0d` oluşturmaz (RAM −300 MB). **Fakat** çok iş parçacıklı `dgemm` N bölümlerinin son 4–5 sütununda ("kuyruk" çekirdeği) farklı toplama yapar → 17 sütun/4,137 eleman 8.7e-14 bağıl farklı. Nihai WC/WF/logit yine bit-özdeş çıktı, ama U çift duyarlıkta özdeş olmadığı ve ilk-dokunuş maliyeti düşünce yoğun yol aynı hızda (3.46 vs 3.65 s) olduğundan **varsayılan yoğun `dgemm` (tam bit-özdeş U)**; `V53H_U=2` düşük-RAM seçeneği olarak duruyor.
* Sayma/Q için CSR seyrek `Cn` (`V54_SPARSEQ`): sonuç özdeş ama fark gürültü altı → varsayılan kapalı.
* Aday: sgemm'in seyrek `Nn` ile hesabı ve `ssyrk` (F ᵀF yarım hesap): kenar-çekirdek farkı (yukarıdaki dgemm kuyruğu gibi) bit-özdeşliği riske atacağından **denenmedi**.

## 4. Doğruluk [Kesin]

Referans: değişmemiş `tek_hamle_v53` (V33_DUMP ile WC/WF). Üç yol karşılaştırıldı: V53 orijinal aritmetik yol, **V54 sıkı mod** (tüm V54 seçenekleri kapalı, yoğun U) ve **V54 varsayılan**.

| | Tam | Hafif |
|---|---|---|
| WC, WF `cmp` (orijinal v53 ikilisi) — orijinal yol / V54 sıkı / V54 varsayılan | **bit-özdeş ×3** | **bit-özdeş ×3** |
| F (özellik matrisi), AFF, RF, A_CC blokları, A_CF, R_C (yoğun karşılığı), Y0 (float), val logitleri — V54 varsayılan vs V53 | **0 farklı eleman** | **0 farklı eleman** |
| Çift duyarlık ara diziler X, T, U | farklı (X: maks mutlak 6.7e-16; T: bağıl 1.5e-12; U: bağıl 4.2e-11; ≤1 ulp mertebesi) | benzer |
| Validation CE / Top-1 / Top-5 | 4.609033 / 16.63 % / 37.99 % | 4.612900 / 16.55 % / 37.67 % |
| Test PPL / CE / Top-1 / Top-5 | **98.092** / 4.585905 / 18.21 % / 39.68 % | **98.212** / 4.587128 / 18.17 % / 39.64 % |

X/T/U farkının kaynağı (V53 raporundaki gibi): orijinal skaler döngülerde derleyicinin açma/FMA birleştirmesi bazı terimleri tek, bazılarını iki yuvarlamayla toplar; bu desen yeniden üretilemez, toplama *sırası* aynıdır, float'a yuvarlanınca fark kaybolur.

**V54'te eklenen özel doğrulamalar**
* **Vektör `tanhf` tüm 2³² float girdide (NaN/inf hariç) glibc 2.39 `tanhf` ile bit-özdeş** (0 fark; `tanhf_test.cpp`, 14 s). glibc `tanhf` fdlibm'dir (CR değil: `(float)tanh((double)x)` ile %13.7 farklı) — bu yüzden port, fdlibm `expm1f`/`tanhf` algoritmasının işlem sırasıyla birebir alındı, **FMA birleştirmesi kapalı** (libm'in `mulss/addss` talimatlarında FMA yok, `objdump` ile görüldü; port fonksiyonlarında `optimize("fp-contract=off")`). Çalışma zamanında 262 k girdilik oz-denetim (`tv_selftest`) başarısız olursa libm'e dönülür.
* **Kapı özdeşliği** matematiksel olarak tam (tanh toplama formülü); yalnız çift-duyarlık yuvarlama farkı var (≈1e-16 → float'a yuvarlayınca ≈3·10⁻⁹/eleman flip olasılığı, 59 M elemanda beklenen ≈0.2). **Bu veride F dizisi 0/58 M elemanda farklı** (`Fc` hash'i özdeş). Bu nedenle varsayılan açık; **genel garanti değildir**: sıkı mod `V54_GATE=0` (libm yolu) mevcut ve WC/WF yine bit-özdeş doğrulandı. U satırlarının aynı anahtarda bit-özdeş olduğu `V54_CHK=1` ile denetlenebilir (denetimde sorun çıkmadı).
* Top-k seçimi tam sıralı (değer, indeks) olduğundan aynı sonucu verir; graf yeniden kullanımı aynı işlevin aynı girdisidir.

Sınır: V53/V54 sonuçları iş-parçacığı sayısına bağlıdır (`rhs_add` kısmi toplamları, OpenBLAS bölümleri); referans 4 iş parçacığıyla üretildi, hash eşitliği 4 iş parçacığı için geçerlidir.

## 5. İş parçacığı çalışması (D aşaması)

Makine: 4 vCPU (Intel Xeon @2.1 GHz, AVX-512; lscpu: 4 çekirdek × 1 iş parçacığı). Ölçülen tepe: **sgemm 149 GF/s (1 çekirdek) → 276 (2) → 282 (3) → 546 (4)**, dgemm ≈232 GF/s (4). Yani 4 vCPU gerçek çekirdek gibi davranıyor. Donanım sayaçları (perf) yok: bellek trafiği ve cache isabeti ölçülemedi — **[Tahmin]**; ölçülenler: duvar süresi, CPU süresi, bağlam değişimleri, sayfa hatası.

**Tam** (model hazır medyan, 3 tur):
```
varyant      modelhazir    duvar    user     sys CPU/duvar   VCSW   ICSW   MINFLT   RSS_MB
T1               7.73     8.44    7.29    1.13      1.00      1     20   553381     1414 x1.00
T2               4.56     5.07    9.22    1.25      2.07     35     35   591212     1429 x1.69
T3               3.86     4.37   11.81    1.23      2.98     94    408   593101     1503 x2.00
T4               3.41     3.89   12.62    1.18      3.55    107   1019   604581     1548 x2.27
T8               3.58     4.07   12.31    1.42      3.37   2523   1508   623476     1548 x2.16
T16              3.57     4.09   12.35    1.42      3.37   5401   1777   642199     1562 x2.17
active           6.15     6.67   25.03    1.20      3.93     23   4067   597913     1549
passive          3.24     3.75   11.67    1.29      3.46   1086    709   609054     1548
omp2_ob4         4.14     4.65   12.58    1.28      2.98     53    467   596015     1472
omp4_ob1         5.45     5.91    8.55    1.29      1.66     84    116   573031     1490
omp4_ob2         4.26     4.78   11.28    1.29      2.63    133    310   596901     1504
bind_close       5.51     6.04    8.66    1.25      1.64     87     84   572222     1487
bind_spread      5.43     5.91    8.67    1.22      1.67     88     90   573360     1491
```
**Hafif**:
```
varyant      modelhazir    duvar    user     sys CPU/duvar   VCSW   ICSW   MINFLT   RSS_MB
T1               5.01     5.56    4.91    0.62      1.00      1      9   310032      703 x1.00
T2               2.99     3.30    6.50    0.68      2.17     35     29   339344      726 x1.68
T3               2.86     3.19    9.26    0.73      3.13     94    331   333369      744 x1.75
T4               2.65     2.99   10.11    0.71      3.62    106    955   339983      774 x1.89
T8               2.55     2.87    8.94    0.81      3.40   1769   1085   364640      777 x1.96
T16              2.43     2.79    8.70    0.91      3.44   3786   1290   382473      783 x2.06
active           4.54     4.90   18.58    0.74      3.94     21   3040   336122      775
passive          2.62     2.91    9.46    0.79      3.52    765    654   349992      775
omp2_ob4         2.91     3.32    9.84    0.67      3.17     56    375   338925      769
omp4_ob1         3.61     3.89    6.09    0.67      1.74     99    105   313987      717
omp4_ob2         2.94     3.27    8.03    0.72      2.68    129    324   339648      731
bind_close       3.50     3.81    5.82    0.68      1.71     90     79   313945      717
bind_spread      3.61     3.93    6.08    0.65      1.71     87     74   313856      717
```
* **Hızlanma:** 1→4 iş parçacığında ×2.27 (tam) / ×1.89 (hafif) — verimlilik %57/%47. CPU süresi 8.4→13.8 s: **≈5.4 CPU-s (%39) bekleme/spin/çekişme** [Tahmin: ayrı sayaç yok; iş parçacığı bekleme süresi = CPU(4) − CPU(1)]. Aşama içi çekirdek kullanımı: sgemm/dgemm fazları 3.9–4.0; V39/V49/ön-hazırlık 2.6–3.3 (seri kısımlar).
* `OMP_WAIT_POLICY=active` 2× yavaş (spin); `passive` −3…5 % (gürültüye yakın); **OpenBLAS iş parçacığı sayısını düşürmek (ob1/ob2) yavaşlatır** (ob1: +%60); OpenMP'yi düşürmek (omp2_ob4) +%20; **bağlama (`OMP_PROC_BIND`) OpenBLAS iş parçacıklarını tek çekirdeğe yığıyor → 1.6× yavaş**.
* 16 iş parçacığı (4 çekirdek aşırı abone): 5 turluk ara ara testte tam 3.36 vs 3.71 s, hafif 2.36 vs 2.56 s, CPU süresi de düşük (12.9 vs 14.8 s) — yani daha *verimli* görünüyor (dinamik zamanlamada daha ince iş parçalama). **Benimsenmedi:** (1) fark tur-arası gürültüye yakın (±0.2 s), (2) referans sonucu `rhs_add` kısmi toplamları ve OpenBLAS bölümleri nedeniyle iş-parçacığı sayısına bağlı; 16'ya geçmek ağırlıkları bit düzeyinde değiştirebilir. Gelecek: `rhs_add`'i sabit 4 mantıksal bölüme bağlayıp OpenMP'yi 16 yapmak bit-özdeşliği koruyarak %5–9 verebilir (denenmedi).
* **Seçim: 4 iş parçacığı** (OMP=OpenBLAS=4, bağlama yok, varsayılan bekleme).

## 6. Hesaplama bütçesi (teorik FLOP = boyutlardan hesaplanan TAHMİN, sayaç değil)

### full (mc=39946; 5 kosu medyan)

| Faz | Is | TEORIK FLOP (GF) | Sure (s) | CPU-s | Cekirdek kullanimi | Ulasilan GF/s | Tepe ile orani |
|---|---|---|---|---|---|---|---|
| onhazir+okuma+F1 | F1 gemm (n+va+te) x192x512 + tanh ~59M | 24.18 | 0.510 | 1.75 | 3.42 | 47.4 | %9 (f32 tepe 546) |
| OOF/Q: K-tablolari+kapi | 6 x sgemm 2001^3 + proj gemm + Sf gemm + kapi/top-k/qtab (FLOP yalniz gemm) | 107.28 | 1.018 | 4.01 | 3.94 | 105.4 | %19 (f32 tepe 546) |
| V39 havuz | ortalama+karisim (bellek bagimli) ~4 flop/eleman | 0.25 | 0.074 | 0.22 | 2.99 | 3.4 | (bellek/ozel fonksiyon) |
| V49 donusumu | istatistik+turetilmis sutunlar ~12 flop/eleman | 0.76 | 0.216 | 0.59 | 2.72 | 3.5 | (bellek/ozel fonksiyon) |
| Gram: F^T F | n x 513^2 sgemm | 59.50 | 0.165 | 0.64 | 3.90 | 360.6 | %66 (f32 tepe 546) |
| Gram: F^T Y | n x 513 skaler ekleme | 0.06 | 0.100 | 0.38 | 3.76 | 0.6 | (bellek/ozel fonksiyon) |
| Gram: slot sayimi (ACC,ACF,RC) | n x 2 slot x 513 ekleme | 0.12 | 0.116 | 0.38 | 3.23 | 1.0 | (bellek/ozel fonksiyon) |
| grup Cholesky + Y0 + X | Cholesky sum sg^3/3 + Y0 (yoluk) + X (513 sutun) | 14.03 | 0.439 | 1.69 | 3.84 | 32.0 | %14 (f64 tepe 232) |
| T=ACF'X + U=ACF'Y0 | T dgemm 513x513xmc + U YOGUN dgemm 513 x 2001 x mc (varsayilan, bit-ozdes); matematiksel asgari U = nz x 513 | 103.04 | 0.738 | 2.90 | 3.93 | 139.6 | %60 (f64 tepe 232) |
| Schur: W_F | Cholesky 513 + 513^2 x 2001 cozum | 1.14 | 0.023 | 0.09 | 4.04 | 49.7 | %21 (f64 tepe 232) |
| W_C = Y0 - X W_F | sgemm mc x 513 x 2001 | 82.01 | 0.300 | 1.18 | 3.94 | 273.4 | %50 (f32 tepe 546) |
| Q_havuz tablosu | ~10 komsu x V x V | 0.08 | 0.027 | 0.11 | 4.11 | 3.0 | (bellek/ozel fonksiyon) |
| **Toplam** | | **392.4** | **3.73** | | | **105.3** | sgemm tepe 546 GF/s |

V53-oncesi yogun Y0 cozumu ek is yuku: +23.1 GF (Y0 yogun/seyrek: 5.9x). U icin yurutulen yogun dgemm 82.0 GF; matematiksel asgari (yalniz sifir-olmayan) 5.3 GF (15.6x). Yurutulen toplam 392.4 GF, asgari toplam 315.7 GF.

### hafif (mc=15165; 5 kosu medyan)

| Faz | Is | TEORIK FLOP (GF) | Sure (s) | CPU-s | Cekirdek kullanimi | Ulasilan GF/s | Tepe ile orani |
|---|---|---|---|---|---|---|---|
| onhazir+okuma+F1 | F1 gemm (n+va+te) x192x512 + tanh ~59M | 24.18 | 0.452 | 1.53 | 3.38 | 53.5 | %10 (f32 tepe 546) |
| OOF/Q: K-tablolari+kapi | 6 x sgemm 2001^3 + proj gemm + Sf gemm + kapi/top-k/qtab (FLOP yalniz gemm) | 107.28 | 0.981 | 3.88 | 3.95 | 109.4 | %20 (f32 tepe 546) |
| V39 havuz | ortalama+karisim (bellek bagimli) ~4 flop/eleman | 0.25 | 0.083 | 0.22 | 2.60 | 3.0 | (bellek/ozel fonksiyon) |
| V49 donusumu | istatistik+turetilmis sutunlar ~12 flop/eleman | 0.76 | 0.213 | 0.56 | 2.62 | 3.5 | (bellek/ozel fonksiyon) |
| Gram: F^T F | n x 513^2 sgemm | 59.50 | 0.184 | 0.72 | 3.93 | 323.4 | %59 (f32 tepe 546) |
| Gram: F^T Y | n x 513 skaler ekleme | 0.06 | 0.089 | 0.33 | 3.75 | 0.7 | (bellek/ozel fonksiyon) |
| Gram: slot sayimi (ACC,ACF,RC) | n x 2 slot x 513 ekleme | 0.12 | 0.068 | 0.26 | 3.87 | 1.7 | (bellek/ozel fonksiyon) |
| grup Cholesky + Y0 + X | Cholesky sum sg^3/3 + Y0 (yoluk) + X (513 sutun) | 3.07 | 0.115 | 0.42 | 3.67 | 26.7 | %12 (f64 tepe 232) |
| T=ACF'X + U=ACF'Y0 | T dgemm 513x513xmc + U YOGUN dgemm 513 x 2001 x mc (varsayilan, bit-ozdes); matematiksel asgari U = nz x 513 | 39.12 | 0.287 | 1.13 | 3.93 | 136.3 | %59 (f64 tepe 232) |
| Schur: W_F | Cholesky 513 + 513^2 x 2001 cozum | 1.14 | 0.020 | 0.08 | 4.15 | 57.2 | %25 (f64 tepe 232) |
| W_C = Y0 - X W_F | sgemm mc x 513 x 2001 | 31.13 | 0.076 | 0.30 | 3.97 | 409.7 | %75 (f32 tepe 546) |
| Q_havuz tablosu | ~10 komsu x V x V | 0.08 | 0.037 | 0.14 | 3.89 | 2.2 | (bellek/ozel fonksiyon) |
| **Toplam** | | **266.7** | **2.61** | | | **102.4** | sgemm tepe 546 GF/s |

V53-oncesi yogun Y0 cozumu ek is yuku: +4.7 GF (Y0 yogun/seyrek: 4.8x). U icin yurutulen yogun dgemm 31.1 GF; matematiksel asgari (yalniz sifir-olmayan) 2.4 GF (13.0x). Yurutulen toplam 266.7 GF, asgari toplam 238.0 GF.

* FLOP/s yalnız yoğun matris fazlarında anlamlıdır. **Yoğun fazlar tepeye yakın:** F ᵀF %66, W_C %50–75, U+T dgemm %59 (f64 tepe 232 GF/s). Toplam yürütülen iş ≈ **392 GF (tam) / 267 GF (hafif)**; sgemm tepesinde (546 GF/s) bunlar ≈ **0.7 s / 0.5 s**. Gerçek süre 3.6 / 2.6 s → **FLOP kapasitesinin ≈ %19–20'si** kullanılıyor; geri kalan zaman bellek-sınırlı geçişler (V39/V49/sayım/F ᵀY), libm `tanh` tablosu (6.4 M), seri başlangıç ve bekleme.
* **1 saniye neden gerçekçi değil [Kesin/Tahmin]:** matematiksel asgari iş (U'nun yalnız sıfır-olmayan terimleri dahil) 316 GF → tepe hızda 0.6 s; ama bit-özdeşlik `dgemm`/`sgemm` çağrılarını (82+82+96+60 GF) olduğu gibi yapmayı gerektirir; bellek-sınırlı ve libm-sınırlı aşamalar (K-tabloları kapı dahil 1.0 s, F1/V39/V49 0.7 s, sayımlar 0.4 s, grup çözümü 0.45 s) birlikte bu makinede ≈ 2.5 s civarında bir taban oluşturuyor [Tahmin: sayaçsız]. 1 s'ye yaklaşmak ancak bit-özdeşliğe veya algoritmaya dokunmakla (ör. sgemm yerine seyrek grafik, ssyrk, TF32/bf16) mümkün — kurallar gereği yapılmadı.

## 7. A/B zamanlar (V53 = 897727b, V54 varsayılan; aynı ölçüm kapsamı)


### full (n=5 tur medyan; model hazir min-maks: V53 5.45-5.90, V54 3.50-3.85)

| Olcum | V53 (897727b) | V54 | Degisim |
|---|---|---|---|
| TAM MODEL HAZIR (s) | 5.544 | 3.633 | -34% |
| Program basi -> zincir (okuma+on hazirlik) | 0.182 | 0.152 | -16% |
| OOF/Q (K-tablolari + kapi) | 2.265 | 1.018 | -55% |
| Ozellik uretimi (F1, V39, V49) | 1.220 | 0.624 | -49% |
| Gram/RHS (F^TF, F^TY, slot sayimi) | 0.374 | 0.377 | +1% |
| Grup Cholesky + Y0 + X | 0.423 | 0.439 | +4% |
| T = ACF^T X + U = ACF^T Y0 | 0.768 | 0.738 | -4% |
| Saf agirlik uretimi (W_F + W_C) | 0.313 | 0.324 | +4% |
| Q_havuz tablosu | 0.051 | 0.027 | -47% |
| Validation degerlendirmesi (ayri) | 0.218 | 0.218 | -0% |
| Tum surec duvar suresi | 6.041 | 4.188 | -31% |
| CPU suresi (user+sys, s) | 21.532 | 14.814 | -31% |
|   sys (sayfa hatasi vb.) | 1.723 | 1.368 | -21% |
| Tepe RAM (MB) | 2124 | 1550 | -27% |

### hafif (n=5 tur medyan; model hazir min-maks: V53 4.35-4.55, V54 2.35-2.71)

| Olcum | V53 (897727b) | V54 | Degisim |
|---|---|---|---|
| TAM MODEL HAZIR (s) | 4.494 | 2.581 | -43% |
| Program basi -> zincir (okuma+on hazirlik) | 0.170 | 0.144 | -15% |
| OOF/Q (K-tablolari + kapi) | 2.229 | 0.981 | -56% |
| Ozellik uretimi (F1, V39, V49) | 1.136 | 0.569 | -50% |
| Gram/RHS (F^TF, F^TY, slot sayimi) | 0.334 | 0.328 | -2% |
| Grup Cholesky + Y0 + X | 0.101 | 0.115 | +14% |
| T = ACF^T X + U = ACF^T Y0 | 0.280 | 0.287 | +2% |
| Saf agirlik uretimi (W_F + W_C) | 0.132 | 0.096 | -27% |
| Q_havuz tablosu | 0.050 | 0.037 | -26% |
| Validation degerlendirmesi (ayri) | 0.226 | 0.239 | +6% |
| Tum surec duvar suresi | 4.817 | 2.881 | -40% |
| CPU suresi (user+sys, s) | 17.519 | 10.494 | -40% |
|   sys (sayfa hatasi vb.) | 1.041 | 0.691 | -34% |
| Tepe RAM (MB) | 1347 | 775 | -42% |

"Saf ağırlık üretimi" = Schur W_F çözümü + W_C (tam: 0.32 s; hafif: 0.10 s); "tam model hazır" = veri okuma sonrası → Q_havuz tablosu (değerlendirme, V42 rezidü denetimi, hash/dump hariç).

## 8. Ablasyon (tam, 3 tur medyan; her satır tek özelliği kapatır)

```
varyant          modelhazir     fark    CPU-s   RSS_MB
tam                    3.60    +0.00     14.6     1545
U_yogun                3.67    +0.07     14.9     1551
kapi_kapali            4.59    +0.99     18.3     1569
reuse_kapali           3.66    +0.06     14.8     1546
siki                   5.24    +1.64     20.9     1568
sparseq_kapali         3.51    -0.09     14.3     1547
tanh_kapali            3.90    +0.30     15.7     1548
topk_kapali            3.66    +0.06     15.1     1547
utab_kapali            3.94    +0.34     15.7     1573
```
Yorum: kapı (özdeşlik+U-tablo) **+0.99 s**, U-tablo tek başına +0.34, vektör tanhf +0.30; top-k/graf yeniden kullanımı/`SPARSEQ` farkı ±0.09 s gürültü altı (ama maliyetsiz ve bit-özdeş olduğundan bırakıldı); `U_yogun` satırı varsayılanla aynı yoldur (fark = gürültü); `siki` = tüm V54 özellikleri kapalı ≈ V53 davranışı (5.24 s).

## 9. Kalan darboğazlar

Tam (3.63 s): OOF/Q (K-tabloları 6×sgemm + kapı) **1.02 s (%28)** → T+U **0.74 s (%20)** → özellik üretimi 0.62 s → grup Cholesky/Y0/X 0.44 s → Gram 0.38 s → W_F+W_C 0.32 s. En büyük tek kalem K-tablolarının sgemm'i (0.24 s, tepe hızda) ve `U=A_CFᵀY0` yoğun dgemm'i (82 GF, 0.35 s). İkisi de bit-özdeşliği için olduğu gibi tutuluyor. Bir sonraki adımlar (kurallara uygun, denenmedi): iş-parçacığı-bağımsız `rhs_add` + 16 iş parçacığı (−%5–9), K-tabloları arası görev paralelliği (OpenBLAS iç içe riski).

## 10. Yeniden üretim

```
cd v54_baglantili_hesaplama
source env_full.sh   # veya env_hafif.sh  (ARGS ve ortam)
V53H_LEAN=1 V53H_FAST=1 ../cpp/tek_hamle_v54 $ARGS        # varsayilan V54
V54_GATE=0 V54_TOPK=0 V54_REUSE=0 V54_TANH=0 V53H_U=0 ...  # siki mod (V53 davranisi)
V53H_U=2 ...     # dusuk-RAM (seyrek-emulasyon U)
./dogrula54.sh   # cmp + fark.py     ./ab54.sh; python3 ab_ozet.py   # A/B     ./olcek2.sh   # is parcacigi
./abl54.sh; python3 abl_ozet.py ham/abl54   # ablasyon      python3 flop_butce.py full "ham/ab54/full_v54_*.txt"
g++ -O3 -march=native -fopenmp -std=c++17 -o tek_hamle_v54 tek_hamle_v54.cpp -L<scipy.libs> -l:libscipy_openblas-*.so -Wl,-rpath,<scipy.libs>
tanhf dogrulama: g++ -O3 -march=native -fopenmp tanhf_test.cpp && ./a.out   (2^32 girdi, ~15 s)
```

## 11. Sorular

1. **Hangi işlemler birbirine bağlıydı?** Bölüm 1: 3→6←5 (F1 + K-tabloları → kapı), 6→7(bariyer)→8(bariyer)→9/10, 10→11→12→13→14 (seri Schur zinciri); bağımsız: K-tablolarının 6'sı, 2001 grup çözümü, T‖U, Q_havuz ‖ 11–14, F1 ‖ K-tabloları.
2. **Hangileri birleştirildi?** tanh→kapı arasında vektör/özdeşlik (kapı artık atanh'sız), K-tablo→U→kapı (anahtar tablosu), V49 üçlüsü (A/A²/A³) tek d ile yerinde, grup çözümü→Xf float dönüşümü→WC (aynı grup geçişinde), W_C = sgemm çıktısı üzerine Y0 ekleme.
3. **Hangi ara RAM yazma/okuma kalktı?** `g_Utr` (n×512; ≈6 geçiş, 1.4 GB), V49 `out` (232 MB r+w), `cat` kopyası (232 MB r+w), 6 tablo için `Cn`/`Nn`/`S` kopya-sıfırlamaları, dört büyük dizinin erken serbest bırakılması (Fc, g_Qf, tr.X…, tepe RAM −574 MB). Toplam dokunulan bellek 3.05→2.47 GB.
4. **Toplam hesaplama işi ne kadar azaldı?** CPU süresi −31 % (tam: 21.5→14.8 s), −40 % (hafif); transandantal: 59 M libm `tanhf` + 118 M libm `atanh/tanh` → vektör tanhf + 6.4 M libm `tanh`; FLOP işi ≈ aynı (392 GF; yoğun dgemm/sgemm bit-özdeşlik için korundu).
5. **CPU kapasitesinden yararlanma?** FLOP kapasitesinin ≈ %19–20'si (392 GF / 3.63 s ≈ 105 GF/s, sgemm tepesi 546); yoğun fazlar %50–75; çekirdek meşguliyeti ≈ 3.5/4 ama %39'u bekleme/çekişme [Tahmin].
6. **En hızlı iş parçacığı sayısı?** Ölçülü ve bit-özdeşliği koruyan: **4** (16 iş parçacığı %5–9 hızlı görünüyor ama gürültü içinde ve ağırlık hash'ini riske atar).
7. **V53 tam?** 5.54 s → **3.63 s** (−34 %); en iyi tur 3.2–3.4 s.
8. **V53 hafif?** 4.49 s → **2.58 s** (−43 %).
9. **RAM?** Tam 2,124 → **1,550 MB** (−27 %; `V53H_U=2` ile 1,250 MB); hafif 1,347 → **775 MB** (−42 %).
10. **Ağırlık hash'leri?** WC, WF, val logitleri **bit-özdeş** (orijinal v53 ikilisi ile `cmp`, hem tam hem hafif; sıkı ve varsayılan mod).
11. **CE/PPL değişti mi?** Hayır: val CE 4.609033 / 4.612900, test PPL 98.092 / 98.212.
12. **Kalan en büyük darboğaz?** K-tabloları + kapı (1.02 s, %28), sonra yoğun U dgemm'i + T (0.74 s).
13. **Commit/push?** Bkz. son mesaj.
