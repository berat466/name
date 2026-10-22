# Tek hamle V6 raporu — önceki hesaptan devam + hızlandırma denemesi

Branch: `claude/formul-avcisi`. Referans: `TEK_HAMLE_V5_RAPORU` (V5 raporu `TEK_HAMLE_V5_RAPOR.md`). V4/V5 dosyaları ve raporları korundu.
Kural: gradyan yok, Adam yok, epok yok, **RLS yok**. Kod: `cpp/tek_hamle_v6.cpp` (V4/V5 çekirdekleri kopya). Ham loglar, komutlar, ağırlık dökümleri: `v6_ham/`.
Ortam: Intel Xeon 2,80 GHz, 4 çekirdek (AVX-512), 16 GB, g++ 13.3.0 `-O3 -march=native -fopenmp` (`v6_ham/ortam.txt`).

## Ana sonuç (kısa)
**Hızlanma gerçekleşmedi.** Aynı (hatta 5 haneye kadar aynı) test kaybı elde edildi, ama önceki Cholesky'den ya da önceki W'den devam eden hiçbir yöntem V4'ün "S,R birikimli + yeniden Cholesky" yolundan hızlı değildi.

| yöntem (5 görev) | test MSE (görev 5) | güncelleme süresi, V4'e göre |
|---|---|---|
| V4: S/R + yeniden Cholesky | aynı | 1× |
| V6 ΔW + yeniden Cholesky | aynı | ≈ 1,0–1,15× (hızlanma yok) |
| V6 ΔW + eski Cholesky ön koşullayıcı (PCG) | aynı | ≈ 1,2–1,35× yavaş (5 görev toplamı); çözüm adımı çoğu görevde 1–5× yavaş |
| V6 eski Cholesky'ye rank-k güncelleme | aynı | görev başına 26–41×, 5 görev toplamı 21–31× yavaş |

Nedenleri işlem maliyetleriyle aşağıda (bölüm "Neden hızlanmadı").

## Kurulum
- MNIST, sınıf-artımlı 5 görev ({0,1},{2,3},{4,5},{6,7},{8,9}), 10 çıkış, one-hot. Fit satırı 55k (görev başına ≈ 11k), test 10k (aynı bölünme, tohum 1, γ=0,125, λ=1).
- Özellik matrisi sabit: W1 yalnız görev-1 verisinden (V4 "sabit-W1", çift yön + rastgele bias), H = 256 / 512 / 1024 (m = H+1 bias dahil). Büyütme yok.
- Kayıp = MSE (ortalama `(z−t)²`, örnek × 10 çıkış). Eski/yeni/toplam = önceki görevlerin / son görevin / görülen tüm sınıfların test örnekleri. "Eski görev kayıp artışı" (unutma) = (görev 5 sonrası eski görev i test MSE) − (görev i'yi öğrendikten hemen sonraki test MSE), i=1..4 ortalaması.
- Süre = duvar saati. "Özellik" = yeni satırlar için `tanh(γ(XW1+b1))` (tüm yöntemlerde ortak). "Güncelleme" = istatistik/rank-k + Cholesky + çözüm. Medyan: H=256, 512 için 3 tekrar, H=1024 için 2 tekrar (aynı komut, aynı tohum). Tüm koşular aynı makinede, art arda.
- Ham FP64 referans (`v4f64`): S ve R tam double toplamla; ağırlık farkı = `‖W − W_ref64‖_F / ‖W_ref64‖_F`.

## Yöntemler ve formüller
- **V4**: `S += FᵀF` (V4'ün `syrk_add`'ı: 2048 satırlık float parça toplamı, parçalar arası double), `R += FᵀT`; her görevde `(S+λI)` yeniden Cholesky, `W` çözülür.
- **V6 ΔW (`v6dw_ref`)**: `W_yeni = W_eski + ΔW`, `(S_yeni+λI) ΔW = ΔR − ΔS·W_eski`. Sağ taraf `ΔR − ΔS W_eski = Fᵀ(T − F W_eski)` olarak yalnız yeni görev satırlarından hesaplanır (eski veri yeniden işlenmez). Sol taraf her görevde yeniden Cholesky.
- **V6 ΔW + PCG (`v6dw_pcg`)**: aynı denklem, ama `(S_yeni+λI)`, **son hesaplanan Cholesky çarpanını** (önceki görevlerin `S`'inin çarpanı) ön koşullayıcı yaparak PCG ile çözülür (`‖r‖/‖b‖ < 1e-9`, en çok 60 yineleme). Yakınsamazsa **güvenli dönüş**: sıfırdan Cholesky, yeni çarpan saklanır.
- **V6 rank-k (`v6chol`)**: görev 1'de `S` kurulup Cholesky alınır (`U = Lᵀ`). Sonraki görevlerde yeni görev satırlarının her biri `UᵀU += x xᵀ` olarak (dchud, 256'lık gruplar, kolon bazlı OpenMP paralel) eski çarpana işlenir; `S` ve `ΔS` oluşturulmaz. `R` birikir, `W = (UᵀU)⁻¹R` iki üçgen çözümle. Sayısal sorunda (kök ≤ 0 / sonlu değil) **güvenli dönüş**: eski çarpan yedeği `U_eski` + yalnız yeni görevin `ΔS`'i ile sıfırdan Cholesky.
- Eski eğitim verisi hiçbir yöntemde yeniden işlenmedi (güvenli dönüşte yalnız yeni görev satırları).

## Sonuçlar — 5 görev (görev 5 sonrası), H = 256 / 512 / 1024

#### H=256 (m=257)

| yöntem | tekrar | test MSE eski | yeni | toplam | doğr. % | eski görev kayıp artışı (unutma) | güncelleme sn, görev 1/2/3/4/5 | güncelleme toplam sn | özellik toplam sn | toplam sn (özellik+güncelleme) | ΔW FP64 ref.'e göre, görev 5 | duvar sn | tepe RAM MB |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| V4: S/R float + yeniden Cholesky | 3 | 0.02538 | 0.03467 | 0.02723 | 91.61 | 0.01394 | 0.027 / 0.023 / 0.021 / 0.024 / 0.023 | 0.118 | 0.334 | 0.451 | 1.5e-03 | 1.02 | 226 |
| V4-FP64 (referans): S/R double + yeniden Cholesky | 3 | 0.02538 | 0.03467 | 0.02723 | 91.61 | 0.01394 | 0.032 / 0.027 / 0.026 / 0.027 / 0.026 | 0.137 | 0.327 | 0.464 | 0.0e+00 | 1.03 | 226 |
| V4-FP32/256: float, 256 satırda double'a boşalt | 3 | 0.02538 | 0.03467 | 0.02723 | 91.61 | 0.01394 | 0.024 / 0.025 / 0.024 / 0.025 / 0.025 | 0.122 | 0.324 | 0.447 | 1.9e-04 | 0.98 | 226 |
| V6 ΔW: eski W + ΔW, yeniden Cholesky | 3 | 0.02538 | 0.03467 | 0.02723 | 91.61 | 0.01394 | 0.030 / 0.026 / 0.023 / 0.026 / 0.029 | 0.134 | 0.321 | 0.455 | 1.4e-03 | 0.98 | 226 |
| V6 ΔW: eski Cholesky ön koşullayıcı (PCG) + güvenli dönüş | 3 | 0.02538 | 0.03467 | 0.02723 | 91.61 | 0.01394 | 0.032 / 0.042 / 0.024 / 0.031 / 0.030 | 0.159 | 0.339 | 0.498 | 1.4e-03 | 1.03 | 229 |
| V6 Cholesky rank-k güncelleme + güvenli dönüş | 3 | 0.02538 | 0.03467 | 0.02723 | 91.61 | 0.01394 | 0.024 / 0.683 / 0.839 / 0.922 / 0.941 | 3.409 | 0.311 | 3.720 | 1.0e-03 | 4.21 | 228 |

#### H=512 (m=513)

| yöntem | tekrar | test MSE eski | yeni | toplam | doğr. % | eski görev kayıp artışı (unutma) | güncelleme sn, görev 1/2/3/4/5 | güncelleme toplam sn | özellik toplam sn | toplam sn (özellik+güncelleme) | ΔW FP64 ref.'e göre, görev 5 | duvar sn | tepe RAM MB |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| V4: S/R float + yeniden Cholesky | 3 | 0.02014 | 0.02643 | 0.02139 | 93.95 | 0.01123 | 0.083 / 0.081 / 0.072 / 0.075 / 0.074 | 0.385 | 0.663 | 1.048 | 4.3e-03 | 1.89 | 234 |
| V4-FP64 (referans): S/R double + yeniden Cholesky | 3 | 0.02014 | 0.02643 | 0.02139 | 93.95 | 0.01123 | 0.124 / 0.118 / 0.120 / 0.130 / 0.119 | 0.611 | 0.695 | 1.306 | 0.0e+00 | 2.20 | 234 |
| V4-FP32/256: float, 256 satırda double'a boşalt | 3 | 0.02014 | 0.02643 | 0.02139 | 93.94 | 0.01123 | 0.080 / 0.073 / 0.073 / 0.083 / 0.076 | 0.385 | 0.645 | 1.031 | 5.2e-04 | 1.94 | 234 |
| V6 ΔW: eski W + ΔW, yeniden Cholesky | 3 | 0.02014 | 0.02643 | 0.02139 | 93.95 | 0.01123 | 0.088 / 0.090 / 0.081 / 0.086 / 0.083 | 0.427 | 0.681 | 1.107 | 4.1e-03 | 1.99 | 234 |
| V6 ΔW: eski Cholesky ön koşullayıcı (PCG) + güvenli dönüş | 3 | 0.02014 | 0.02643 | 0.02139 | 93.95 | 0.01123 | 0.077 / 0.115 / 0.082 / 0.095 / 0.093 | 0.461 | 0.677 | 1.138 | 4.1e-03 | 2.04 | 241 |
| V6 Cholesky rank-k güncelleme + güvenli dönüş | 3 | 0.02014 | 0.02643 | 0.02139 | 93.95 | 0.01122 | 0.083 / 3.307 / 2.967 / 2.374 / 3.098 | 11.829 | 0.675 | 12.505 | 2.9e-03 | 13.33 | 238 |

#### H=1024 (m=1025)

| yöntem | tekrar | test MSE eski | yeni | toplam | doğr. % | eski görev kayıp artışı (unutma) | güncelleme sn, görev 1/2/3/4/5 | güncelleme toplam sn | özellik toplam sn | toplam sn (özellik+güncelleme) | ΔW FP64 ref.'e göre, görev 5 | duvar sn | tepe RAM MB |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| V4: S/R float + yeniden Cholesky | 2 | 0.01620 | 0.02125 | 0.01720 | 95.37 | 0.00907 | 0.328 / 0.288 / 0.274 / 0.295 / 0.267 | 1.452 | 1.376 | 2.828 | 1.1e-02 | 4.38 | 251 |
| V4-FP64 (referans): S/R double + yeniden Cholesky | 2 | 0.01620 | 0.02125 | 0.01720 | 95.37 | 0.00907 | 0.495 / 0.466 / 0.443 / 0.466 / 0.435 | 2.306 | 1.317 | 3.622 | 0.0e+00 | 5.13 | 251 |
| V4-FP32/256: float, 256 satırda double'a boşalt | 2 | 0.01620 | 0.02125 | 0.01720 | 95.37 | 0.00907 | 0.283 / 0.278 / 0.269 / 0.278 / 0.275 | 1.383 | 1.309 | 2.691 | 1.5e-03 | 4.14 | 251 |
| V6 ΔW: eski W + ΔW, yeniden Cholesky | 2 | 0.01620 | 0.02125 | 0.01720 | 95.36 | 0.00907 | 0.309 / 0.291 / 0.282 / 0.293 / 0.295 | 1.471 | 1.327 | 2.798 | 1.0e-02 | 4.30 | 251 |
| V6 ΔW: eski Cholesky ön koşullayıcı (PCG) + güvenli dönüş | 2 | 0.01620 | 0.02125 | 0.01720 | 95.36 | 0.00907 | 0.261 / 0.472 / 0.292 / 0.475 / 0.261 | 1.761 | 1.334 | 3.095 | 1.0e-02 | 4.72 | 275 |
| V6 Cholesky rank-k güncelleme + güvenli dönüş | 2 | 0.01620 | 0.02125 | 0.01720 | 95.37 | 0.00907 | 0.289 / 7.375 / 6.596 / 8.174 / 8.569 | 31.002 | 1.334 | 32.336 | 7.2e-03 | 33.85 | 269 |

(Tablo `v6_ham/analiz.py` ile ham loglardan üretildi. Tüm yöntemlerde test MSE aynı: H=256 toplam 0,02723, H=512 0,02139, H=1024 0,01720; farklar beşinci hane altında. Doğruluk %91,6 / %93,9 / %95,4.)

### Test MSE (ana ölçüt) — hepsi aynı
Eski görev kaybı ve unutma yöntemler arasında aynı: H=256 unutma 0,01394, H=512 0,01123 (rank-k 0,01122), H=1024 0,00907. Kayıp artışı yöntemden değil görev sayısından geliyor (sınıf sayısı arttıkça MSE artar). V6 yöntemleri kaybı ne düşürdü ne yükseltti.

## Sayısal doğruluk: FP32 vs FP64 istatistik birikimi
V5'te görülen ağırlık sapmasının nedeni: V4 `syrk_add`, 2048 satırlık parçayı float'ta toplayıp sonra double'a ekliyor.
| H | yöntem | ΔW görev 1 | görev 2 | görev 3 | görev 4 | görev 5 | istatistik (syrk) sn, 5 görev | FP64'e göre istatistik süresi |
|---|---|---|---|---|---|---|---|---|
| 256 | FP32 (2048 satır float toplam) | 6.2e-03 | 3.1e-03 | 2.2e-03 | 1.8e-03 | 1.5e-03 | 0.101 | 0.85× |
| 256 | FP32, 256 satırda double'a boşalt | 8.2e-04 | 3.6e-04 | 2.7e-04 | 2.1e-04 | 1.9e-04 | 0.106 | 0.89× |
| 256 | FP64 tam (referans) | 0.0e+00 | 0.0e+00 | 0.0e+00 | 0.0e+00 | 0.0e+00 | 0.118 | 1.00× |
| 256 | V6 rank-k (görev 1 float syrk, sonrası double rank-k) | 6.2e-03 | 2.6e-03 | 1.8e-03 | 1.3e-03 | 1.0e-03 | 3.405 | 28.73× |
| 512 | FP32 (2048 satır float toplam) | 1.3e-02 | 7.2e-03 | 5.6e-03 | 5.0e-03 | 4.3e-03 | 0.283 | 0.55× |
| 512 | FP32, 256 satırda double'a boşalt | 1.7e-03 | 8.9e-04 | 7.2e-04 | 6.0e-04 | 5.2e-04 | 0.280 | 0.55× |
| 512 | FP64 tam (referans) | 0.0e+00 | 0.0e+00 | 0.0e+00 | 0.0e+00 | 0.0e+00 | 0.510 | 1.00× |
| 512 | V6 rank-k (görev 1 float syrk, sonrası double rank-k) | 1.3e-02 | 6.3e-03 | 4.4e-03 | 3.5e-03 | 2.9e-03 | 11.808 | 23.14× |
| 1024 | FP32 (2048 satır float toplam) | 2.4e-02 | 1.6e-02 | 1.4e-02 | 1.2e-02 | 1.1e-02 | 0.923 | 0.52× |
| 1024 | FP32, 256 satırda double'a boşalt | 2.8e-03 | 2.1e-03 | 1.7e-03 | 1.6e-03 | 1.5e-03 | 0.866 | 0.48× |
| 1024 | FP64 tam (referans) | 0.0e+00 | 0.0e+00 | 0.0e+00 | 0.0e+00 | 0.0e+00 | 1.789 | 1.00× |
| 1024 | V6 rank-k (görev 1 float syrk, sonrası double rank-k) | 2.4e-02 | 1.3e-02 | 1.0e-02 | 8.6e-03 | 7.2e-03 | 30.900 | 17.27× |

- [Kesin] FP32 birikimi, FP64 referansa göre ağırlıkta H büyüdükçe artan bir sapma yaratıyor (görev 1: 6×10⁻³ / 1,3×10⁻² / 2,4×10⁻²). Test MSE'ye etkisi görülmedi (5 hane).
- [Kesin] **256 satırda bir double'a boşaltmak** (`v4f32c256`) sapmayı 7–10 kat azaltıyor (görev 5: 1,5×10⁻³ → 1,9×10⁻⁴; 4,3×10⁻³ → 5,2×10⁻⁴; 1,1×10⁻² → 1,5×10⁻³) ve istatistik süresini artırmıyor (FP32'ye göre 1,05× / 0,99× / 0,94×).
- Tam FP64 kesin referans ama istatistik süresi FP32'nin ≈ 1,2× (H=256) ve ≈ 1,8–1,9× (H=512, 1024) kadar. Uyarı: FP64 çekirdeğini ben yazdım (V4'ün ayarlı float çekirdeği kadar optimize değil), bu oran saf hassasiyet maliyeti değil.
- V6 rank-k: sapma görev 1'in float `S`'inden miras kalıyor, sonraki double güncellemeler sapmayı kademeli azaltıyor (görev 5: 1,0×10⁻³ / 2,9×10⁻³ / 7,2×10⁻³). Çarpan double'da olmasına rağmen `c256` kadar iyi değil.
- İyileştirme (V4 dosyalarını değiştirmedim): `syrk` çağrısını 256 satırlık dilimlerle yapmak, bedava sayılır bir doğruluk kazancı.

## Neden hızlanmadı — işlem maliyeti
Bir görevde n ≈ 11 000 yeni satır, m = H+1 özellik. (MAC = çarpma-toplama)
| H | m | özellik n·784·H | ΔS = n·m²/2 | Cholesky m³/3 | rank-n güncelleme 2·n·m² | PCG yineleme 20·m² | PCG başa baş yineleme m/60 | ölçülen: Cholesky+çözüm / (syrk+Cholesky+çözüm), V4, 5 görev |
|---|---|---|---|---|---|---|---|---|
| 256 | 257 | 2.2e+09 | 3.6e+08 | 5.7e+06 | 1.5e+09 | 1.3e+06 | 4.3 | %14 |
| 512 | 513 | 4.4e+09 | 1.4e+09 | 4.5e+07 | 5.8e+09 | 5.3e+06 | 8.6 | %27 |
| 1024 | 1025 | 8.8e+09 | 5.8e+09 | 3.6e+08 | 2.3e+10 | 2.1e+07 | 17.1 | %36 |

Görevlerin işlem maliyeti özet:
1. [Kesin] **Cholesky + çözüm, güncelleme süresinin küçük/orta bir kısmı**: V4'te `Cholesky + çözüm`, (`syrk` + Cholesky + çözüm) süresinin H=256'da %14'ü, H=512'de %27'si, H=1024'te %36'sı. Özellik hesabı (`n·784·H`) dahil toplam sürede payı %3,8 / %9,7 / %18,7. Yani, yeniden Cholesky'yi tamamen **sıfır maliyete** indiren kusursuz bir yöntem bile bu kurulumda H=256'da toplam süreyi en fazla %4, H=1024'te en fazla %19 kısaltırdı. Asıl maliyet `ΔS = FᵀF` (n·m²/2) ve özellik hesabında; bunlar yeni verinin her satırını okumayı gerektirir ve hiçbir "devam" yöntemi onu kaldırmaz.
2. [Kesin] **Rank-k Cholesky güncellemesi n ≫ m olduğunda yanlış araç.** Her satır için dchud ≈ 2m² MAC; n satır → 2·n·m², toplu yöntemin `ΔS` maliyetinin (n·m²/2) 4 katı, üstüne `Cholesky` (m³/3) yine ihmal edilebilir. Ek olarak güncelleme sıralı kolon işlemi ve bellek bant genişliğine bağlı (BLAS3 gibi bloklanamıyor). Ölçülen: görev başına güncelleme H=256'da 0,68–0,94 sn (V4: 0,024), H=512'de 2,4–3,3 sn (V4: 0,08), H=1024'te 6,6–8,6 sn (V4: 0,29) → görev başına 26–41× yavaş; 5 görev toplamı (görev 1 aynı yol) 21–31×.
3. [Kesin] **PCG için önceki Cholesky iyi bir ön koşullayıcı değil** (yineleme sayıları ölçüldü). [Muhtemel] nedeni: yeni görev `S`'ye yaklaşık eski görevlerin toplamı kadar yeni bilgi ekliyor (`ΔS`'in rankı m, n≫m); `M⁻¹A` özdeğerleri kümelenmiyor (özdeğer dağılımını ölçmedim). PCG yineleme sayıları (aşağıdaki tablo) 28–195; her yineleme ≈ 20·m² MAC (matris-vektör 10 sağ taraf + iki üçgen çözüm); başa baş yineleme sayısı m/60 = 4 / 9 / 17. Gözlenen 28–195 yineleme, başa baş sayısının 4–20 katı. Üstelik ön koşullayıcı her görevde daha eski ve yineleme sayısı artıyor (maxit=300 ile 64 → 114 H=256'da, 105 → 195 H=1024'te).
4. [Kesin] ΔW formu (eski W + düzeltme) **maliyeti değiştirmiyor**: sağ taraf `Fᵀ(T−F W_eski)` n·m·10 MAC (ihmal edilebilir), ama sol taraf aynı matris, aynı çarpan ihtiyacı. Ölçülen süre V4'ün ≈ 1,0–1,15 katı (sağ taraf için ek hesap).

### PCG ayrıntısı (`v6dw_pcg`, tohum 1)
| H | maxit | görev | PCG yineleme | bağıl artık | durum | Cholesky+çözüm (V4) sn | PCG çözüm sn |
|---|---|---|---|---|---|---|---|
| 256 | 60 | 2 | 60 | 1.7e-09 | FALLBACK (yeniden Cholesky) | 0.0043 | 0.0122 (+ yeniden Cholesky 0.0035) |
| 256 | 60 | 3 | 28 | 8.6e-10 | yakınsadı | 0.0034 | 0.0047 (+ yeniden Cholesky 0.0000) |
| 256 | 60 | 4 | 40 | 6.7e-10 | yakınsadı | 0.0031 | 0.0056 (+ yeniden Cholesky 0.0000) |
| 256 | 60 | 5 | 46 | 6.2e-10 | yakınsadı | 0.0032 | 0.0062 (+ yeniden Cholesky 0.0000) |
| 256 | 300 | 2 | 64 | 8.9e-10 | yakınsadı | 0.0043 | 0.0089 (+ yeniden Cholesky 0.0000) |
| 256 | 300 | 3 | 83 | 7.3e-10 | yakınsadı | 0.0034 | 0.0139 (+ yeniden Cholesky 0.0000) |
| 256 | 300 | 4 | 100 | 9.7e-10 | yakınsadı | 0.0031 | 0.0131 (+ yeniden Cholesky 0.0000) |
| 256 | 300 | 5 | 114 | 7.9e-10 | yakınsadı | 0.0032 | 0.0164 (+ yeniden Cholesky 0.0000) |
| 512 | 60 | 2 | 60 | 1.5e-07 | FALLBACK (yeniden Cholesky) | 0.0203 | 0.0312 (+ yeniden Cholesky 0.0205) |
| 512 | 60 | 3 | 37 | 5.3e-10 | yakınsadı | 0.0193 | 0.0211 (+ yeniden Cholesky 0.0000) |
| 512 | 60 | 4 | 49 | 6.6e-10 | yakınsadı | 0.0220 | 0.0242 (+ yeniden Cholesky 0.0000) |
| 512 | 60 | 5 | 58 | 7.1e-10 | yakınsadı | 0.0189 | 0.0885 (+ yeniden Cholesky 0.0000) |
| 512 | 300 | 2 | 81 | 8.5e-10 | yakınsadı | 0.0203 | 0.0462 (+ yeniden Cholesky 0.0000) |
| 512 | 300 | 3 | 110 | 7.3e-10 | yakınsadı | 0.0193 | 0.0614 (+ yeniden Cholesky 0.0000) |
| 512 | 300 | 4 | 131 | 7.3e-10 | yakınsadı | 0.0220 | 0.0682 (+ yeniden Cholesky 0.0000) |
| 512 | 300 | 5 | 149 | 8.5e-10 | yakınsadı | 0.0189 | 0.0755 (+ yeniden Cholesky 0.0000) |
| 1024 | 60 | 2 | 60 | 2.8e-06 | FALLBACK (yeniden Cholesky) | 0.0959 | 0.1446 (+ yeniden Cholesky 0.1364) |
| 1024 | 60 | 3 | 46 | 6.8e-10 | yakınsadı | 0.1029 | 0.1062 (+ yeniden Cholesky 0.0000) |
| 1024 | 60 | 4 | 60 | 2.3e-09 | FALLBACK (yeniden Cholesky) | 0.1057 | 0.1401 (+ yeniden Cholesky 0.1223) |
| 1024 | 60 | 5 | 25 | 5.4e-10 | yakınsadı | 0.0967 | 0.0761 (+ yeniden Cholesky 0.0000) |
| 1024 | 300 | 2 | 105 | 8.4e-10 | yakınsadı | 0.0959 | 0.2399 (+ yeniden Cholesky 0.0000) |
| 1024 | 300 | 3 | 140 | 8.2e-10 | yakınsadı | 0.1029 | 0.3304 (+ yeniden Cholesky 0.0000) |
| 1024 | 300 | 4 | 163 | 7.3e-10 | yakınsadı | 0.1057 | 0.4103 (+ yeniden Cholesky 0.0000) |
| 1024 | 300 | 5 | 195 | 7.6e-10 | yakınsadı | 0.0967 | 0.4507 (+ yeniden Cholesky 0.0000) |

(Doğrudan çözümün, aynı görevde Cholesky+çözüm süresi yanında verildi.) maxit=60 ile bazı görevlerde yakınsamadı ve yeniden Cholesky'ye düşüldü (H=256: görev 2; H=512: görev 2; H=1024: görev 2 ve 4); maxit=300 ile hiç düşmedi ama çözüm adımı 2,1–5,1× yavaş. maxit=60'ta, yakınsayan görevlerde PCG çözümü V4'ün Cholesky+çözümünün 1,0–4,7 katı; tek istisna H=1024 görev 5 (taze ön koşullayıcı, 25 yineleme): 0,8× — fakat o ön koşullayıcının Cholesky'si görev 4'te ödendi, 5 görev toplamı yine daha yavaş (H=1024: 1,76 sn vs V4 1,45 sn).

### Mikro kıyas: rank-k güncellemesi ne zaman kazanır?
Eski durum görev 1'den; k yeni satır (görev 2'den); her k için 5 tekrar medyanı. Rank-k = güncelleme + çözüm; V4 yolu = `ΔS` (syrk) + `S`'e ekle + Cholesky + çözüm.

| H | k (yeni satır) | rank-k güncelleme+çözüm sn | V4: ΔS+Cholesky+çözüm sn | oran (rank-k / V4) | W bağıl farkı |
|---|---|---|---|---|---|
| H | k (yeni satır) | rank-k güncelleme+çözüm sn | V4: ΔS+Cholesky+çözüm sn | oran (rank-k / V4) | W bağıl farkı |
|---|---|---|---|---|---|
| 256 | 4 | 0.00050 | 0.00346 | 0.15× | 1.62e-07 |
| 256 | 16 | 0.00045 | 0.00324 | 0.14× | 1.02e-06 |
| 256 | 64 | 0.00109 | 0.00337 | 0.32× | 7.64e-06 |
| 256 | 256 | 0.02169 | 0.00429 | 5.05× | 5.67e-05 |
| 256 | 1024 | 0.08318 | 0.00521 | 15.96× | 4.09e-04 |
| 256 | 4096 | 0.29488 | 0.01065 | 27.70× | 2.64e-03 |
| 512 | 4 | 0.00194 | 0.02028 | 0.10× | 3.55e-07 |
| 512 | 16 | 0.00190 | 0.01976 | 0.10× | 2.16e-06 |
| 512 | 64 | 0.01429 | 0.01976 | 0.72× | 1.56e-05 |
| 512 | 256 | 0.08342 | 0.02387 | 3.49× | 1.16e-04 |
| 512 | 1024 | 0.28756 | 0.02513 | 11.44× | 8.94e-04 |
| 512 | 4096 | 1.01279 | 0.04026 | 25.16× | 5.42e-03 |
| 1024 | 4 | 0.00819 | 0.10157 | 0.08× | 6.09e-07 |
| 1024 | 16 | 0.00786 | 0.09620 | 0.08× | 4.24e-06 |
| 1024 | 64 | 0.06273 | 0.10344 | 0.61× | 3.07e-05 |
| 1024 | 256 | 0.22428 | 0.09837 | 2.28× | 2.39e-04 |
| 1024 | 1024 | 0.35137 | 0.11250 | 3.12× | 1.72e-03 |
| 1024 | 4096 | 3.11411 | 0.16638 | 18.72× | 1.22e-02 |

- [Kesin] Rank-k yalnız **k ≲ 64 yeni satırda** V4 yolundan hızlı (0,08–0,7×); k ≥ 256'dan itibaren yavaş (2–5×), k = 4096'da 19–28× yavaş. Başa baş noktası üç H için de k ≈ 100 civarı (teorik tahminimden, m/4,5, farklı: H=1024'te teori ≈ 230, ölçüm ≈ 100; V4 yolunda `S` kopyası ve çekirdek verimliliği farkı). Görev başına ≈ 11 000 satır olduğundan rank-k yanlış rejimde. Anlamlı kullanım: sürekli, çok küçük gruplarla (≲ 64 satır) güncelleme ve her güncellemeden sonra ağırlık gerekiyorsa.
- Rank-k ve V4 arasındaki W farkı k büyüdükçe artıyor (k=4: ~10⁻⁷, k=4096: 10⁻²): V4'ün float `syrk` birikimi yüzünden (rank-k'nin double), rank-k'den değil.

## Güvenli dönüş testi
- `v6chol` H=256 ve H=512'de görev 3'te güncelleme zorla başarısız sayıldı (`zorla_fallback_gorevi=3`). Dönüş: eski çarpan + yeni görev `ΔS`'iyle sıfırdan Cholesky. Sonuç: test MSE değişmedi (H=256 görev 3 toplam 0,013938, görev 5 0,027226; H=512 görev 5 0,02139); H=256'da ağırlıklar normal rank-k yoluna göre 4,8×10⁻⁴–7,9×10⁻⁴ bağıl farklı (görev 3-5) (dönüş yolu `ΔS`'yi float `syrk` ile kuruyor), FP64 referansa göre 1,2×10⁻³ (normal rank-k: 1,0×10⁻³) — aynı büyüklükte. Ağırlık dökümleri: `v6_ham/agirliklar_zorla_fallback_H256/`, `v6_ham/agirliklar_normal_v6chol_H256/`.
- Doğal (zorlanmamış) dönüşler: `v6chol` hiçbirini gerektirmedi (kök hep pozitif, kararlı); `v6dw_pcg` yukarıda listelendi. Hiçbir koşuda Cholesky başarısız olmadı.

## Sınırlar ve açık noktalar
- Tek tohum (1) ve tek veri bölünmesi. Süre tekrarları medyan (3 / 2 tekrar). Küçük süreler (milisaniye) için ölçüm gürültüsü ± birkaç ms.
- Rank-k çekirdeğini ben yazdım (BLAS yok, kolon-bazlı OpenMP). Daha iyi bir çekirdek farkı daralttırabilir; ama işlem sayımı (2·n·m² MAC, `syrk`'in 4 katı) ve mikro kıyastaki başa baş (k ≈ 100) kurulumu belirliyor: [Muhtemel] n ≈ 11 000'de hiçbir çekirdek bunu kapatmaz.
- PCG yalnız "son Cholesky" ön koşullayıcısıyla denendi; yineleme sayısını azaltacak başka bir ön koşullayıcı (örn. düşük rank düzeltmeli) denenmedi.
- FP64 süresi farklı çekirdek olduğu için hassasiyet maliyetinin üst sınırı.
- Büyüyen özellik uzayı bu deneyde yok.

## İyileştirme fikirleri (ölçülmedi)
- V4'te `syrk_add`'ı 256 satırlık dilimlerle çağırmak: sapma 7–10× azalıyor, süre aynı (ölçüldü: `v4f32c256`).
- Rank-k yalnız küçük gruplar için (k ≲ 64); görev düzeyinde toplu `ΔS` + Cholesky.
- Asıl hız kaynağı `ΔS` ve özellik hesabı: daha ayarlı veya düşük hassasiyetli (ör. TF32/FP16 tensör çekirdeği) çarpım, ya da yeni görev satırlarından örnekleme.
- Büyük m için (m ≳ 4000) Cholesky payı artar (m³): o rejimde ön koşullu yinelemeli çözüm yeniden değerlendirilebilir; burada ölçülmedi.

## Komutlar (gerçek, `v6_ham/v6log/manifest.tsv`)
Biçim: `tek_hamle_v6 MOD H TOHUM γ λ GÖREV ZORLA_FALLBACK MAXIT` (mikro kıyas: `tek_hamle_v6 mikro H TOHUM`). Çalıştırma betiği: `v6_ham/calistir_script.sh`. Derleme: `g++ -O3 -march=native -fopenmp -o tek_hamle_v6 tek_hamle_v6.cpp`.

| komut | duvar sn | kullanıcı CPU sn | tepe RAM MB |
|---|---|---|---|
| `./tek_hamle_v6 v4f32 256 1 0.125 1 5 0 60` | 1.21 | 3.19 | 226 |
| `./tek_hamle_v6 v4f64 256 1 0.125 1 5 0 60` | 1.03 | 3.12 | 226 |
| `./tek_hamle_v6 v4f32c256 256 1 0.125 1 5 0 60` | 0.99 | 3.19 | 226 |
| `./tek_hamle_v6 v6dw_ref 256 1 0.125 1 5 0 60` | 0.97 | 3.22 | 226 |
| `./tek_hamle_v6 v6dw_pcg 256 1 0.125 1 5 0 60` | 1.03 | 3.34 | 229 |
| `./tek_hamle_v6 v6chol 256 1 0.125 1 5 0 60` | 4.60 | 17.51 | 228 |
| `./tek_hamle_v6 v4f32 512 1 0.125 1 5 0 60` | 1.87 | 6.43 | 234 |
| `./tek_hamle_v6 v4f64 512 1 0.125 1 5 0 60` | 2.21 | 7.52 | 234 |
| `./tek_hamle_v6 v4f32c256 512 1 0.125 1 5 0 60` | 1.82 | 6.30 | 234 |
| `./tek_hamle_v6 v6dw_ref 512 1 0.125 1 5 0 60` | 1.99 | 6.87 | 234 |
| `./tek_hamle_v6 v6dw_pcg 512 1 0.125 1 5 0 60` | 2.14 | 7.35 | 241 |
| `./tek_hamle_v6 v6chol 512 1 0.125 1 5 0 60` | 14.91 | 56.57 | 238 |
| `./tek_hamle_v6 v4f32 1024 1 0.125 1 5 0 60` | 4.49 | 15.84 | 251 |
| `./tek_hamle_v6 v4f64 1024 1 0.125 1 5 0 60` | 5.21 | 18.87 | 251 |
| `./tek_hamle_v6 v4f32c256 1024 1 0.125 1 5 0 60` | 4.20 | 15.09 | 251 |
| `./tek_hamle_v6 v6dw_ref 1024 1 0.125 1 5 0 60` | 4.27 | 15.39 | 251 |
| `./tek_hamle_v6 v6dw_pcg 1024 1 0.125 1 5 0 60` | 4.81 | 17.16 | 275 |
| `./tek_hamle_v6 v6chol 1024 1 0.125 1 5 0 60` | 36.00 | 140.96 | 269 |
| `./tek_hamle_v6 v6chol 256 1 0.125 1 5 3 60` | 4.70 | 17.80 | 229 |
| `./tek_hamle_v6 v6chol 512 1 0.125 1 5 3 60` | 14.20 | 55.35 | 242 |
| `./tek_hamle_v6 v6dw_pcg 256 1 0.125 1 5 0 300` | 1.02 | 3.38 | 229 |
| `./tek_hamle_v6 v6dw_pcg 512 1 0.125 1 5 0 300` | 2.10 | 7.58 | 241 |
| `./tek_hamle_v6 v6dw_pcg 1024 1 0.125 1 5 0 300` | 5.30 | 19.71 | 275 |
| `./tek_hamle_v6 v4f32 256 1 0.125 1 5 0 60` | 0.92 | 3.04 | 226 |
| `./tek_hamle_v6 v4f64 256 1 0.125 1 5 0 60` | 0.96 | 3.12 | 226 |
| `./tek_hamle_v6 v4f32c256 256 1 0.125 1 5 0 60` | 0.96 | 3.13 | 226 |
| `./tek_hamle_v6 v6dw_ref 256 1 0.125 1 5 0 60` | 1.02 | 3.35 | 226 |
| `./tek_hamle_v6 v6dw_pcg 256 1 0.125 1 5 0 60` | 1.00 | 3.35 | 229 |
| `./tek_hamle_v6 v6chol 256 1 0.125 1 5 0 60` | 4.21 | 15.86 | 228 |
| `./tek_hamle_v6 v4f32 512 1 0.125 1 5 0 60` | 1.98 | 6.98 | 234 |
| `./tek_hamle_v6 v4f64 512 1 0.125 1 5 0 60` | 2.20 | 7.74 | 234 |
| `./tek_hamle_v6 v4f32c256 512 1 0.125 1 5 0 60` | 1.94 | 6.72 | 234 |
| `./tek_hamle_v6 v6dw_ref 512 1 0.125 1 5 0 60` | 2.05 | 7.15 | 234 |
| `./tek_hamle_v6 v6dw_pcg 512 1 0.125 1 5 0 60` | 2.04 | 7.28 | 241 |
| `./tek_hamle_v6 v6chol 512 1 0.125 1 5 0 60` | 13.33 | 51.75 | 238 |
| `./tek_hamle_v6 v4f32 1024 1 0.125 1 5 0 60` | 4.28 | 15.42 | 251 |
| `./tek_hamle_v6 v4f64 1024 1 0.125 1 5 0 60` | 5.06 | 18.54 | 251 |
| `./tek_hamle_v6 v4f32c256 1024 1 0.125 1 5 0 60` | 4.08 | 14.62 | 251 |
| `./tek_hamle_v6 v6dw_ref 1024 1 0.125 1 5 0 60` | 4.33 | 15.58 | 251 |
| `./tek_hamle_v6 v6dw_pcg 1024 1 0.125 1 5 0 60` | 4.64 | 16.86 | 275 |
| `./tek_hamle_v6 v6chol 1024 1 0.125 1 5 0 60` | 31.70 | 124.27 | 269 |
| `./tek_hamle_v6 v4f32 256 1 0.125 1 5 0 60` | 1.02 | 3.27 | 226 |
| `./tek_hamle_v6 v4f64 256 1 0.125 1 5 0 60` | 1.03 | 3.35 | 226 |
| `./tek_hamle_v6 v4f32c256 256 1 0.125 1 5 0 60` | 0.98 | 3.22 | 226 |
| `./tek_hamle_v6 v6dw_ref 256 1 0.125 1 5 0 60` | 0.98 | 3.23 | 226 |
| `./tek_hamle_v6 v6dw_pcg 256 1 0.125 1 5 0 60` | 1.05 | 3.46 | 229 |
| `./tek_hamle_v6 v6chol 256 1 0.125 1 5 0 60` | 4.05 | 15.34 | 228 |
| `./tek_hamle_v6 v4f32 512 1 0.125 1 5 0 60` | 1.89 | 6.49 | 234 |
| `./tek_hamle_v6 v4f64 512 1 0.125 1 5 0 60` | 2.16 | 7.60 | 234 |
| `./tek_hamle_v6 v4f32c256 512 1 0.125 1 5 0 60` | 1.94 | 6.78 | 234 |
| `./tek_hamle_v6 v6dw_ref 512 1 0.125 1 5 0 60` | 1.94 | 6.76 | 234 |
| `./tek_hamle_v6 v6dw_pcg 512 1 0.125 1 5 0 60` | 1.96 | 6.99 | 241 |
| `./tek_hamle_v6 v6chol 512 1 0.125 1 5 0 60` | 12.28 | 47.93 | 238 |
| `./tek_hamle_v6 mikro 256 1` | 0.17 | 0.01 | 215 |
| `./tek_hamle_v6 mikro 512 1` | 0.18 | 0.01 | 217 |
| `./tek_hamle_v6 mikro 1024 1` | 0.17 | 0.02 | 220 |
| `./tek_hamle_v6 mikro 256 1` | 2.60 | 9.51 | 258 |
| `./tek_hamle_v6 mikro 512 1` | 8.22 | 30.14 | 271 |
| `./tek_hamle_v6 mikro 1024 1` | 21.20 | 77.69 | 312 |

Toplam çalışma süresi: ana deney betiği 241 sn (limit 1200 sn) + mikro kıyas ve zorla-fallback tekrarı ≈ 1 dk; hiçbir koşu atlanmadı (`atlanan.txt` yok).

## Ham terminal çıktısı örneği (`./tek_hamle_v6 v6dw_pcg 256 1 0.125 1 5 0 60`)
```
V6 mod=v6dw_pcg H=256 tohum=1 gamma=0.125 lambda=1 gorev=5 zorla_fb=0 maxit=60 (4 cekirdek)
ADIM|v6dw_pcg|1|0.125|1|256|1|257|-1.000000|0.000284|0.000284|-1.0000|0.9991|0.9991|0.0726|0.0290|0.0041|0.0007|0.0337|-
GOREV|v6dw_pcg|1|256|1|1|0.000284|0.9991
ADIM|v6dw_pcg|1|0.125|1|256|2|257|0.003805|0.010325|0.007008|0.9910|0.9770|0.9841|0.0670|0.0262|0.0035|0.0122|0.0418|FALLBACK pcg_it=60 relres=1.7e-09
GOREV|v6dw_pcg|1|256|2|1|0.003805|0.9910
GOREV|v6dw_pcg|1|256|2|2|0.010325|0.9770
ADIM|v6dw_pcg|1|0.125|1|256|3|257|0.013072|0.015858|0.013938|0.9670|0.9632|0.9658|0.0615|0.0239|0.0000|0.0047|0.0286|pcg_it=28 relres=8.6e-10
GOREV|v6dw_pcg|1|256|3|1|0.006840|0.9858
GOREV|v6dw_pcg|1|256|3|2|0.019527|0.9476
GOREV|v6dw_pcg|1|256|3|3|0.015858|0.9632
ADIM|v6dw_pcg|1|0.125|1|256|4|257|0.019079|0.020361|0.019397|0.9486|0.9386|0.9461|0.0650|0.0227|0.0000|0.0056|0.0283|pcg_it=40 relres=6.7e-10
GOREV|v6dw_pcg|1|256|4|1|0.009337|0.9806
GOREV|v6dw_pcg|1|256|4|2|0.026275|0.9192
GOREV|v6dw_pcg|1|256|4|3|0.022233|0.9445
GOREV|v6dw_pcg|1|256|4|4|0.020361|0.9386
ADIM|v6dw_pcg|1|0.125|1|256|5|257|0.025384|0.034674|0.027226|0.9238|0.8850|0.9161|0.0594|0.0204|0.0000|0.0062|0.0266|pcg_it=46 relres=6.2e-10
GOREV|v6dw_pcg|1|256|5|1|0.011338|0.9783
GOREV|v6dw_pcg|1|256|5|2|0.033267|0.8947
GOREV|v6dw_pcg|1|256|5|3|0.031866|0.8975
GOREV|v6dw_pcg|1|256|5|4|0.026121|0.9204
GOREV|v6dw_pcg|1|256|5|5|0.034674|0.8850
OZET_ISLEM|v6dw_pcg|H=256|toplam_sn=0.87|tepe_RAM_MB=229|fallback_sayisi=1
```
(Biçim: `ADIM|mod|tohum|γ|λ|H|görev|m|test_MSE_eski|yeni|toplam|doğr_eski|yeni|toplam|özellik_sn|istatistik_sn|Cholesky_sn|çözüm_sn|güncelleme_sn|ek`; `GOREV|mod|tohum|H|t|i|test_MSE_i|doğr_i`.)

## Hatalar / düzeltmeler
- Mikro kıyas modunu eklerken ilk derleme `gsh` değişkeni hatasıyla başarısız oldu, bu yüzden ilk mikro çağrısı eski ikiliyle "bilinmeyen mod" verdi (ham stderr korundu: `v6_ham/v6log/mikro_H*_ilk_deneme_eski_ikili_bilinmeyen_mod.stderr`); zorla-fallback tekrarı eski ikili ile (aynı kod yolu) koşuldu, sonra düzeltilip mikro kıyas yeni ikiliyle koşuldu. Ana deney (`v4*`, `v6*` modları) mikro kıyas eklenmeden önceki ikiliyle alındı; mikro kıyas yalnız ek bir dal.
- Başka hata yok.
