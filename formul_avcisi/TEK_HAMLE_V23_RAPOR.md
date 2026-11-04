# TEK HAMLE V23 — Öğrenme hedeflerini doğrudan ağırlık üretim formülüne katma

Dal `claude/formul-avcisi` · kod `cpp/tek_hamle_v23.cpp` · ham loglar/komutlar `v23_ham/`. H=1024, c=2, aynı metin/bölme. Tamamlanan tohumlar: [1, 2, 3]. Tüm modeller **yalnız eğitimden**; doğrulama (α,T) seçimi ve kontrol için, test bağımsız. Sonradan CE gradyanı/η düzeltmesi **yok**; tek ağırlık hesabı = tek ridge çözümü.

## 1. Türetilen formül

Aday çerçeve `W=(FᵀΩF+Λ)⁻¹FᵀΩZ*` bu problem için şu forma indirildi:

`W* = (FᵀF + Λ(Q*))⁻¹ Fᵀ Q*`,  `Q* = Q*(κ)`,  `κ = max(κ_CE, κ_1, κ_5)`,  `Λ = λ*·I`, `λ* = argmin_λ Cp(λ; Q*)`

- **Öğretim matrisi Q\* (veriden, ilk çözümden önce):** her eğitim satırı için **kendisi hariç (leave-one-out)** eğitim sayımlarından interpolasyonlu trigram/bigram/unigram dağılımı `P̂_i` (geri çekilme gücü b eğitim LOO CE'siyle seçilir). Böylece öğretim sinyali bağlamsal bilgiyi (hangi kelime hangi 2-kelimelik bağlamdan sonra geldi) taşır ve satırın kendi etiketini içermez (sızıntı yok). `Q*_i=(1−κ)P̂_i+κ·e_{y_i}` (κ>0) veya `(1+κ)P̂_i−κ·π` (κ<0, düzleştirme).
- **CE hedefi → κ_CE:** Q\*'ın eğitim CE'si hedefe eşitlenir: `(1/n)Σ −ln[(1−κ)P̂_i(y_i)+κ] = CE*`. κ'da monoton tek skaler denklem (ikiye bölme, ağırlıklar hesaplanmadan çözülür). CE\* ve PPL aynı kısıt: tek denklem.
- **Top-k hedefleri → κ_1, κ_5 (kapalı form):** y_i, Q\*_i'de ilk-1 ⇔ `κ/(1−κ) > g1_i = max_{v≠y}(P̂_v−P̂_y)`; ilk-5 ⇔ `κ/(1−κ) > g5_i` (5. en büyük fark). Oranı ≥ hedef yapan en küçük κ = `r/(1+r)`, `r` = g'nin hedef-kuantili. (Marj koşulu `Q_y−Q_v>0`; ridge'in karesel kaybı ile kısmen taşınabilen karesel/doğrusal vekil.)
- **Genelleme → Λ:** Mallows Cp: `Cp(λ)=SSE(λ)+2σ̂²·df(λ)·V`, `SSE(λ)=‖Q\*‖²−⟨W,R⟩−λ‖W‖²` (ridge normal denkleminden, ek gemm yok), `df(λ)=m−λ·tr((FᵀF+λI)⁻¹)`, `σ̂²` en esnek modelden. Cp eğitimden hesaplanır; `Q*` değiştiği için `λ*` da hedefe bağlıdır.
- **Ω = I:** örnek ağırlıklandırma V12'de başarısızdı, tekrarlanmadı; hedefler Ω'ya sokulmadı. (Bu bir eksiklik olarak not edilir.)

**Tek denklem kurulabildi mi?** Evet: `(FᵀF+λ*I)W=FᵀQ*` doğrusal. Ancak bunun yolu CE ve Top-k kısıtlarını **öğrencinin logitlerine** değil **öğreticinin dağılımına** koymaktır. Öğrencinin gerçek CE'si/Top-k'si üzerindeki kısıtlar W'da doğrusal olmayan ve (Top-k için) kesiklidir: `CE(W)=−ln softmax` dışbükey ama hedef eşitliği `CE(W)≤c` dışbükey kısıt olup W için kapalı formlu çözümü yok (Lagrange çarpanı → softmax'lı doğrusal olmayan denklem); Top-k sıralama koşulları `z_y−z_v≥δ` n·V doğrusal eşitsizlik (SVM/QP) — tek Cholesky'ye indirgenemez. Bu yüzden: kontrollü yaklaşım = hedefi öğreticiye koy, ridge ile en küçük kareler izdüşümünü al; sonucu **ölç**.

## 2. Kritik kontrol: yalnız hedef CE değişince ağırlıklar değişiyor mu? (tohum ortalaması)

| yapılandırma | κ | λ | öğretici eğitim CE (PPL) | öğretici Top-1 / Top-5 (eğitim) | ‖ΔW‖/‖W‖ (A_PPL200'e göre) | öğrenci eğitim PPL | doğr. PPL | **test PPL** | test Top-1 | test Top-5 |
|---|---|---|---|---|---|---|---|---|---|---|
| V23 hedef CE=ln200, λ=Cp | -0.65540 | 3 | 5.298 (200) | nan% / nan% | 0.0000 | 315.2 | 268.8 | **249.28** | 11.16% | 24.87% |
| V23 hedef CE=ln150, λ=Cp | 0.00004 | 10 | 5.011 (150) | 13.0% / 33.3% | 0.6465 | 284.2 | 252.5 | **232.84** | 13.40% | 27.26% |
| V23 hedef CE=ln100, λ=Cp | 0.00072 | 10 | 4.605 (100) | 13.0% / 33.9% | 0.6468 | 284.1 | 252.5 | **232.75** | 13.42% | 27.27% |
| V23 CE=ln100 + Top-1≥%20 + Top-5≥%40, λ=Cp | 0.01725 | 10 | 3.365 (29) | 20.0% / 56.4% | 0.6545 | 279.5 | 250.6 | **230.70** | 13.87% | 27.42% |
| V23 κ=0 (saf sayım öğretici), λ=Cp | 0.00000 | 10 | 5.141 (171) | 13.0% / 33.2% | 0.6466 | 284.3 | 252.5 | **232.85** | 13.40% | 27.26% |

Okuma (tohum 1 ağırlık farkları, bölüm 6): **hedef CE gerçekten ağırlıkları değiştiriyor — ama yalnız hedef öğreticinin kendi CE'sinin (5,14) üstünde ya da altında kalmasına göre.** PPL 200 hedefi (CE 5,298) öğretici CE'sinin üstünde olduğu için κ=−0,655 (unigram'a düzleştirme) üretti → ağırlıklar PPL150/100'den %64 farklı ve test PPL 249'a bozuldu (hedefi *gevşetmek* sinyali bozuyor: bu "iyileştirme" değil). PPL 150→100 arasında ise κ yalnız 4e-5→7e-4; ‖ΔW‖/‖W‖≈0,003 (ihmal edilebilir). Top-1/Top-5 hedefleri κ≈0,017 ile ağırlıkları ≈%0,3 (aynı λ'da) değiştiriyor ve test Top-1'i 13,4→13,9% yapıyor. [Kesin: loglar]

**Neden CE hedefi neredeyse hiç κ üretmiyor?** CE ortalaması ağır kuyrukludur: κ-karışımı her satırın kaybını `−ln κ` ile sınırlıyor (κ=7e-4 → ≤7,3 nat); öğreticinin ortalama CE'sini 5,14'ten 4,605'e indiren şey, birkaç uç satırın kaybını tavanlamaktır, öğretim sinyalinin bilgisini değiştirmek değil. Yani **CE hedefi matematiksel olarak (öğreticide) sağlanıyor ama hangi kelimenin öğretileceğini değiştirmiyor**; "100 PPL" burada gerçek öğrenme başarısı değil. [Muhtemel: mekanizma yorumu; sayılar Kesin]

**Öğrenci hedefe ulaşamadı:** öğretici eğitim CE'si 4,605 (hedef) iken öğrencinin gerçek eğitim CE'si 5,65 (PPL 284) — hedef–gerçek farkı ≈1,0 nat. Öğrenci (1025 rastgele özellikli ridge) öğreticinin dağılımını temsil edemiyor (özellik kapasitesi), ayrıca olasılık-uzayı en küçük kareleri ve softmax kalibrasyonu CE'yi doğrudan küçültmüyor.

## 3. Tüm yapılandırmalar (tohum ortalaması; eğitim / doğrulama / test ayrı)

| yapılandırma | λ | (α,T) | eğitim CE (PPL) top1 / top5 | doğr. CE (PPL) top1 / top5 | **test CE (PPL)** top1 / top5 | genelleme farkı (doğr.−eğitim CE) |
|---|---|---|---|---|---|---|
| Ridge one-hot, λ=V17 seçimi (referans, bu koşuda eğitim-yalnız) | 10 | α=0.2,T=0.01 | 5.1367 (170.2) 22.0% / 32.6% | 5.4168 (225.2) 15.0% / 28.0% | **5.3420 (208.9) 15.7% / 29.9%** | 0.280 |
| Ridge one-hot, λ=Cp ile | 1000 | α=0.2,T=0.01 | 5.3363 (207.7) 18.7% / 29.1% | 5.4364 (229.6) 14.5% / 27.4% | **5.3583 (212.4) 15.6% / 29.5%** | 0.100 |
| V23 hedef CE=ln200, λ=Cp | 3 | α=0.15,T=0.005 | 5.7532 (315.2) 7.9% / 20.5% | 5.5941 (268.8) 10.2% / 23.4% | **5.5186 (249.3) 11.2% / 24.9%** | -0.159 |
| V23 hedef CE=ln150, λ=Cp | 10 | α=0.15,T=0.005 | 5.6499 (284.2) 12.3% / 23.1% | 5.5316 (252.5) 12.4% / 25.3% | **5.4503 (232.8) 13.4% / 27.3%** | -0.118 |
| V23 hedef CE=ln100, λ=Cp | 10 | α=0.15,T=0.005 | 5.6492 (284.1) 12.4% / 23.1% | 5.5312 (252.5) 12.4% / 25.4% | **5.4500 (232.8) 13.4% / 27.3%** | -0.118 |
| V23 CE=ln100 + Top-1≥%20 + Top-5≥%40, λ=Cp | 10 | α=0.15,T=0.005 | 5.6330 (279.5) 13.1% / 23.3% | 5.5241 (250.6) 12.8% / 25.4% | **5.4411 (230.7) 13.9% / 27.4%** | -0.109 |
| V23 κ=0 (saf sayım öğretici), λ=Cp | 10 | α=0.15,T=0.005 | 5.6499 (284.3) 12.3% / 23.1% | 5.5316 (252.5) 12.4% / 25.3% | **5.4504 (232.8) 13.4% / 27.3%** | -0.118 |
| V23 hedef CE=ln100, λ=V17 (aynı λ) | 10 | α=0.15,T=0.005 | 5.6513 (284.7) 12.3% / 23.0% | 5.5327 (252.8) 12.4% / 25.3% | **5.4513 (233.1) 13.4% / 27.2%** | -0.119 |
| V23 CE+Top-1+Top-5, λ=V17 (aynı λ) | 10 | α=0.15,T=0.005 | 5.6354 (280.2) 13.0% / 23.3% | 5.5255 (251.0) 12.7% / 25.4% | **5.4425 (231.0) 13.8% / 27.4%** | -0.110 |
| V23 κ=0, λ=V17 (aynı λ) | 10 | α=0.15,T=0.005 | 5.6520 (284.9) 12.3% / 23.0% | 5.5330 (252.9) 12.3% / 25.3% | **5.4517 (233.2) 13.4% / 27.2%** | -0.119 |

## 4. Öğreticinin kendi kalitesi (yalnız eğitim sayımları, LOO)

- `OGRETICI b=1 egitim-LOO CE=6.5795 (PPL 720.2)`
- `OGRETICI b=2 egitim-LOO CE=6.1715 (PPL 478.9)`
- `OGRETICI b=4 egitim-LOO CE=5.8267 (PPL 339.2)`
- `OGRETICI b=8 egitim-LOO CE=5.5523 (PPL 257.8)`
- `OGRETICI b=16 egitim-LOO CE=5.3504 (PPL 210.7)`
- `OGRETICI b=32 egitim-LOO CE=5.2187 (PPL 184.7)`
- `OGRETICI b=64 egitim-LOO CE=5.1514 (PPL 172.7)`
- `OGRETICI b=128 egitim-LOO CE=5.1408 (PPL 170.9)`
- `OGRETICI SECILEN b=128 ; kappa=0 ogretici (saf sayim, LOO): egitim CE=5.1408 (PPL 170.9) Top-1=0.1295 Top-5=0.3325 ; unigram CE=5.8496 (PPL 347.1) ; ogretici hazirlama sure=0.50s`

Öğretici (LOO sayım) kendi başına zayıf: eğitim-LOO CE ≈ 5,5 (PPL ≈ 250) — ölçülmüş trigram (PPL 137) sayım+düzgünleştirmesinden çok daha kötü; yani burada kullanılan basit interpolasyon, güçlü bir öğretmen değil. [Muhtemel: b ızgarasının kenarı genişletildi, bkz. log]

## 5. κ ve hedefler

- `HEDEF_KAPPA|PPL=200|CE*=5.29832|kappa_CE=-0.65540`
- `HEDEF_KAPPA|PPL=150|CE*=5.01064|kappa_CE=0.00004`
- `HEDEF_KAPPA|PPL=100|CE*=4.60517|kappa_CE=0.00072`
- `HEDEF_KAPPA|CE*=ln100: kappa_CE=0.00072 kappa_Top1(>=0.2)=0.01725 kappa_Top5(>=0.4)=0.00791 -> kappa=max=0.01725`

## 6. Ağırlık farkları (tohum 1, ‖Wa−Wb‖/‖Wb‖)

| a | b | fark |
|---|---|---|
| ridge_hot_lamV17 | ridge_hot_Cp | 2.1038 |
| ridge_hot_lamV17 | A_PPL200 | 11.0207 |
| ridge_hot_lamV17 | A_PPL150 | 4.2556 |
| ridge_hot_lamV17 | A_PPL100 | 4.2498 |
| ridge_hot_lamV17 | B_PPL100_Top1_Top5 | 4.0971 |
| ridge_hot_lamV17 | K0_saf_sayim_ogretici | 4.2562 |
| ridge_hot_lamV17 | A_PPL100_lamV17 | 4.2498 |
| ridge_hot_lamV17 | B_PPL100_Top1_Top5_lamV17 | 4.0971 |
| ridge_hot_lamV17 | K0_lamV17 | 4.2562 |
| ridge_hot_Cp | A_PPL200 | 3.6253 |
| ridge_hot_Cp | A_PPL150 | 1.4030 |
| ridge_hot_Cp | A_PPL100 | 1.3999 |
| ridge_hot_Cp | B_PPL100_Top1_Top5 | 1.3221 |
| ridge_hot_Cp | K0_saf_sayim_ogretici | 1.4032 |
| ridge_hot_Cp | A_PPL100_lamV17 | 1.3999 |
| ridge_hot_Cp | B_PPL100_Top1_Top5_lamV17 | 1.3221 |
| ridge_hot_Cp | K0_lamV17 | 1.4032 |
| A_PPL200 | A_PPL150 | 0.6441 |
| A_PPL200 | A_PPL100 | 0.6444 |
| A_PPL200 | B_PPL100_Top1_Top5 | 0.6519 |
| A_PPL200 | K0_saf_sayim_ogretici | 0.6442 |
| A_PPL200 | A_PPL100_lamV17 | 0.6444 |
| A_PPL200 | B_PPL100_Top1_Top5_lamV17 | 0.6519 |
| A_PPL200 | K0_lamV17 | 0.6442 |
| A_PPL150 | A_PPL100 | 0.0029 |
| A_PPL150 | B_PPL100_Top1_Top5 | 0.0717 |
| A_PPL150 | K0_saf_sayim_ogretici | 0.0006 |
| A_PPL150 | A_PPL100_lamV17 | 0.0029 |
| A_PPL150 | B_PPL100_Top1_Top5_lamV17 | 0.0717 |
| A_PPL150 | K0_lamV17 | 0.0006 |
| A_PPL100 | B_PPL100_Top1_Top5 | 0.0689 |
| A_PPL100 | K0_saf_sayim_ogretici | 0.0031 |
| A_PPL100 | A_PPL100_lamV17 | 0.0000 |
| A_PPL100 | B_PPL100_Top1_Top5_lamV17 | 0.0689 |
| A_PPL100 | K0_lamV17 | 0.0031 |
| B_PPL100_Top1_Top5 | K0_saf_sayim_ogretici | 0.0734 |
| B_PPL100_Top1_Top5 | A_PPL100_lamV17 | 0.0703 |
| B_PPL100_Top1_Top5 | B_PPL100_Top1_Top5_lamV17 | 0.0000 |
| B_PPL100_Top1_Top5 | K0_lamV17 | 0.0734 |
| K0_saf_sayim_ogretici | A_PPL100_lamV17 | 0.0031 |
| K0_saf_sayim_ogretici | B_PPL100_Top1_Top5_lamV17 | 0.0719 |
| K0_saf_sayim_ogretici | K0_lamV17 | 0.0000 |
| A_PPL100_lamV17 | B_PPL100_Top1_Top5_lamV17 | 0.0689 |
| A_PPL100_lamV17 | K0_lamV17 | 0.0031 |
| B_PPL100_Top1_Top5_lamV17 | K0_lamV17 | 0.0734 |

## 7. Karşılaştırma (H=1024, test PPL)

| model | test PPL | not |
|---|---|---|
| V17 Ridge (eğitim+doğrulama modeli) | 206,8 | rapor |
| V20 CE düzeltmesi | 198,9 | rapor |
| trigram referansı | 137 | ölçülmüş |
| bu koşuda one-hot Ridge (eğitim-yalnız, λ=V17) | 208.9 | doğru karşılaştırma tabanı |
| **V23** (CE=ln100 hedefli, λ=V17) | 233.1 | Top-1 13.4% |
| **V23** (CE+Top-1+Top-5 hedefli, λ=V17) | 231.0 | Top-1 13.8%, Top-5 27.4% |

## 8. Sınırlar ve dürüst değerlendirme

- **Hedef PPL≤100, CE≤4,605, Top-1≥%20, Top-5≥%40 hiçbiri gerçek test verisinde sağlanmadı.** En iyi V23 test PPL'i ≈211 (b=8, tek tohum) / ≈231 (ana protokol, 3 tohum), Ridge (one-hot) 208,5–208,9; V20 198,9 (hata düzeltmeli) bu çalışmada geçilmedi. Test Top-1 13,4–15,6%, Top-5 27–30%.
- Hedefler formülün içine girdi (κ, Λ üzerinden, ilk çözümden önce) ve ağırlıklar hedefe bağlı olarak değişti (Top-1/Top-5 hedefi %5, düzleştirme κ<0 %64), fakat **CE hedefi pratikte ağırlıkları değiştirmiyor** (κ tavanlama etkisi) ve Top-k hedefi yalnız öğreticide sağlanıyor (eğitim Top-1 %20,0 / Top-5 %56 öğreticide), öğrencide değil (13–17%).
- **Matematiksel engel (cevap, soru 5):** (i) CE≤c ve Top-k koşulları öğrencinin logitlerinde doğrusal değil (softmax) ve Top-k kesikli/n·V eşitsizlik; tek bir doğrusal sistemle çözülemez. (ii) Öğreticiye koymak çözülebilir ama öğrencinin kapasitesiyle sınırlı: ridge yalnız en küçük kareler izdüşümü verir; hedef CE ile gerçek CE arasında ≈1 nat fark ölçüldü. (iii) Genelleme için Cp, eğitim kaybı ve serbestlik derecesine dayanır; softmax-CE'ye değil kareli kayba göre kalibre olduğu için one-hot Ridge'de λ'yı yanlış (1000, kenar) seçti.
- Öğretici zayıf: basit LOO interpolasyon (b ızgarası kenarda) ölçülmüş trigramdan (137) çok uzak; LOO eğitim PPL 171–258. Daha güçlü öğretici (Kneser–Ney, ayrı b'ler, 4-gram) denenmedi. [Tahmin: öğretici kalitesi tavan belirliyor]
- Ω=I; hedefe bağlı örnek ağırlığı denenmedi. Top-k için marj/karesel vekil yalnız öğretici düzeyinde kullanıldı, öğrenci kaybına konmadı.
- 3 tohum ana protokolde; duyarlılık yalnız tohum 1. İlk deneme (b ızgarası ≤8, ilk çalıştırma) logu dosyaya yazılmadan silindi; yerine b=8 sabit duyarlılık koşusu kaydedildi (aynı sayılar: A_PPL100 211,30 / B 210,72 / K0 211,31 tohum 1).
- Dürüst sonuç: ana soru için **hayır — bu formülle baştan daha iyi ağırlık üretilemedi**; hedef koşullu öğretim matrisi eğitim–doğrulama farkını küçülttü ama test PPL'ini iyileştirmedi.


## 8a. Duyarlılık koşusu (tohum 1): öğretici yumuşatma gücü b=8 sabit
Ana protokol b'yi eğitim-LOO CE ile seçti (b=128, ızgara kenarı; LOO CE 5,141). Bu çok yumuşak öğretici (unigram'a yakın) öğrenciyi bozdu (test PPL ≈232–249). Aynı kodda b=8 sabitlendiğinde (daha keskin öğretici, LOO CE 5,552; tek tohum, `v23_ham/v23sens/`):

| yapılandırma (b=8, tohum 1) | λ | test PPL | Top-1 | Top-5 | eğitim PPL | doğr. PPL | genelleme farkı |
|---|---|---|---|---|---|---|---|
| one-hot Ridge, λ=V17 | 10 | 208.53 | 15.72% | 29.96% | 166.9 | 225.4 | 0.300 |
| V23 CE=ln100, λ=V17 | 10 | 211.07 | 15.54% | 30.06% | 223.0 | 228.1 | 0.022 |
| V23 CE+Top-1+Top-5, λ=V17 | 10 | 210.48 | 15.60% | 30.02% | 219.6 | 227.5 | 0.036 |
| V23 κ=0 (saf sayım), λ=V17 | 10 | 211.08 | 15.54% | 30.06% | 223.1 | 228.1 | 0.022 |
| V23 CE=ln100, λ=Cp (=30) | 30 | 211.30 | 15.56% | 30.00% | 224.1 | 228.3 | 0.019 |
| one-hot Ridge, λ=Cp (=1000) | 1000 | 213.60 | 15.50% | 29.40% | 210.0 | 230.1 | 0.091 |

Bu koşuda ağırlık farkları: A_PPL200↔A_PPL150 0,0002, A_PPL200↔A_PPL100 0,0008 (CE hedefi 200/150/100 ağırlıkları fiilen değiştirmiyor; hepsi κ<6e-4); Top-1/Top-5 hedefi ≈%5 değiştiriyor. [Kesin: log]
Sonuç: sayım-öğretici ile yapılan ridge, aynı λ'da one-hot Ridge'e göre **test PPL'i iyileştirmedi** (211,1 vs 208,5); yalnız eğitim–doğrulama farkını belirgin küçülttü (0,30→0,02). λ'yı Cp ile seçmek one-hot Ridge'de zarar verdi (213,6; λ=1000 ızgara kenarı).

## 9. Çalıştırma

Derleme: `g++ -O3 -march=native -fopenmp -std=c++17 -o tek_hamle_v23 cpp/tek_hamle_v23.cpp`; komut: `./tek_hamle_v23 2 1024 TOHUM GAMMA LAMBDA [top1=0.20] [top5=0.40]` ((γ,λ)=(4,10),(4,100),(2,1) V17 seçimi).

| komut | wall s | user CPU s | RAM MB |
|---|---|---|---|
| `./tek_hamle_v23 2 1024 1 4 10` | 104.43 | 308.44 | 734 |
| `./tek_hamle_v23 2 1024 2 4 100` | 106.38 | 326.30 | 734 |
| `./tek_hamle_v23 2 1024 3 2 1` | 111.72 | 305.15 | 734 |
