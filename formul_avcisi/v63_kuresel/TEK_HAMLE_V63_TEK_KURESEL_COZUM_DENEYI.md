# V63: bütün ağırlıkları tek küresel hesaplamayla üretme deneyi — sonuç: KISMİ BAŞARI (gerçek, doğrulanmış işlem azalması; "tek operatör" tamamen değil)

Kod: `cpp/tek_hamle_v63.cpp` (V61 + `V63_ARROW=1|2`, varsayılan KAPALI; `V63_CHECK=1` yapı doğrulaması). V54–V62 dosyaları değişmedi. Ayar/veri: V59 ortak veri, hafif ayar, aynı oturumda ardışık A/B (VM gürültülü: toplam sürelere güvenme, işlem sayılarına ve bileşen sürelerine bak).

## 1) Bütün formül (V61 kaynağından)
Girdi: Fᵢ=tanh(γ(xᵢW+b)) (513), son-kelime slotu w(i) ve (varsa) çift slotu p(i) (C bloğu, mc=V+#çift). Sayımlar: A_CC=ΣcᵢcᵢᵀΛ-li, A_CF=ΣcᵢFᵢᵀ, A_FF=ΣFFᵀ, R_C=Σcᵢe_{yᵢ}ᵀ (6,4 % dolu → 3,9 %), R_F=ΣFᵢe_{yᵢ}ᵀ.
Tek hedef: **W=M⁻¹R**, M=[[A_CC+Λ, A_CF],[A_FC, A_FF+λ_F I]], R=[R_C;R_F]. V61'in zinciri bunu blok-eleme ile çözer:
Y0=(A_CC+Λ)⁻¹R_C, X=(A_CC+Λ)⁻¹A_CF, T=A_CFᵀX, U=A_CFᵀY0, (A_FF+λI−T)W_F=R_F−U, W_C=Y0−XW_F.

## 2) Özdeşlikler (eleme)
**(i) Ok-başı yapı [Kesin, kodda ve veride doğrulandı].** Her örnek tam bir kelime slotu w ve en çok bir çift slotu p içerir (aynı grup=son kelime). Bu yüzden her grup matrisi
A_g+Λ = [[n_w+λ_w, bᵀ],[b, diag(d_p)]], b_p=n_p, d_p=n_p+λ_p (çift–çift girişler tam 0).
`V63_CHECK`: 286 grupta çift–çift dışı köşegen maks=0, |A[w,p]−A[p,p]|=0. Eski kod bu matrisi **yoğun** Cholesky ile çözüyordu (grup boyutu ≤1300). Tersi kapalı formda: s=n_w+λ_w−Σ b_p²/d_p; x_w=(r_w−Σ b_p r_p/d_p)/s; x_p=(r_p−b_p x_w)/d_p → **O(grup×sütun)**, Cholesky ve üçgen çözümler tamamen ortadan kalkıyor.
**(ii) U=A_CFᵀY0=XᵀR_C** (A+Λ simetrik; V57 özdeşliği) → Y0'ın yoğun kurulumu ve mc×mf×V dgemm yok; R_C seyrek olduğundan 2·mf·nnz(R_C).
**(iii) T=ZᵀZ**, Z satırları: çift için a_p/√d_p, kelime için (a_w−Σ b_p a_p/d_p)/√s → T için X'e gerek yok, dsyrk ile yarı işlem (21→10,5 GF @2001). **Uygulanmadı** (süre); matematik doğrulandı, sayısal test yok.
**(iv) Nihai küresel ifade:** W_F=(A_FF+λI−ZᵀZ)⁻¹(R_F−XᵀR_C); W_C=Y0−X W_F; X_p=(a_p−b_p x_w)/d_p, Y0 aynı kapalı formla. G(D,Θ) budur; "eski işlemleri gizleyen" değil, ok-başı kapalı formu + seyrek XᵀR_C kullanıyor.

## 3) Deney (A=V61, B=V63_ARROW=1 [ok-başı], C=V63_ARROW=2 [ok-başı + seyrek U])
| Ölçüm | V61 | V63 (=1) | V63 (=2) |
|---|---|---|---|
| Grup çözümü g45, 2.001 / 5.001 / 10.001 | 0,92 / 1,92 / 1,50 s | 0,13 / 0,15 / 0,19 s | 0,10 / 0,125 / 0,14 s |
| Grup çözümü işlem (madd), 2k / 5k / 10k | 1,0e10 / 1,8e10 / 1,9e10 | ≈7e7 / 1,0e8 / 1,2e8 (≈145× / 180× / 160× az) | aynı |
| U (g9), 2k / 5k / 10k | 0,61 / 3,81 / 5,66 s | 0,66 / 2,84 / 5,79 s | 0,75 / 2,17 / 3,16 s |
| U işlem (madd) 2k / 5k / 10k | 3,0e10 / 9,5e10 / 2,2e11 | 3,0e10 / 9,5e10 / 2,2e11 | 4,3e9 / 7,5e9 / 8,5e9 (7× / 13× / 26× az) |
| Tam model hazır (gürültülü) 2k / 5k / 10k | 8,7 / 50,1* / 26,7 s | 6,1 / 13,4 / 20,6 s | 5,9 / 15,7 / 24,9 s |
| Tepe RSS 2k / 5k / 10k | 1,46 / 4,36 / 6,39 GB | 1,39 / 4,26 / 6,30 GB | 1,39 / 4,27 / 6,32 GB |
| Rezidu (gruplar) 2k / 5k / 10k | 1,2e-13 / 3,7e-13 / 1,5e-13 | 2,6e-14 / 5,0e-14 / 4,5e-14 | aynı |
| Val CE 2k / 5k / 10k | 4,503665 / 5,079437 / 5,441941 | aynı | aynı |

\* V61 5.001 koşusunda VM gürültüsü (aynı kod önceki oturumda 24,9 s). Toplam süre kazancı gürültüden ayrıştırılamadı; kanıtlı olan: işlem sayısı ve grup/U bileşen süreleri.
**Ağırlık farkı (5.001, dump):** WC 52 236 / 186 M eleman farklı (=%0,03), maks |Δ|=9,3e-10 (bağıl 2,0e-9); WF 11 / 2,6 M, maks 1,8e-12 (bağıl 2,6e-11); =2 için WC 52 686, WF 17, aynı maksimumlar; hepsi sonlu. 2.001: WC 582 / 58,6 M, WF 4 / 1,0 M (maks 1,4e-14). **Bit-eşit değil.** İlk ayrışma: X (double) grup çözümünde — Cholesky ile ok-başı eleme farklı sırada toplar; Y0'ın float'a yuvarlanmış karması **aynı** (4611f253…), yani fark float yuvarlamasında büyük ölçüde siliniyor; U (=2) ayrıca XᵀR_C formülü olduğundan ek ~1e-12 bağıl fark (V57'de açıklanan neden). Yeni yöntem daha doğru (rezidu 5× küçük).

## Sorular
1. **Tek ifadeye indirebildin mi?** Tek hedef W=M⁻¹R ve bunun blok-eleme zinciri çıkarıldı; kapalı formda (iv) tüm ara değişkenler X, Y0'a indi (tam sembolik indirgeme yok; Schur çözümü kaçınılmaz).
2. **Kaç ara hesap kalktı?** Grup Cholesky + Y0/X üçgen çözümleri (3 aşama: g4,g5,g6) → tek geçiş kapalı form; =2'de Y0d kurulumu + mc×mf×V U dgemm de kalktı. İşlem: grup çözümü ≈150–180× az, U 7–26× az.
3. **Tamamen kaldırılan klasik işlemler:** grup Cholesky ve grup üçgen çözümleri (A_CC tarafı). **Kalan:** 513×513 Schur Cholesky (T,U sonrası, ~0,04 s), T hesabı (dgemm; dsyrk ile yarıya iner), W_C=Y0−XW_F (2·mc·mf·V; **mat. kaçınılmaz** — çıktı mc×V yoğun).
4. **Gerçek işlem sayısı:** yukarıdaki tablo; toplam V61'de A_CC+U ≈ 1,3e10–2,4e11 madd → 1,2e8 + 4e9–9e9 (=2).
5. **Ağırlıklar aynı mı?** Bit-eşit değil; WC ≤9,3e-10, WF ≤1,8e-12 mutlak (float ağırlık ölçeği 0,46/0,07); elemanların %99,97'si bit-eşit.
6. **Öğrenme kalitesi:** Val CE üç boyutta 6 hane aynı; rezidu iyileşti. Test PPL ölçülmedi.
7. **Ölçekleme:** grup çözümü O(mc·(mf+ns)) (V61: O(Σsg²(mf+ns)), sg≤1300 sabit veri bağımlı); U O(mf·nnz(R_C)); 100.000 kelimede bunlar doğrusal. **Engel:** W_C=Y0−XW_F (mc×V yoğun, 100k'da ≫TB) — büyük V için W_C açık biçimde üretilmemeli, F̃=F−Σ_{slot}X ile logit=F̃W_F+ΣY0 (örnek başına Y0 seyrek) olarak tutulmalı (kimlik matematiksel; uygulanmadı).
8. **Doğrudan çözüm engeli:** M=[[A_CC+Λ,A_CF],[A_FC,A_FF+λI]] tam tersini (tek operatör) mc×mc yoğunlaşan Schur gerektirir; F-boyutu 513 ≪ mc olduğundan C'yi eleyip 513×513 Schur çözmek asimptotik olarak en ucuz yol (Cholesky 513³/3 kaçınılmaz). W_F, tüm eğitim istatistiklerine (A_FF,R_F,T,U) bağlı global sistem olduğundan "ara adımsız" üretilemez; kaldırılabilen sadece A_CC tarafı ve U'nun yoğunluğu.

## Ayrı sonuç: daha ucuz algoritma
Ok-başı kapalı form (+ seyrek U) **bulundu, kodlandı ve doğrulandı**: aynı matematiksel problem, grup çözümünde ≈150–180× az aritmetik. Ana sürüme bağlanmadı (varsayılan kapalı; bit-eşit değil, yalnız sayısal eşdeğer). **Yetişmeyenler:** T için dsyrk, W_C'nin örtük/faktörlü kullanımı, test PPL, 50k/100k testi (yalnız karmaşıklık analizi).
