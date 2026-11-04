# TEK HAMLE V25 — Öğrenme bilgisi nerede kayboluyor? (ölçüm/teşhis)

Dal `claude/formul-avcisi` · kod `cpp/tek_hamle_v25.cpp` (V24 kopyası, ölçüm aracı) · ham loglar/komutlar `v25_ham/`. Aynı 50k token, bölme, sözlük; öğretici (EM ağırlıklı trigram, eğitim sayımları, **eğitim satırları leave-one-out**) ve tüm ağırlıklar yalnız eğitimden; doğrulama yalnız λ ve (α,T) seçimi; test yalnız son ölçüm. **H=1024, tohum 1** (H=512 ve 3 tohum süre nedeniyle yapılmadı). Tüm modeller aynı kural: λ ∈ küçük ızgara, (α,T) ve λ doğrulama kalibre CE ile. Her sistemde iki hedef ayrı çözülür: `W_Y` (etiket) ve `W_Q` (trigram dağılımı), her biri tek `(ZᵀZ+λI)⁻¹ZᵀHedef` çözümü.

**Önemli not (hata düzeltmesi):** ilk koşu, Gram matrisinin yalnız bir üçgenini kopyalamam yüzünden yanlıştı (ağırlıklar kayıp gibi görünüyordu); simetrik doldurma ile yeniden çalıştırıldı. Aşağıdaki sayılar düzeltilmiş koşudan.

## 1. Özet tablo (λ doğrulama CE ile seçilmiş; test)

Ölçülen iki ayrı başarı: **(i) Q'ya yaklaşma** = açıklanan varyans `1 − ‖ZW−Q‖²/‖Q−π‖²` (π eğitim unigram), KL(Q‖öğrenci); **(ii) gerçek etiketlerde CE/PPL**. İkisi aynı şey değildir.

| sistem (Z) | boyut | hedef | λ | Q açıklanan varyans eğitim / doğr. / test | KL(Q‖öğr.) test | eğitim PPL | doğr. PPL | **test PPL** | test Top-1 / Top-5 | eğitim–doğr. CE farkı |
|---|---|---|---|---|---|---|---|---|---|---|
| A_X_dogrudan(K=192) | 193 (193 yoğun + 0 seyrek) | W_Y | 10 | – (W_Y, Q'yu hedeflemez) | 0.739 | 248.9 | 246.7 | **228.77** | 13.6% / 28.1% | -0.009 |
| A_X_dogrudan(K=192) | 193 (193 yoğun + 0 seyrek) | W_Q | 1 | 0.249 / 0.299 / 0.304 | 0.749 | 285.8 | 255.6 | **237.92** | 13.3% / 26.4% | -0.111 |
| B_F(H=1024) | 1025 (1025 yoğun + 0 seyrek) | W_Y | 10 | – (W_Y, Q'yu hedeflemez) | 0.728 | 166.9 | 225.4 | **208.53** | 15.7% / 30.0% | 0.300 |
| B_F(H=1024) | 1025 (1025 yoğun + 0 seyrek) | W_Q | 10 | 0.375 / 0.413 / 0.420 | 0.694 | 246.6 | 239.1 | **223.87** | 14.4% / 27.8% | -0.031 |
| C1_F+w_kimligi(P=2001) | 3026 (1025 yoğun + 2001 seyrek) | W_Y | 10 | – (W_Y, Q'yu hedeflemez) | 0.622 | 107.0 | 214.8 | **200.47** | 16.9% / 30.5% | 0.697 |
| C1_F+w_kimligi(P=2001) | 3026 (1025 yoğun + 2001 seyrek) | W_Q | 10 | 0.645 / 0.750 / 0.751 | 0.604 | 201.9 | 226.6 | **212.23** | 15.4% / 29.0% | 0.116 |
| C2_F+w_kimligi+sik_cift(P=4001) | 5026 (1025 yoğun + 4001 seyrek) | W_Y | 10 | – (W_Y, Q'yu hedeflemez) | 0.608 | 90.8 | 212.5 | **198.05** | 17.0% / 30.2% | 0.850 |
| C2_F+w_kimligi+sik_cift(P=4001) | 5026 (1025 yoğun + 4001 seyrek) | W_Q | 10 | 0.656 / 0.758 / 0.758 | 0.603 | 198.4 | 225.7 | **210.81** | 15.6% / 29.1% | 0.129 |
| C0_yalniz_kimlik_(F_yok)(P=4001) | 4002 (1 yoğun + 4001 seyrek) | W_Y | 10 | – (W_Y, Q'yu hedeflemez) | 0.589 | 134.2 | 218.7 | **203.86** | 16.2% / 29.6% | 0.488 |
| C0_yalniz_kimlik_(F_yok)(P=4001) | 4002 (1 yoğun + 4001 seyrek) | W_Q | 10 | 0.596 / 0.718 / 0.715 | 0.592 | 194.7 | 226.7 | **212.22** | 15.6% / 28.7% | 0.152 |
| *trigram n-gram (referans, aynı protokol)* | – | – | – | 1 / 1 / 1 (Q'nun kendisi) | 0 | – | 145.6 | **138.87** | 16.0% / 35.8% | – |

Yoğun sütun sayıları farklı (A: 193, B: 1025, C: 1025 + seyrek); kapasite farkı açıktır, sonuçlar yalnız "temsil kalitesi" olarak okunmamalıdır. Tüm λ satırları ve (α,T) seçimleri `v25_ham/v25log/` dosyasındadır.

## 2. Tüm λ değerleri (test PPL / Q açıklanan varyans test)

| sistem | hedef | λ | (α,T) | eğitim PPL | doğr. PPL | test PPL | Q açıklanan varyans (test) | trigram bağlamı GÖRÜLEN satırlarda test CE / Top-1 | yalnız bigram görülen CE / Top-1 |
|---|---|---|---|---|---|---|---|---|---|
| A_X_dogrudan(K=192) | W_Y | 1 | 0.2,0.01 | 248.7 | 246.7 | 228.92 | -0.208 | 5.4969 / 13.9% | 5.2977 / 13.0% |
| A_X_dogrudan(K=192) | W_Q | 1 | 0.15,0.005 | 285.8 | 255.6 | 237.92 | 0.304 | 5.5469 / 13.0% | 5.3118 / 13.9% |
| A_X_dogrudan(K=192) | W_Y | 10 | 0.2,0.01 | 248.9 | 246.7 | 228.77 | -0.178 | 5.4968 / 13.9% | 5.2959 / 12.9% |
| A_X_dogrudan(K=192) | W_Q | 10 | 0.15,0.005 | 286.1 | 255.7 | 238.00 | 0.303 | 5.5473 / 13.1% | 5.3120 / 14.0% |
| A_X_dogrudan(K=192) | W_Y | 100 | 0.2,0.01 | 252.7 | 246.8 | 228.67 | -0.000 | 5.5003 / 13.9% | 5.2870 / 13.8% |
| A_X_dogrudan(K=192) | W_Q | 100 | 0.15,0.005 | 289.6 | 256.8 | 238.97 | 0.296 | 5.5520 / 13.0% | 5.3147 / 13.8% |
| B_F(H=1024) | W_Y | 10 | 0.2,0.01 | 166.9 | 225.4 | 208.53 | -1.177 | 5.3829 / 16.2% | 5.2485 / 14.7% |
| B_F(H=1024) | W_Q | 10 | 0.25,0.01 | 246.6 | 239.1 | 223.87 | 0.420 | 5.4792 / 14.3% | 5.2656 / 14.5% |
| B_F(H=1024) | W_Y | 100 | 0.2,0.01 | 175.8 | 225.4 | 209.16 | -0.812 | 5.3890 / 15.8% | 5.2450 / 14.8% |
| B_F(H=1024) | W_Q | 100 | 0.25,0.01 | 250.2 | 240.9 | 225.47 | 0.423 | 5.4866 / 14.0% | 5.2720 / 14.6% |
| C1_F+w_kimligi(P=2001) | W_Y | 10 | 0.35,0.02 | 107.0 | 214.8 | 200.47 | -0.836 | 5.3566 / 16.8% | 5.1813 / 17.1% |
| C1_F+w_kimligi(P=2001) | W_Q | 10 | 0.25,0.01 | 201.9 | 226.6 | 212.23 | 0.751 | 5.4378 / 15.0% | 5.1867 / 16.4% |
| C1_F+w_kimligi(P=2001) | W_Y | 100 | 0.2,0.01 | 165.5 | 220.1 | 204.15 | -0.651 | 5.3723 / 16.1% | 5.2048 / 16.2% |
| C1_F+w_kimligi(P=2001) | W_Q | 100 | 0.25,0.01 | 236.0 | 233.8 | 219.12 | 0.528 | 5.4634 / 14.9% | 5.2319 / 14.8% |
| C2_F+w_kimligi+sik_cift(P=4001) | W_Y | 10 | 0.25,0.02 | 90.8 | 212.5 | 198.05 | -1.065 | 5.3424 / 16.9% | 5.1734 / 17.1% |
| C2_F+w_kimligi+sik_cift(P=4001) | W_Q | 10 | 0.25,0.01 | 198.4 | 225.7 | 210.81 | 0.758 | 5.4275 / 15.2% | 5.1875 / 16.4% |
| C0_yalniz_kimlik_(F_yok)(P=4001) | W_Y | 10 | 0.2,0.01 | 134.2 | 218.7 | 203.86 | 0.266 | 5.3796 / 16.2% | 5.1847 / 16.2% |
| C0_yalniz_kimlik_(F_yok)(P=4001) | W_Q | 10 | 0.25,0.01 | 194.7 | 226.7 | 212.22 | 0.715 | 5.4332 / 15.5% | 5.1962 / 15.9% |

## 3. Ek tanı: okuma katmanı (kalibre softmax vs doğrusal okuma) — AYNI W, farklı okuma

Ana protokolde (V9–V24) ridge çıktısı `z` kalibre softmax `P=(1−α)π+α·softmax(z/T)` ile olasılığa çevrilir. Q'ya MSE ile iyi yaklaşan `z` zaten olasılık ölçeğindedir; bu yüzden **aynı ağırlıkları** doğrusal okumayla da değerlendirdim: `P=(1−α)π+α·max(z,ε)/Σmax(z,ε)` (α∈{0,5…1}, ε∈{1e-5,1e-4,1e-3}, **doğrulama CE ile** seçildi; test yalnız son ölçüm). Bu yeni ağırlık üretimi değil, okuma tanısıdır; Top-1/Top-5 bu okumada ölçülmedi.

| H | sistem | hedef | λ | softmax-kalibre test PPL (ana protokol) | doğrusal okuma test PPL | (α,ε) | Q açıklanan varyans (test) |
|---|---|---|---|---|---|---|---|
| 1024 | B_F(H=1024) | W_Y | 10 | 208.5 | **194.6** | 0.5,1e-05 | – |
| 1024 | B_F(H=1024) | W_Q | 10 | 223.9 | **175.1** | 0.8,1e-05 | 0.420 |
| 1024 | B_F(H=1024) | W_Y | 100 | 209.2 | **189.5** | 0.5,1e-05 | – |
| 1024 | B_F(H=1024) | W_Q | 100 | 225.5 | **172.5** | 0.8,1e-05 | 0.423 |
| 1024 | C2_F+w_kimligi+sik_cift(P=4001) | W_Y | 10 | 198.1 | **175.0** | 0.5,1e-05 | – |
| 1024 | C2_F+w_kimligi+sik_cift(P=4001) | W_Q | 10 | 210.8 | **152.8** | 0.8,1e-05 | 0.758 |
| 1024 | C0_yalniz_kimlik_(F_yok)(P=4001) | W_Y | 10 | 203.9 | **136.8** | 0.8,0.0001 | – |
| 1024 | C0_yalniz_kimlik_(F_yok)(P=4001) | W_Q | 10 | 212.2 | **137.5** | 1,1e-05 | 0.715 |
| 512 | B_F(H=512) | W_Y | 1 | 215.7 | **182.2** | 0.5,1e-05 | – |
| 512 | B_F(H=512) | W_Q | 1 | 230.1 | **169.6** | 0.8,1e-05 | 0.404 |
| 512 | B_F(H=512) | W_Y | 100 | 225.0 | **172.9** | 0.8,1e-05 | – |
| 512 | B_F(H=512) | W_Q | 100 | 236.5 | **175.8** | 0.8,1e-05 | 0.314 |
| 512 | C1_F+w_kimligi(P=2001) | W_Y | 1 | 206.0 | **163.8** | 0.5,1e-05 | – |
| 512 | C1_F+w_kimligi(P=2001) | W_Q | 1 | 213.0 | **146.5** | 0.9,1e-05 | 0.957 |
| 512 | C1_F+w_kimligi(P=2001) | W_Y | 100 | 212.5 | **150.1** | 0.8,0.0001 | – |
| 512 | C1_F+w_kimligi(P=2001) | W_Q | 100 | 224.1 | **152.2** | 0.9,1e-05 | 0.507 |
| 512 | C2_F+w_kimligi+sik_cift(P=4001) | W_Y | 1 | 203.0 | **166.3** | 0.5,1e-05 | – |
| 512 | C2_F+w_kimligi+sik_cift(P=4001) | W_Q | 1 | 210.1 | **144.8** | 0.9,1e-05 | 0.958 |

## 4. Karar soruları

**A — X→F bilgi aktarımını artırıyor mu?** [Kesin, ölçülen] Evet bu kapasitede: Q açıklanan varyans (test) X: 0,30 → F: 0,42; gerçek etiket PPL (kalibre softmax) 228,8 → 208,5. [Muhtemel] Kapasite karışık (193 vs 1025 yoğun sütun), artışın tamamı "tanh dönüşümü" değildir; eşit boyutlu doğrusal özellik kontrolü yapılmadı. F yine de kelime-çifti kimliğini çözmüyor.

**B — Bağlam kimliği açıklanan öğretici bilgisini %43'ün üzerine çıkarıyor mu?** [Kesin] Evet: F+w kimliği (H=1024, λ=10) %75, H=512 λ=1'de %96 (test, W_Q); sık çiftleri eklemek yalnız +%0,7. Sebep: V24'ün EM ağırlıklarına göre Q'nun %95'i bigram+unigram (trigram bileşeni %4,6), yani `w` kimliği Q'nun neredeyse tamamını doğrusal üretir. Yalnız kimlik (F yok, C0) da %72 açıklıyor → **bilgi kaybının yeri: F'nin kelime kimliği/çift bilgisini taşıyamaması** [Kesin: kimlik eklenince %42 → %75–96].

**C — Açıklanan bilgi artınca test PPL de düşüyor mu?** Evet ama kalibre-softmax okumada az: W_Y 208,5 → 200,5 → 198,1 (F → F+w → F+w+çift; H=1024); W_Q 223,9 → 212,2 → 210,8; Q'yu %96 açıklayan H=512 C1 bile 213. **Q'ya MSE yaklaşması ≠ PPL başarısı** [Kesin]. Doğrusal okumada aynı ağırlıklarla PPL çok daha düşük (bölüm 3): B W_Q 223,9 → 175,1; C2 W_Q 152,8; H=512 C1/C2 W_Q 146,5 / 144,8; yalnız-kimlik C0 136,8–137,5.

**D — Kimlikli kapalı form çözüm trigramın 138,9'una yaklaşabilir mi?** [Kesin] Evet: yalnız-kimlik ridge + doğrusal okuma test PPL 136,8 (W_Y) / 137,5 (W_Q), trigram 138,9, bigram 144,9. Fakat bu **klasik n-gram tablosunun ridge ile yeniden ifadesidir** (kimlik sütunları = sayım tablosu); yeni ağırlık-üretim başarısı olarak sunulmuyor. F eklenince doğrusal okumada kötüleşiyor (H=1024 C2 W_Q 152,8 vs C0 137,5) [Muhtemel: rastgele özelliklerin gürültüsü, aynı λ]. Ana mimariye eklenmedi.

**E — Eksik bilgi sayısal gösterilebilir mi?** [Kesin] Merdiven (W_Q, test açıklanan varyans): X 0,30 → F 0,42 → F+w kimliği 0,75 (λ=10) / 0,96 (H=512, λ=1) → +sık çift 0,76; yalnız kimlik 0,72. Eksik bilgi: kelime kimliği/çift bilgisi (F'de yok, X'te doğrusal olarak yok). İkinci, **bağımsız** kayıp okuma katmanında: aynı W için kalibre softmax → doğrusal okuma PPL farkı C0 W_Q'da 75 PPL (212,2 → 137,5), B W_Q'da 49 PPL (223,9 → 175,1).

**V24 düzeltmesi:** [Kesin] V24'ün "bilgi aktarımı ≈%43, kayıp F temsilinde" sonucu açıklanan varyans için doğru; ama "öğretici başarısı öğrenciye geçmiyor (PPL 222)" kısmı büyük ölçüde **softmax-kalibrasyon okumasının** sonucuydu: aynı F ve W_Q doğrusal okumada 175 PPL (H=512: 170) veriyor. V9–V24'teki tüm "Ridge" PPL değerleri bu okumaya bağlıdır; bu bulgu V17/V20 karşılaştırmalarını (206,8 / 198,9) etkiler ve doğrulanması gerekir (tek tohum). [Muhtemel] Darboğaz iki katlı: (1) F kimlik bilgisini taşımıyor, (2) softmax(z/T) kalibrasyonu olasılık-ölçekli regresyon çıktısı için kayıplı okuma.

## 5. Sınırlamalar

- Tek tohum (1); H=1024 ana; H=512 yalnız B/C1/C2 (A ve C0 yok). 3 tohum yapılmadı.
- İlk koşuda Gram matrisini yalnız bir üçgenle kopyalama hatası vardı (B ≈ 274 PPL); düzeltildi, B V24 sonuçlarını birebir yeniden üretti (208,53 / 223,87). Hatalı koşunun logu saklanmadı.
- Doğrusal okuma yalnız CE/PPL ile ölçüldü (Top-k yok); (α,ε) 18 noktalık ızgara, doğrulamayla. Seyrek kimlik λ'sı F ile aynı; ayrı λ denenmedi. Sık çift sayısı 2000 (19 646 çiftten).
- Q eğitim satırlarında leave-one-out; kimlik özellikleri yalnız geçmiş bağlam kelimelerinden (hedef sızıntısı yok). Kimlikli W_Y sistemlerinde yüksek eğitim–doğrulama farkı (0,70–0,85 nat) aşırı uyumu gösterir.
- C0/C1/C2 n-gram sayımının doğrusal yeniden ifadesidir; V11/V12 kimlik denemeleriyle fark: burada kimlikler Q'ya karşı ve doğrusal okumayla ölçüldü. V11/V12 sayıları yeniden koşturulmadı.
- Kapasite farkı (A: 193, B: 1025, C: 1025 + 2001/4001 sütun) sonuçları yalnız "temsil kalitesi"ne bağlamayı engeller.

## 6. Öneri (uygulanmadı)

1. V9–V24'ün kalibre-softmax okumasını doğrusal olasılık okumasıyla yeniden ölçmek (3 tohum, V17/V20 protokolü): Ridge'in gerçek PPL'ini belirlemek. Bu, hedef-koşullu ağırlık üretimini değerlendirme ölçüsü bakımından değiştirir.
2. Kimlik bloğu ve yoğun F için ayrı λ; Q hedefinin olasılık uzayında kalması; F'yi yalnız kimliğin açıklayamadığı artık üzerinde kullanmak. Hedefi V24'teki ulaşılabilirlik sertifikasıyla birlikte yazmak.
3. ≈137 PPL değerleri n-gram tablosunun yeniden ifadesidir; "öğrenme kazancı" olarak raporlanmamalı. Gerçek ölçüt: F+kimliğin kimlik-yalnız modele göre ek kazancı (burada doğrusal okumada negatif).

## 7. Süre ve bellek


| komut | wall s | user CPU s | RAM MB |
|---|---|---|---|
| `./tek_hamle_v25 2 1024 1 4 10 ABCDE` | 171.15 | 485.00 | 1313 |
| `./tek_hamle_v25 2 512 1 1 1 BCD` | 72.82 | 241.99 | 1109 |
| `./tek_hamle_v25r 2 1024 1 4 10 BDE` | 83.10 | 281.74 | 1312 |
| `./tek_hamle_v25r 2 512 1 1 1 BCD` | 67.01 | 230.11 | 1109 |

