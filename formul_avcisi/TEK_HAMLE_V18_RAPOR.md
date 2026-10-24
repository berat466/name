# Tek hamle V18 raporu — daha küçük λ ile ridge (λ ∈ 0,001 … 1000)

Branch: `claude/formul-avcisi`. Referans: `TEK_HAMLE_V17_RAPOR.md`. V8–V17 dosyaları değişmedi (V18 yeni dosyada). Formül aynı (V12 ridge, Cholesky, 256 satırlık FP64 birikim), V17'nin kalibrasyon-sonrası CE ile seçimi korundu; yalnız λ ızgarası {0,001; 0,01; 0,1; 0,5; 1; 10; 100; 1000} (8 değer × 4 γ = 32 konfigürasyon). Adam/RLS/epok yok. 20 dk sınırı içinde.
Kod: `cpp/tek_hamle_v18.cpp`. Ham loglar (tüm 32 konfigürasyon, 3 tohum): `v18_ham/v18log/`.
Aynı tohum, veri ve bölme (50 000 token, 80/10/10 blok, `ref` özelliği, çift-yön W₁); V17 seçimi aynı koşuda λ ≥ 1'e kısıtlanarak elde edildi (V17'nin 206,37 / 207,82 / 206,28 sonuçlarını bire bir tekrarladı).

## Açık cevap
**Hayır: daha küçük λ değerleri kaybı anlamlı biçimde düşürmedi.**
- [Kesin] **H=1024'te V18 seçimi V17 ile aynı konfigürasyonu seçti (3/3 tohum)**: test PPL **206,8** (ortalama; 206,37 / 207,82 / 206,28), top-1 %16,1, top-5 %30,0 — V17 ile birebir aynı. Küçük λ'nın doğrulama CE'si daha iyi olmadı (γ=4, λ ∈ [0,001; 10] aralığında kalibre doğrulama CE'si düz: 5,418–5,424).
- [Kesin] H=512'de çok küçük kazanç: V17 212,34 → V18 212,00 PPL (−0,34, ≈ %0,2); tohum 1: 213,10 → 212,55 (λ=0,1), tohum 3: 212,48 → 212,03 (λ=0,001), tohum 2 aynı. Tohum std'sinden küçük; anlamlı değil.
- Araştırma hedefleri (PPL < 200, top-1 > %17): **karşılanmadı**.
- Trigram (137) uzak.

## Sonuçlar (3 tohum, c=2)
| H | seçim | PPL tohum 1 / 2 / 3 | PPL ort. | test CE ort. | top-1 ort. | top-5 ort. | eğitim CE (alt örnek) ort. | doğr. CE (kalibre) ort. | seçilen (γ, λ) tohum 1 / 2 / 3 | ağırlık üretimi sn |
|---|---|---|---|---|---|---|---|---|---|---|
| 1024 | V17 (λ ≥ 1; referans) | 206.37 / 207.82 / 206.28 | 206.82 | 5.3318 | 0.1618 | 0.2999 | 5.1348 | 5.4168 | (4, 10) ; (4, 100) ; (2, 1) | 1.40 |
| 1024 | **V18 (λ ∈ 0,001…1000)** | 206.37 / 207.82 / 206.28 | 206.82 | 5.3318 | 0.1618 | 0.2999 | 5.1348 | 5.4168 | (4, 10) ; (4, 100) ; (2, 1) | 1.40 |
| 512 | V17 (λ ≥ 1; referans) | 213.10 / 211.43 / 212.48 | 212.34 | 5.3582 | 0.1529 | 0.2948 | 5.3464 | 5.4434 | (1, 1) ; (2, 1) ; (1, 1) | 0.43 |
| 512 | **V18 (λ ∈ 0,001…1000)** | 212.55 / 211.43 / 212.03 | 212.00 | 5.3566 | 0.1534 | 0.2943 | 5.3291 | 5.4415 | (1, 0.1) ; (2, 1) ; (1, 0.001) | 0.43 |

Unigram PPL 281,15; trigram 137,4 (top-1 0,160).

## Sayısal kararlılık ve Cholesky (H=1024, tohum 1; tüm konfigürasyonlar)
Kolonlar: γ, λ | `min L_jj` | koşul proxy `(maks L_jj/min L_jj)²` | bağıl rezidü `‖(S+λI)W−R‖_F/‖R‖_F` | eğitim CE (alt örnek, kendi α,T'siyle) | doğrulama ham CE | doğrulama kalibre CE | (α,T)
```
TUM KONFIGURASYONLAR: gamma lambda | pivot_min(L_jj) | cond_proxy | rezidu | egitim CE | dogrulama ham CE | dogrulama kalibre CE | (alpha,T)
  0.5 0.001 | 1.723e-01 | 1.342e+06 | 4.09e-06 | 5.1150 | 6.4544 | 5.4278 | a=0.2 T=0.01
  0.5 0.01 | 2.050e-01 | 9.478e+05 | 3.43e-06 | 5.1495 | 6.4456 | 5.4286 | a=0.2 T=0.01
  0.5 0.1 | 3.958e-01 | 2.543e+05 | 1.64e-06 | 5.2882 | 6.4538 | 5.4381 | a=0.2 T=0.01
  0.5 0.5 | 7.979e-01 | 6.258e+04 | 7.01e-07 | 5.4020 | 6.4918 | 5.4583 | a=0.2 T=0.01
  0.5 1 | 1.107e+00 | 3.252e+04 | 3.74e-07 | 5.4408 | 6.4806 | 5.4692 | a=0.2 T=0.01
  0.5 10 | 3.315e+00 | 3.627e+03 | 9.65e-08 | 5.5094 | 6.4490 | 5.4970 | a=0.2 T=0.01
  0.5 100 | 1.027e+01 | 3.788e+02 | 3.35e-08 | 5.5540 | 6.3842 | 5.5086 | a=0.2 T=0.01
  0.5 1000 | 3.219e+01 | 3.942e+01 | 1.35e-08 | 5.6668 | 6.4597 | 5.5559 | a=0.2 T=0.01
  1 0.001 | 1.152e+00 | 3.000e+04 | 1.27e-06 | 5.1057 | 6.4465 | 5.4229 | a=0.2 T=0.01
  1 0.01 | 1.158e+00 | 2.971e+04 | 1.30e-06 | 5.1068 | 6.4462 | 5.4229 | a=0.2 T=0.01
  1 0.1 | 1.213e+00 | 2.708e+04 | 1.24e-06 | 5.1178 | 6.4429 | 5.4228 | a=0.2 T=0.01
  1 0.5 | 1.426e+00 | 1.959e+04 | 1.05e-06 | 5.1578 | 6.4366 | 5.4241 | a=0.2 T=0.01
  1 1 | 1.647e+00 | 1.469e+04 | 7.73e-07 | 5.1938 | 6.4358 | 5.4261 | a=0.2 T=0.01
  1 10 | 3.732e+00 | 2.861e+03 | 2.90e-07 | 5.3614 | 6.4809 | 5.4475 | a=0.2 T=0.01
  1 100 | 1.072e+01 | 3.473e+02 | 9.03e-08 | 5.4868 | 6.4164 | 5.4831 | a=0.2 T=0.01
  1 1000 | 3.257e+01 | 3.849e+01 | 2.32e-08 | 5.5776 | 6.3887 | 5.5202 | a=0.25 T=0.01
  2 0.001 | 5.134e+00 | 1.512e+03 | 5.29e-07 | 5.1058 | 6.4366 | 5.4194 | a=0.2 T=0.01
  2 0.01 | 5.135e+00 | 1.511e+03 | 5.58e-07 | 5.1059 | 6.4366 | 5.4194 | a=0.2 T=0.01
  2 0.1 | 5.150e+00 | 1.502e+03 | 5.06e-07 | 5.1065 | 6.4364 | 5.4194 | a=0.2 T=0.01
  2 0.5 | 5.214e+00 | 1.466e+03 | 5.35e-07 | 5.1093 | 6.4358 | 5.4193 | a=0.2 T=0.01
  2 1 | 5.293e+00 | 1.422e+03 | 4.81e-07 | 5.1126 | 6.4351 | 5.4192 | a=0.2 T=0.01
  2 10 | 6.484e+00 | 9.478e+02 | 3.84e-07 | 5.1614 | 6.4301 | 5.4195 | a=0.2 T=0.01
  2 100 | 1.263e+01 | 2.504e+02 | 1.66e-07 | 5.3182 | 6.4603 | 5.4355 | a=0.2 T=0.01
  2 1000 | 3.412e+01 | 3.509e+01 | 5.48e-08 | 5.4675 | 6.3684 | 5.4760 | a=0.25 T=0.01
  4 0.001 | 1.461e+01 | 1.867e+02 | 2.51e-07 | 5.1100 | 6.4292 | 5.4182 | a=0.2 T=0.01
  4 0.01 | 1.461e+01 | 1.867e+02 | 2.46e-07 | 5.1100 | 6.4292 | 5.4182 | a=0.2 T=0.01
  4 0.1 | 1.461e+01 | 1.866e+02 | 2.62e-07 | 5.1100 | 6.4292 | 5.4182 | a=0.2 T=0.01
  4 0.5 | 1.463e+01 | 1.861e+02 | 2.31e-07 | 5.1103 | 6.4292 | 5.4182 | a=0.2 T=0.01
  4 1 | 1.466e+01 | 1.854e+02 | 2.56e-07 | 5.1107 | 6.4291 | 5.4182 | a=0.2 T=0.01
  4 10 | 1.511e+01 | 1.746e+02 | 2.38e-07 | 5.1170 | 6.4284 | 5.4179 | a=0.2 T=0.01
  4 100 | 1.887e+01 | 1.122e+02 | 2.07e-07 | 5.1688 | 6.4285 | 5.4181 | a=0.2 T=0.01
  4 1000 | 3.814e+01 | 2.808e+01 | 7.74e-08 | 5.3464 | 6.3797 | 5.4385 | a=0.2 T=0.01
OZET|V17_secim(lambda>=1)|c=2|H=1024|tohum=1|gam=4|lam=10|alpha=0.2|T=0.01|egitim_CE=5.1170|val_CE_kalibre=5.4179|test_CE=5.3296|PPL=206.37|top1=0.1622|top5=0.2998|agirlik_uretim_sn=1.39
```
- [Kesin] **Rezidü küçük ve λ'ya göre kararlı**: tüm 32 konfigürasyonda ≤ 4,1×10⁻⁶ (en kötüsü γ=0,5, λ=0,001); koşul proxy'si 4×10¹ … 1,3×10⁶ aralığında, min `L_jj` 0,17 … 32. Yani λ=0,001'de bile çözüm sayısal olarak tutarlı (FP64 birikim + double Cholesky).
- [Kesin] **Cholesky başarısızlıkları**: tohum 1 ve 2 (H=1024) 0/32, H=512 üç tohum 0/32; **tohum 3, H=1024: 3/32** — (γ=2, λ=0,001), (γ=4, λ=0,001), (γ=4, λ=0,01). Büyük γ'da `tanh` doygunluğu `FᵀF`'i neredeyse rank-eksik yapıyor; çok küçük λ pozitif belirliliği kurtarmıyor. Başarısız konfigürasyonlar seçimden dışlandı (seçimi etkilemedi: seçilen konfigürasyonlar başarısızlar arasında değildi). Rezidü yalnız tohum 1 (H=1024 ve 512) için ölçüldü.
- [Kesin] **Aşırı uyum**: küçük λ'da eğitim CE'si belirgin düşüyor, doğrulama CE'si ise yaklaşık aynı kalıyor: γ=0,5: λ=0,001 → eğitim 5,115, doğr. kalibre 5,428 (fark 0,313); λ=1000 → eğitim 5,667, doğr. 5,556 (eğitim doğrulamadan yüksek). γ=4, λ ≤ 10: eğitim 5,110–5,117, doğr. 5,418–5,424 (fark ≈ 0,31). Yani küçük λ eğitim verisini daha çok uyduruyor ama genelleme kazancı yok: gap büyürken test kaybı değişmiyor.

## Süre ve RAM
Ağırlık üretimi (W₁ + eğitim+doğrulama istatistiği + 1 Cholesky/çözüm): H=1024 ≈ 1,31–1,49 sn (V17 ile aynı), H=512 ≈ 0,38–0,47 sn. λ küçülünce Cholesky süresi değişmedi. Koşu başına toplam 38–74 sn (32 konfigürasyon aranıyor; tohum 1 koşuları kararlılık/rezidü ölçümü yüzünden daha uzun). Tepe RAM 439 MB (H=1024), 269 MB (H=512). Süre/RAM ölçümleri:

| komut | duvar sn | kullanıcı CPU sn | tepe RAM MB |
|---|---|---|---|
| `./tek_hamle_v18 2 1024 1 1` | 74.04 | 243.24 | 439 |
| `./tek_hamle_v18 2 512 1 1` | 40.43 | 138.48 | 269 |
| `./tek_hamle_v18 2 1024 2 0` | 68.42 | 223.97 | 439 |
| `./tek_hamle_v18 2 1024 3 0` | 59.80 | 201.57 | 439 |
| `./tek_hamle_v18 2 512 2 0` | 39.74 | 138.46 | 269 |
| `./tek_hamle_v18 2 512 3 0` | 38.23 | 133.92 | 269 |

(Son argüman 1 = kararlılık/rezidü ölçümü açık.)

## Sınırlar
- Tek metin, 50 000 token; 3 tohum yalnız W₁ tohumları. λ ızgarası 8 değer; λ<0,001 denenmedi.
- Eğitim CE alt örnek (her 8. eğitim satırı, 4 980 satır) üzerinde ve her konfigürasyonun kendi (α,T)'siyle hesaplandı.
- Rezidü yalnız tohum 1'de ölçüldü; koşul sayısı yerine ucuz bir proxy kullanıldı (gerçek koşul sayısı hesaplanmadı).
- Kaybı düşürmeyen sonuçlar (küçük λ) dahil tüm konfigürasyonlar ham logda.

## Komutlar (gerçek)
Derleme: `g++ -O3 -march=native -fopenmp -o tek_hamle_v18 tek_hamle_v18.cpp`. Çalıştırma: `./tek_hamle_v18 2 H TOHUM KARARLILIK` (H=512, 1024; tohum 1–3; KARARLILIK=1 yalnız tohum 1). Girdi `/tmp/v8data` (V8 veri hazırlığı + `uni_tr.f64`, `uni_trva.f64`).
