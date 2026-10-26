# Tek Hamle AI — V79: ağırlık üretiminde gereksiz matematik

Dal `claude/formul-avcisi` · taban V78 `83df01d` (dokunulmadı) · kod `cpp/tek_hamle_v79.cpp` · betik/ham çıktı `v79_agirlik_matematiksel_sadelestirme/` · süre: 30 dk sınırı, ~12 dk'da bitirildi (yeni bulgu kalmadı).

Etiketler: **[Kesin]** bu oturumda ölçülen/koddan okunan, **[Muhtemel]** güçlü çıkarım, **[Tahmin]** boşluk doldurma.

## 0. Sonuç (dürüst)

* **Ağırlık üretimindeki gereksiz işlem bulundu ama çok küçük:** tek bit-özdeş sadeleştirme = V49'da σ_j² toplamının 170 sütunda tekrar edilmesinin kaldırılması: **−5.0e7 madd (1.5e8 flop), %0.007**. Süre kazancı ölçülemedi (gürültü altı). Model hazırlama V78 5.23 s → V79 5.17 s (5 çift medyan; fark gürültü), tam program 8.62 → 8.81 s (gürültü, 5 çiftin 2'sinde V79 hızlı). **Hız hedefleri (model <4 s, program <8 s) tutmadı**; bunlar hedef, sonuç değil.
* **F1/rmsA 340 sütunu:** tam eşdeğer kapalı biçim **yok** (kanıt §2). Kaldırılamadı.
* Ağırlıklar V78 ile **bit-özdeş**: Fc, AFF, RF, ACF, RC, Y0f, X, T, U, W_F karmaları, val/test Top-1/Top-5, test CE/PPL (262.930), seçilen α/ε/α_Q — 2.001, 5.001, 10.001'de (`ham/d*k_7[89].txt`). Tek fark `lin_table` son hane (V78'in kendi `critical`-sırası kaynaklı gürültü; V78'i kendisiyle karşılaştırınca da çıkıyor).
* **Klasik ağırlık üretimi (isteğin ilk cümlesi):** hızlı yol bayrakları kapalı (`V76_LEGACY=1`, hiçbir V6x/V7x bayrağı) V78 ikilisiyle bu VM'de **bitmiyor**: 234.6 s'de 13.97 GB RSS'de işletim sistemi tarafından öldürüldü (RC −9, `ham/klasik.time`). Ulaştığı noktaya kadar: özellik hazırlığı 13.05 s + slot sayımı 29.13 s + C-bloğu Cholesky/Schur hazırlığı 184.93 s ≈ **227 s** ve W_F'ye hâlâ ulaşılmamıştı. Yani klasik/hızlı ≥ 227 / 5.2 ≈ **≥43×** (alt sınır, ağırlıklar çıkmadı) ve RAM ≥ 13.97 / 1.53 ≈ **≥9×**. Doğru bir "kaç kat" için klasik yolun daha büyük RAM'li makinede bitmesi gerekir.

## 1. Ağırlık üretiminin parçalanması (V78, 10.001, `ham/A78_3.txt`, model hazırlama ≈5.2 s)

| aşama | süre | gerçek aritmetik | not |
|---|---|---|---|
| veri okuma + `expand_s` | 0.14 + 0.11 s | kopya | bellek |
| 1. F1 (gemm 0.22 + tanh 0.21 + tahsis/sıfırlama 0.20) | 0.67 s | gemm 294 816×192×512 = **2.90e10 madd**, tanh 1.5e8 | tahsis+sıfırlama aritmetik değil |
| 5. K-grafı + s(K) (6 tablo) | 1.19 s (K+kapı toplam 1.59) | seyrek kNN 0.54 Gmadd + s(K) | **en pahalı duvar süresi** |
| 6. kapı (tanh toplama formülü) | 0.15 s | 1.9e8 bölme | |
| V39 ortalama + karışım | 0.08 + 0.17 s | 1.5e8 ekleme + 1.9e9 madd | |
| V49 (μ, σ, cm/cs, apply) | 0.55 s | ~6e8 işlem | |
| 2. FᵀF | 0.50 s | **3.89e10 madd** (syrk yarım) | **en pahalı aritmetik** |
| 3. FᵀY | 0.13 s | 1.5e8 ekleme | |
| 4. slot toplamları | 0.21 s (+ id 0.07) | 3.0e8 ekleme | |
| 7. RC/Y0 (kapalı form) | 0.09 s | ~3.8e7 | |
| 8. T | 0.19 s | **1.12e10 madd** (dgemm) | |
| 8. U (CSC, gerçek sıfır-olmayan) | 0.12 s | 1.43e8 | |
| 9. Schur + W_F | 0.14 s | Cholesky 4.5e7 + trsm 2.6e9 | |
| Q | 0.12 s | O(nnz) | |
| 10. örtük W_C | ~0 | — (V64'ten beri hiç oluşmuyor) | |

**Soru 1:** aritmetik olarak en pahalı: **FᵀF (3.89e10 madd)**, sonra F1 gemm (2.9e10) ve T (1.12e10); duvar süresinde en pahalı: **K-grafı/s(K)** (1.2–1.6 s). [Kesin: kod + zamanlayıcı; madd boyutlardan]

## 2. F1 / rmsA — tam eşdeğer hesap mümkün mü? **Hayır.**

Koddan: `rmsA = mean_{j<512} ( sqrt( (1/n) Σ_i (x_ij − μ_j)² ) + 1e-12 )`, burada eğitim satırı i için
x_ij = **float**( V39karışım( kapı( **float**( tanh( γ (X_i·W1[:,j] + sparse_i + b_j) ) ), T_K[anahtar_i, j] ) ) ).

* Gram / norm / ortak projeksiyon yalnız z = X·W1 için (doğrusal) kullanılabilir. σ_j² ise z'nin **tanh → kapı (f+t)/(1+ft) → komşu-ortalama karışımı → float yuvarlama** zincirinin karesinin toplamıdır; bu bileşke z'nin polinomu değil. Σ_i φ_j(z_i)² için z'nin ikinci momentinden (Gram) çıkan bir kapalı özdeşlik yok: momentler yalnız φ doğrusal/ikinci derece olsaydı yeterliydi.
* Kapı satırı i'nin (fold, son kelime) anahtarına bağlı T_K ile, karışım ise son-kelime komşu ortalamalarına bağlı; yani bileşke her satırda farklı bir fonksiyon.
* Bit-özdeş (veya "tam eşdeğer") rmsA, ara `float` yuvarlamalarını da içerir; bunlar kapalı biçimle yeniden üretilemez.
* Dolayısıyla 340 sütun **eğitim satırlarında oluşturulmak zorunda**. Zaten V77'den beri val/test satırlarında oluşturulmuyor. Eğitimdeki payı ≈ F1 gemm'in %66'sı (~0.15 s) + tanh (~0.14 s) + kapı + karışım + μ/σ geçişleri ≈ 0.5–0.7 s [Tahmin: orantılı payla]. Yaklaşık/örneklemeli rmsA kullanılmadı (kural).

## 3. FᵀF / FᵀY / U / T / Schur — bağımlılık incelemesi

| soru | cevap |
|---|---|
| FᵀF ve FᵀY ortak veriden daha az madd? | FᵀY one-hot toplamıdır (çarpma yok, 1.5e8 ekleme); FᵀF tam Gram (3.89e10). Y bloğunu Gram'a katmak ek çarpma getirmez ama toplamı da azaltmaz. Birleştirme yok. |
| FᵀF simetrik yarım mı? | Evet, V73'ten beri. Sabit sütun satırı/sütunu 513/(513²/2) = %0.4 — ihmal. |
| T: aynı katkı tekrar mı? | T simetrik ⇒ Cholesky yalnız üst üçgeni okur ⇒ üst üçgen blokları (madd −3.7e9). **Tekrar denendi (`V77_TSYM=1` V79'da):** 5.001 ve 10.001'de W_F karması aynı, ama **2.001'de W_F karması değişti** (`3630d647bdb0675a` → `7fb4ab0fe5d1c31e`, `ham/tsym1_2k_79.txt`). Bit-özdeşlik sağlanmadığı için varsayılan **kapalı** kaldı. |
| T = Z'Z (rank-1 toplam) | V78'de ölçüldü: madd yarıya iniyor, süre aynı, W_F değişiyor. Kapalı. |
| U | V72 CSC zaten yalnız 279 500 gerçek sıfır-olmayan × 513. Tekrar yok. |
| Schur matrisinin tüm elemanları zorunlu mu? | Cholesky yalnız üst üçgeni okur, alt üçgen zaten kullanılmıyor (V77 NaN-zehir kanıtı). 513² oluşturma ihmal edilebilir; baskın iş V sağ-taraflı iki trsm (2.6e9) ve her sağ taraf sütunu W_F için zorunlu. Daha az işlemle aynı çözüm veren bloklu/örtük biçim bulunamadı. |
| K-grafı | OOF tablosu f = tam − fold f sayımları **doğrusal**, ama benzerlik **√sayım** üzerinde: Gram(√(C−D)) tam Gram'dan doğrusal türetilemez. Fold'da değişmeyen satırlar yalnız %35–37 ve rare kelimeler (V78 raporunda numpy ölçümü) → yeniden kullanım küçük. V60 seyrek matematiği korundu. |

## 4. Uygulanan tek sadeleştirme: σ_j² tekrarının kaldırılması (`V79_SD`)

* **Eşdeğerlik:** V49 `mod=2`'de A sütunu j<170 için (a) μ/σ geçişi σ_j² = Σ_i (x_ij − μ_j)² toplar, (b) cm/cs geçişi p=2 türev sütununun toplamı sa[3j+1] = Σ_i d² toplar; d = x_ij − μ_j − 0.0 = x_ij − μ_j (0.0 çıkarmak bit değiştirmez), her ikisi aynı i sırasında aynı çift duyarlıkla toplanır ⇒ **aynı bit**. Bu yüzden σ_j² (j<170) cm/cs geçişinden alınır, μ/σ geçişi yalnız j ≥ 170 için çalışır; `rmsA` aynı sırayla (j = 0..511) toplanır.
* **Kanıt:** Fc, AFF, RF, ACF, RC, Y0f, X, T, U, W_F karmaları V78 ile aynı (2.001, 5.001, 10.001) ⇒ rmsA birebir aynı.

| ölçüm | eski (V78) | yeni (V79) |
|---|---|---|
| σ geçişi: sütun sayısı | 512 | 342 |
| çarpma (d·d) | 1.51e8 | 1.01e8 (**−5.0e7**) |
| toplama (çıkarma + ekleme) | 3.02e8 | 2.02e8 (**−1.0e8**) |
| toplam madd (mul+add çifti) | 1.51e8 | 1.01e8 (**−5.0e7**) |
| eklenen aritmetik | — | 170 kopya (sa→sd) |
| bellek trafiği (F okuma) | 512 sütun×n×4 B | 342 sütun (**170 sütun × 294 816 × 4 B ≈ −0.2 GB okuma**) |
| aşama süresi (V49 μ/σ+cm/cs) | 0.47 s | ölçülebilir fark yok (gürültü ±0.05) |
| ağırlık karması | `d54c462f219c825b` (W_F, 10.001) | aynı |

Bu bir **gerçek** işlem azaltımıdır (hesap başka yere taşınmadı) ama V78 toplamının %0.007'si; süreye yansımıyor.

## 5. A/B (aynı oturum, 5 dönüşümlü çift, 10.001, 4 iş parçacığı; `ham/A78_*`, `ham/B79_*`)

| | V78 medyan [min–maks] | V79 medyan [min–maks] |
|---|---|---|
| model hazırlama | 5.23 [4.84–5.61] | 5.17 [4.99–5.68] |
| tam program | 8.62 [8.21–9.52] | 8.81 [8.41–9.25] |
| çift farkları (V79−V78) | +0.60, +0.16, +0.63, −0.58, −0.31 | işaret karışık ⇒ fark yok |
| test CE / PPL | 5.571887 / 262.930 | aynı |
| RSS | 1530 MB | 1530 MB |

Sonuç: **V79 V78'e göre ölçülebilir hız kazandırmıyor** (bu oturumda V78'in kendisi 8.6 s çıktı, görevdeki 9.24 s'den hızlı: VM durumu). İşlem azaltımı gerçek ama küçük.

## 6. Tamamlanmamış / denenmemiş adaylar

* **ACF kelime-slotu = Σ çift slotları + kalan:** geçerli özdeşlik (ok-başı yapı), −~6.5e7 ekleme, ama float toplama sırası değişir ⇒ bit-özdeş değil; ölçülmedi, bağlanmadı.
* **RC kelime satırı = Σ çift satırları + kalan:** sayımlar tamsayı olduğundan bit-özdeş olurdu, ama yalnız 6e5 toplama ⇒ değmez.
* **F tahsis + sıfırlama (0.20 s):** aritmetik değil, tür değişikliği gerektirir.
* **K-grafı (en pahalı duvar):** √ nedeniyle doğrusal türetme yok; yeni sadeleştirme bulunamadı.

## 7. Soruların cevabı

1. En pahalı aşama: aritmetikte **FᵀF** (3.89e10 madd), duvarda **K-grafı/s(K)** (1.2–1.6 s).
2. Tamamen kaldırılan: V49'da 170 sütunun σ² toplamı (tekrar).
3. Azalan: **−5.0e7 madd** (−1.5e8 flop).
4. Eklenen: 170 skaler kopya (ihmal).
5. Net: **−5.0e7 madd ≈ −%0.007** (V78 toplamı ≈5e11).
6. Model hazırlama: 5.17 s (V78 5.23 s; fark yok).
7. Tam program: 8.81 s (V78 8.62 s; fark gürültü).
8. Ağırlıklar V78 ile aynı mı: **evet, bit-özdeş** (tüm karmalar).
9. Test PPL: **262.930 korundu**.
10. Zorunlu kalan: F1 gemm (tüm 512 sütun eğitimde, rmsA için), FᵀF tam Gram, K-grafı √-sayım benzerliği, T (dgemm; bit-özdeşlik için), U, Schur'un V sağ-taraflı iki trsm'i, slot toplamları, kapı/V39 karışımı.

## 8. Yeniden üretim
```
cd cpp && g++ -O3 -march=native -fopenmp -std=c++17 -o tek_hamle_v79 tek_hamle_v79.cpp -L$SL -l:libscipy_openblas*.so -Wl,-rpath,$SL
cd v79_agirlik_matematiksel_sadelestirme && ./dogrula.sh d10k /tmp/v59_10001 && ./klasik.sh
```
Bayrak: `V79_SD=0` V78 yoluna döner.
