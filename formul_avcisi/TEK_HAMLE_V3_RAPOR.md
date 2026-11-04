# Tek hamle V3 raporu — W1/bias kontrolleri + matematiksel hafıza

Branch: `claude/formul-avcisi`. Kural: gradyan yok, Adam yok, epok yok; tek çözüm `W = (S + λI)⁻¹ R`, Cholesky (ters alınmadı).
Kod: `cpp/tek_hamle_v3.cpp` (V2 dosyaları değişmedi). Ham çıktılar: `v3_ham/` (kosu1_yon/, kosu2_ve_hafiza/, ortam.txt).
Veri: MNIST, 55k fit / 5k doğrulama / 10k test. γ ve λ yalnız doğrulama kümesinden seçildi; test seçimde kullanılmadı.

## Kapsam değişikliği (kullanıcı kararı)
Tam ikinci koşu iptal edildi. Yapılanlar: (1) W1/bias kontrolleri tam (6 mod × H=1024,2048 × 3 tohum); (2) hafıza yalnız H=1024, μ ∈ {0,1}, kazanan özellikle;
(3) yalnız kazanan yöntem aynı tohum ve komutla bağımsız tekrar edildi. H=2048 hafıza taraması YAPILMADI. μ=0.5/2/5 bu raporda YOK (kod destekliyor, koşulmadı).

## Ortam (ham: `v3_ham/ortam.txt`)
Intel Xeon @ 2,80 GHz, 4 çekirdek (1 iş parçacığı/çekirdek), AVX2/AVX-512, L2 4 MiB, L3 33 MiB, 16 GB RAM, KVM sanal makine; g++ 13.3.0, `-O3 -march=native -fopenmp`; OMP_NUM_THREADS ayarsız (4 iş parçacığı).
Süre: duvar saati. Tepe RAM: koşu 1'de programın kendi `getrusage` değeri (çıktıdaki `RAM=`), koşu 2'de sarmalayıcının çocuk-süreç `maxrss` değeri.

## Deney 1+2 — W1/bias kontrolleri
Komut (her satır için): `./tek_hamle_v3 yon MOD H TOHUM`, MOD ∈ rastgele|cift|ayni|ryon_cbias|cyon_rbias|rr, H ∈ 1024,2048, TOHUM ∈ 1,2,3 (36 koşu).
Izgara: γ ∈ {0.0625,0.125,0.25,0.5,1,2}, λ ∈ {1,10,100,1000}. Aynı tohum aynı örnek çiftlerini verir (eşleştirilmiş). Rastgele yön: N(0,I) → birim norm. Rastgele bias: N(0,σ_b²), σ_b = aynı çiftlerin V2 bias'larının std'si.

| H | yöntem | test ort. (%) | test, tohum 1/2/3 | doğrulama ort. (%) | seçilen γ | seçilen λ | tek model sn | ayar dahil toplam sn | tepe RAM MB |
|---|---|---|---|---|---|---|---|---|---|
| 1024 | rastgele ELM (referans) | 93.37 | 93.30 / 93.50 / 93.30 | 94.84 | 2,1,1 | 10,1,10 | 2.9 | 19.9 | 332 |
| 1024 | çift, farklı sınıf (V2) | 96.03 | 96.14 / 95.99 / 95.96 | 96.97 | 0.125,0.125,0.125 | 1,1,10 | 2.9 | 20.5 | 332 |
| 1024 | çift, aynı sınıf | 96.09 | 96.08 / 96.15 / 96.03 | 97.07 | 0.125,0.125,0.25 | 1,1,1 | 2.9 | 20.3 | 332 |
| 1024 | rastgele yön + çift bias | 93.80 | 93.76 / 93.75 / 93.90 | 95.27 | 1,1,1 | 10,100,1 | 3.0 | 20.4 | 332 |
| 1024 | çift yön + rastgele bias | 96.43 | 96.43 / 96.41 / 96.46 | 97.25 | 0.125,0.25,0.125 | 1,1,1 | 2.8 | 20.3 | 332 |
| 1024 | rastgele yön + rastgele bias | 94.13 | 94.15 / 94.10 / 94.14 | 95.61 | 0.5,0.5,0.5 | 1,1,1 | 2.9 | 19.8 | 332 |
| 2048 | rastgele ELM (referans) | 95.51 | 95.51 / 95.45 / 95.58 | 96.45 | 1,1,1 | 1,1,1 | 7.9 | 63.3 | 553 |
| 2048 | çift, farklı sınıf (V2) | 97.17 | 96.95 / 97.28 / 97.29 | 97.81 | 0.125,0.125,0.125 | 1,1,1 | 8.0 | 62.7 | 553 |
| 2048 | çift, aynı sınıf | 97.08 | 97.10 / 96.98 / 97.15 | 97.76 | 0.125,0.125,0.25 | 1,1,1 | 7.9 | 63.3 | 553 |
| 2048 | rastgele yön + çift bias | 95.59 | 95.68 / 95.46 / 95.63 | 96.71 | 1,1,1 | 1,1,1 | 8.2 | 61.8 | 553 |
| 2048 | çift yön + rastgele bias | 97.32 | 97.29 / 97.28 / 97.40 | 97.94 | 0.125,0.25,0.125 | 1,1,1 | 7.7 | 61.8 | 554 |
| 2048 | rastgele yön + rastgele bias | 95.72 | 95.74 / 95.76 / 95.66 | 96.88 | 0.25,0.5,0.25 | 1,1,1 | 7.8 | 60.4 | 553 |

Tek model = W1/b1 üretimi + 60k örnek için FᵀF, FᵀT + bir Cholesky (ayar araması HARİÇ). Toplam = 24 noktalı ayar ızgarası + son çözüm + test.
Izgara kenarı (γ=0,0625 veya 2): yalnız rastgele ELM H=1024 tohum 1'de (γ=2, üst kenar; diğer iki tohum γ=1). Diğer 35 koşuda seçilen γ ızgaranın içinde.

### Okuma
- [Kesin] Kazanç yönden geliyor. Yön veri çiftinden gelince H=1024'te +2,7 puan (93,37 → 96,03), H=2048'de +1,7 puan (95,51 → 97,17).
- [Kesin] Bias'ın veri orta noktasından hesaplanması katkı vermiyor: çift yön + rastgele bias (96,43 / 97,32) çift yön + çift bias'tan (96,03 / 97,17) iyi; rastgele yön + çift bias (93,80 / 95,59) rastgele ELM'den (93,37 / 95,51) yalnız 0,1–0,4 puan iyi.
- [Kesin] Etiket gerekmiyor: aynı sınıftan çift (96,09 / 97,08) farklı sınıftan çiftle (96,03 / 97,17) aynı düzeyde.
- [Kesin] Rastgele yön + rastgele bias (σ eşit) rastgele ELM'e göre +0,8 (H=1024) ve +0,2 (H=2048) puan: bias ölçeği küçük bir etki (V2'nin sabit 0,5'i yerine veriye uygun σ_b).
- [Muhtemel] Çift farkları verinin yayıldığı alt uzayda kalan yönler veriyor, rastgele Gauss yönler çoğunlukla gürültü boyutlarına düşüyor. Bunu PCA yönü veya veri alt uzayına izdüşüm kontrolüyle ölçmedim.
- Kazanan (doğrulama ortalaması en yüksek, her iki H'de): `cyon_rbias`. Çift yönteme göre fark H=1024'te +0,4, H=2048'de +0,15 puan; H=2048'de eşleşmiş tohum farkları (cyon_rbias − cift) +0,34 / 0,00 / +0,11 puan — küçük.

### Ham terminal çıktısı örneği (`./tek_hamle_v3 yon cyon_rbias 1024 1`, koşu 1)
```
YON mod=cyon_rbias H=1024 tohum=1  sigma_b(cift ciftlerinden)=3.149 ort_b=-0.147
  gamma=0.0625 lambda=   1 val=0.9700
  gamma=0.0625 lambda=  10 val=0.9600
  gamma=0.0625 lambda= 100 val=0.9344
  gamma=0.0625 lambda=1000 val=0.9026
  gamma=0.1250 lambda=   1 val=0.9720
  gamma=0.1250 lambda=  10 val=0.9714
  gamma=0.1250 lambda= 100 val=0.9644
  gamma=0.1250 lambda=1000 val=0.9454
  gamma=0.2500 lambda=   1 val=0.9716
  gamma=0.2500 lambda=  10 val=0.9712
  gamma=0.2500 lambda= 100 val=0.9700
  gamma=0.2500 lambda=1000 val=0.9616
  gamma=0.5000 lambda=   1 val=0.9688
  gamma=0.5000 lambda=  10 val=0.9678
  gamma=0.5000 lambda= 100 val=0.9668
  gamma=0.5000 lambda=1000 val=0.9642
  gamma=1.0000 lambda=   1 val=0.9644
  gamma=1.0000 lambda=  10 val=0.9644
  gamma=1.0000 lambda= 100 val=0.9640
  gamma=1.0000 lambda=1000 val=0.9622
  gamma=2.0000 lambda=   1 val=0.9586
  gamma=2.0000 lambda=  10 val=0.9586
  gamma=2.0000 lambda= 100 val=0.9590
  gamma=2.0000 lambda=1000 val=0.9584
SECILEN gamma=0.1250 lambda=1 val=0.9720  TEST=0.9643  tek_model=2.8s  arama_dahil_toplam=21.1s  RAM=332MB
OZET|cyon_rbias|1024|1|0.1250|1|0.9720|0.9643|2.8|21.1|332|0
```

## Deney 3 — V3 matematiksel hafıza (H=1024, özellik `cyon_rbias`, μ ∈ {0,1})
Komut: `./tek_hamle_v3 hafiza cyon_rbias 1024 TOHUM /tmp/mnist_bin 0,1`, TOHUM = 1,2,3.
Özellik dönüşümü (W1,b1) bir kez üretilip sabit; ardından görevler sırayla: eski ham veri tekrar kullanılmadı, yalnız S,R saklandı:
`S_acc = S_yeni + μ·S_acc`, `R_acc = R_yeni + μ·R_acc`, `W = (S_acc + λI)⁻¹ R_acc`.
Görev bölmeleri (sınıf-artımlı, 10 çıkışlı argmax): `g5` = {0,1},{2,3},{4,5},{6,7},{8,9}; `g2` = {0–4},{5–9}.
Metrikler test kümesinde, SON görevden sonra: eski = önceki görevlerin sınıflarındaki doğruluk, yeni = son görev, toplam = 10 sınıf.
Unutma = ortalama (görev i'yi öğrendikten hemen sonraki doğruluğu − sonda aynı görevin doğruluğu), son görev hariç.

| bölme | μ | eski (%) | yeni (%) | toplam (%) | unutma (puan) | tohum bazlı toplam |
|---|---|---|---|---|---|---|
| g5 | 0 | 0,00 | 99,07 | 19,65 | 99,78 | 19,64 / 19,69 / 19,61 |
| g5 | 1 | 96,79 | 95,16 | 96,47 | 1,91 | 96,48 / 96,41 / 96,51 |
| g2 | 0 | 0,00 | 98,05 | 47,66 | 99,08 | 47,57 / 47,70 / 47,72 |
| g2 | 1 | 97,28 | 95,61 | 96,47 | 1,87 | 96,48 / 96,41 / 96,51 |

- [Kesin] μ=0 (eski istatistik atılıyor): eski görevler tamamen unutuluyor (eski doğruluk %0), toplam doğruluk sınıf payına iniyor (≈1/5 ve ≈1/2).
- [Kesin] μ=1: eski görevler korunuyor, toplam doğruluk %96,47 (3 tohum) ve bu birikimli V2 çözümüyle aynı (aşağıda denklik).
- [Muhtemel] μ=1'deki ~1,9 puanlık "unutma" gerçek unutma değil: bir görevde yalnız 2 (veya 5) sınıf ayırt edilirken sonda 10 sınıf ayırt ediliyor; sınıf sayısı arttıkça sınıf-artımlı doğruluk düşer. Birikimli (ortak) eğitim aynı sonucu verdiği için bu yöntemden değil görevin zorluğundan geliyor.
- Seçilen γ,λ (doğrulamadan): μ=1 için γ=0,125 / 0,25 / 0,125, λ=1; μ=0 için ayarlar farklı (ham çıktıdaki `SEC|` satırları).
- μ=0.5, 2, 5 bu raporda ölçülmedi.

### μ=1 denklik kontrolü (ham: `DENKLIK|` satırları)
Aynı fit satırları tek geçişte (karışık sırada) işlenip birikimli V2 istatistiği kuruldu; görevlerden birikimli kurulan istatistikle karşılaştırıldı (g5, H=1024):

| tohum | S göreli fark | W göreli fark | test birikimli V2 | test V3 μ=1 |
|---|---|---|---|---|
| 1 | 3,5e-7 | 1,4e-3 | 0,9648 | 0,9648 |
| 2 | 3,9e-7 | 3,6e-4 | 0,9641 | 0,9641 |
| 3 | 3,3e-7 | 1,5e-3 | 0,9651 | 0,9651 |

- [Kesin] Test doğruluğu birebir aynı. S farkı ~1e-7 (float32 toplama sırası), W farkı 3,6e-4–1,5e-3 (λ=1'de koşullanma yüzünden büyütülmüş): sayısal eşitlik, bit düzeyinde eşitlik değil.

### Süre ve RAM (hafıza, H=1024)
- Tek koşu duvar saati: 37–42 sn. Bunun ~27–29 sn'si ayar araması, ~10 sn'si son faz.
- Artımlı adım (yeni görevin istatistiği + 1 Cholesky): g5 ≈ 0,55–0,68 sn, g2 ≈ 1,2–1,5 sn. Tüm eğitim verisini baştan işleyip bir çözüm: ≈ 2,3–2,6 sn (`DENKLIK|` satırı, `birikimli_yeniden_egitim`).
- Tepe RAM 421 MB (deney kolaylığı için sınıf başına S ayrı tutuluyor: 10 × 8,4 MB + özellikler). Yöntemin gerçek durumu tek S (8,4 MB) + R.

### Ham terminal çıktısı örneği (`./tek_hamle_v3 hafiza cyon_rbias 1024 1 /tmp/mnist_bin 0,1`, koşu 1)
```
HAFIZA|cyon_rbias|1024|1|g5|1.0|0.1250|1|0.9720|0.9678|0.9526|0.9648|0.0195|0.68|0.9882|0.9613|0.9594|0.9607|0.9526
DENKLIK|cyon_rbias|1024|1|g5|S_goreli_fark=3.493e-07|W_goreli_fark=1.445e-03|test_birikimli_V2=0.9648|test_V3_mu1=0.9648|birikimli_yeniden_egitim=2.46s|artimli_adim_ort=0.68s
HAFIZA|cyon_rbias|1024|1|g2|1.0|0.1250|1|0.9720|0.9728|0.9564|0.9648|0.0191|1.53|0.9728|0.9564
HAFIZA|cyon_rbias|1024|1|g2|0.0|0.2500|1|0.4876|0.0000|0.9786|0.4757|0.9912|1.29|0.0000|0.9786
HAFIZA|cyon_rbias|1024|1|g5|0.0|0.5000|1|0.1946|0.0000|0.9904|0.1964|0.9976|0.57|0.0000|0.0000|0.0000|0.0000|0.9904
HAFIZA_SURE|cyon_rbias|1024|1|arama(faz1)=28.6s|toplam=41.7s|RAM=421MB|sinif_istatistik_toplam_6gamma=14.5s
SEC|cyon_rbias|1024|1|g5|mu=0.0|gamma=0.5000|lambda=1|val=0.1946
SEC|cyon_rbias|1024|1|g5|mu=1.0|gamma=0.1250|lambda=1|val=0.9720
SEC|cyon_rbias|1024|1|g2|mu=0.0|gamma=0.2500|lambda=1|val=0.4876
SEC|cyon_rbias|1024|1|g2|mu=1.0|gamma=0.1250|lambda=1|val=0.9720
```

## Bağımsız ikinci koşu (yalnız kazanan yöntem, aynı komut, aynı tohum)
Sarmalayıcı: `cpp/calistir_olc.py` (komutu çalıştırır, ham çıktıyı kaydeder, duvar + CPU süresi + tepe RAM ölçer). Manifest: `v3_ham/kosu2_ve_hafiza/manifest.tsv`.

| komut | duvar sn | kullanıcı CPU sn | tepe RAM MB |
|---|---|---|---|
| `./tek_hamle_v3 hafiza cyon_rbias 1024 1 /tmp/mnist_bin 0,1` | 41.89 | 141.81 | 421 |
| `./tek_hamle_v3 hafiza cyon_rbias 1024 2 /tmp/mnist_bin 0,1` | 37.61 | 127.30 | 421 |
| `./tek_hamle_v3 hafiza cyon_rbias 1024 3 /tmp/mnist_bin 0,1` | 37.42 | 126.68 | 421 |
| `./tek_hamle_v3 hafiza cyon_rbias 1024 1 /tmp/mnist_bin 0,1` | 39.72 | 135.22 | 421 |
| `./tek_hamle_v3 hafiza cyon_rbias 1024 2 /tmp/mnist_bin 0,1` | 37.58 | 127.67 | 421 |
| `./tek_hamle_v3 hafiza cyon_rbias 1024 3 /tmp/mnist_bin 0,1` | 37.51 | 127.10 | 421 |
| `./tek_hamle_v3 yon cyon_rbias 1024 1` | 19.59 | 71.73 | 332 |
| `./tek_hamle_v3 yon cyon_rbias 1024 2` | 19.69 | 72.46 | 332 |
| `./tek_hamle_v3 yon cyon_rbias 1024 3` | 18.89 | 69.60 | 332 |
| `./tek_hamle_v3 yon cyon_rbias 2048 1` | 57.43 | 210.83 | 553 |
| `./tek_hamle_v3 yon cyon_rbias 2048 2` | 58.60 | 215.05 | 553 |
| `./tek_hamle_v3 yon cyon_rbias 2048 3` | 57.28 | 209.92 | 554 |

Doğrulama (ham dosyalar program ile karşılaştırıldı):
- [Kesin] `yon cyon_rbias`, H=1024 ve 2048, tohum 1/2/3: seçilen γ, λ, doğrulama ve test doğruluğu koşu 1 ile koşu 2'de **birebir aynı** (6/6).
- [Kesin] `hafiza cyon_rbias` H=1024, tohum 1/2/3: tüm doğruluk, γ, λ, unutma ve denklik satırları (zaman alanı hariç) iki koşuda **birebir aynı** (3/3). Not: hafıza deneyinin "koşu 1"i de sarmalayıcıyla alındı; koşu 2 hafıza için ayrı bir tekrar.
- Süreler koşular arasında ~±5% değişti (hafıza tohum 1: 41,9 → 39,7 sn); doğruluk değişmedi.

## Sınırlar
- Hafıza yalnız H=1024, μ ∈ {0,1}, kazanan özellik (`cyon_rbias`), 2 bölme; H=2048 ve μ=0.5/2/5 yapılmadı.
- Özellik dönüşümü (çift yönler) tüm sınıfların fit örneklerinden kuruldu; dönüşüm geleceğin sınıflarını görmüştü. Yalnız ilk görev verisinden kurulan (`cift_A`) ve veriden bağımsız (`rastgele`) özellik varyantları kodda var, bu raporda koşulmadı.
- Koşu 1 yon sürelerinde dış sarmalayıcı yoktu; süre/RAM programın kendi ölçümü.
