# V57 — U hesabının matematiksel kısaltması (15 dk)

Referans: V54 `1cf9b09`, V56 `76ee1e4` (değişmedi). Deney kopyası: `cpp/tek_hamle_v57.cpp` (V56 + `V57_UX=1`; varsayılan kapalı).

**Karar: KISMİ** — işlem sayısı düştü, süre/RAM azaldı, ama bit-bit eşitlik bozuldu → ana sürüme **alınmadı** (onay bekliyor).

1. **Eşdeğerlik:** evet, kaynak koddan. `Y0=(A_CC+Λ)⁻¹R_C`, `X=(A_CC+Λ)⁻¹A_CF`, A_CC+Λ simetrik (blok-köşegen SPD) → `U=A_CFᵀY0=A_CFᵀ(A_CC+Λ)⁻¹R_C=Xᵀ R_C` (X zaten grup çözümünde üretiliyor). R_C tamsayı sayımlardır, 5.13 M sıfır olmayan (slot,y) konumu. **Y0 yine gerekli** (W_C=Y0−X·W_F), yalnız U için gereksiz yoğun çarpım kalktı.
2. **İşlem sayısı (teorik tahmin):** U: 82.0 GF → 5.3 GF (−%93.5); tam model ≈ 344 → ≈ 267 GF. Yeni yol: `h_u_sparse(X, R_C)` (yalnız sıfır olmayan konumlar; yoğun `Y0d` 640 MB kurulmuyor).
3. **U süresi (tam, 3 koşu ara ara):** yoğun dgemm yolu 0.74 / 1.07 / 0.82 s → seyrek `XᵀR_C` 0.60 / 0.53 / 0.53 s (≈ −%30; bellek-sınırlı: 5.3 GF için 0.53 s ≈ 10 GF/s — FLOP azalması süreye aynı oranda yansımadı).
4. **Tam model hazır:** UX kapalı 4.80 / 5.05 / 4.42 s → UX açık 4.49 / 4.04 / 4.29 s (medyan 4.80 → 4.29, ≈ −%11). **VM bu sırada yavaş/dalgalıydı** (V54 referansı 3.63 s idi, bu oturumda 4.4–5.0 s); mutlak değerlere değil çifte bakın. Tepe RAM 1,587 → 1,278 MB (−%19).
5. **Ağırlıklar ve PPL:** hash'ler **değişti** (WC `696a6b95…` vs `ddab7b8c…`, WF `724c31a1…` vs `a9dc7b3f…`). Fark: U 1.01 M / 1.03 M elemanda farklı (maks mutlak 1.9e-11, bağıl 4.4e-9 — toplama sırası); **WC 79 / 79.9 M eleman** (maks mutlak 1.8e-12, bağıl 6.9e-6 sıfıra yakın değerlerde), **WF 4 / 1.03 M eleman** (≤ 8.9e-8 = 1 float ulp), val logit 25 / 9.96 M eleman (maks mutlak 7e-12). Validation CE **4.609033 (aynı)**, Top-1/Top-5 aynı (16.63 % / 37.99 %), **test PPL 98.092 (aynı)**, test CE 4.585905, Top-1 18.21 %, Top-5 39.68 % — yani öğrenme kalitesi değişmedi, yalnız 1-ulp seviyesinde sayısal fark var.
6. **Kalan en büyük darboğaz:** K-tabloları + kapı (≈ 1.0 s, %28), sonra `X`'in grup çözümü + T=A_CFᵀX dgemm (≈ 0.55 s) ve W_C sgemm'i (82 GF, 0.3 s). Y0 yalnız W_C için çözülüyor; W_C = A⁻¹(R_C − A_CF·W_F) biçimi daha fazla FLOP gerektirdiği için denenmedi.

Sınırlar: tek veri/tek tohum; 3 çiftlik A/B, VM gürültülü; GFLOP değerleri teorik tahmindir (sayaç yok). Bit-özdeş ana sürüm (V54/V56) olduğu gibi duruyor.
