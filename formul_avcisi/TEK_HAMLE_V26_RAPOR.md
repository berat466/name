# TEK HAMLE V26 — Ağırlık okuma hatasını düzelt ve gerçek öğrenmeyi ölç

Dal `claude/formul-avcisi` · kodlar `cpp/tek_hamle_v26.cpp` (V17/V20 ağırlıkları, 3 okuma), `cpp/tek_hamle_v26c.cpp` (V25 doğrulaması) · ham loglar/komutlar `v26_ham/`.

**Protokol.** Aynı veri/bölme/tohumlar (3 tohum, V17 (γ,λ) seçimleri: H=1024 (4,10),(4,100),(2,1); H=512 (1,1),(2,1),(1,1)). **Tüm ağırlıklar yalnız eğitimden** (V20 dâhil: `W=W0−ηD`, η∈{0,0.003,0.005,0.0075,0.01}), bu yüzden sayılar V17/V20 raporlarındaki eğitim+doğrulama modelli değerlerle birebir karşılaştırılamaz; eski sayılar **yeniden ölçüm değildir**, yalnız referanstır. Aynı ağırlık üç şekilde okunur, parametreler yalnız doğrulama CE ile seçilir, test hiçbir seçime girmez:

- `SOFTMAX_KABA`: V17 protokolü `P=(1−α)π+α·softmax(z/T)`, 10 α × 11 T (V9–V24 kalibrasyonu).
- `SOFTMAX_İNCE`: aynı formül, 20 α × 22 T (genişletilmiş bütçe; softmax'ın yalnız kaba ızgara yüzünden kaybetmediğini kontrol etmek için).
- `DOĞRUSAL`: `P=(1−α)π+α·max(z,ε)/Σ_j max(z_j,ε)`, 20 α × 8 ε (ε∈1e-7…1e-2). Bütçe softmax-kaba ile aynı mertebede (160 vs 110), softmax-ince'den küçük (480).

Okuma değişirken W değişmez (aynı model nesnesi). Her okumada olasılık toplamı sapması (|Σp−1|) ve z≤0 oranı ölçüldü.

## 1. Test PPL — aynı ağırlıklar, farklı okuma (3 tohum ortalaması; yalnız-eğitim protokolü)

| H | model (ağırlık) | eski rapor PPL (referans, yeniden ölçüm DEĞİL) | softmax-kaba | softmax-ince | **doğrusal** | doğrusal − softmax-ince |
|---|---|---|---|---|---|---|
| 512 | V17_Ridge(eta=0) | 212,3 | 214.8 | 212.9 | **182.5** | -30.4 |
| 512 | V20_CE(eta kaba-softmax secimi) | 202,8 | 205.6 | 204.8 | **191.6** | -13.1 |
| 512 | V20_CE(eta dogrusal secimi; FARKLI agirlik) | – | 214.8 | 212.9 | 182.5 | -30.4 |
| 1024 | V17_Ridge(eta=0) | 206,8 | 208.9 | 205.6 | **192.2** | -13.3 |
| 1024 | V20_CE(eta kaba-softmax secimi) | 198,9 | 202.3 | 199.9 | **201.4** | +1.5 |
| 1024 | V20_CE(eta dogrusal secimi; FARKLI agirlik) | – | 208.9 | 205.6 | 192.2 | -13.3 |

(tohum sayısı: H=512: 3, H=1024: 3)

## 2. Top-1 / Top-5 (test, 3 tohum ort.) — PPL düşerken doğruluk ne oluyor?

| H | model | okuma | test PPL | Top-1 | Top-5 | eğitim–doğr. CE farkı | z≤0 oranı | y kırpılan oran | |Σp−1| maks |
|---|---|---|---|---|---|---|---|---|---|
| 512 | V17_Ridge(eta=0) | SOFTMAX_KABA | 214.8 | 15.0% | 29.3% | 0.096 | 0.478 | 0.000 | 8.3e-14 |
| 512 | V17_Ridge(eta=0) | SOFTMAX_INCE | 212.9 | 15.0% | 29.6% | 0.148 | 0.478 | 0.000 | 4.3e-14 |
| 512 | V17_Ridge(eta=0) | DOGRUSAL | 182.5 | 14.8% | 33.4% | 0.922 | 0.478 | 0.166 | 6.3e-14 |
| 512 | V20_CE(eta kaba-softmax secimi) | SOFTMAX_KABA | 205.6 | 15.1% | 30.4% | 0.328 | 0.487 | 0.000 | 4.2e-14 |
| 512 | V20_CE(eta kaba-softmax secimi) | SOFTMAX_INCE | 204.8 | 15.4% | 30.4% | 0.343 | 0.487 | 0.000 | 6.7e-14 |
| 512 | V20_CE(eta kaba-softmax secimi) | DOGRUSAL | 191.6 | 14.7% | 32.9% | 0.827 | 0.487 | 0.176 | 4.2e-14 |
| 1024 | V17_Ridge(eta=0) | SOFTMAX_KABA | 208.9 | 15.7% | 29.9% | 0.280 | 0.485 | 0.000 | 4.7e-14 |
| 1024 | V17_Ridge(eta=0) | SOFTMAX_INCE | 205.6 | 15.7% | 30.0% | 0.372 | 0.485 | 0.000 | 4.9e-14 |
| 1024 | V17_Ridge(eta=0) | DOGRUSAL | 192.2 | 15.3% | 33.7% | 1.057 | 0.485 | 0.198 | 8.0e-14 |
| 1024 | V20_CE(eta kaba-softmax secimi) | SOFTMAX_KABA | 202.3 | 15.0% | 30.5% | 0.666 | 0.490 | 0.000 | 4.7e-14 |
| 1024 | V20_CE(eta kaba-softmax secimi) | SOFTMAX_INCE | 199.9 | 15.5% | 30.7% | 0.677 | 0.490 | 0.000 | 4.0e-14 |
| 1024 | V20_CE(eta kaba-softmax secimi) | DOGRUSAL | 201.4 | 14.9% | 33.0% | 1.029 | 0.490 | 0.203 | 4.1e-14 |

## 3. Tohum başına test PPL (softmax-kaba / softmax-ince / doğrusal) ve seçilen parametreler

| H | tohum | model | η | softmax-kaba | softmax-ince | doğrusal | seçilen (α,T) kaba | (α,T) ince | (α,ε) doğrusal |
|---|---|---|---|---|---|---|---|---|---|
| 1024 | 3 | V17_Ridge(eta=0) | 0 | 208.55 | 204.52 | 191.90 | alpha=0.2,T=0.01 | alpha=0.25,T=0.015 | alpha=0.5,eps=1e-05 |
| 1024 | 3 | V20_CE(eta kaba-softmax secimi) | 0.005 | 202.39 | 199.63 | 198.34 | alpha=0.2,T=0.01 | alpha=0.25,T=0.0125 | alpha=0.5,eps=1e-05 |
| 1024 | 2 | V17_Ridge(eta=0) | 0 | 209.73 | 206.36 | 190.11 | alpha=0.2,T=0.01 | alpha=0.25,T=0.0125 | alpha=0.6,eps=0.0001 |
| 1024 | 2 | V20_CE(eta kaba-softmax secimi) | 0.0075 | 201.17 | 199.07 | 200.33 | alpha=0.25,T=0.01 | alpha=0.3,T=0.0125 | alpha=0.5,eps=1e-05 |
| 1024 | 1 | V17_Ridge(eta=0) | 0 | 208.53 | 205.81 | 194.65 | alpha=0.2,T=0.01 | alpha=0.25,T=0.015 | alpha=0.5,eps=1e-07 |
| 1024 | 1 | V20_CE(eta kaba-softmax secimi) | 0.0075 | 203.39 | 201.03 | 205.54 | alpha=0.25,T=0.01 | alpha=0.25,T=0.0125 | alpha=0.5,eps=1e-07 |
| 512 | 3 | V17_Ridge(eta=0) | 0 | 214.60 | 213.03 | 179.73 | alpha=0.2,T=0.01 | alpha=0.25,T=0.0125 | alpha=0.6,eps=1e-05 |
| 512 | 3 | V20_CE(eta kaba-softmax secimi) | 0.01 | 205.51 | 205.06 | 188.80 | alpha=0.25,T=0.01 | alpha=0.3,T=0.0125 | alpha=0.5,eps=1e-07 |
| 512 | 2 | V17_Ridge(eta=0) | 0 | 214.04 | 211.79 | 187.06 | alpha=0.2,T=0.01 | alpha=0.25,T=0.0125 | alpha=0.6,eps=1e-05 |
| 512 | 2 | V20_CE(eta kaba-softmax secimi) | 0.01 | 206.10 | 204.79 | 196.31 | alpha=0.25,T=0.01 | alpha=0.25,T=0.0125 | alpha=0.5,eps=1e-06 |
| 512 | 1 | V17_Ridge(eta=0) | 0 | 215.69 | 213.93 | 180.73 | alpha=0.2,T=0.01 | alpha=0.25,T=0.0125 | alpha=0.6,eps=1e-05 |
| 512 | 1 | V20_CE(eta kaba-softmax secimi) | 0.01 | 205.12 | 204.43 | 189.78 | alpha=0.25,T=0.01 | alpha=0.3,T=0.0125 | alpha=0.5,eps=1e-07 |

## 4. Okuma kazancı ve öğrenme kazancı ayrı (3 tohum ort. test PPL)

| H | V17 softmax-ince | V17 doğrusal | **okuma kazancı** (aynı W) | V20 softmax-ince | V20 doğrusal | V20 W'nin öğrenme kazancı (softmax okumada) | V20 W'nin öğrenme kazancı (doğrusal okumada) | en iyi (W, okuma) |
|---|---|---|---|---|---|---|---|---|
| 1024 | 205.6 | 192.2 | -13.3 | 199.9 | 201.4 | -5.7 | +9.2 | V17+doğrusal (192.2) |
| 512 | 212.9 | 182.5 | -30.4 | 204.8 | 191.6 | -8.2 | +9.1 | V17+doğrusal (182.5) |

- [Kesin] Okuma kazancı (aynı W): H=1024'te V17 ağırlıkları −13,4 PPL, H=512'de −30,4 PPL (ince softmax'a göre; kaba V17 izgarasına göre −16,7 / −32,3).
- [Kesin] Softmax ızgarasını 4,4× genişletmek (kaba→ince) tek başına −0,8…−3,3 PPL kazandırdı: V9–V24 kalibrasyonu hafif eksik bütçeliydi, ama asıl fark ızgaradan değil, okumanın biçiminden geliyor (doğrusal okuma 160 noktalı ızgarayla ince softmax'ın (480 nokta) 13–30 PPL altında).
- [Kesin] **V20'nin tek CE düzeltmesinin kazancı okumaya özgü**: softmax okumada V17→V20 −5,7 (H=1024) / −8,2 (H=512) PPL; doğrusal okumada **+9,2 / +9,1 PPL (kötüleşme)**. V20 gradyanı softmax-kalibre CE için hesaplanmıştı; aynı ağırlıklar doğrusal okumada daha kötü.
- [Kesin] Bu protokolde (yalnız eğitim ağırlıkları, λ softmax okumasıyla seçilmiş) en iyi sonuç **V17 Ridge ağırlıkları + doğrusal okuma**: H=1024 192,2; H=512 **182,5** (H=512 H=1024'ten iyi: doğrusal okumada aşırı uyum, eğitim–doğrulama farkı 0,92–1,06 nat; eğitim PPL ≈ 71).
- [Muhtemel] λ, γ yalnız softmax okuması için seçilmişti; doğrusal okuma için ayrıca tarama yapılmadı (V25'te H=512 λ=100 doğrusal 172,9 görüldü), dolayısıyla doğrusal sonuçlar muhtemelen alt sınırdır değil, iyileştirilebilir.
- Top-1/Top-5 (bölüm 2): doğrusal okuma Top-5'i belirgin artırıyor (H=1024 29,9→33,7%; H=512 29,3→33,4%), Top-1 hafif düşüyor (15,7→15,3%; 15,0→14,8%). PPL kazancı sıralama doğruluğundan çok olasılık kütlesinin dağılımından geliyor.

## 5. V25 bulgusunun doğrulanması (tohum 1; aynı W'ya softmax ve doğrusal okuma)

Okuma parametreleri doğrulamayla (softmax-kaba V17 izgarası = V25 ana protokolü, softmax-ince 22T×20α, doğrusal 8ε×20α). V25 ham değerleri karşılaştırma için son sütunda.

| H | sistem | hedef | λ | softmax-kaba test PPL | softmax-ince | **doğrusal** (Top-1 / Top-5) | V25'te doğrusal (18 noktalı ızgara) | okuma sınırı |
|---|---|---|---|---|---|---|---|---|
| 1024 | B_F(H=1024) | W_Y | 10 | 208.5 | 205.8 | **194.7** (15.0% / 33.4%) | 194.6 | KENAR |
| 1024 | B_F(H=1024) | W_Q | 10 | 223.9 | 220.6 | **174.5** (14.5% / 32.8%) | 175.1 | ic |
| 1024 | B_F(H=1024) | W_Y | 100 | 209.2 | 206.3 | **189.6** (15.3% / 34.0%) | 189.5 | ic |
| 1024 | B_F(H=1024) | W_Q | 100 | 225.5 | 221.4 | **172.2** (14.6% / 32.8%) | 172.5 | ic |
| 1024 | C2_F+w_kimligi+sik_cift(P=4001) | W_Y | 10 | 198.1 | 195.6 | **175.0** (17.1% / 35.7%) | 175.0 | KENAR |
| 1024 | C2_F+w_kimligi+sik_cift(P=4001) | W_Q | 10 | 210.8 | 210.3 | **152.8** (15.6% / 34.4%) | 152.8 | ic |
| 1024 | C0_yalniz_kimlik_(F_yok)(P=4001) | W_Y | 10 | 203.9 | 201.1 | **135.7** (16.3% / 35.4%) | 136.8 | ic |
| 1024 | C0_yalniz_kimlik_(F_yok)(P=4001) | W_Q | 10 | 212.2 | 208.5 | **137.5** (15.8% / 35.2%) | 137.5 | KENAR |
| 512 | B_F(H=512) | W_Y | 1 | 215.7 | 213.9 | **180.7** (14.8% / 33.6%) | 182.2 | ic |
| 512 | B_F(H=512) | W_Q | 1 | 230.1 | 225.6 | **169.6** (14.4% / 33.1%) | 169.6 | ic |
| 512 | B_F(H=512) | W_Y | 100 | 225.0 | 225.0 | **171.6** (14.1% / 32.8%) | 172.9 | ic |
| 512 | B_F(H=512) | W_Q | 100 | 236.5 | 234.2 | **175.1** (13.7% / 31.5%) | 175.8 | ic |
| 512 | C1_F+w_kimligi(P=2001) | W_Y | 1 | 206.0 | 201.2 | **162.7** (16.4% / 35.2%) | 163.8 | ic |
| 512 | C1_F+w_kimligi(P=2001) | W_Q | 1 | 213.0 | 213.0 | **146.7** (15.4% / 34.3%) | 146.5 | ic |
| 512 | C1_F+w_kimligi(P=2001) | W_Y | 100 | 212.5 | 211.5 | **150.2** (15.8% / 35.2%) | 150.1 | ic |
| 512 | C1_F+w_kimligi(P=2001) | W_Q | 100 | 224.1 | 218.5 | **152.2** (15.0% / 33.9%) | 152.2 | ic |
| 512 | C2_F+w_kimligi+sik_cift(P=4001) | W_Y | 1 | 203.0 | 202.0 | **166.4** (16.4% / 34.7%) | 166.3 | ic |
| 512 | C2_F+w_kimligi+sik_cift(P=4001) | W_Q | 1 | 210.1 | 210.1 | **144.8** (15.9% / 34.6%) | 144.8 | ic |

- [Kesin] V25 bulgusu tekrarlandı: aynı W, doğrusal okuma → B (F) W_Q ≈ 174–175 (V25: 175,1), C2 W_Q 152,8 (V25: 152,8), yalnız-kimlik 135,7 (W_Y) / 137,5 (W_Q) (V25: 136,8 / 137,5). Sayılar V25 ile ± birkaç PPL içinde (ızgara farkı: 160 vs 18 nokta).
- [Kesin] Softmax okuması W_Q için yetersiz: aynı W_Q yüksek açıklanan varyansa rağmen softmax-ince 208–221 PPL verir; doğrusal okuma 137–175.
- **Yalnız kimlikle trigram/bigram tablosunu ridge ile yeniden ifade etmek yeni öğrenme başarısı değildir** (V25'te belirtildi); yukarıdaki kimlikli satırlar kimlik bloğunun n-gram tablosu gibi davranmasının ölçümüdür.

## 6. Süre ve bellek

| H | tohum | Ridge (gram+Cholesky+çözüm) s | V20 CE düzeltmesi (gradyan+D) s | kalibrasyon tabloları (5 η): kaba / ince softmax / doğrusal s | toplam s | tepe RAM MB |
|---|---|---|---|---|---|---|
| 1024 | 3 | 4.30 | 12.94 | 2.58 / 4.31 / 0.31 | 55.0 | 792 |
| 1024 | 2 | 9.71 | 12.08 | 3.10 / 4.72 / 0.35 | 61.8 | 792 |
| 1024 | 1 | 9.41 | 12.61 | 2.83 / 4.41 / 0.33 | 59.3 | 792 |
| 512 | 3 | 3.75 | 5.35 | 2.87 / 4.45 / 0.33 | 35.5 | 650 |
| 512 | 2 | 3.68 | 6.05 | 2.61 / 4.14 / 0.30 | 34.2 | 651 |
| 512 | 1 | 3.70 | 6.45 | 3.14 / 4.55 / 0.35 | 37.1 | 651 |

**Uyarı:** Bu süreler tutarsız: V25-doğrulama koşusu (`v26c`) aynı anda çalıştığı için 4 çekirdek paylaşıldı; V26 Ridge süreleri normalde ≈1,7 s (H=1024), V20 düzeltmesi ≈6 s (V22/V25 temiz ölçümleri) iken burada 2–4× büyük görünüyor. Süre karşılaştırması için yalnız oranlar anlamlı: doğrusal okuma kalibrasyon tablosu softmax-kabadan ≈7–9× hızlı (üstel hesap yok), ince softmax ızgarası kabadan ≈1,5× yavaş. Okuma (tek değerlendirme) 0,3–0,8 s, doğrusal en hızlısı. [Kesin: oranlar loglardan; mutlak süreler güvenilmez]

## 7. Sonuç

**Ana soru: formülle ürettiğimiz ağırlıklar yetersiz mi, yoksa softmax yüzünden bilgiyi yanlış mı okuyorduk?**

- [Kesin] Softmax kalibrasyonu **ciddi bir okuma kaybı** yaratıyordu: aynı V17 Ridge ağırlıkları doğrusal okumada H=1024'te 205,6 → 192,2, H=512'de 212,9 → 182,5 PPL (3 tohum, yalnız-eğitim protokolü, parametreler yalnız doğrulamada). Bu fark softmax ızgarasını genişletmekle kapanmıyor.
- [Kesin] Bu iyileşme "okuma kazancıdır"; ağırlık değişmedi. Öğrenme kazancı olarak sunulamaz. Aynı zamanda ağırlıkların kalitesi softmax okuma altında olduğundan kötü görünüyordu.
- [Kesin] **V20'nin CE düzeltmesi bu protokolde doğrusal okumayla en iyi sonucu vermiyor**; V17 + doğrusal okuma V20'yi (her iki okumayla) geçiyor. V20'nin PPL kazancı softmax'a özgüymüş. Dolayısıyla V19–V21'in "CE düzeltmesi" iyileştirmeleri okuma katmanına bağımlı bir bulgudur.
- [Kesin] Doğrusal okuma Top-5'i artırıyor (+3,8 pp), Top-1'i hafif düşürüyor (−0,4 pp); eğitim–doğrulama farkı çok büyüyor (≈1,0 nat; eğitim PPL ≈ 71 vs test 192) → bu okumayla eğitim CE'si güvenilir bir ölçüt değil; seçim yalnız doğrulamayla yapılmalı.
- [Muhtemel] 100 PPL'e yaklaşmak için bu okuma tek başına yetmez (en iyi 3-tohum ≈ 182,5); ama V25'te kimlik+doğrusal okumayla ≈ 137–150 görüldü ki bu n-gram tablosunun yeniden ifadesidir.
- Doğrusal okuma V26 **okuma seçeneği** olarak uygulandı (`tek_hamle_v26.cpp`), ana algoritmaya birleştirilmedi.

## 8. Sınırlamalar

- 3 tohum, tek metin/bölme; test n=4 980. λ/γ doğrusal okuma için yeniden seçilmedi. Doğrusal okumada ε=1e-7 sıkça ızgara kenarında seçildi (etkisi, α karışımı nedeniyle küçük; ε→0 sınırı) ve α=0,5–0,8 aralığında.
- V20 burada yalnız-eğitim protokolüyle (η doğrulamayla, V20 raporu: eğitim+doğrulama modeli): "eski PPL" sütunu referanstır, birebir yeniden ölçüm değildir.
- Süreler çekirdek paylaşımı nedeniyle güvenilmez (bölüm 6). Sayısal doğruluk: tüm okumalarda |Σp−1| ≤ 1e-13 ✓; negatif/sıfır z oranı ≈ %48 (V17/V20, doğrusal ve softmax aynı z), doğrusal okumada gerçek sınıfın kırpılma oranı %17–20.
- V25 doğrulaması tek tohum; ızgaralar farklı olduğundan tam eşitlik beklenmez.

## 9. Çalıştırma

Derleme: `g++ -O3 -march=native -fopenmp -std=c++17 cpp/tek_hamle_v26.cpp -o tek_hamle_v26`; komut: `./tek_hamle_v26 2 H TOHUM GAMMA LAMBDA`. V25 doğrulaması: `./tek_hamle_v26c 2 H 1 GAMMA LAMBDA MOD` (`BDE` / `BCD`).

| komut | wall s | user CPU s | RAM MB |
|---|---|---|---|
| `./tek_hamle_v26 2 1024 1 4 10` | 59.67 | 109.33 | 792 |
| `./tek_hamle_v26 2 1024 2 4 100` | 61.87 | 111.03 | 792 |
| `./tek_hamle_v26 2 1024 3 2 1` | 55.25 | 101.16 | 792 |
| `./tek_hamle_v26 2 512 1 1 1` | 37.18 | 65.00 | 651 |
| `./tek_hamle_v26c 2 1024 1 4 10 BDE` | 251.06 | 490.88 | 1313 |
| `./tek_hamle_v26 2 512 2 2 1` | 34.35 | 63.98 | 651 |
| `./tek_hamle_v26 2 512 3 1 1` | 35.62 | 62.65 | 650 |
| `./tek_hamle_v26c 2 512 1 1 1 BCD` | 105.96 | 299.80 | 1109 |

