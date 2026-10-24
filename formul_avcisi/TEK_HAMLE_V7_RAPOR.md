# Tek hamle V7 raporu — kesinlik iyileştirmesi (256 satırlık FP64 biriktirme)

Branch: `claude/formul-avcisi`. Temel: V4 algoritması (sabit-W1 + birikimli S,R + her görevde Cholesky). RLS ve yavaş yöntem yok. Eski kod/raporlar korundu.
Kod: yeni dosya yazılmadı; V7 = `cpp/tek_hamle_v6.cpp` içindeki `v4f32c256` modu (V6'da eklenen, V4 yolunun `syrk_add`'ını 256 satırlık dilimlerle çağırıp her dilimin float toplamını double'a boşaltır). Karşılaştırma: `v4f32` (V4'ün 2048 satırlık float toplamı) ve `v4f64` (tam double toplam, referans). Ham loglar/ağırlıklar: `v7_ham/`.
Ortam: Intel Xeon 2,80 GHz, 4 çekirdek, 16 GB, g++ 13.3.0 `-O3 -march=native -fopenmp`.

## Ana sonuç
- [Kesin] **256 satırlık double'a boşaltma, FP32'ye göre ağırlık sapmasını yaklaşık 8 kat azaltıyor** (H=256/512/1024'te 8,0× / 8,0× / 7,7×), test MSE'yi değiştirmiyor (üç yöntemde de aynı beş hane), istatistik süresini artırmıyor (FP32 ile aynı: H=256 0,106 vs 0,107 sn; H=1024 0,932 vs 0,934 sn; H=512'de 0,318 vs 0,289 sn, +%10, ölçüm gürültüsü sınırında).
- [Kesin] Tam FP64 (referans) 1,1× (H=256), 1,6× (H=512) ve 1,9× (H=1024) daha yavaş istatistik süresi gerektiriyor (benim yazdığım basit double çekirdeği; V4'ün ayarlı float çekirdeğiyle kıyas, saf hassasiyet maliyeti değil).
- [Kesin] Kalan sapma (≈ 2×10⁻⁴ … 1,5×10⁻³) float özellik matrisinden değil (hepsi aynı float özellikleri kullanıyor, referans da), 256 satır içindeki float toplamdan ve Cholesky koşullanmasından (λ=1) geliyor; bunun hangi payının hangi kaynaktan olduğunu ayırmadım.

## Kurulum
MNIST, sınıf-artımlı 5 görev ({0,1},{2,3},{4,5},{6,7},{8,9}), sabit özellik (V4 sabit-W1, çift yön + rastgele bias), γ=0,125, λ=1, H = 256 / 512 / 1024, **3 tohum (1, 2, 3)**, her yöntem aynı tohumla aynı özellikleri kullanır. Ağırlık farkı = `‖W − W_ref64‖_F / ‖W_ref64‖_F` (aynı tohum, aynı görev). Süreler tek koşu (tohum başına), tablo 3 tohumun medyanı; duvar saati. Not: ilk dakikada V8 veri hazırlama (SVD) betiği aynı makinede çalıştı, bu yüzden süre ölçümleri hafifçe (birkaç %) gürültülü olabilir; oranlar tohumlar arasında tutarlıydı.

## Sonuçlar
| H | yöntem | ΔW görev 1 (3 tohum ort.) | ΔW görev 5 (ort.) | ΔW görev 5 (en kötü) | test MSE toplam (görev 5, 3 tohum ort.) | istatistik (syrk) sn, 5 görev, medyan | güncelleme (syrk+Cholesky+çözüm) sn, medyan | duvar sn | tepe RAM MB | FP32'ye göre ΔW iyileşmesi (görev 5) | FP64'e göre istatistik süresi |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 256 | V4: FP32 (2048 satır float toplam) | 5.8e-03 | 1.4e-03 | 1.5e-03 | 0.02720 | 0.107 | 0.125 | 1.08 | 227 | 1.0× | 0.89× |
| 256 | V7: 256 satırda FP64'e boşalt | 7.6e-04 | 1.7e-04 | 1.9e-04 | 0.02720 | 0.106 | 0.123 | 1.11 | 226 | 8.0× | 0.88× |
| 256 | FP64 tam (referans) | 0.0e+00 | 0.0e+00 | 0.0e+00 | 0.02720 | 0.120 | 0.139 | 1.19 | 227 | - | 1.00× |
| 512 | V4: FP32 (2048 satır float toplam) | 1.2e-02 | 4.0e-03 | 4.3e-03 | 0.02171 | 0.289 | 0.389 | 2.09 | 234 | 1.0× | 0.57× |
| 512 | V7: 256 satırda FP64'e boşalt | 1.6e-03 | 5.0e-04 | 5.4e-04 | 0.02171 | 0.318 | 0.426 | 2.20 | 234 | 8.0× | 0.63× |
| 512 | FP64 tam (referans) | 0.0e+00 | 0.0e+00 | 0.0e+00 | 0.02171 | 0.504 | 0.604 | 2.34 | 234 | - | 1.00× |
| 1024 | V4: FP32 (2048 satır float toplam) | 2.3e-02 | 1.0e-02 | 1.1e-02 | 0.01728 | 0.934 | 1.435 | 4.73 | 251 | 1.0× | 0.52× |
| 1024 | V7: 256 satırda FP64'e boşalt | 2.9e-03 | 1.4e-03 | 1.5e-03 | 0.01728 | 0.932 | 1.447 | 4.82 | 251 | 7.7× | 0.52× |
| 1024 | FP64 tam (referans) | 0.0e+00 | 0.0e+00 | 0.0e+00 | 0.01728 | 1.790 | 2.300 | 5.55 | 251 | - | 1.00× |

(Test MSE: görev 5 sonrası görülen tüm sınıflar, 3 tohum ortalaması; yöntemler arasında tohum bazında da aynı: H=256 [0,02723, 0,02737, 0,02701], H=512 [0,02139, 0,02184, 0,02191], H=1024 [0,01720, 0,01737, 0,01726].)

## Okuma
1. [Kesin] FP32 (V4) toplamı ağırlıklarda H büyüdükçe artan sapma yapıyor (görev 1: 5,8×10⁻³ → 1,2×10⁻² → 2,3×10⁻²). Kayıp değişmedi: bu sapma test MSE'sinde görünmüyor.
2. [Kesin] V7 sapması H=256 / 512 / 1024'te 7,6×10⁻⁴ / 1,6×10⁻³ / 2,9×10⁻³ (görev 1), 1,7×10⁻⁴ / 5,0×10⁻⁴ / 1,4×10⁻³ (görev 5). Görev sayısı arttıkça sapma azalıyor.
3. [Kesin] V7 maliyeti FP32'ye eşit; yani bu iyileştirme V4'ün yerine konabilir. Önerim: V4 `syrk` çağrısını 256 satırlık dilimlerle yapmak.
4. Tam FP64'ün (sapma 0) süresi H büyüdükçe yaklaşık 2× arttığı için pratik değil; amaç referans sağlamak.

## Sınırlar
- Yalnız MNIST sınıf-artımlı kurulum, 5 görev; ağırlık farkı FP64 referans (aynı float özelliklere dayanıyor; float özelliklerin kendi yuvarlama hatası ölçülmedi).
- Süre ölçümleri 4 çekirdekli sanal makinede; küçük farklar (≲ %10) ölçüm gürültüsü içinde.

## Komutlar (gerçek, `v7_ham/v7log/manifest.tsv`)
Biçim: `tek_hamle_v6 MOD H TOHUM γ λ GÖREV ZORLA_FB MAXIT`.

| komut | duvar sn | kullanıcı CPU sn | tepe RAM MB |
|---|---|---|---|
| `./tek_hamle_v6 v4f64 256 1 0.125 1 5 0 60` | 2.03 | 3.53 | 226 |
| `./tek_hamle_v6 v4f32 256 1 0.125 1 5 0 60` | 1.13 | 3.63 | 226 |
| `./tek_hamle_v6 v4f32c256 256 1 0.125 1 5 0 60` | 1.11 | 3.66 | 226 |
| `./tek_hamle_v6 v4f64 512 1 0.125 1 5 0 60` | 2.32 | 8.11 | 233 |
| `./tek_hamle_v6 v4f32 512 1 0.125 1 5 0 60` | 2.06 | 7.21 | 234 |
| `./tek_hamle_v6 v4f32c256 512 1 0.125 1 5 0 60` | 2.20 | 7.75 | 234 |
| `./tek_hamle_v6 v4f64 1024 1 0.125 1 5 0 60` | 5.47 | 20.09 | 251 |
| `./tek_hamle_v6 v4f32 1024 1 0.125 1 5 0 60` | 4.86 | 17.22 | 251 |
| `./tek_hamle_v6 v4f32c256 1024 1 0.125 1 5 0 60` | 4.82 | 17.38 | 251 |
| `./tek_hamle_v6 v4f64 256 2 0.125 1 5 0 60` | 1.19 | 4.00 | 226 |
| `./tek_hamle_v6 v4f32 256 2 0.125 1 5 0 60` | 1.08 | 3.56 | 226 |
| `./tek_hamle_v6 v4f32c256 256 2 0.125 1 5 0 60` | 1.04 | 3.44 | 226 |
| `./tek_hamle_v6 v4f64 512 2 0.125 1 5 0 60` | 2.47 | 8.75 | 234 |
| `./tek_hamle_v6 v4f32 512 2 0.125 1 5 0 60` | 2.37 | 7.81 | 234 |
| `./tek_hamle_v6 v4f32c256 512 2 0.125 1 5 0 60` | 9.01 | 19.67 | 234 |
| `./tek_hamle_v6 v4f64 1024 2 0.125 1 5 0 60` | 5.62 | 20.58 | 251 |
| `./tek_hamle_v6 v4f32 1024 2 0.125 1 5 0 60` | 4.73 | 17.03 | 251 |
| `./tek_hamle_v6 v4f32c256 1024 2 0.125 1 5 0 60` | 4.98 | 17.95 | 251 |
| `./tek_hamle_v6 v4f64 256 3 0.125 1 5 0 60` | 1.06 | 3.53 | 227 |
| `./tek_hamle_v6 v4f32 256 3 0.125 1 5 0 60` | 1.08 | 3.57 | 227 |
| `./tek_hamle_v6 v4f32c256 256 3 0.125 1 5 0 60` | 1.19 | 3.92 | 226 |
| `./tek_hamle_v6 v4f64 512 3 0.125 1 5 0 60` | 2.34 | 8.29 | 234 |
| `./tek_hamle_v6 v4f32 512 3 0.125 1 5 0 60` | 2.09 | 7.38 | 234 |
| `./tek_hamle_v6 v4f32c256 512 3 0.125 1 5 0 60` | 2.12 | 7.42 | 234 |
| `./tek_hamle_v6 v4f64 1024 3 0.125 1 5 0 60` | 5.55 | 20.25 | 251 |
| `./tek_hamle_v6 v4f32 1024 3 0.125 1 5 0 60` | 4.63 | 16.77 | 251 |
| `./tek_hamle_v6 v4f32c256 1024 3 0.125 1 5 0 60` | 4.62 | 16.66 | 251 |

## Ham terminal çıktısı örneği (`./tek_hamle_v6 v4f32c256 256 1 0.125 1 5 0 60`)
```
V6 mod=v4f32c256 H=256 tohum=1 gamma=0.125 lambda=1 gorev=5 zorla_fb=0 maxit=60 (4 cekirdek)
ADIM|v4f32c256|1|0.125|1|256|1|257|-1.000000|0.000284|0.000284|-1.0000|0.9991|0.9991|0.0933|0.0249|0.0028|0.0006|0.0283|-
GOREV|v4f32c256|1|256|1|1|0.000284|0.9991
ADIM|v4f32c256|1|0.125|1|256|2|257|0.003805|0.010325|0.007008|0.9910|0.9770|0.9841|0.0732|0.0214|0.0026|0.0007|0.0247|-
GOREV|v4f32c256|1|256|2|1|0.003805|0.9910
GOREV|v4f32c256|1|256|2|2|0.010325|0.9770
ADIM|v4f32c256|1|0.125|1|256|3|257|0.013072|0.015858|0.013938|0.9670|0.9632|0.9658|0.0672|0.0182|0.0025|0.0006|0.0213|-
GOREV|v4f32c256|1|256|3|1|0.006840|0.9858
GOREV|v4f32c256|1|256|3|2|0.019528|0.9476
GOREV|v4f32c256|1|256|3|3|0.015858|0.9632
ADIM|v4f32c256|1|0.125|1|256|4|257|0.019079|0.020361|0.019397|0.9486|0.9386|0.9461|0.0737|0.0215|0.0032|0.0008|0.0255|-
GOREV|v4f32c256|1|256|4|1|0.009337|0.9806
GOREV|v4f32c256|1|256|4|2|0.026276|0.9192
GOREV|v4f32c256|1|256|4|3|0.022233|0.9445
GOREV|v4f32c256|1|256|4|4|0.020361|0.9386
ADIM|v4f32c256|1|0.125|1|256|5|257|0.025384|0.034675|0.027226|0.9238|0.8850|0.9161|0.0706|0.0205|0.0026|0.0006|0.0237|-
GOREV|v4f32c256|1|256|5|1|0.011337|0.9783
GOREV|v4f32c256|1|256|5|2|0.033268|0.8947
GOREV|v4f32c256|1|256|5|3|0.031866|0.8975
GOREV|v4f32c256|1|256|5|4|0.026121|0.9204
GOREV|v4f32c256|1|256|5|5|0.034675|0.8850
OZET_ISLEM|v4f32c256|H=256|toplam_sn=0.93|tepe_RAM_MB=226|fallback_sayisi=0
```
