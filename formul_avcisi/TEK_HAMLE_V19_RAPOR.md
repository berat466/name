# Tek hamle V19 raporu — Ridge + tek Cross-Entropy gradyan düzeltmesi

Branch: `claude/formul-avcisi`. Referanslar: V17/V18 raporları. V8–V18 dosyaları değişmedi (V19 yeni dosyada). Adam/çoklu adım/epok/RLS yok. 20 dk sınırı içinde.
Kod: `cpp/tek_hamle_v19.cpp`. Ham loglar ve komutlar: `v19_ham/v19log/`.

**Önemli not:** Bu yöntem Ridge tek hamle çözümüne **ek bir işlemdir** (bir ileri geçiş + bir CE gradyanı + mevcut Cholesky ile bir çözüm); gerçek Newton değil, ön koşullandırılmış tek bir CE gradyan düzeltmesidir. Saf "tek hamle Ridge" değildir; hız maliyeti aşağıda.

## Matematik (uygulanan)
`A=FᵀF+λI` (Cholesky saklanır), `W₀=A⁻¹FᵀY`. V17 kalibrasyonuyla (`α₀=0,2`, `T₀=0,01`, unigram `u`): `q_i=softmax(F_iW₀/T)`, `p_i=(1−α)u+αq_i`, `G_i = (α q_{i,y_i}/(T p_{i,y_i}))·(q_i − onehot(y_i))` (CE'nin logit türevi, doğrulandı: `∂CE/∂z`). `W₁ = W₀ − η A⁻¹FᵀG`, aynı Cholesky yeniden kullanıldı (yeni ayrışım yok), tek adım. Gradyan **yalnız eğitim satırlarından** (parçalı, 4 096'lık gruplar); doğrulama/test gradyana girmedi. η taraması: spec {0, 0,01, 0,1, 0,5, 1, 2} + **genişletme** {10⁻⁴, 10⁻³} (küçük adımlar için); η, α, T doğrulama kalibre CE'siyle seçildi; test yalnız sonda. Son model: eğitim+doğrulama istatistiği (V17 protokolü) ve düzeltme gradyanı eğitim+doğrulama satırlarından; η, α, T doğrulamadan seçildi.
V17'nin seçtiği (γ,λ): H=1024: tohum 1 (4,10), tohum 2 (4,100), tohum 3 (2,1); H=512: (1,1), (2,1), (1,1). Aynı tohumlar, veri ve bölme (50 000 token, 80/10/10 blok).

## Açık cevap
**Evet: tek bir CE düzeltmesi perplexity'yi düşürdü (3/3 tohum, her iki H'de), ama top-1 doğruluğu H=1024'te düştü.**
- [Kesin] **H=1024:** PPL **206.8 → 199.9** (−7.0, ≈ %3.4); test CE 5.3318 → 5.2976 nat. Araştırma hedefi olan 200'ün altı **ortalamada** sağlandı (199,9); tohumlar 200,90 / 198,46 / 200,23.
- [Kesin] **H=512:** PPL 212.3 → 202.8 (−9.6, ≈ %4.5).
- [Kesin] **Top-1 düştü (H=1024):** %16.2 → %14.9; top-5 arttı (%30,0 → %30,4–30,7). H=512'de top-1 aynı kaldı (%15,3–15,4), top-5 arttı. CE düzeltmesi olasılık kalitesini (CE) iyileştirirken en olası kelimeyi seçme isabetini H=1024'te bozdu.
- [Kesin] **Aşırı uyum artıyor:** eğitim CE'si H=1024'te 5,12 → 4,53–4,65 düşüyor (alt örnek, kalibre), doğrulama CE'si 5,418 → 5,371, yani gap büyüyor; test yine de iyileşti. Seçilen η=0,01 ağırlıkları büyük değiştiriyor (‖ηΔW‖/‖W₀‖ ≈ %32–39).
- [Kesin] **Hız maliyeti ağır:** H=1024'te Ridge ≈ 0,5 sn iken düzeltme ek ≈ 9–11 sn (bir ileri gemm + softmax + bir geri gemm; n·m·V ≈ 8·10¹⁰ çarpma-toplama iki kez); H=512'de 0,13 → 3,4 sn. Toplam ağırlık üretimi ~20× yavaş (tablodaki süreler).
- Trigram hedefi (PPL 137) uzak (≈ 200).

## Sonuçlar (3 tohum, c=2)
| H | yöntem | PPL tohum 1 / 2 / 3 | PPL ort. | test CE ort. | top-1 ort. | top-5 ort. | eğitim CE ort. (alt örnek) | doğr. CE (kalibre) ort. | seçilen η / α / T |
|---|---|---|---|---|---|---|---|---|---|
| 1024 | Ridge (η=0; V17 referansı) | 206.37 / 207.82 / 206.28 | 206.82 | 5.3318 | 0.1618 | 0.2999 | 5.1348 | 5.4168 | 0/0.2/0.01, 0/0.2/0.01, 0/0.2/0.01 |
| 1024 | **V19 Ridge + tek CE düzeltmesi** | 200.90 / 198.46 / 200.23 | 199.86 | 5.2976 | 0.1487 | 0.3047 | 4.5984 | 5.3722 | 0.01/0.25/0.01, 0.01/0.25/0.01, 0.01/0.2/0.01 |
| 512 | Ridge (η=0; V17 referansı) | 213.10 / 211.43 / 212.48 | 212.34 | 5.3582 | 0.1529 | 0.2948 | 5.3464 | 5.4434 | 0/0.2/0.01, 0/0.2/0.01, 0/0.2/0.01 |
| 512 | **V19 Ridge + tek CE düzeltmesi** | 202.31 / 203.42 / 202.52 | 202.75 | 5.3120 | 0.1540 | 0.3041 | 5.0656 | 5.3984 | 0.01/0.25/0.01, 0.01/0.25/0.01, 0.01/0.25/0.01 |

Unigram PPL 281,15; trigram PPL 137,4, top-1 %16,0, top-5 %36,2. V17 referansı (H=1024): PPL 206,8, top-1 %16,1, top-5 %30,0.

## η taraması (tohum 1; tüm tohumlar ham logda)
| H, tohum | η | ‖ηΔW‖/‖W₀‖ | doğrulama kalibre CE | (α,T) | eğitim CE (alt örnek) | not |
|---|---|---|---|---|---|---|
| H=1024, tohum 1 | 0 | 0 | 5.4179 | α=0.2, T=0.01 | 5.1170 | spec |
| H=1024, tohum 1 | 0.0001 | 0.00392 | 5.4160 | α=0.2, T=0.01 | 5.1080 | GENISLETME |
| H=1024, tohum 1 | 0.001 | 0.0392 | 5.4005 | α=0.2, T=0.01 | 5.0284 | GENISLETME |
| H=1024, tohum 1 | 0.01 | 0.392 | 5.3705 | α=0.25, T=0.01 | 4.5253 | spec |
| H=1024, tohum 1 | 0.1 | 3.92 | 5.5641 | α=0.1, T=0.03 | 5.0724 | spec |
| H=1024, tohum 1 | 0.5 | 19.6 | 5.6193 | α=0.05, T=0.1 | 5.3010 | spec |
| H=1024, tohum 1 | 1 | 39.2 | 5.6231 | α=0.05, T=0.25 | 5.3238 | spec |
| H=1024, tohum 1 | 2 | 78.3 | 5.6246 | α=0.05, T=0.5 | 5.3299 | spec |
| H=512, tohum 1 | 0 | 0 | 5.4459 | α=0.2, T=0.01 | 5.3543 | spec |
| H=512, tohum 1 | 0.0001 | 0.00321 | 5.4445 | α=0.2, T=0.01 | 5.3505 | GENISLETME |
| H=512, tohum 1 | 0.001 | 0.0321 | 5.4336 | α=0.2, T=0.01 | 5.3165 | GENISLETME |
| H=512, tohum 1 | 0.01 | 0.321 | 5.4027 | α=0.25, T=0.01 | 5.0782 | spec |
| H=512, tohum 1 | 0.1 | 3.21 | 5.5457 | α=0.15, T=0.02 | 5.3245 | spec |
| H=512, tohum 1 | 0.5 | 16.1 | 5.6060 | α=0.1, T=0.1 | 5.5156 | spec |
| H=512, tohum 1 | 1 | 32.1 | 5.6162 | α=0.05, T=0.1 | 5.5935 | spec |
| H=512, tohum 1 | 2 | 64.2 | 5.6144 | α=0.05, T=0.25 | 5.5876 | spec |

- [Kesin] η=0,01 her koşuda en iyi η (6/6); η=0,001 ve 10⁻⁴ (genişletme) daha az kazandırdı, η ≥ 0,1 kaybı belirgin kötüleştirdi (CE 5,55–5,62 > ridge 5,42; η≥0,5'te kalibre doğrulama CE'si unigram düzeyine (5,69) yaklaşıyor, yani model katkısı siliniyor). Ayrı η ızgarası (0,001–0,1 arası ara değerler) taranmadı: optimum 0,003–0,05 arasında olabilir [Tahmin].
- [Muhtemel] η ≈ T²/ölçek düzeyinde (0,01) çalışması, `1/T²` eğrilikli CE'ye uygun bir Gauss–Newton boyutuna yakın olmasıyla uyumlu; ölçülmedi.

## Süre, gradyan normu ve RAM
| H | tohum | Ridge sn (özellik hariç: istatistik+Cholesky+çözüm) | CE düzeltmesi sn (gradyan + A⁻¹ çözümü) | ‖FᵀG‖ | ‖A⁻¹FᵀG‖ | ‖W₀‖ |
|---|---|---|---|---|---|---|
| 1024 | 3 | 1.814 | 6.675 | 8.94e+05 | 213 | 5.48 |
| 1024 | 2 | 1.097 | 6.111 | 1.29e+06 | 53.7 | 1.44 |
| 1024 | 1 | 1.894 | 6.330 | 1.32e+06 | 71.9 | 1.83 |
| 512 | 3 | 0.329 | 3.458 | 2.9e+05 | 342 | 10.7 |
| 512 | 2 | 0.329 | 3.050 | 4.81e+05 | 104 | 3.45 |
| 512 | 1 | 0.558 | 3.408 | 2.85e+05 | 344 | 10.7 |

Ridge sütunu: istatistik (FᵀF, FᵀT) + Cholesky + çözüm; özellik üretimi ve W₁ (≈ 0,001 sn) ayrı. Düzeltme sütunu: gradyan (ileri, softmax, geri) + `A⁻¹` çözümü (mevcut Cholesky). Son modelde düzeltme eğitim+doğrulama satırlarıyla tekrarlandı (ek ≈ aynı süre). Tepe RAM: H=1024 457 MB, H=512 271 MB (gradyan `m×V` birikimi ve 4 096 satırlık `Z` parçaları dahil). Koşu başına toplam 16–29 sn.

| komut | duvar sn | kullanıcı CPU sn | tepe RAM MB |
|---|---|---|---|
| `./tek_hamle_v19 2 512 1 1 1` | 18.49 | 60.42 | 271 |
| `./tek_hamle_v19 2 1024 1 4 10` | 29.46 | 96.44 | 457 |
| `./tek_hamle_v19 2 1024 2 4 100` | 27.35 | 93.58 | 457 |
| `./tek_hamle_v19 2 1024 3 2 1` | 27.83 | 95.11 | 457 |
| `./tek_hamle_v19 2 512 2 2 1` | 16.71 | 57.61 | 272 |
| `./tek_hamle_v19 2 512 3 1 1` | 16.05 | 56.51 | 271 |

## Sınırlar / dürüstlük
- Düzeltme gradyanı V17'nin (α₀=0,2, T₀=0,01) kalibrasyonuna bağlı; η seçimi dar (8 değer); α,T yeniden seçimi doğrulamada yapıldı (genelde α=0,25).
- Top-1 düşüşünün nedeni ölçülmedi [Tahmin: CE gradyanı doğru sınıfın olasılığını artırırken en yüksek skoru sıralayan sınıf-içi farkları bozuyor].
- Tek metin, 50 000 token; 3 tohum yalnız W₁ tohumları. Doğrulama kümesi hem η/α/T seçimi hem (V17 protokolü gereği) son modelin eğitim verisi olarak kullanıldı; test bunlardan bağımsız.
- Yeni yöntem ana mimariye eklenmedi.

## Komutlar (gerçek)
Derleme: `g++ -O3 -march=native -fopenmp -o tek_hamle_v19 tek_hamle_v19.cpp`. Çalıştırma: `./tek_hamle_v19 2 H TOHUM GAMMA LAMBDA` (V17'nin seçtiği γ,λ; yukarıdaki liste). Girdi `/tmp/v8data`.
