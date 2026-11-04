# Tek hamle V17 raporu — kalibrasyon sonrası optimum seçimi

Branch: `claude/formul-avcisi`. Referans: `TEK_HAMLE_V16_RAPOR.md`. V8–V16 dosyaları değişmedi (V17 yeni dosyada). Ağırlık formülü değişmedi (V12 ridge + Cholesky, 256 satırlık FP64 birikim); yalnız seçim kuralı. Adam/RLS/epok yok. 10 dk sınırı içinde.
Kod: `cpp/tek_hamle_v17.cpp`. Ham loglar (tüm 16 konfigürasyonun doğrulama sonuçları dahil): `v17_ham/v17log/`.

## Yöntem
Her (γ,λ) için eğitim-yalnız modelin doğrulama skorları bir kez hesaplandı; hepsine aynı V9 kalibrasyonu (`P=(1−α)P_unigram+α softmax(z/T)`; α ∈ {0, .05, .1, .15, .2, .25, .35, .5, .75, 1}, T ∈ {0,005; 0,01; 0,02; 0,03; 0,05; 0,1; 0,25; 0,5; 1; 2; 4}). γ ∈ {0,5; 1; 2; 4}, λ ∈ {1; 10; 100; 1000}.
- **Eski (V12) kural**: (γ,λ) kalibrasyondan **önceki** ham doğrulama CE'siyle (α=1, V8 sıcaklık ızgarası); sonra o konfigürasyon için V9 kalibrasyonu doğrulamada.
- **Yeni (V17) kural**: (γ,λ,α,T) birlikte **kalibre doğrulama CE'si** en düşük olan.
- Seçilen tek konfigürasyon eğitim+doğrulama istatistiğiyle yeniden çözülüp test kümesinde bir kez değerlendirildi; test hiçbir seçimde kullanılmadı. Unigram: doğrulama için eğitimden, test için eğitim+doğrulamadan (V9 protokolü). Aynı veri/bölme (50 000 token, 80/10/10 blok), `ref` özelliği, çift-yön W₁.

## Açık cevap
**Evet: ağırlık formülünü değiştirmeden, yalnız seçim kuralını kalibrasyon sonrası doğrulama CE'sine çevirerek perplexity düştü.** H=512'de 217.0 → 212.3 (−4.7 PPL; test CE 5.3800 → 5.3582 nat); H=1024'te 218.3 → 206.8 (−11.5 PPL; test CE 5.3860 → 5.3318 nat). 3 tohumun hepsinde aynı yönde.
- [Kesin] V12'nin 216,8'i (H=512, tohum 1) bire bir tekrarlandı; yeni kural aynı tohumda 213,1.
- [Kesin] Kazanç yalnız seçimden geliyor: ağırlık hesabı aynı, farklı (γ,λ) seçiliyor. Eski kural kalibrasyon öncesi ham CE'yi en iyi yapan büyük λ'ya (1000) gidiyor; kalibre kayıp için küçük λ (1–100) daha iyi.
- [Kesin] top-1/top-5 de arttı (H=1024: top-1 ≈ %15,0 → %16,1, top-5 ≈ %29,0 → %30,0); top-1 trigramın (%16,0) düzeyine geldi, perplexity hâlâ çok uzak (≈ 207 vs 137).
- Trigram hedefi (137) sağlanmadı.

## Sonuçlar (3 tohum, c=2)
| H | seçim | PPL tohum 1 / 2 / 3 | PPL ort. | test CE ort. | top-1 ort. | top-5 ort. | seçilen (γ, λ, α, T) tohum 1 / 2 / 3 | ağırlık üretimi sn (ort.) |
|---|---|---|---|---|---|---|---|---|
| 512 | V12 eski (γ,λ ham CE ile) | 216.82 / 216.91 / 217.29 | 217.0 | 5.3800 | 0.1497 | 0.2893 | (4, 1000, 0.25, 0.01) ; (4, 1000, 0.2, 0.01) ; (4, 1000, 0.2, 0.01) | 0.50 |
| 512 | **V17 yeni (hepsi kalibre doğrulama CE ile)** | 213.10 / 211.43 / 212.48 | 212.3 | 5.3582 | 0.1529 | 0.2948 | (1, 1, 0.2, 0.01) ; (2, 1, 0.2, 0.01) ; (1, 1, 0.2, 0.01) | 0.36 |
| 1024 | V12 eski (γ,λ ham CE ile) | 218.28 / 218.34 / 218.38 | 218.3 | 5.3860 | 0.1505 | 0.2903 | (2, 1000, 0.25, 0.01) ; (2, 1000, 0.25, 0.01) ; (2, 1000, 0.25, 0.01) | 1.44 |
| 1024 | **V17 yeni (hepsi kalibre doğrulama CE ile)** | 206.37 / 207.82 / 206.28 | 206.8 | 5.3318 | 0.1618 | 0.2999 | (4, 10, 0.2, 0.01) ; (4, 100, 0.2, 0.01) ; (2, 1, 0.2, 0.01) | 1.45 |

Unigram PPL 281,15 (CE 5,6389); trigram PPL 137,4 (CE 4,923, top-1 0,160, top-5 0,362).

## Tüm konfigürasyonların doğrulama sonuçları (tohum 1; tüm tohumlar ham logda)

**H=512, tohum 1**
```
TUM KONFIGURASYONLAR (dogrulama): gamma lambda | ham CE (V8 T izgarasi, a=1) | kalibre CE (V9 izgarasi) | (alpha, T)
  0.5 1 | 6.4762 | 5.4832 | a=0.2 T=0.01
  0.5 10 | 6.4256 | 5.5011 | a=0.2 T=0.01
  0.5 100 | 6.3862 | 5.5198 | a=0.2 T=0.01
  0.5 1000 | 6.5303 | 5.5773 | a=0.15 T=0.005
  1 1 | 6.4764 | 5.4459 | a=0.2 T=0.01
  1 10 | 6.4676 | 5.4648 | a=0.2 T=0.01
  1 100 | 6.3935 | 5.4948 | a=0.2 T=0.01
  1 1000 | 6.4279 | 5.5424 | a=0.2 T=0.01
  2 1 | 6.4667 | 5.4464 | a=0.2 T=0.01
  2 10 | 6.4728 | 5.4468 | a=0.2 T=0.01
  2 100 | 6.4180 | 5.4574 | a=0.2 T=0.01
  2 1000 | 6.3757 | 5.5008 | a=0.25 T=0.01
  4 1 | 6.4697 | 5.4471 | a=0.2 T=0.01
  4 10 | 6.4645 | 5.4471 | a=0.2 T=0.01
  4 100 | 6.4287 | 5.4479 | a=0.2 T=0.01
  4 1000 | 6.3596 | 5.4671 | a=0.25 T=0.01
```

**H=1024, tohum 1**
```
TUM KONFIGURASYONLAR (dogrulama): gamma lambda | ham CE (V8 T izgarasi, a=1) | kalibre CE (V9 izgarasi) | (alpha, T)
  0.5 1 | 6.4806 | 5.4692 | a=0.2 T=0.01
  0.5 10 | 6.4490 | 5.4970 | a=0.2 T=0.01
  0.5 100 | 6.3842 | 5.5086 | a=0.2 T=0.01
  0.5 1000 | 6.4597 | 5.5559 | a=0.2 T=0.01
  1 1 | 6.4358 | 5.4261 | a=0.2 T=0.01
  1 10 | 6.4809 | 5.4475 | a=0.2 T=0.01
  1 100 | 6.4164 | 5.4831 | a=0.2 T=0.01
  1 1000 | 6.3887 | 5.5202 | a=0.25 T=0.01
  2 1 | 6.4351 | 5.4192 | a=0.2 T=0.01
  2 10 | 6.4301 | 5.4195 | a=0.2 T=0.01
  2 100 | 6.4603 | 5.4355 | a=0.2 T=0.01
  2 1000 | 6.3684 | 5.4760 | a=0.25 T=0.01
  4 1 | 6.4291 | 5.4182 | a=0.2 T=0.01
  4 10 | 6.4284 | 5.4179 | a=0.2 T=0.01
  4 100 | 6.4285 | 5.4181 | a=0.2 T=0.01
  4 1000 | 6.3797 | 5.4385 | a=0.2 T=0.01
```

## Notlar / sınırlar
- [Kesin] **Seçim ızgara kenarında**: yeni kural çoğunlukla λ=1'i (ızgaranın alt ucu; H=512'de 3/3, H=1024'te 1/3, diğerleri λ=10 ve 100) seçti. Şartname ızgarasında kaldım; λ<1 denenmedi, daha fazla kazanç olabilir [Tahmin].
- α=0,2 ve T=0,01 her koşuda seçildi (kararlı); γ değişken (1–4).
- Ham CE ile seçilen (γ,λ)'nın kalibrasyon sonrası uygun olmadığı doğrulandı: seçim kuralı sonuca H=1024'te 11 PPL'e kadar etki ediyor. Bu, V11/V12/V16'daki bazı karşılaştırmaların (kimlik torbası, log hedef) yeniden değerlendirilmesi gerekebileceğini gösterir [Muhtemel; bu raporda yeniden koşulmadı].
- Seçim doğrulama kümesine uyum riski: 4 980 doğrulama örneği, 16 (γ,λ) × 140 (α,T) aday. Test kazancı aynı yönde, ama bu riski sıfırlamaz.
- Tek metin, tek veri bölünmesi; 3 tohum yalnız W₁ tohumları.

## Süre ve RAM
Ağırlık üretimi (W₁ + eğitim+doğrulama istatistiği + 1 Cholesky/çözüm): H=512 ≈ 0,33–0,53 sn, H=1024 ≈ 1,37–1,55 sn. Yeni seçim kuralı ek ağırlık hesabı gerektirmedi (16 konfigürasyonun doğrulama skorları zaten hesaplanıyor; kalibrasyon C++ içinde, skor dökümü yok). Toplam süre/RAM (`manifest.tsv`):

| komut | duvar sn | kullanıcı CPU sn | tepe RAM MB |
|---|---|---|---|
| `./tek_hamle_v17 2 512 1` | 14.1 | 45.3 | 230 |
| `./tek_hamle_v17 2 512 2` | 13.8 | 45.3 | 230 |
| `./tek_hamle_v17 2 512 3` | 11.9 | 40.6 | 230 |
| `./tek_hamle_v17 2 1024 1` | 25.3 | 79.8 | 410 |
| `./tek_hamle_v17 2 1024 2` | 25.5 | 82.7 | 410 |
| `./tek_hamle_v17 2 1024 3` | 25.5 | 84.2 | 410 |

## Komutlar (gerçek)
Derleme: `g++ -O3 -march=native -fopenmp -o tek_hamle_v17 tek_hamle_v17.cpp`. Çalıştırma: `./tek_hamle_v17 2 H TOHUM` (H=512, 1024; tohum 1–3). Girdi: `/tmp/v8data` (V8 veri hazırlığı + `uni_tr.f64`, `uni_trva.f64`).
