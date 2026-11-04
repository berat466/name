# Tek hamle V16 raporu — Hadamard ölçekleme ve logaritmik hedef dönüşümü

Branch: `claude/formul-avcisi`. Referans: `TEK_HAMLE_V12_RAPOR.md`. V8–V15 dosyaları değişmedi (V16 yeni dosyada). Ana algoritmaya eklenmedi. 15 dk sınırı içinde; **E yöntemi (tek CE düzeltmesi) süre nedeniyle ATLANDI.** Adam/RLS/uzun döngü yok.
Kod: `cpp/tek_hamle_v16.cpp`, `metin_kalibrasyon_v16.py` (V9 kalibrasyonu, V16 skor dizini). Ham loglar: `v16_ham/v16log/`.
Kurulum: aynı 50 000 token/bölme, H=512, tek tohum; γ ∈ {0,5; 1; 2; 4}, λ ∈ {1, 10, 100, 1000} (V12 aralığı), sıcaklık V8 ızgarası (seçim: doğrulama CE, V12 kuralı); sonra tüm yöntemlere eşit V9 kalibrasyonu (genişletilmiş ızgara, yalnız doğrulamada). Test yalnız sonda.

## Yöntemler
- **A**: ridge, yoğun giriş `[e_{t−2}, e_{t−1}, D]` (V12 `ref`, s=0).
- **B**: giriş `[e_{t−2}, e_{t−1}, D, I]`, `I = s·(e_{t−1}⊙e_{t−2})`; s ∈ {8, 32} (s=0 → A). s'yi tohum 1'de doğrulama CE ile karşılaştırdım; s=1 V10'da zaten denenmişti (kazanç yok). Gömmeler birim norm olduğundan `I` elemanları küçük (≈0,015); s=8 → normu ≈ 1.
- **C**: `T' = log(T+ε)`, ε ∈ {0,001 (C1), 0,01 (C2)}: `W=(FᵀF+λI)⁻¹FᵀT'`. One-hot'ta `T' = c₀·1 + c₁·T`, `c₁=log((1+ε)/ε)`, `c₀=log ε`; uygulama `R' = c₁FᵀT + c₀Fᵀ1`.
- **D**: B+C birleşimi (s ∈ {8,32} ile C1/C2); ayrı koşu gerekmedi (aynı koşuda her özellik kümesi için A/C1/C2).

## Açık cevap
1. **Hadamard ölçekleme gerçek öğrenme kazancı sağlamadı; kötüleştirdi.** Kalibre PPL: s=0: **216,8**; s=8: 221,8; s=32: 231,1 (s büyüdükçe monoton kötü). Kalibrasyonsuz (α=1) ham skor PPL'i de kötü: 521 → 532 → 577; ham top-1 %15,0 → %14,5 → %13,1.
2. **Logaritmik hedef yalnız ölçek/bias dönüşümü; matematiksel olarak ridge'e eşdeğer.** Aynı (γ,λ) için `Z_C − c₁Z_A` satır içinde sabit: tüm örnekler için `max_v−min_v` ≤ 2,4×10⁻⁵ (float gürültüsü; ε=0,001: 1,9–2,4×10⁻⁵, ε=0,01: 1,4–1,7×10⁻⁵). Satır sabiti softmax'ta iptal olur, `c₁` bir sıcaklık ölçeğidir (`T' = T/c₁`). Yani C, A'dan matematiksel olarak farklı bir model değil.
3. **C'nin raporlanan sonucu A'dan kötü, ama bu eşdeğersizlikten değil seçim kuralından**: V8 kuralı (γ,λ,T'yi kalibrasyondan **önce** doğrulama CE ile seçmek) C için farklı bir (γ=1, λ=1000) seçti; A γ=4'ü seçmişti. Aynı konfigürasyonda ikisi aynı olasılıkları verir. Sonuç: C'nin PPL'i 234–235 (s=0). Bunun anlamı "log hedef daha kötü" değil, "seçim kuralı kalibrasyondan önceki CE'ye dayandığı için sonuç ölçek seçimine duyarlı". [Muhtemel; kalibrasyon sonrası CE ile seçim yapılmadı.]
4. **D (B+C)**: A'dan kötü (221,8 → 235–242). Ek kazanç yok.
5. Trigram 137'ye yaklaşılamadı; en iyi yine V12 A (216,8).

## Sonuçlar (H=512, tohum 1; kalibrasyon sonrası = V9 genişletilmiş ızgara)
| s | yöntem | seçilen γ/λ | α=1 ham PPL | **kalibre PPL** | test CE | top-1 | top-5 | ağırlık üretimi sn |
|---|---|---|---|---|---|---|---|---|
| 0 | A ridge | 4 / 1000 | 521,1 | **216,8** | 5,379 | 0,1492 | 0,2900 | 0,45 |
| 0 | C1 (ε=0,001) | 1 / 1000 | 510,7 | 234,2 | 5,456 | 0,1323 | 0,2697 | 0,52 |
| 0 | C2 (ε=0,01) | 1 / 1000 | 506,4 | 235,0 | 5,460 | 0,1319 | 0,2703 | 0,50 |
| 8 | B ridge | 1 / 100 | 531,8 | 221,8 | 5,402 | 0,1466 | 0,2801 | 0,33 |
| 8 | D = B+C1 | 1 / 1000 | 524,4 | 235,1 | 5,460 | 0,1327 | 0,2645 | 0,33 |
| 8 | D = B+C2 | 1 / 1000 | 519,5 | 235,8 | 5,463 | 0,1317 | 0,2653 | 0,33 |
| 32 | B ridge | 0,5 / 10 | 577,3 | 231,1 | 5,443 | 0,1323 | 0,2691 | 0,44 |
| 32 | D = B+C1 | 0,5 / 1000 | 599,3 | 258,5 | 5,555 | 0,1100 | 0,2376 | 0,41 |
| 32 | D = B+C2 | 0,5 / 100 | 573,6 | 241,7 | 5,488 | 0,1231 | 0,2578 | 0,42 |

Referans: unigram PPL 281; trigram 137. "Ağırlık üretimi" = W₁ + eğitim+doğrulama istatistiği + 1 Cholesky/çözüm (ayar araması hariç): 0,33–0,52 sn, V12'nin 0,4–0,6 sn'siyle aynı düzey; ek özellik (64 boyut) ve hedef dönüşümü hız kaybı getirmedi. Her özellik kümesi için ayar dahil toplam 115–129 sn (3 yöntem × 16 ayar × 9 sıcaklık); tepe RAM 326–338 MB.

## Sınırlar
- Tek tohum; B için yalnız s ∈ {8, 32}; ε iki değer; E yapılmadı; 3 tohum doğrulaması yapılmadı (kazanan A zaten V12'nin 3 tohumlu sonucuyla aynı: 217,0 ± 0,2).
- Seçim kuralı (kalibrasyondan önceki CE) C/D sonuçlarını etkiledi; kalibrasyon-sonrası CE ile seçim yapılsaydı C'nin A ile aynı sonucu vermesi beklenir (eşdeğerlik kontrolüne dayanarak) [Tahmin; denenmedi].
- Eşdeğerlik kontrolü yalnız γ=1, λ=100 için ve doğrulama satırlarında yapıldı.

## Komutlar (gerçek)
Derleme: `g++ -O3 -march=native -fopenmp -o tek_hamle_v16 tek_hamle_v16.cpp`. Çalıştırma: `./tek_hamle_v16 2 512 1 S` (S=0, 8, 32); kalibrasyon: `python3 -I metin_kalibrasyon_v16.py cift_sS_M 2 512 1` (M=A,1,2). Ölçüm (`manifest.tsv`):
- `./tek_hamle_v16 2 512 1 0`: wall_s=130.90, maxrss_MB=326
- `./tek_hamle_v16 2 512 1 8`: wall_s=122.17, maxrss_MB=338
- `./tek_hamle_v16 2 512 1 32`: wall_s=114.56, maxrss_MB=338
