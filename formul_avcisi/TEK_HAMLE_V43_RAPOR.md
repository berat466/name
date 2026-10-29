# TEK HAMLE V43 — 1, 2 ve 3 geçişle öğrenmeyi güçlendir

Kod: `cpp/tek_hamle_v43.cpp` (V42 kopyası; V42 ve öncesi dokunulmadı). Ham çıktılar: `v43_ham/`. Sistem: tek F×512, bağlantılı havuz (β=0.9), kelime+çift kimliği (C0), intercept, gruplu Ridge; tohum 1, tek bölünme. 1. geçiş V42 ile **birebir** aynı (PPL 124.92, doğr. CE 4.8753 korunuyor).

## Geçiş mekaniği
Geçiş t≥2: eğitim satırları için z=ZW_t (F kısmı + seyrek kimlik satırları), residual R_t, sonra ΔW=(ZᵀZ+Λ)⁻¹ZᵀR_t. ZᵀR_t: C kısmı seyrek toplama, F kısmı FᵀR (sgemm). Çözüm: **aynı** gruplu A_CC çarpanı (yeniden çarpanlanıyor, yalnız V sağ taraf), aynı Schur Cholesky'si (yeniden kullanıldı), X=A_CC⁻¹A_CF yeniden kullanıldı. F, havuz, Gram, X **yeniden üretilmedi**. W_{t+1}=W_t+ηΔW; η∈{0.25,0.5,1,1.5} **yalnız doğrulama CE'siyle** seçildi (test seçimde kullanılmadı). Doğrusal okuma α,ε doğrulamada yeniden seçildi (protokol gereği).
İki residual tanımı: **res0** doğrusal R=Y−z; **res1** okuma-tutarlı R=(Y−P̂)/α, P̂=(1−α)π+α·max(z,ε)/Σmax(z,ε).

## Matematiksel uyarı (ölçümden önce)
Doğrusal residual için: R=Y−ZW₁ ve W₁=(G+Λ)⁻¹ZᵀY ⇒ ZᵀR=ZᵀY−GW₁=ΛW₁ ⇒ **ΔW=(G+Λ)⁻¹ΛW₁**. Yani res0 geçişi yeni veri bilgisi taşımaz; yalnız W₁'in büzülmesini azaltır (iteratif iyileştirme; sınırı OLS çözümü). Geçişlerin "yeni öğrenme" sayılabilmesi için tek geçişli λ kontrolünü geçmesi gerekir. res1 okuma doğrusal olmadığından teorik olarak farklıdır (kırpma ve normalizasyon), ancak okuma çoğu satırda z>ε ve Σz≈1 bölgesinde afin olduğundan etkisinin küçük olması beklenir [Tahmin].

## Sonuçlar
λ_C=100. (a) λ_F=3000 (V42 seçimi), (b) λ_F=1000 (tek geçiş λ kontrolünün doğrulamada en iyisi).
| Yöntem | doğr. CE (PPL) | **test PPL** | Top-1 / Top-5 | η | ek süre (geçiş başına) |
|---|---|---|---|---|---|
| **V42, 1 geçiş, λ_F=3000** | 4.8753 (131.0) | **124.92** | 0.1610 / 0.3673 | – | – |
| V43 res0, 2 geçiş (λ_F=3000) | 4.8719 | 124.67 | 0.1622 / 0.3681 | 0.25 | +5.7 s* |
| V43 res0, 3 geçiş | 4.8722 (2. geçişten kötü → durdu) | 124.71 | 0.1647 / 0.3695 | 0.25 | +3.7 s* |
| V43 res1, 2 geçiş (λ_F=3000) | 4.8714 | 124.60 | 0.1620 / 0.3675 | 0.25 | +7.6 s* |
| V43 res1, 3 geçiş | 4.8715 (iyileşmedi) | 124.66 | 0.1647 / 0.3693 | 0.25 | +2.0 s |
| **Tek geçiş, λ kontrolü (λ_F=1000, doğrulamada en iyi)** | **4.8687 (130.1)** | **124.43** | 0.1624 / 0.3679 | – | 0 |
| V43 res0, 2 geçiş (λ_F=1000) | 4.8691 | 124.49 | 0.1637 / 0.3697 | 0.25 | +5.4 s* |
| V43 res1, 2 geçiş (λ_F=1000) | 4.8685 | 124.47 | 0.1637 / 0.3691 | 0.25 | +3.9 s |
| V43 res1, 3 geçiş (λ_F=1000) | 4.8706 (kötüleşti) | 124.62 | 0.1655 / 0.3715 | 0.25 | +3.0 s |
(*) süreler gürültülü (aynı iş 1.9–5.7 s). Geçiş başına ek süre bileşenleri (ör. res0, λ_F=3000, geçiş 2): residual + FᵀR **2.1 s** (eğitim logitleri n×513×2001 gemm ≈120 GFLOP + FᵀR aynı büyüklükte), gruplu+Schur çözüm **1.8 s**, η taraması (4 doğrulama değerlendirmesi) **1.8 s**; sonraki geçişlerde 0.9 / 0.5 / 1–2 s. η taramasını ve doğrulama değerlendirmesini çıkarmak ek süreyi yaklaşık yarıya indirir, ama yine de geçiş başı ≥ ~2 s.
Tek geçiş toplam süresi (λ_F=1000, 3 tekrar): F+havuz+Gram 0.93–1.06 s, C bloğu çözümü 0.39–1.03 s, λ_F çözümü 0.12–0.16 s → **≈1.5–2.2 s** (V42 medyanı 2.22 s ile tutarlı).

## Cevap: yeni öğrenme mi, aynı sonucun tekrarı mı?
- **Doğrusal residual (res0): aynı matematik.** ΔW=(G+Λ)⁻¹ΛW₁ özdeşliği ve ölçüm uyumlu: 2. geçiş η=0.25'te bile ancak doğrulama CE'sini 4.8753→4.8719 yapıyor; **tek geçişte λ_F'yi 3000→1000 yapmak daha iyi (4.8687, test 124.43)**. Üstelik λ_F=1000 üzerinde 2. geçiş hiçbir kazanç vermedi (4.8691). Yani res0, yalnızca daha küçük bir λ'nın kaba bir taklidi.
- **Okuma-tutarlı residual (res1):** λ_F=3000'de 4.8714 (res0'dan 0.0005 iyi), λ_F=1000'de 4.8685 (tek geçişten 0.0002 iyi, test 124.47 vs 124.43 — iyi değil). Fark gürültü içinde; ölçülebilir yeni bilgi yok. Okuma bölgesi neredeyse afin olduğundan beklenen buydu.
- Geçişlerle gözlenen tek tutarlı değişim Top-1/Top-5'te (+0.0013…0.0037) ama PPL'e yansımıyor ve λ kontrolüyle aynı yönde; 3. geçiş doğrulamada hep kötüleşti ya da durdu.
- Hedefler: PPL < 122.90 ve toplam süre < 7 s: **ikisi de sağlanmadı.** En iyi sonuç 124.43 (tek geçiş, daha iyi λ_F); 2 geçiş ≈ 6–8 s ek maliyetle aynı seviyede.

## Karar
Tek geçişte durmak daha verimli. Önerilen değişiklik 2–3 geçiş değil, λ_F ızgarasının inceltilmesi: λ_F=1000 doğrulamada 3000'den daha iyi (V42 ızgarası yalnız {3000,10000,…} idi, yani V42'nin λ_F seçimi alt kenardaydı). Yeni varsayılan tek geçiş; kodda `V43_PASSES=1` (varsayılan).

## Sınırlar
- Tek tohum/tek bölünme; η ızgarası {0.25…1.5}, en iyi η her zaman alt uçta (0.25) — daha küçük η (0.1) denenmedi ama kazanç zaten ihmal edilebilir.
- Yalnız iki residual tanımı; CE-gradyan ağırlıklı (IRLS benzeri) residual denenmedi.
- Süre ölçümleri gürültülü; geçiş maliyetinin baskın kısmı n×V boyutlu eğitim logit/FᵀR gemm'leri (ağırlık çözümünden pahalı).
