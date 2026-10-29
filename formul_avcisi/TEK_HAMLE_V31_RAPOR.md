# TEK HAMLE V31 — Kelime kimliğini koru, 4 F ile eksik bilgiyi tamamla

Dal `claude/formul-avcisi` · kod `cpp/tek_hamle_v31.cpp` (V30 kopyası) · ham loglar/komutlar `v31_ham/`. H=512 (4×128), yalnız eğitim protokolü, V26 doğrusal okuma (α,ε yalnız doğrulama). Seçimler doğrulama CE ile; test seçime girmedi. Tamamlanan tohumlar: [1, 2, 3].

## 0. Özet (H=512, doğrusal okuma, test)

- [Kesin] **Çiftsiz kimlik C=[ID(w_{t−2}),ID(w_{t−1})] yalnız ridge: 144,1 PPL** (tohumdan bağımsız, çünkü rastgele özellik yok; λ_C=20 doğrulamayla). Ortak çözüm **kimlik + dört F: 140,3 PPL** (3 tohum: 139,8 / 141,2 / 139,9; λ_C=100, λ_F=3000). F'nin eklediği gerçek test kazancı ≈ 3,8 PPL (%2,6), hem trigram bağlamı görülen hem görülmeyen satırlarda.
- [Kesin] V30'a (153,4) göre büyük iyileşme; **136'nın altına inilemedi** (140,3). V30'da anılan "yalnız kimlik 135,7" referansı **çift (trigram bağlamı) kimliği** içeren farklı bir özellik kümesiydi; aynı koşulda yeniden ölçüm (bölüm 4): 133,2 (λ=20). Yani V31'in çiftsiz C+4F'si, çift kimlikli yalnız-kimlik tablosunun gerisinde: trigram-bağlamı kimliği, 4 F'nin matematiksel bilgisinden daha değerli çıktı.
- [Kesin] Kontrol: λ_F→∞ (F kapalı) yalnız-kimlik sonucuna aynen dönüyor (doğrulama CE eşit); ayrı düzenlileştirme ve Schur çözümü doğru çalışıyor (rezidü 4e-7).
- Kimlik n-gram sayımlarına dayanır; bu sonuçlar bağımsız genelleme başarısı olarak sunulmuyor.

## 1. Formül ve hesap

`Z=[C,F₁,F₂,F₃,F₄]`, `C=[ID(w_{t−2}),ID(w_{t−1})]` (4002 seyrek sütun; yoğun one-hot **veri** matrisi yok), F zinciri V28 gibi: `F₁=tanh(γ₁X A₁+b₁)`, `F_i=tanh(γ₂[X,F₁..F_{i−1}]A_i+b_i)` (γ₂=0,5); **F girdisine kimlik girmez** (kimlik yalnız ortak çözümde, doğrudan sütun olarak).

Tek ortak normal denklem: `(ZᵀZ+Λ)W=ZᵀY`, `Λ=diag(λ_C I_C, λ_F I_F)`:

```
[ A_CC+λ_C I   A_CF        ] [W_C]   [R_C]
[ A_FC         A_FF+λ_F I  ] [W_F] = [R_F]
```
Blok eliminasyon (Schur): `W_F=(A_FF+λ_F I−A_FC(A_CC+λ_C I)⁻¹A_CF)⁻¹(R_F−A_FC(A_CC+λ_C I)⁻¹R_C)`, `W_C=(A_CC+λ_C I)⁻¹(R_C−A_CF W_F)`. `A_CC` (4002²) sayım matrisi (diyagonal = kelime sayıları, çapraz = (t−2,t−1) çift sayıları) çift duyarlıkla Cholesky; F bloğu 513×513. Bu, iki bağımsız ridge'in toplamı **değildir**: çapraz terimler `A_CF` Schur tamamlayıcıda ve `W_C`'nin F'ye bağlı düzeltmesinde yer alır. Çözücü doğrudandır (yaklaşık yöntem yok); rezidü bölüm 6'da.

## 2. Deney A/B — taramalar (tohum 1; doğrulama CE ile)

Yalnız kimlik `W_C=(CᵀC+λ_C I)⁻¹CᵀY`:

| λ_C | doğrulama CE (PPL) |
|---|---|
| 1 | 5.0625 (158.0) |
| 10 | 5.0244 (152.1) |
| 20 | 5.0183 (151.2) |
| 100 | 5.0317 (153.2) |
| 300 | 5.0550 (156.8) |
| 1000 | 5.0851 (161.6) |

Ortak çözüm (λ_C × λ_F), doğrulama PPL (`1e9` = F katkısı kapalı kontrolü):

| λ_C \ λ_F | 1 | 20 | 100 | 300 | 1000 | 3000 | 10000 | 1e9 |
|---|---|---|---|---|---|---|---|---|
| 1 | 161.8 | 157.5 | 157.1 | – | 157.3 | – | – | 158.0 |
| 10 | 158.8 | 153.3 | 151.5 | – | 150.0 | – | – | 152.1 |
| 100 | 164.7 | 156.9 | 152.4 | 149.8 | 147.6 | **147.0** | 147.6 | 153.2 |
| 300 | – | – | – | 156.6 | 153.4 | 152.0 | 151.9 | 156.8 |
| 1000 | – | – | – | 168.0 | 165.8 | 164.8 | 164.7 | 161.6 |

Seçilen: **λ_C=100, λ_F=3000** (iki eksende de ızgara içinde: ilk taramada λ_C=100 ve λ_F=1000 kenardaydı, bu yüzden λ_C∈{300,1000} ve λ_F∈{3000,10000} eklendi). Yalnız kimlik için λ_C=20 (komşular 10 ve 100 daha kötü). F kapalı kontrol (`λ_F=1e9`) her λ_C için yalnız-kimlik doğrulama CE'sini **aynen** veriyor (ör. λ_C=100: 5,0317 = 5,0317): ortak sistem F katkısı kapatılınca yalnız-kimliğe dönüyor.

## 3. Sonuçlar (test; doğrusal okuma)

| tohum | sistem | λ_C / λ_F | test CE | test PPL | Top-1 | Top-5 | eğitim PPL | doğr. PPL | eğitim–doğr. CE farkı |
|---|---|---|---|---|---|---|---|---|---|
| 1 | yalnız kimlik C | 20 / – | 4.9704 | **144.1** | 16.2% | 35.6% | 39 | 151 | 1.350 |
| 1 | **V31 kimlik + 4F (ortak)** | 100 / 3000 | 4.9401 | **139.8** | 15.6% | 35.7% | 52 | 147 | 1.048 |
| 2 | yalnız kimlik C | 20 / – | 4.9704 | **144.1** | 16.2% | 35.6% | 39 | 151 | 1.350 |
| 2 | **V31 kimlik + 4F (ortak)** | 100 / 3000 | 4.9499 | **141.2** | 15.5% | 36.0% | 54 | 148 | 1.006 |
| 3 | yalnız kimlik C | 20 / – | 4.9704 | **144.1** | 16.2% | 35.6% | 39 | 151 | 1.350 |
| 3 | **V31 kimlik + 4F (ortak)** | 100 / 3000 | 4.9409 | **139.9** | 15.6% | 35.8% | 54 | 148 | 1.007 |
| **ort. (n=3)** | yalnız kimlik C | | | **144.1** | 16.2% | 35.6% | 39 | 151 | 1.350 |
| **ort. (n=3)** | **V31 kimlik + 4F** | | | **140.3** | 15.6% | 35.8% | 53 | 148 | 1.020 |

## 4. Referans netleştirmesi: V30'daki "yalnız kimlik 135,7" hangi özelliklerdi?

V25/V26c'nin `C0` sistemi `[ID(w_{t−1}) (2001), en sık 2000 (w_{t−2},w_{t−1}) **çift** kimliği]` idi; yani **sıralı çift (trigram bağlamı) bilgisi** içeriyordu. V31'in `C`'si ise görevdeki gibi yalnız `[ID(w_{t−2}),ID(w_{t−1})]` (çift kimliği yok). İkisi farklı özellik kümeleri; V30 raporunda "yalnız kimlik" etiketi bu farkı belirtmiyordu. Aynı koşulda yeniden ölçüm (tohum 1, v26c `E` modu, doğrusal okuma, λ doğrulamayla):

| λ | doğrulama CE (PPL) | test PPL | Top-1 / Top-5 |
|---|---|---|---|
| 10 | 4.9544 (141.8) | 135.7 | 16.3% / 35.4% |
| 20 | 4.9356 (139.2) | 133.2 | 16.6% / 36.4% |
| 100 | 4.9625 (142.9) | 134.7 | 16.2% / 35.8% |

Doğrulamayla seçilen λ=20: test PPL **133.2** (çift kimlikli, n-gram tablosu benzeri). Bu, V31'in `C` (çiftsiz) yalnız-kimlik referansından (bölüm 3) farklıdır; **adil V31 karşılaştırması çiftsiz C ile yapılır.** Çift kimlikli "C+çift" ortak çözüm bu raporda denenmedi (süre).

## 5. Kimlik ve F'nin ayrı katkısı (ortak çözümün tohum 1 logitleri; aynı W)

| okuma | test PPL | Top-1 / Top-5 | eğitim–doğr. CE farkı |
|---|---|---|---|
| tam ortak: C+F | 139.8 | 15.6% / 35.7% | 1.048 |
| yalnız C kısmı (C·W_C) | 167.3 | 15.1% / 34.0% | 1.138 |
| yalnız F kısmı (F·W_F) | 210.4 | 10.1% / 26.5% | 0.156 |
| ayrı yalnız-kimlik çözümü (λ_C=20) | 144.1 | 16.2% / 35.6% | 1.350 |

Parçalar kendi α,ε'larıyla (doğrulamayla) okundu; C kısmı tek başına yalnız-kimlik çözümünden daha **kötü** (ortak çözümde F, kimlik ağırlıklarını da değiştirir: `W_C` ortak çözümün düzeltilmiş hâli); F kısmı tek başına zayıf. Birlikte daha iyi: F, kimliğin açıklayamadığı kalan bilgiyi tamamlıyor.

## 6. F gerçekten yeni bilgi ekliyor mu? Görülmüş / görülmemiş bağlamlar

| tohum | yalnız kimlik C test PPL | V31 kimlik+4F test PPL | **F'nin test kazancı** | bağlam GÖRÜLDÜ: C → C+F (PPL) | yalnız bigram: C → C+F (PPL) |
|---|---|---|---|---|---|
| 1 | 144.1 | 139.8 | +4.3 PPL (3.0%) | 146.7 → 143.5 | 138.6 → 132.1 |
| 2 | 144.1 | 141.2 | +2.9 PPL (2.0%) | 146.7 → 143.9 | 138.6 → 135.5 |
| 3 | 144.1 | 139.9 | +4.2 PPL (2.9%) | 146.7 → 143.3 | 138.6 → 132.9 |
| **ort.** | 144.1 | 140.3 | +3.8 PPL | | |

- [Kesin] Ortak çözümde F, yalnız-kimliğe göre ortalama 3.8 PPL kazandırıyor; kazanç hem trigram bağlamı görülen hem görülmeyen (yalnız bigram) satırlarda var. Sonuç **136'nın altına inmedi**: ortalama 140.3. (136 referansı çift kimlikli farklı bir yalnız-kimlik setiydi, bölüm 4; çiftsiz C ile referans 144.1.)
- [Kesin] V30'a göre (153,4; kimlik F'lerin içinde, seyrek ağırlıkla) V31 ayrı düzenlileştirmeyle belirgin iyileşti: kimliği F içinde seyreltmek yerine doğrudan korumak ve F'yi güçlü düzenlileştirmek (λ_F≈3000 ≫ λ_C=100) kazandırdı. F neredeyse "küçük düzeltme" olarak kullanılıyor; λ_F=1e9 (F kapalı) ve λ_F=1 (F serbest) ikisi de daha kötü.
- [Kesin] Eğitim PPL yalnız-kimlikte ≈39 → ortak ≈52: ortak çözüm eğitimde daha az ezberliyor, doğrulama/test daha iyi. Kimlik n-gram sayımlarına dayanır; bu **yeni bağımsız genelleme olarak sunulmuyor**.

## 7. Sayısal doğruluk, süre, bellek

- Çözüm rezidüsü (seçilen ortak çözüm, tohum 1): Z^T(ZW-Y)+Lambda W||/||Z^T Y|| = 2.725e-07 (float gemm ile olculdu; blok eliminasyon Cholesky cift duyarlik, cozum float). Tüm okumalarda olasılık toplamı sapması ≤ 1e-13.
- Süre (tohum 1, tek süreç değil: bölüm 4 kontrolü ile ardışık): C-blok Cholesky (4002²) + Schur hazırlığı ≈ λ_C=1: 11.26s, λ_C=10: 11.30s, λ_C=100: 10.26s; her λ_F için 513×513 çözüm + `W_C` yeniden yapımı ≈ 0,4–1,0 s. F zinciri üretimi + Gram ≈ 1,3 s.

| komut | wall s | user CPU s | RAM MB |
|---|---|---|---|
| `./tek_hamle_v31 2 512 1 1 1 0.5 Z 1 Y - 1,10,20,100 1,10,100 1,20,100,1000,1e9` | 64.02 | 196.66 | 1326 |
| `./tek_hamle_v31 2 512 1 1 1 0.5 Z 1 Y - 20 100,300,1000 300,1000,3000,10000,1e9` | 63.45 | 207.51 | 1311 |
| `./tek_hamle_v26c 2 512 1 1 10 E` | 24.54 | 75.16 | 1011 |
| `./tek_hamle_v26c 2 512 1 1 20 E` | 23.00 | 71.80 | 1012 |
| `./tek_hamle_v26c 2 512 1 1 100 E` | 23.19 | 71.57 | 1011 |
| `./tek_hamle_v31 2 512 2 2 1 0.5 Z 1 Y - 10,20,100 100 3000,1e9` | 42.24 | 131.00 | 1311 |
| `./tek_hamle_v31 2 512 3 1 1 0.5 Z 1 Y - 10,20,100 100 3000,1e9` | 40.04 | 126.42 | 1311 |

## 8. Sınırlamalar

- Seçim yalnız tohum 1 doğrulamasında; tohum 2–3 yalnız λ_C=100, λ_F=3000 (ve yalnız-kimlik için kendi doğrulamasıyla λ_C∈{10,20,100}) ile koşuldu. H=1024 yapılmadı. Ortak sistemde C+çift (hash/sık çift) yok. γ₁, γ₂, F zinciri değişmedi.
- F zinciri girdisinde kimlik yok (görev tanımı gereği); V30'daki "F girdisine kimlik" ile bu çalışmada F'nin kimlikten bağımsız olması karşılaştırılmadı (aynı program içinde).
- Q hedefi/açıklanan varyans ölçülmedi (yalnız Y). Yalnız-kimlik ve ortak çözüm n-gram sayımlarına dayalıdır.
- Yeni yapı ana mimariye eklenmedi.


