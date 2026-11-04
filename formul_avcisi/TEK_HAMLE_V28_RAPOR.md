# TEK HAMLE V28 — 4 bağlantılı F ile (X erişimli) bilgi kaybını giderme

Dal `claude/formul-avcisi` · kod `cpp/tek_hamle_v28.cpp` (V27 kopyası) · ham loglar/komutlar `v28_ham/`. Yalnız eğitim protokolü; V26 doğrusal okuma (`P=(1−α)π+α·max(z,ε)/Σmax(z,ε)`, α∈20, ε∈8 değer yalnız **doğrulama CE ile**); softmax yok. (γ₁,λ) V17 seçimleri; **γ₂** (F₂..F₄ tanh ölçeği) doğrulamayla seçildi: **γ₂=0.5** (bölüm 4). Tohum 1/2/3 için H=512: (1,1),(2,1),(1,1); H=1024: (4,10),(4,100),(2,1).

## 1. Yapı

- `F₁=tanh(γ₁XA₁+b₁)` (V17 yöntemi). `F_i=tanh(γ₂·[X̃,F̃₁..F̃_{i−1}]·A_i+b_i)`, `i=2,3,4`; `~` = blok ölçeklendirme. Yön üretimi (her aşamada aynı kural): hedefleri farklı iki eğitim satırının birleşik girişleri farkı birim vektöre normalize edilir, bias V17'deki gibi çift orta noktasından (std ile N(0,σ²)). Yalnız eğitim satırları kullanılır; val/test yalnız dönüştürülür; etiketler özelliğe girmez (etiket yalnız "farklı hedef" çift seçiminde, V17 ile aynı).
- **Ölçek:** ham X ile F blokları farklı ölçekte; her blok, eğitim satır-normu RMS'i 1 olacak şekilde ölçeklendi (ölçekler bölüm 5'te). V27 (X yok) ölçeksiz, birebir yeniden üretildi (`V27_X_YOK`).
- Ağırlıklar: `R₀=T`, `W_i=(F_iᵀF_i+λI)⁻¹F_iᵀR_{i−1}` (yoğun artık yerine `F_iᵀT−Σ_{j<i}F_iᵀF_jW_j`), `Ŷ=ΣF_iW_i`. E: dört F tek ortak ridge. T = Y (gerçek etiket) veya Q (trigram öğretici, eğitim satırlarında LOO). Q'dan gelen sonuçlar bağımsız öğrenme olarak sunulmaz.

## 2. Ana sonuç (doğrusal okuma; test PPL; tohum ort.)

| H | sistem | hedef | test PPL | Top-1 | Top-5 | eğitim PPL | doğr. PPL | eğitim–doğr. CE farkı | Q açıklanan varyans (test) |
|---|---|---|---|---|---|---|---|---|---|
| 512 (n=3) | A · tek F | Y | **182.5** | 14.8% | 33.3% | 76 | 191 | 0.922 | – |
| 512 (n=3) | A · tek F | Q | **170.3** | 14.4% | 32.7% | 105 | 180 | 0.537 | 0.397 |
| 512 (n=3) | B · V27 bağlı 4F (X yok), F₁..F₄ | Y | **191.5** | 13.7% | 32.1% | 95 | 200 | 0.746 | – |
| 512 (n=3) | B · V27 bağlı 4F (X yok), F₁..F₄ | Q | **177.9** | 13.2% | 31.4% | 128 | 191 | 0.400 | 0.294 |
| 512 (n=3) | C/D · V28 X-bağlantılı 4F, F₁..F₄ | Y | **180.7** | 13.9% | 32.3% | 95 | 189 | 0.692 | – |
| 512 (n=3) | C/D · V28 X-bağlantılı 4F, F₁..F₄ | Q | **176.7** | 13.3% | 31.6% | 127 | 188 | 0.390 | 0.311 |
| 512 (n=3) | V28 X-bağlantılı, yalnız F₁ | Y | **186.4** | 13.2% | 31.5% | 108 | 196 | 0.591 | – |
| 512 (n=3) | V28 X-bağlantılı, yalnız F₁ | Q | **183.9** | 12.6% | 30.5% | 144 | 195 | 0.306 | 0.260 |
| 512 (n=3) | E · V28 ortak ridge (4F tek çözüm) | Y | **176.5** | 14.6% | 33.1% | 85 | 185 | 0.781 | – |
| 512 (n=3) | E · V28 ortak ridge (4F tek çözüm) | Q | **171.0** | 14.1% | 32.2% | 116 | 182 | 0.446 | 0.357 |
| 1024 (n=3) | A · tek F | Y | **192.2** | 15.3% | 33.7% | 69 | 199 | 1.057 | – |
| 1024 (n=3) | A · tek F | Q | **172.3** | 14.4% | 32.9% | 98 | 182 | 0.616 | 0.430 |
| 1024 (n=3) | B · V27 bağlı 4F (X yok), F₁..F₄ | Y | **195.6** | 14.0% | 32.7% | 81 | 202 | 0.918 | – |
| 1024 (n=3) | B · V27 bağlı 4F (X yok), F₁..F₄ | Q | **176.1** | 13.3% | 31.8% | 118 | 188 | 0.465 | 0.324 |
| 1024 (n=3) | C/D · V28 X-bağlantılı 4F, F₁..F₄ | Y | **185.2** | 13.8% | 32.7% | 92 | 193 | 0.748 | – |
| 1024 (n=3) | C/D · V28 X-bağlantılı 4F, F₁..F₄ | Q | **176.6** | 13.2% | 31.7% | 123 | 188 | 0.428 | 0.320 |
| 1024 (n=3) | V28 X-bağlantılı, yalnız F₁ | Y | **189.2** | 13.6% | 32.2% | 99 | 197 | 0.688 | – |
| 1024 (n=3) | V28 X-bağlantılı, yalnız F₁ | Q | **180.6** | 13.0% | 31.2% | 130 | 192 | 0.388 | 0.292 |
| 1024 (n=3) | E · V28 ortak ridge (4F tek çözüm) | Y | **183.4** | 14.2% | 33.2% | 86 | 191 | 0.805 | – |
| 1024 (n=3) | E · V28 ortak ridge (4F tek çözüm) | Q | **173.3** | 13.7% | 32.1% | 117 | 184 | 0.458 | 0.351 |

## 3. Aşama aşama katkı (3 tohum ort.; V28 X-bağlantılı vs V27)

| H | zincir | aşama | eklenen sütun | Y: test PPL | Y: eğitim / doğr. PPL | Y: kalan hata (test) | Q: açıklanan varyans (test) | Q: ek | Q: test PPL |
|---|---|---|---|---|---|---|---|---|---|
| 512 | V28 (X erişimli) | F1 | 129 | 186.4 | 108 / 196 | 0.9722 | 0.260 | +0.260 | 183.9 |
| 512 | V28 (X erişimli) | F2 | 128 | 182.9 | 100 / 192 | 0.9715 | 0.287 | +0.027 | 180.7 |
| 512 | V28 (X erişimli) | F3 | 128 | 181.6 | 96 / 190 | 0.9709 | 0.301 | +0.014 | 178.3 |
| 512 | V28 (X erişimli) | F4 | 128 | 180.7 | 95 / 189 | 0.9705 | 0.311 | +0.010 | 176.7 |
| 512 | V27 (X yok) | F1 | 129 | 186.4 | 108 / 196 | 0.9722 | 0.260 | +0.260 | 183.9 |
| 512 | V27 (X yok) | F2 | 128 | 185.4 | 104 / 195 | 0.9710 | 0.285 | +0.026 | 178.8 |
| 512 | V27 (X yok) | F3 | 128 | 188.4 | 97 / 197 | 0.9714 | 0.293 | +0.008 | 177.6 |
| 512 | V27 (X yok) | F4 | 128 | 191.5 | 95 / 200 | 0.9725 | 0.294 | +0.001 | 177.9 |
| 1024 | V28 (X erişimli) | F1 | 257 | 189.2 | 99 / 197 | 0.9715 | 0.292 | +0.292 | 180.6 |
| 1024 | V28 (X erişimli) | F2 | 256 | 186.8 | 95 / 195 | 0.9717 | 0.307 | +0.015 | 178.6 |
| 1024 | V28 (X erişimli) | F3 | 256 | 186.0 | 93 / 194 | 0.9717 | 0.314 | +0.007 | 177.5 |
| 1024 | V28 (X erişimli) | F4 | 256 | 185.2 | 92 / 193 | 0.9716 | 0.320 | +0.006 | 176.6 |
| 1024 | V27 (X yok) | F1 | 257 | 189.2 | 99 / 197 | 0.9715 | 0.292 | +0.292 | 180.6 |
| 1024 | V27 (X yok) | F2 | 256 | 190.9 | 86 / 199 | 0.9720 | 0.316 | +0.024 | 176.7 |
| 1024 | V27 (X yok) | F3 | 256 | 194.1 | 83 / 202 | 0.9733 | 0.322 | +0.006 | 176.3 |
| 1024 | V27 (X yok) | F4 | 256 | 195.6 | 81 / 202 | 0.9742 | 0.324 | +0.002 | 176.1 |

## 4. γ₂ seçimi (H=512, tohum 1; seçim doğrulama PPL ile, test yalnız bilgi)

| γ₂ | V28 Y F₁..F₄ doğr. PPL | (bilgi) test PPL | V28 Q F₁..F₄ doğr. PPL |
|---|---|---|---|
| 0.25 | 189.8 | 181.7 | 190.1 |
| 0.5 ← seçilen | 188.1 | 180.1 | 187.0 |
| 1 | 190.3 | 182.1 | 186.3 |
| 2 | 195.2 | 186.1 | 188.3 |

## 5. Blok ölçekleri (tohum 1, H=512; ham satır-normu RMS ve uygulanan ölçek)

- `V28_X_BAGLANTILI_4F(4x128)|F2 girdisi|X(genislik 192): ham satir-norm RMS=1.8634 uygulanan olcek=0.5366 ; F1(genislik 128): ham satir-norm RMS=3.1351 uygulanan olcek=0.3190 ; toplam giris boyutu=320`
- `V28_X_BAGLANTILI_4F(4x128)|F3 girdisi|X(genislik 192): ham satir-norm RMS=1.8634 uygulanan olcek=0.5366 ; F1(genislik 128): ham satir-norm RMS=3.1351 uygulanan olcek=0.3190 ; F2(genislik 128): ham satir-norm RMS=2.0727 uygulanan olcek=0.4825 ; toplam giris boyutu=448`
- `V28_X_BAGLANTILI_4F(4x128)|F4 girdisi|X(genislik 192): ham satir-norm RMS=1.8634 uygulanan olcek=0.5366 ; F1(genislik 128): ham satir-norm RMS=3.1351 uygulanan olcek=0.3190 ; F2(genislik 128): ham satir-norm RMS=2.0727 uygulanan olcek=0.4825 ; F3(genislik 128): ham satir-norm RMS=3.2648 uygulanan olcek=0.3063 ; toplam giris boyutu=576`

## 6. Kritik sorular

**H=512 (n=3 tohum)**

1. *X'i yeniden vermek V27 kaybını azaltıyor mu?* Y hedefi, F₁..F₄ test PPL: V27 191.5 → V28 180.7 (tek F 182.5; yalnız F₁ 186.4). Q açıklanan varyans (test): V27 0.294 → V28 0.311 (tek F 0.397).
2. *F₃, F₄ yeni bilgi açıklıyor mu?* V28 Q-zincirde ek açıklanan varyans (test) F₂ +0.027, F₃ +0.014, F₄ +0.010 (F₁ 0.260).
3. *%40–43 sınırı aşılıyor mu?* En yüksek açıklanan varyans (test): bağlı zincir 0.311, ortak ridge 0.357, tek F 0.397. Sınır aşılmadı.
4. *Eğitim iyileşmesi testte de görülüyor mu?* Y hedefi eğitim PPL: tek F 76, V28 zinciri 95; doğrulama: 191 vs 189; test: 182.5 vs 180.7.
5. *Tek F'den başarılı ve hızlı mı?* Ortak ridge (E) Y test PPL 176.5 (tek F 182.5). Süre (zincir başına toplam, hazırlık+iki hedef+değerlendirme; tohum ort.): tek F 9.5 s, V28 zinciri 16.6 s, V27 15.7 s, ortak ridge 13.4 s (değerlendirme ağırlıklı; zincir 4 aşama değerlendirdiği için daha uzun).

**H=1024 (n=3 tohum)**

1. *X'i yeniden vermek V27 kaybını azaltıyor mu?* Y hedefi, F₁..F₄ test PPL: V27 195.6 → V28 185.2 (tek F 192.2; yalnız F₁ 189.2). Q açıklanan varyans (test): V27 0.324 → V28 0.320 (tek F 0.430).
2. *F₃, F₄ yeni bilgi açıklıyor mu?* V28 Q-zincirde ek açıklanan varyans (test) F₂ +0.015, F₃ +0.007, F₄ +0.006 (F₁ 0.292).
3. *%40–43 sınırı aşılıyor mu?* En yüksek açıklanan varyans (test): bağlı zincir 0.320, ortak ridge 0.351, tek F 0.430. Sınır aşılmadı.
4. *Eğitim iyileşmesi testte de görülüyor mu?* Y hedefi eğitim PPL: tek F 69, V28 zinciri 92; doğrulama: 199 vs 193; test: 192.2 vs 185.2.
5. *Tek F'den başarılı ve hızlı mı?* Ortak ridge (E) Y test PPL 183.4 (tek F 192.2). Süre (zincir başına toplam, hazırlık+iki hedef+değerlendirme; tohum ort.): tek F 16.4 s, V28 zinciri 21.1 s, V27 20.5 s, ortak ridge 19.9 s (değerlendirme ağırlıklı; zincir 4 aşama değerlendirdiği için daha uzun).

## 6b. Ana soruya cevap

- [Kesin] **X'i F₂–F₄'e yeniden vermek V27 kaybını büyük ölçüde giderdi (Y hedefi):** H=512 V27 191,5 → V28 180,7; H=1024 195,6 → 185,2 (3 tohum). V27'de ek aşamalar eğitim PPL'ini düşürüp doğrulama/testi bozuyordu; V28'de her aşama doğrulama ve testi de iyileştiriyor (H=512 test: 186,4 → 182,9 → 181,6 → 180,7).
- [Kesin] **Aynı toplam sütunla tek F (λ=V17 seçimi) geçildi:** H=512 tek F 182,5 → V28 zinciri 180,7 (−1,8), ortak ridge **176,5** (−6,0); H=1024 192,2 → 185,2 (−7,0), ortak ridge 183,4 (−8,8). Ortak tek ridge, ardışık artık çözümden daha iyi (çapraz ilişkiler önemli); V28 zincirin Top-1/Top-5 değeri tek F'nin biraz altında (Top-1 13,9 vs 14,8; Top-5 32,3 vs 33,3, H=512).
- [Kesin, **önemli uyarı**] Bu kazancın ne kadarının yapıdan, ne kadarının λ uyumsuzluğundan geldiği **ayrılmadı**: λ ve γ₁ softmax okuması için V17'de seçilmişti; V25/V26c'de aynı H=512 tek F, doğrusal okumada λ=100 ile (tohum 1) 171,6 PPL vermişti (bu çalışmadaki E tohum 1: 180,9; ortalama 176,5). Yani **ayarlanmış tek F, V28'in ortak ridge'inden daha iyi olabilir**; E ve zincir için λ taranmadı. Bu raporda "yapı tek F'yi yendi" sonucu yalnız **aynı λ** için geçerlidir.
- [Kesin] **%40–43 sınırı aşılmadı:** Q açıklanan varyans (test) en yüksek H=512 ortak ridge 0,357, H=1024 0,351; tek F 0,397 / 0,430. Bağlı zincir 0,311 / 0,320. F₃, F₄ ek açıklanan varyans: +0,014/+0,010 (H=512), +0,007/+0,006 (H=1024) — küçük ama sıfır değil (V27: +0,008/+0,001).
- [Muhtemel] X erişimi, sonraki aşamaların ham girdideki bilgiyi yeniden kullanmasını sağlıyor; kazancın bir kısmı ayrıca düşük γ₂ (0,5) ile daha az doygun tanh'tan gelebilir. Bunu ayıran kontrol yapılmadı.
- Hız: zincirler tek F'den yavaş (tohum ort. toplam 16,6 s vs 9,5 s H=512; çoğu 4 aşamalı değerlendirme), saf ağırlık üretimi 1–4 s/aşama. [Kesin: loglar]

## 7. Sonuç ve sınırlamalar

- [Kesin] Sonuçlar yukarıdaki tablolardadır; hiçbir sistem için "başarı" varsayılmadı. Karşılaştırma tek F ile aynı toplam sütun sayısında yapıldı (4×h/4 + sabit sütun).
- Ölçülen şey yeni veri değil, mevcut bağlam bilgisinin daha iyi temsil edilmesidir: F'ler aynı X'ten ve önceki F'lerden türetilir.
- γ₂ yalnız H=512 tohum 1'de doğrulamayla seçildi (ızgara {0.25,0.5,1,2}) ve diğer tüm koşulara taşındı; seçim ızgara kenarında olabilir (bölüm 4). Aşama başına ayrı λ/γ taraması yapılmadı. Ölçek yöntemi tek (satır-norm RMS); alternatif ölçeklendirme denenmedi.
- Q hedefli sonuçlar öğretici bilgisinin aktarımını ölçer; bağımsız öğrenme başarısı olarak okunmamalıdır.
- Süreler tek süreç içinde, ancak değerlendirme (4 aşamalı okuma + Q açıklanan varyans) dahil; saf ağırlık üretimi süreleri ilgili logların `asama sure` alanlarında (aşama başına 0,6–4 s).
- Yeni yapı ana mimariye eklenmedi.

## 8. Çalıştırma ve süre/RAM

Derleme: `g++ -O3 -march=native -fopenmp -std=c++17 cpp/tek_hamle_v28.cpp -o tek_hamle_v28`; komut: `./tek_hamle_v28 2 H TOHUM GAMMA1 LAMBDA GAMMA2 SCVJ` (S tek F, C V28 X-bağlantılı zincir, V V27 yeniden üretimi, J ortak ridge).

| komut | wall s | user CPU s | RAM MB |
|---|---|---|---|
| `./tek_hamle_v28 2 512 1 1 1 1 SCVJ` | 56.89 | 111.33 | 899 |
| `./tek_hamle_v28 2 512 1 1 1 2 SCVJ` | 56.27 | 111.07 | 899 |
| `./tek_hamle_v28 2 512 1 1 1 0.5 SCVJ` | 56.71 | 111.44 | 899 |
| `./tek_hamle_v28 2 512 1 1 1 0.25 SCVJ` | 57.78 | 114.72 | 899 |
| `./tek_hamle_v28 2 512 2 2 1 0.5 SCVJ` | 53.82 | 113.91 | 899 |
| `./tek_hamle_v28 2 512 3 1 1 0.5 SCVJ` | 55.46 | 111.94 | 899 |
| `./tek_hamle_v28 2 1024 1 4 10 0.5 SCVJ` | 76.39 | 202.03 | 1021 |
| `./tek_hamle_v28 2 1024 2 4 100 0.5 SCVJ` | 74.46 | 201.38 | 1021 |
| `./tek_hamle_v28 2 1024 3 2 1 0.5 SCVJ` | 83.45 | 206.85 | 1021 |

