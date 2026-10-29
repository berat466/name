# V76: tek formülün baştan sona sadeleştirilmesi — sonuç: BÜTÜN PROGRAM 17,1 s → 12,9 s (10.001 kelime, medyan); doğrulanmış, bit-eşit

Kod: `cpp/tek_hamle_v76.cpp` (V75 + aşağıdaki değişiklikler; **varsayılan yol = bütün doğrulanmış hızlı yollar açık**, `V76_LEGACY=1` eski bayrak modeli). Betikler: `v76_sadelestirme/kos.sh` (tek koşu), `ab.sh` (V75↔V76 dönüşümlü 5 çift), `dogrula.sh` (karma/CE doğrulaması), ham çıktılar `ham/`, tablolar `ab_tablo.txt`, `ab_ozet.json`. V65–V75 dosyaları değişmedi. Formül, çekirdek, havuz aynı; yeni öğrenme algoritması yok.
Başlangıçta program hızlı yolların açık olduğunu basar: `V76_YOL|ok-basi=2 ortuk_WC=1 seyrek_Q=1 kompakt=1 CSC_U=1 RHS=1 SYRK=1 akis_val_test=1 AVX512_8alfa=1 ... seyrek K-graf=1 seyrek Q/Cn=1` (herhangi biri kapalıysa `V76_UYARI`).

## 1) Matematiksel bağımlılık grafiği ve sınıflandırma (10.001; n=295 998 eğitim, 36 852 val, 36 852 test; mf=513, mc=42 651)
Nihai çıktı: z(x) = (F−Σₛ Xₛ)·W_F + Σₛ Y0ₛ ; olasılık = (1−α_Q)[(1−α)Ptr + α·max(z,ε)/Σmax(z,ε)] + α_Q·q_w (tüm V sınıfı üzerinde); seçim (α,ε,α_Q) yalnız validation'dan.

| Düğüm | Ürettiği | Zorunlu mu | Sonuç / kısa gerekçe |
|---|---|---|---|
| F1 = tanh(γ(XW+b)) | n×513 | Evet | gemm n·192·512 = 2,9e10 madd; X'in 3. bloğu D=e₂−e₁ doğrusal bağımlı (K 192→128 yapılabilir, 1/3 az madd) ama float yuvarlaması değişir → bit-eşit değil → **uygulanmadı** |
| 6 K-tablosu (OOF) → seyrek kNN → s(K) → anahtarlı kapı (U-tablo) → F kapısı | graf, 37 787×512 tablo | Evet (model tanımı) | V60/V61 seyrek: 0,27 G madd (yoğun V³: 6,0e12). **Bu görevde:** Uu/Tt tahsisleri seri sıfırlama yerine paralel ilk-dokunuş (aşağıda) |
| V39 ortalama/karışım | Mean[V×513] | Evet | küçük |
| V49 (A, (A−μ)², (A−μ)³ standartlaştırma) | F'nin 513 sütunu | Evet | 4 geçiş (μ/σ, cm/cs, uygula): hepsi F'yi okur; ham-moment tek geçişe birleştirmek bit-eşitliği bozar → uygulanmadı |
| FᵀF, FᵀY, slot toplamları (ACF, RC, nₖ) | AFF, RF, ACF, RCc, NK | Evet | aynı F'yi 3 tüketici okur (≈1,8 GB); tek geçişte birleştirmek RF'nin toplama sırasını değiştirir (bit-eşitlik) → uygulanmadı. FᵀF: ssyrk yarım (V73) |
| FᵀY parça toplamı | RF[513×V] | Evet | V73: sınıf-ana parça; **V76:** parçaların birleştirilmesi [c][j]→[j][c] 64×64 karoyla (aynı toplama sırası, bit-eşit) |
| A_CC grup çözümü → X, Y0 | Xd/Xf [mc×513], Y0c | Evet | V63 ok-başı kapalı form (n_k'dan); Cholesky yok |
| T = A_CFᵀX | 513×513 | Evet | dgemm 1,12e10 madd. T=ZᵀZ+dsyrk 5,6e9 (V71) var ama +175 MB RAM, ölçülen süre kazancı (≈0,08 s) gürültüde → **bağlanmadı** |
| U = XᵀR_C | 513×V | Evet (min.) | V72 CSC: nnz(R_C)·513 = 1,43e8 madd (eski 8,5e9) |
| Schur: Cholesky 513³/3 (4,5e7), W_F çözümü mf²·V (2,6e9) | W_F | Evet | kaçınılmaz doğrusal cebir (513'lük sistem) |
| W_C = Y0 − X W_F | – | **Kaldırıldı (V64)** | örtük okuma |
| Q_havuz satırları | – | Evet | V64: V×V yok, satır başına seyrek+π |
| Val/test logitleri z (n×V) ve tam softmax | blok akışı | Evet (3 gemm) | 3 × 1,89e11 = 5,67e11 madd; val ikinci gemm'i **tek kaçınılabilir aritmetik tekrar** (aşağıda) |
| Seçim (lin tablosu, α_Q, rank/Top-k) | – | Evet | tüm V üzerinden; V76: AVX-512 şeritleri |

## 2) Bu görevde bulunan ve uygulanan iyileştirmeler (her biri: eşdeğerlik → kod → gerçek veri → doğrulama → ölçüm → karar)
**A. Val/test akışının vektörleştirilmesi (8 eps ve 8 α şeritte).** Eşdeğerlik: lin tablosunda S_e = Σ_v max(z_v,ε_e) (8 ε) ve 8 α için p_v=(1−α)pm_v+α q_v, sm=Σp, py=p_y/sm, rank=#{p_v/sm>py} — her şerit skaler yoldaki aynı işlem dizisi (`_mm512_max/add/fmadd/div`, toplama v artan). Kod: `fuseRow8` ve lin AVX bloğu (`V76_VEC`, varsayılan açık; skaler yol `V76_VEC=0`). Doğrulama: 8 α için CE (15 hane), Top-1/Top-5 sayıları ve test CE/Top-k **skaler (V75 kodu) ile birebir aynı**; yalnızca lin tablosunun ε/α toplamlarında son hanede (1e-15) fark var — V75'te de iş parçacığı sırasına bağlı `critical` toplamından kaynaklanan aynı kaynak. Ölçüm: val aşama-1 3,38→1,63 s, aşama-2 4,11→2,63 s. **Bağlandı.**
**B. Uyarlanabilir blok boyutu (tampon bütçesi 320 MB → V=10.001'de 8192 satır).** OpenBLAS sgemm her çağrıda B(513×V)'yi paketler; küçük M'de çağrı başına maliyet baskın (256 satır: 28 s; 512: 19 s; 1024: 17 s; 2048: 14,8 s; 4096: 13,7 s; 8192: 13,1–16 s; 16384: 11,4–12,7 s, hep V76_VEC açık). Tampon V ile ters orantılı (genel; V=100k'da 800 satır). Test: 2,84→2,06 s. **Bağlandı** (hesap aynı; yalnız parça sınırı; sgemm satır-bağımsız → bit-eşit; CE/PPL aynı).
**C. Seri sıfırlama yerine paralel ilk-dokunuş.** Tt (nu×512 double, 155 MB) ve Uu (77 MB) `vector` seri sıfırlama + seri sayfa hatasıydı; `ubuf` (tamamı yazılıyor) + paralel yazım. Ölçüm: gate (k8) 1,10→0,45 s, K+kapı 2,56→1,93 s (aynı oturum, 3 çift). Ek: `operator new` ön-dokunuşu (V54'te kapalıydı) varsayılan açık (≈ −0,2…0,3 s, gürültü sınırında). RF birleştirme karolama: bit-eşit (RF karması aynı). **Bağlandı.**
**D. V76 varsayılan yolu.** Bayraklar elle verilmiyor; açıkça 0 verilmedikçe açık. **Bağlandı.**

## 3) Denenen ve KAZANÇ SAĞLAMAYAN / BAĞLANMAYAN adaylar (nedeniyle)
| Aday | Sonuç | Neden |
|---|---|---|
| Val logitlerini önbellekte tutup ikinci gemm'i kaldırmak (`V76_ZCACHE`) | Daha yavaş (12,4–16,2 s vs 12,0) | 1,47 GB taze bellek: SYS 10–15 s (bu VM'de ilk-dokunuş çok pahalı), RSS 1,6→2,7 GB. **Kaçınılabilir aritmetik: 1,89e11 madd (≈1,4 s)** — bellek takası kaybettiriyor |
| THP (MADV_HUGEPAGE) | medyan 13,4 vs 12,1 s | sıkıştırma duraklamaları, SYS dalgalanması |
| Serbest bırakılan büyük blokları havuzlama (`V76_POOL`) | yavaş (13,8–14,6 s), RSS 2,9 GB | havuz bellek biriktiriyor → daha fazla ilk-dokunuş |
| OMP_WAIT_POLICY=PASSIVE | fark yok | |
| V71 T=ZᵀZ (dsyrk) | madd −5,6e9, süre ±0 | +175 MB RAM, gürültüde |
| F1 için K 192→128 | bit-eşit değil | X'in 3. bloğu doğrusal bağımlı; float yuvarlama değişir |
| V49 ham-moment tek geçiş, FᵀF/FᵀY/slot tek geçiş | bit-eşit değil | toplama sırası değişir |
| kNN iç döngüsü sadeleştirme (K-blok emülasyonu kaldırma) | uygulanmadı | sgemm bitlerini zaten yakalayamıyor ama V60/V75 ile bit-eşitliği bozar; kazanç ≈0,15–0,3 s |

## 4) Ölçüm (10.001 kelime, bütün program; aynı oturum, 5 dönüşümlü çift, 4 iş parçacığı)
| Ölçüm (s, medyan [min–max]) | V75 | V76 |
|---|---|---|
| **Bütün program** | **17,10 [16,87–17,15]** | **12,85 [12,40–13,23]** (−4,25 s, −24,9 %) |
| Model hazırlama (`MODEL_HAZIR`) | 6,57 [6,28–6,84] | 6,41 [6,03–6,45] |
| F üretimi (f1) | 0,81 | 0,78 |
| FᵀF / FᵀY | 0,53 / 0,43 | 0,51 / 0,46 |
| K-grafı + kapı (f2) | 1,85 | 1,73 |
| V49 | 0,93 | 0,88 |
| U / T / Schur / grup çözümü | 0,14 / 0,21 / 0,19 / 0,15 | 0,13 / 0,19 / 0,16 / 0,16 |
| Validation (aşama-1 + aşama-2) | 3,38 + 4,11 = 7,49 | 1,63 + 2,63 = 4,26 |
| Test | 2,84 | 2,06 |
| Kullanıcı CPU süresi | 57,1 | 41,9 |
| Tepe RAM (MAXRSS) | 1,563 GB | 1,563 GB |
| Test PPL / CE / Top-1 / Top-5 | 262,930 / 5,571887 / 13,70 % / 31,12 % | 262,930 / 5,571887 / 13,70 % / 31,12 % |
| Val CE / PPL | 5,441941 / 230,890 | 5,441941 / 230,890 |
Tekrarlar: V75 [16,99; 17,15; 16,87; 17,10; 17,15], V76 [13,23; 12,85; 12,40; 12,86; 12,77]. VM dalgalanması: aynı ikilinin bütün-program süresi oturumlar arasında 11,4–15 s (V76) ve 16–19 s (V75) arasında oynadı; bu tabloda karşılaştırma yalnız aynı oturumdaki dönüşümlü çiftlerdir. Önceki oturumlarda V75 17,1–20,4 s, V76'nın en hızlı koşusu 11,4 s.
**Model hazırlama süresi neredeyse değişmedi (−0,2 s):** kazanç tamamen değerlendirme (val+test) aşamasından (10,4 → 6,3 s).

## 5) Doğruluk
- `dogrula.sh` (10.001): AFF `26401df5ea0b1dea`, RF `18b4555f8bd71339`, ACF `8ea5f941b27af912`, X `e2858fa1ad353f51`, U `bc55ec1d32364d8d`, W_F `d54c462f219c825b` — **V73 referansıyla bit-eşit** (RF birleştirme karolaması dahil). 2.001: AFF `e361bcb1efb2548e`, RF `b0b28f8c4c793346`, ACF `a63960be3a8cae69`, U `8702ee7932712763`, W_F `3630d647bdb0675a` — V73/V75 ile aynı.
- Val/test CE, PPL, Top-1/Top-5: 2.001, 5.001, 10.001'de V75 ile aynı (altı hane); 8 α için CE 15 hane ve Top-k sayıları skaler yolla aynı.
- Logit (Zval) karması akış yolunda üretilmez (n×V saklanmıyor); eşdeğerlik CE(15 hane)/Top-k üzerinden. Rezidu (ok-başı) 3e-14 mertebesi (V64 raporu). Ağırlıklar sonlu.

## 6) Sözlük ölçeği (aynı kod yolu; tüm hızlı yollar açık)
| V | eğitim örneği | val/test örneği | bütün program | model hazır | tepe RAM | test PPL |
|---|---|---|---|---|---|---|
| 2.001 | 296 k | 36,9 k | 5,9–6,1 s | 4,2–4,4 s | 1,35 GB | 88,893 |
| 5.001 | 296 k | 36,9 k | 8,5–9,0 s | 5,1–5,5 s | 1,51 GB | 173,827 |
| 10.001 | 296 k | 36,9 k | 12,7 s | 6,3–6,5 s | 1,56 GB | 262,930 |
| 20.001 (gerçek: 12 kitaplık genişletilmiş veri, **farklı veri**: 4,3× eğitim, 4,3× val/test) | 1,28 M | 160 k | 85–93 s | 31–37 s | 6,44 GB | 252,531 |
20.001 satırı ölçekleme kanıtıdır, 10.001 ile PPL karşılaştırması yapılamaz (farklı veri). Eval maliyeti ∝ n_val·V·513 (=1,6e12 madd ×3 @20k).
**50.000 / 100.000:** gerçek veri yok (eğitimde ≤ 30 876 farklı kelime) → eğitim yapılmadı. Kod yolunda V×V / V³ yok. V ile büyüyen zorunlu bellek: W_F 513·V·4 B (100k: 205 MB), RF/U/Schur sağ taraf mf×V double ×3 (100k: ≈1,2 GB), akış tamponu sabit 320 MB (satır sayısı V ile ters), QRows/CSC nnz'ye bağlı (V'den bağımsız). Zorunlu hesap: eval 3·n_eval·513·V madd (doğrusal V), W_F çözümü 513²·V, FᵀY n·513 (V'den bağımsız) — hepsi doğrusal; V=100k'da 36,9 k val/test için eval gemm ≈ 5,7e12 madd (≈ 45 s; ölçülmedi, 10k'dan doğrusal ekstrapolasyon).

## 7) Aritmetik ve bellek hareketi (10.001; analitik sayım, ölçülen süre ayrı)
| Kalem | V75 | V76 |
|---|---|---|
| Eval logit gemm (3 geçiş) | 5,67e11 madd | 5,67e11 madd (aynı) |
| lin tablosu ε-toplamları (skaler komut) | 36 852·8·V = 2,9e9 | 3,7e8 AVX-512 komutu (÷8) |
| 8-α sm + rank geçişleri | ≈5,9e9 skaler | ≈7,4e8 AVX-512 komutu (÷8) |
| Eval blok başına B(513×V) paketleme | 18 blok × 3 geçiş | 5 blok × 3 geçiş (−13 × 3 paketleme ≈ 3,2 GB okuma/yazma) |
| Tt/Uu seri sıfırlama | 232 MB seri yazma + seri ilk-dokunuş | paralel, sıfırlama yok |
| Diğer model aşamaları | aynı | aynı |
**Kaldırılan aritmetik: V75'e göre 0 madd** (formül aynı, gemm 5,67e11 aynı); kazanç komut/paketleme/bellek-hareketi düzeyinde (≈ 7,4e9 skaler komut ≈ 1,1e9 vektör komutu, ≈ 3 GB paketleme trafiği). Seri boyunca (V54 çizgisine göre) kaldırılanlar: K-graf V³ (1,2e13), 6 Q + Cfull + Pn/Qt (≥ 7 GB), grup Cholesky/üçgen çözümler (1,9e10), yoğun U (2,2e11), W_C (2,2e11 + 1,7 GB), ACCB/seen/posy (~1 GB), n×V val/test matrisleri (1,5 GB ×2) = yaklaşık 1,2e13 + 4,5e11 madd, 12 ara matris/tablo (K-graf S, 6×Q, Cn dense, Y0d, W_C, Pn, Qt, ACCB, seen, posy, Zv, Ze).

## 8) Cevaplar
1. **Tamamen kaldırılan hesap (V76 toplam)**: V76'da yeni: 3 (seri sıfırlama ×2, tek-örnek pm/qd/skaler 8× tekrar yürütmeleri komut düzeyinde); V54'ten beri 12 ara matris/hesap (bkz. §7).
2. **Hiç oluşmayan ara matrisler**: K-graf S (V×V), 6 yoğun Q, Cfull/Cn yoğun, Y0d, W_C, Pn, Qt, ACCB, seen, posy, val/test logit matrisleri (n×V); Tt/Uu seri sıfırlanmış kopya yok.
3. **Kurtulunan işlem**: V75→V76: aritmetik madd 0; ≈7,4e9 skaler komut → ≈1,1e9 vektör; V54→V76 ≈ 1,2e13 madd.
4. **Model hazırlama**: 6,4 s (V75 6,6 s; aynı oturumda ölçülen).
5. **Validation + test dahil toplam**: 12,85 s (medyan; 12,40–13,23). V75 aynı oturumda 17,10 s.
6. **RAM**: 1,56 GB (V75 aynı).
7. **Test PPL tamamen korundu**: 262,930; val CE 5,441941; Top-1 %13,70; Top-5 %31,12; karmalar V73 ile bit-eşit.
8. **50k–100k**: yoğun V×V/V³ yok; kalan zorunlu: eval logit gemm ∝ n·V·513, W_F çözümü 513²·V, mf×V tabloları; gerçek veri yok (20k gerçek veriyle doğrulandı).
9. **Kaldırılamayan işlemlerin gerekçesi**: 3 logit gemm'i (tam softmax V sınıfın hepsini gerektirir; val ikinci gemm'i yalnızca 1,47 GB önbellekle kaçınılabilir — ölçüldü, kaybettiriyor); Schur 513³/3 + W_F çözümü (513'lük SPD sistem tanımı gereği); FᵀF ve F üretimi (n·513²/2, n·192·512); kNN/s(K) (modelin K tabloları); V49/FᵀY/slot toplamı birleştirme bit-eşitliği bozar (yeniden-sıralama).
10. **V76 V75'ten hızlı mı?** Evet: bütün program 17,10 → 12,85 s (−25 %), aynı oturumda dönüşümlü 5/5 çiftte; model hazırlama değil, validation/test akışı iyileşti.

## 9) Kalan / yapılmayanlar
- Hedef "15 s altı": **sağlandı** (12,4–13,2 s, 5/5). Ek kazanç ancak bit-eşitliği bozan yeniden-sıralamalar (V49/FᵀY/slot/FᵀF birleştirme, F1 K-azaltma, kNN sadeleştirme) veya bellek–hız takasları (logit önbelleği) ile mümkün; bunlar ölçüldü/analiz edildi ve bağlanmadı.
- VM'de ilk-dokunuş (taze bellek) maliyeti çok dalgalı (SYS 2–15 s); bu yüzden RAM'i artıran seçenekler (ZCACHE, T-syrk, THP, havuz) bilinçli reddedildi.
- Test PPL dışındaki 50k/100k çıktıları ölçülmedi (veri yok).
