# TEK HAMLE V53 — CPU hızlandırma: Y0 seyrek hesaplama

Dal: `claude/formul-avcisi` · Başlangıç commit: `6b57e99` · Kaynak: `cpp/tek_hamle_v53h.cpp` (V53'ün kopyası; **`cpp/tek_hamle_v53.cpp` ve `cpp/tek_hamle_v51b.cpp` değişmedi**) · Deney klasörü: `v53_cpu_hizlandirma/` (betikler + ham çıktılar `ham/`)

Bu bir öğrenme deneyi değil, hız işi: matematik, veri, hiperparametre, seçilen V53 iyileştirmeleri (A+C+D) aynen duruyor. Etiketler: **[Kesin]** ölçüldü/doğrulandı, **[Muhtemel]**, **[Tahmin]**.

## 0. Sonuç tablosu (gerçek ölçümler, medyan; ayrıntı bölüm 6)

| Ölçüm | V53 tam önce | V53 tam sonra | V53 hafif önce | V53 hafif sonra |
|---|---|---|---|---|
| **Model hazır** (orijinal ikili, log'dan; 3 tur) | 18.0 s (16.2–19.0) | **5.6 s** (5.1–6.3) | 8.2 s (7.9–8.7) | **4.4 s** (4.25–4.44) |
| Model hazır (aynı ölçüm düzeneğinde orijinal aritmetik; 5 tur) | 17.2 s | 5.6 s | 6.4 s | 4.4 s |
| Y0 çözümü (duvar payı) | 4.98 s | 0.09 s | 1.28 s | 0.03 s |
| Y0 ile ilgili toplam (çözüm + float dönüşüm + U + ekleme) | 6.83 s | 0.76 s | 1.51 s | 0.26 s |
| Gram/RHS | 0.84 s | 0.41 s | 0.57 s | 0.34 s |
| Schur/Cholesky (grup Chol. + Y0 + X + T + U) | 8.54 s | 1.19 s | 1.94 s | 0.36 s |
| Tepe RAM | 7,304 MB (orijinal tam program) / 4,229 MB (aynı düzenek) | **2,124 MB** | 3,578 MB / 2,393 MB | **1,346 MB** |
| Validation CE | 4.609033 | 4.609033 | 4.612900 | 4.612900 |
| Ağırlık eşitliği | Referans | **bit-özdeş (WC, WF, val logits)** | Referans | **bit-özdeş** |

Hedefler: tam < 10 s → **5.6 s** ✓; hafif < 5 s → **4.4 s** ✓ [Kesin]. Test (yalnız nihai doğrulamada): tam **PPL 98.092**, CE 4.585905, Top-1 %18.21, Top-5 %39.68 — orijinal ikili ile birebir aynı; hafif **PPL 98.212**, CE 4.587128, Top-1 %18.17, Top-5 %39.64 — orijinal ile aynı [Kesin].

**Ölçüm notu (dürüstlük):** önceki raporda "21.7–25.2 s" yazan tam-sürüm süresi, taze (soğuk) VM'de alındı; bu VM'de ilk koşular 1.5–2× yavaş (yeni sayfa hatası maliyeti). Aynı değişmemiş orijinal ikili bu oturumda ısındıktan sonra 16.2–19.0 s verdi; karşılaştırma için tüm "önce/sonra" ölçümleri ısınma turundan sonra, ara ara (önce→sonra→sonra…) alınan 5 turun medyanıdır. Soğuk durumda "sonra" da yavaşlıyor (ör. 7–12 s) — hız kazancı oranı korunuyor.

## 1. Y0'ın gerçek tanımı (kaynak koddan) ve 8 soru

**Tanım [Kesin]:** `Y0 = (A_CC + Λ_C)⁻¹ R_C` — `A_CC = CᵀC` (slot eşzamanlılık sayıları), `R_C = CᵀY` (slot × hedef-kelime sayıları), `Λ_C` = λ_kelime=85 / λ_çift=50 (köşegen). Kaynakta: V41 grup çözümü `spd_solve(Gm, sg, Rm, nr)` ile `Rm = [R_C satırları | A_CF satırları]` (nr = V+513 = 2514 sağ taraf), çıktı `Y0g` (mc×V çift duyarlık), sonra `Y0f` (float), `U = A_FCᵀ Y0` (dgemm) ve `W_C = Y0 − X·W_F`.

1. **Girdiler:** slot sayıları (113,046 eğitim satırı; her satır 2 slot: son kelime + (varsa) çift), etiketler y, λ. Q/OOF ve F bloğu Y0'a girmez (F, `ACF`/`X` üzerinden Schur'da birleşir).
2. **Boyutlar:** mc = 39,946 (tam; hafif 15,165) · `R_C`, `Y0`: mc × 2001 = 79.9 M eleman (640 MB double) · `A_CC`: mc×mc yoğun olsaydı 12.8 GB; gerçekte blok-köşegen (2001 blok, en büyük 882, Σsg² = 6.93 M = 55 MB) · `X`, `A_CF`: mc×513.
3. **Yoğun/seyrek [Kesin]:** `R_C`'nin yapısal sıfır-olmayan konumları = (son kelime grubu g, o kelimeyi izleyen hedef y) çiftleri = **5.13 M / 79.9 M = %6.42**. Kalan **%93.6'sı yapısal olarak sıfır**.
4. **Sıfırların yeniden işlendiği yerler (orijinal):** (a) grup başına 2001 yerine ~g'nin izleyicisi kadar sütun yeterken 2001 sütun çözülüyordu → Y0 çözüm işlemi (Σsg²·sütun) yoğun **1.39×10¹⁰ madd**, yoğun olmayan **2.35×10⁹ (5.9× az)**; (b) `RC` (640 MB) sıfırla doldurulup sayılıyor; (c) `Y0d`→`Y0f` float dönüşümü (80 M eleman); (d) `U = A_FCᵀY0` dgemm 513×39,946×2001 (82 GF) %93.6 sıfır; (e) `W_C = Y0f − tmp` 80 M eleman geçişi; (f) tüm bunlar için mc×V'lik 640/640/320/320 MB geçici diziler (sayfa-hatası + sıfırlama).
5. **Hesaplanmak zorunda olanlar:** her grup g için yalnız `succ(g)` sütunları (g'den sonra gözlenen y'ler). Diğer sütunlar tanım gereği **tam sıfır**.
6. **Katkı:** `Y0` iki yere girer: Schur sağ tarafı `RF − U` (U = A_FCᵀY0) ve `W_C = Y0 − X·W_F`. İkisi de yalnız Y0'ın sıfır-olmayan konumlarına duyarlı.
7. **Hiç kullanılmayanlar (model için):** orijinal program tek koşuda ek olarak V46/V50 araştırma yollarını çalıştırıyor: ρ-varyantları için **ikinci bir yoğun Y0q çözümü** (`RCq`, `Y0q`, `Uq`, mc×V çift duyarlık ×3), 6 sistemli V50 karşılaştırması (her biri yeniden `weights()` + eğitim/val/test logit), `fullEval` eğitim logitleri vb. Nihai model ρ=0 olduğundan bunlar model üretimine ait değil: orijinal tam program **65 s**, model hazır **18 s**.
8. **Maliyet [Kesin]:** model-hazırın içinde Y0-ilişkili işler **6.83 s / 17.2 s (%40)** (çözüm 4.98 + float dönüşüm/U/ekleme 1.85); RAM: `RC`+`Y0g`+`Y0f`+`tmp` ≈ 1.9 GB.

**Seyrek hesabın doğruluğunun kanıtı [Kesin]:** sütunlar birbirinden bağımsız çözülür (`(A_g+Λ)y = r`); sağ taraf sütunu tamamen sıfırsa ileri/geri yerleştirme tam sıfır döner. `R_C`'nin sıfır olmayan konumları gözlemden (grup, y) çiftleri olarak *kesin* bilinir; "küçük değer sıfır sayıldı" yok. Ampirik doğrulama: yoğun yoldaki `RC` (79.9 M eleman) ve `Y0f` (79.9 M eleman) seyrek yolun yoğun karşılığıyla **bit-bit aynı** (0 farklı eleman).

## 2. Uygulanan değişiklikler (hepsi aynı matematik; her biri ölçüldü)

| # | Değişiklik | Neden | Etki |
|---|---|---|---|
| 1 | **Y0'ı yalnız `succ(g)` sütunlarında hesapla/sakla** (RC, Y0 kompakt: 41 MB, yoğun 640 MB yerine) | yapısal sıfırlar | Y0 çözümü 5.9× az işlem; RC/Y0d/Y0f yoğun dizileri yok |
| 2 | **Grup çözücü**: orijinal `spd_solve` sütun-sütun, adım-adım (stride nr) skaler döngüydü. Yerine: aynı Cholesky (`h_chol`, orijinaliyle aynı kod) + **AVX-512 kayıtçı-bloklu** ileri/geri yerleştirme (32 sütun kayıtlarda; her sütunda toplama sırası i artan = orijinalle aynı); büyük gruplarda (≥128: 46 grup) Cholesky'ler paralel, çözüm sütun-karoları iş parçacıklarına | cache-düşmanı skaler döngü asıl darboğazdı (Y0+X çözümü 7–8.8 s) | grup çözümü 5.4–8.8 s → 0.3–0.9 s |
| 3 | Sayım: yoğun `RC`/`ACC` yok; satırlar gruba göre (kararlı, i artan) sıralanıp grup başına paralel sayılıyor; ACF toplama sırası orijinalle aynı | yoğun sayım + sıfır-doldurma + sayfa hatası | Gram/RHS 0.84 → 0.41 s |
| 4 | `U = A_FCᵀ Y0`: tam moddaki varsayılan **aynı dgemm** (bit-özdeş); Y0d geçici, paralel kuruluyor (ilklenmemiş tampon) | aynı çağrıyı koruyup yalnız yoğun matrisi ucuz kurmak | U: 3.2 s → 0.7–0.9 s. Seçenek `V53H_U=1`: seyrek U (0.3 s; bkz. 5) |
| 5 | `W_C`: `gemm(X_f, W_F)` doğrudan çıktıya + yalnız yoluk konumlarına `Y0` ekleme (ilklenmemiş WC tamponu, `Xf` grup çözümünde üretiliyor) | 320 MB'lık 3 ayrı geçici dizi ve `Y0f−tmp` geçişi | Saf ağırlık 3.38 → 0.26 s |
| 6 | **OOF/Q**: K-tablolarındaki aday-sıralama ve Q-tablosu döngüleri `omp parallel` (her kelime bağımsız); V49 istatistikleri sütun-bloklarıyla paralel (her sütunun toplam sırası aynı); V49 `apply`: A/A²/A³ üçlüsü için `d` bir kez, yerinde, aynı aritmetik | seri döngüler / 232 MB ek tahsis | V49 1.07 → 0.23 s; K-tabloları (6 tablo) ≈1.0 s; seri halinin ayrı ölçümü yok, OOF/Q toplamı 2.2–2.4 s |
| 7 | `cat` (F birleştirme) tek aşamada `move` | gereksiz kopya | −0.13 s; RAM ≈−232 MB [Muhtemel] |
| 8 | Lean yol (`V53H_LEAN=1`): yalnız nihai model (ρ=0) + Q karışımı + val; araştırma yolları (V46/V50 varyantları, eğitim logitleri, E1/E2 değerlendirmeleri) yok | model üretimiyle ilgisiz | program 65 s → 5.6 s toplam |

Özgün V53 kaynağı ve ikilisi dokunulmadan duruyor; tüm referans çıktılar o ikiliden üretildi.

### Ablasyon (tam konfigürasyon, 3 tur medyan)

| Varyant | Model hazır | Grup çözümü (duvar) | RAM |
|---|---|---|---|
| Tam hızlı yol | **5.42 s** | 0.34 s | 2,124 MB |
| − seyrek Y0 (tüm 2001 sütun, aynı AVX çekirdeği) | 7.15 s (+1.73) | 1.43 s | 3,290 MB |
| − AVX-512 çekirdeği (seyrek Y0 duruyor) | 5.76 s (+0.34) | 0.64 s | 2,124 MB |
| ikisi de kapalı | 7.58 s (+2.16) | 1.75 s | 3,289 MB |
| (orijinal aritmetik yol, naif `spd_solve`+yoğun) | 17.2 s | ~5–8 s | 4,229 MB |

Yani 17.2 → 7.6 s farkın büyük kısmı **naif skaler `spd_solve` ve yoğun diziler** (kernel+bellek düzeni), 7.6 → 5.4 s ise **Y0'ın yapısal sıfırlarını hiç işlememek** ve AVX çekirdeği [Kesin].

## 3. Doğruluk (orijinal ikili ile dosya düzeyinde)

Referans: **değişmemiş `tek_hamle_v53`** (`V33_DUMP` ile WC/WF). `cmp` sonuçları:

| Yol | Tam: WC / WF | Hafif: WC / WF |
|---|---|---|
| orijinal aritmetik yol (lean harness) | **bit-özdeş / bit-özdeş** | bit-özdeş / bit-özdeş |
| **hızlı-tam (varsayılan)** | **bit-özdeş / bit-özdeş** | **bit-özdeş / bit-özdeş** |
| hızlı + seyrek U (`V53H_U=1`) | farklı / farklı | özdeş / farklı |

Hızlı-tam yol ile orijinal yol arasında (tam ve hafif): **özellik matrisi F, AFF, RF, ACCB (Gram blokları), ACF, RC (yoğun karşılığı), Y0 (float), W_C, W_F, validation logitleri: hepsi bit-özdeş** (0 farklı eleman). Validation CE/Top-1/Top-5 ve test metrikleri aynı. 

Çift duyarlıklı **ara** diziler fark eder: `X = A⁻¹A_CF` (maks mutlak 6.7e-16, bağıl 6.5e-10 yalnız sıfıra yakın elemanlarda; elemanların %38'i), `T = A_FCX` (maks mutlak 3.6e-12; **bağıl 1.5e-12**), `U` (maks mutlak 2.3e-13, bağıl 4.2e-11). **Kaynak [Kesin]:** orijinal skaler döngüler, derleyicinin açma/FMA birleştirmesi nedeniyle bazı terimleri tek yuvarlamayla (FMA) bazılarını iki yuvarlamayla toplar (assembly'de aynı döngüde hem `vfnmadd231sd` hem `vmulsd+vsubsd` görülüyor); bu desen kod konumuna bağlı ve yeniden üretilemez. Toplama *sırası* aynıdır. Fark ≤1 ulp mertebesinde ve float'a yuvarlanınca (W_C, W_F, logitler) kayboluyor: bu nedenle nihai çıktılar bit-özdeş.

**Seyrek U seçeneği (`V53H_U=1`, varsayılan değil):** U'yu seyrek hesaplar (0.3 s), toplama sırası dgemm'den farklı → tam konfigürasyonda W_F'de **10/1,026,513** eleman (bağıl ≤1.5e-7), W_C'de **3,094/79.9 M** eleman (bağıl ≤2.6e-4 yalnız sıfıra çok yakın değerlerde, mutlak ≤2.9e-11) ve val logitlerinde 458/9.96 M eleman (mutlak ≤9.3e-10) farklı; CE/Top-k aynı. "Bit-özdeş ağırlık" hedefi nedeniyle **varsayılan yapılmadı**; kazanç ≈0.3–0.4 s.

Tahmin sıralaması: val Top-1/Top-5 (tam 0.1663/0.3799, hafif 0.1655/0.3767) üç yolda da aynı.

## 4. Süre dökümü (5 tur medyan; aşamalar ayrık, alt kalemler ayrı satır)

**V53 tam** (mc=39,946)

| Aşama | Önce | Sonra |
|---|---|---|
| Veri okuma | 0.049 | 0.049 |
| Program başı → zincir | 0.176 | 0.181 |
| OOF/Q (V51 K-tabloları + kapı) | 2.264 | 2.275 |
| Özellik üretimi (F1, V39, V49) | 1.136 | 1.169 |
| Gram/RHS | 0.838 | 0.406 |
| Schur/Cholesky (grup Chol. + Y0 + X + T + U) | 8.539 | 1.190 |
| ↳ Y0 çözümü (duvar payı) | 4.984 | 0.088 |
| ↳ Y0 ilişkili toplam | 6.825 | 0.761 |
| Saf ağırlık üretimi (W_F + W_C) | 3.383 | 0.258 |
| Q_havuz tablosu (aynı fonksiyon; fark bellek durumundan [Tahmin]) | 0.179 | 0.051 |
| Diğer/ölçülmeyen | 0.529 | 0.046 |
| **Tam model hazır** | **17.19** | **5.60** |
| Validation değerlendirmesi (ayrı) | 0.403 | 0.238 |
| Tepe RAM | 4,229 MB | 2,124 MB |

**V53 hafif** (mc=15,165)

| Aşama | Önce | Sonra |
|---|---|---|
| OOF/Q | 2.248 | 2.174 |
| Özellik üretimi | 1.132 | 1.104 |
| Gram/RHS | 0.568 | 0.341 |
| Schur/Cholesky | 1.940 | 0.362 |
| ↳ Y0 çözümü | 1.280 | 0.026 |
| ↳ Y0 ilişkili toplam | 1.505 | 0.260 |
| Saf ağırlık | 0.226 | 0.136 |
| Q + diğer + program başı (okuma dahil) | 0.343 | 0.260 |
| **Tam model hazır** | **6.44** | **4.39** |
| Tepe RAM | 2,393 MB | 1,346 MB |

Orijinal değişmemiş ikili (tam program; 3 tur): model hazır **18.0 s** (özellik 4.06, Gram 0.64, Chol+Schur 11.37 [grup Chol.+Y0+X 7.24], ağırlık 1.19), toplam program **65 s**, RAM **7,304 MB**; hafif: 8.2 s, 31 s, 3,578 MB.

"Model hazır" tanımı: veri okuma sonrası → özellikler → Gram/RHS → grup Cholesky/Y0/X → Schur → W_F, W_C → Q_havuz tablosu. Hariç: validation/test değerlendirmesi, V42 rezidü denetimi (40 grupta; ≈0.17 s, hızlı yolda ayrıca ölçülüp çıkarıldı), karma hash/dump.

## 5. Kalan darboğazlar [Kesin]

Tam, sonra (5.6 s): OOF/Q **2.28 s (%41)** = 6 K-tablosu (≈1.0 s; sgemm V³ + aday sıralaması + Q tablosu) + **kapı** (58 M `atanh`+`tanh`, çift duyarlık libm, ≈1.0 s) + s(K) projeksiyonu; özellik üretimi **1.17 s (%21)** (F1 `tanhf` ≈0.65 s, V39 ≈0.2 s, V49 0.23 s); Schur/Chol 1.19 s (%21); Gram/RHS 0.41 s. Sıradaki hız fırsatları (**uygulanmadı**, çünkü bit düzeyinde sonucu değiştirir veya risklidir): kapıda `tanh(atanh f + g·U) = (f+t)/(1+f·t)` özdeşliği (matematiksel olarak aynı, ancak double yuvarlama farkı; ≈0.5 s kazanç) ve K-tablolarını tablo düzeyinde paralel çalıştırmak (OpenBLAS iç içe iş parçacığı riski).

## 6. Dürüst sınırlar

- **Ölçüm değişkenliği:** VM'de taze bellek sayfalarının ilk dokunuşu çok pahalı; soğuk koşular 1.5–2× yavaş. Tüm kıyaslar ısınmadan sonra, ardışık/ara ara, 5 tur medyan; yine de aynı oturumda önceki "21.7–25.2 s" ile bu oturumdaki "16.2–19.0 s" farkı bu etkiden.
- **Donanım/iş parçacığı bağımlılığı:** AVX-512 çekirdeği bu makineye (AVX-512 var) özel; yoksa `h_solve_fast` (AVX'siz, ≈+0.3 s) devreye giriyor. dgemm/sgemm çağrıları orijinalle aynı olduğundan 4 iş parçacığında bit-özdeşlik doğrulandı; farklı iş parçacığı sayısı orijinalde de OpenBLAS toplama sırasını değiştirebilir (test edilmedi).
- Tek veri kümesi (2,001 kelime, 113,046 eğitim satırı); 5,000 kelimelik deney sunucuda yok.
- Lean yol yalnız ρ=0 nihai modeli üretir; V46/V50 araştırma varyantları için orijinal `tek_hamle_v53` kullanılır (değişmedi).
- Test metrikleri yalnız `test_son.sh` ile nihai doğrulamada alındı; hızlandırma kararlarında kullanılmadı (zaman ölçümleri `V53H_TEST=0`).

## 7. Yeniden üretim

```
v53_cpu_hizlandirma/env_full.sh | env_hafif.sh      # ortam + komut (ARGS)
V53H_LEAN=1 V53H_FAST=1 V53H_U=0 ../cpp/tek_hamle_v53h $ARGS      # hızlı-tam (varsayılan, bit-özdeş)
V53H_LEAN=1 ../cpp/tek_hamle_v53h $ARGS                           # orijinal aritmetik yol (aynı ölçüm düzeneği)
./ref_orijinal.sh   # değişmemiş ikiliden referans WC/WF   ./dogrula.sh   # cmp + fark özeti (fark.py)
./olc.sh 5; python3 ozet.py   # zaman protokolü      ./ablasyon.sh   # ablasyon      ./test_son.sh   # nihai test
```
Derleme: `g++ -O3 -march=native -fopenmp -std=c++17 -o tek_hamle_v53h tek_hamle_v53h.cpp -L<scipy.libs> -l:libscipy_openblas-….so -Wl,-rpath,<scipy.libs>`.

## 8. Sorular

1. **Y0'ın ne kadarı gereksiz hesaplanıyordu?** Y0'ın **%93.6'sı** (74.8 M / 79.9 M eleman) yapısal olarak sıfırdı; Y0 çözüm işlemlerinin **%83'ü** (1.39×10¹⁰ → 2.35×10⁹ madd, 5.9× az) ve U/W_C geçişlerinin aynı oranı gereksizdi.
2. **Neyi kaldırdım?** Yoğun Y0/RC/ACC dizileri ve sıfır sütunların çözümü; naif skaler `spd_solve`; Y0d→Y0f ve `Y0f−tmp` geçişleri; araştırma yolları (ikinci yoğun Y0q çözümü, 6 sistem, eğitim logitleri); seri döngüler (K-tablo, V49).
3. **Matematiksel eşdeğerlik korundu mu?** Evet: aynı formüller, aynı toplama sırası; yalnız yapısal olarak sıfır olan sütunlar atlandı (bit-özdeş RC/Y0 ile kanıtlı).
4. **Ağırlıklar aynı mı?** Evet — **WC, WF ve val logitleri orijinal ikiliyle bit-özdeş** (cmp), hem tam hem hafif. Yalnız çift duyarlık ara diziler (X, T, U) ≤1 ulp seviyesinde farklı (kaynak: derleyici FMA/açma deseni).
5. **Validation CE değişti mi?** Hayır: 4.609033 (tam), 4.612900 (hafif); test PPL 98.092 / 98.212 aynı.
6. **V53 tam kaç saniye?** **5.6 s** (medyan; 5.1–6.3) — orijinal 16.2–19.0 s (aynı oturum), önceki raporda 21.7–25.2 s.
7. **V53 hafif?** **4.4 s** (4.25–4.44) — orijinal 7.9–8.7 s (önceki raporda 9.2 s).
8. **Tepe RAM?** Tam: 7,304 → **2,124 MB** (−%71); hafif: 3,578 → **1,346 MB** (−%62).
9. **En büyük kalan darboğaz?** OOF/Q (V51 K-tabloları + atanh/tanh kapısı): 2.28 s = %41 (tam), %50 (hafif); sonra özellik üretimi (1.2 s).
10. **Commit/push?** Bkz. son mesaj.
