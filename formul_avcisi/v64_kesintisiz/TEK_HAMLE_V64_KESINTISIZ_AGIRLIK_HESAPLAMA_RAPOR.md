# V64: ağırlıkları gereksiz ara matris oluşturmadan üret — sonuç: BAŞARILI (varsayılan KAPALI; mantıksal eşdeğer, bit-eşit değil)

Kod: `cpp/tek_hamle_v64.cpp` (V63 + `V64_IMPL=1` örtük W_C, `V64_SPQ=1` seyrek Q_havuz, `V64_COMPACT=1` kompakt ok-başı + V×V `seen/posy` kaldırma; V63_ARROW=2 ile birlikte). V61/V63 dosyaları değişmedi. Veri/ayar: V59 ortak veri, hafif ayar, LEAN+FAST; A/B/A/B aynı oturumda ardışık (V63=a, V64=b).

## Matematik
1. **Örtük W_C.** W_C=Y0−XW_F ⇒ z=C^TW_C+F^TW_F = C^TY0+(F^T−C^TX)W_F. Örnek i için F̃ᵢ=Fᵢ−Σ_{slot∈i}X_slot (≤2 slot), z=F̃ᵢW_F+Σ_slot Y0_slot (Y0 kompakt/seyrek). Aynı sgemm (n×513×V) F·W_F yerine F̃·W_F; mc×513×V sgemm ve mc×V yoğun W_C yazımı yok. **W_C artık açık üretilmiyor** (örtük gösterim; tüm ağırlıkları açıkça isteyen tüketici için `V64_IMPL=0`).
2. **Q_havuz.** Tüketici: h_fuse'ta yalnız örnek başına tek satır q_w (context son kelimesi). q_w = sparse(≤11 satır, RC_w/n_w ve komşu karışımı) + c_w·π (veya Ptr) → satır-başına seyrek liste + tek katsayı; V×V `Qt` ve V×V `Pn` hiç oluşmuyor, her örnekte q vektörü O(V) ile yeniden kurulup h_fuse'a veriliyor (h_fuse zaten O(V)/örnek). Formül h_qtab ile aynı, normalizasyon toplamı yalnız sıralı toplam farkıyla (~1e-16).
3. **A_CC kompakt.** Ok-başı yapıda A[w,p]=A[p,p]=n_p ⇒ grup için gereken tek veri slot sayıları n_k. `ACCB` (Σsg² double) yerine `NK[mc]`; `seen`(V×V char) ve `posy`(V×V int) yerine grup başına sıralı y listesi + ikili arama (aynı sy sıralaması).

## Doğruluk (2.001, test dahil, HASH+dump)
- Val CE 4,503665 / **Test CE 4,487430, PPL 88,893**, Top1 0,2151 — V63 ve V64'te **aynı**.
- Val logitleri (Zval, 73,7 M eleman): V63↔V64(IMPL+SPQ) **maks |Δz|=4,8e-7** (maks |z|=0,987; elemanların %95'i son bitte farklı: yuvarlama sırası, sonlu). Kompakt mod (COMPACT) ↔ kompaktsız V64: **0 farklı eleman** (bit-eşit).
- Ok-başı rezidu (kompakt modda NK ile ölçülen): 1,85e-14 (2k), 3,0e-14 (10k); KABUL.
- 5.001 / 10.001 Val CE: 5,079437 / 5,441941 her dörtlü koşuda aynı.
İlk ayrışma: Zval'de F̃W_F+Y0 toplama sırası (eski: (F W_F)+(Y0−XW_F) float). Eski ağırlıklar kopyalanmadı.

## Ölçüm (model hazır = V53H_MODEL_HAZIR, değerlendirme hariç; tepe RSS = MAXRSS)
| Ölçüm | 2.001 V63 (a1,a2) → V64 (b1,b2) | 5.001 V63 → V64 | 10.001 V63 → V64 | 10.001 V64+COMPACT |
|---|---|---|---|---|
| Model hazır süresi | 8,5; 6,7 → 6,5; 5,9 s | 18,5; 13,8 → 9,4; 8,0 s | 29,5; 26,1 → **10,1; 9,8 s** | 10,0 / 9,2 / 9,25 s |
| Tepe RSS | 1,39 → 1,39 GB | 3,01 → **1,89 GB** | 6,32 → **3,12 GB** | **2,67 GB** |
| W_C oluşturma (g11) | 0,26–0,28 → 0 s | 2,0–2,7 → 0 s | 4,4–4,8 → **0 s** | 0 |
| Q_havuz tablosu (q1) | 0,04 → 0,05 s | 1,4–1,7 → 0,05 s | 5,8–6,1 → **0,14 s** | – |
| U (g9, seyrek XᵀR_C) | 0,7–1,0 | 1,9–2,1 | 2,9–3,0 | 2,9 |
| Grup çözümü g45 (ok-başı) | 0,09 | 0,11–0,16 | 0,13–0,16 | – |
| Val CE | 4,503665 | 5,079437 | 5,441941 | 5,441941 |
Süreler iki ardışık tekrarda aynı yönde ve tutarlı (10.001'de 2,6–3× hızlanma); 2.001'de fark küçük/gürültü içinde.
**Kaldırılan işlem (madd):** W_C sgemm mc·513·V = 3,0e10 / 9,5e10 / 2,2e11 (2k/5k/10k) → 0; yerine F̃ oluşturma n·2·513 (≈1,5e7, ihmal). **Kaldırılan bellek (10.001):** W_C 1,71 GB + Pn 0,80 GB + Qt 0,80 GB = 3,3 GB (ölçülen RSS farkı 3,2 GB); COMPACT ek: `seen`+`posy` 0,5 GB (ölçülen −0,4 GB).
**Tahmin süresi:** val logit+okuma seçimi (e1a, 14 940 örnek; lin_table seçimi dahil, gürültülü) 12,2 → 4,3–10,5 s; temiz "1.000 örnek" süresi **ölçülemedi** (e1a kararlı değil) → kesin sonuç olarak sunmuyorum.

## Sorular
1. **V64 model hazırlama:** 10.001'de 9,2–10,1 s (V63: 26–29 s); 5.001'de 8,0–9,4 s; 2.001'de 5,9–6,5 s.
2. **V63'e göre kazanç:** ≈16–20 s (10k), ≈5–10 s (5k), ≈1–2 s (2k, gürültü).
3. **Kaldırılan aritmetik:** 10k'da 2,2e11 madd (W_C) (+ V×V Q/Pn kurulumu ≈ 2·V²·knn).
4. **RAM tasarrufu:** 10k'da 6,32→3,12 GB (−3,2 GB); COMPACT ile 2,67 GB (−3,65 GB toplam). 5k'da −1,1 GB.
5. **Tamamen kaldırılan ara matrisler:** W_C (mc×V), Pn (V×V), Qt (V×V), ACCB (Σsg²), seen (V×V), posy (V×V). (V61'den: 6×Q, Cfull.)
6. **Ağırlık/logit aynı mı?** W_C artık oluşmuyor. Logitler: maks fark 4,8e-7 (yuvarlama), CE/PPL aynı.
7. **Val CE / test PPL:** değişmedi (4,503665 / 88,893 @2k).
8. **50k–100k:** V×V yoğun yapı kalmadı (kod taramasıyla: Pn, Qt, seen, posy, ACCB, W_C hepsi kapalı/yok); kalan bellek **doğrusal**: ACF/Xd (double) + Xf mc×513 (≈ mc·20 B·513/… 50k kelimede mc~1e5–5e5 ⇒ 1–4 GB ×3), U/RF/WF mf×V (513·V·(8+8+4) B ⇒ 1,3 GB @50k, 2,6 GB @100k), değerlendirme için n×V logit (n=val örnek sayısı; 100k'da 15 k örnekte 6 GB, satır bloklarıyla akıtılabilir; uygulanmadı). Gizli V² yok; ama **uçtan uca 50k/100k gerçek veri koşusu yapılmadı** (veri yok); sentetik kNN+s(K) testi V61'de yapıldı.
9. **Kalan en pahalı zorunlu hesap:** U=XᵀR_C (2,9 s @10k, 2·mf·nnz(R_C)) ve Gram/K-tablo/özellik aşamaları (f0 4,8 s: F üretimi, FᵀF, FᵀY); değerlendirme (lin_table ve h_fuse: n×V). Zorunlu minimum: nnz(R_C)·513 madd, 513³/3 (Schur) ve V·513² (W_F).

## Dürüstlük notları / yetişmeyenler
- Q_havuz 'tamamen seyrek' değil: örnek başına q vektörü V uzunluğunda yeniden kurulur (h_fuse'ın O(V)/örnek maliyetiyle aynı mertebe, bellek V×V yok). h_fuse 9× (alpha taraması) çağrılıyor; bu maliyet ölçülmedi ayrı.
- Test PPL yalnız 2.001'de ölçüldü (88,893, aynı). 5k/10k test PPL ölçülmedi.
- `V64_IMPL`/`V64_SPQ`/`V64_COMPACT` varsayılan KAPALI (bit-eşit değil; yalnız mantıksal eşdeğer + CE aynı); ana sürüme bağlanmadı.
- Değerlendirme logit akıtması (n×V blokları), T için dsyrk, U için daha az bellek hareketi yapılmadı.
