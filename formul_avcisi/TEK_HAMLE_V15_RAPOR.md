# Tek hamle V15 raporu — adım adım ileri + yerel geri düzeltme

Branch: `claude/formul-avcisi`. V8–V14 dosyaları değişmedi (V15 yeni dosyada). 10 dk sınırında bitirildi; epok/Adam/RLS/tekrarlı optimizasyon yok.
Kod: `cpp/tek_hamle_v15.cpp`, `metin_kalibrasyon_v15.py` (V9 kalibrasyonunun V15 skor dizini kopyası). Ham loglar/komutlar: `v15_ham/v15log/`.

## Algoritma
Eğitim satırları sıralı bloklara bölündü (metnin blok sırası; bloklar arası karıştırma yok). Her blok t için tek ileri + tek geri düzeltme, yalnız önceki durum `W_{t−1}` ve yeni blok (geçmiş ham veri yeniden işlenmedi, S/R birikimi yok):
`E_t = T_t − F_t W_{t−1}`; `ΔW_t = (F_tᵀF_t + λI)⁻¹ F_tᵀE_t` (Cholesky); `W_t = W_{t−1} + ΔW_t`; `W_0 = 0`. (Bu, `‖T_t − F_tW‖² + λ‖W − W_{t−1}‖²` problemini her blokta tam çözmeye eşdeğer: önceki ağırlığa yakın durma cezalı, yalnız yeni bloğa uyum.)
Özellik: V12 `ref` (K=192), `W₁` çift-yön, sabit; H=256 ve 512; γ: H=512 için 4 (V12'nin seçtiği), H=256 için 1 (**ayrıca doğrulanmadı**). Blok boyutu B ∈ {1000, 4000, 8000} ve λ ∈ {10, 100, 1000} yalnız doğrulamada (mini kalibre CE: α∈{0,15..0,5}, T∈{0,005..0,05}); sonra V9 kalibrasyonu (aynı protokol) ile nihai ölçüm. Test modeli: eğitim bloklarından sonra doğrulama bloklarıyla aynı kuralla devam. Test yalnız sonda.

## Açık cevap
**Hayır — yerel düzeltmeli adım adım yöntem, V12 ridge'den daha iyi değil, daha kötü; eski blokları belirgin biçimde unutuyor.**

| Model | H | test CE | PPL | top-1 | top-5 | güncelleme/ağırlık üretimi |
|---|---|---|---|---|---|---|
| V12 ridge + V9 kalibrasyonu (referans) | 512 | 5,379 | **216,8** | 0,1492 | 0,2900 | ≈ 0,4 sn |
| **V15 yerel düzeltme** (B=8000, λ=1000) | 512 | 5,405 | **222,4** | 0,1488 | 0,2773 | 5 blok: ileri 1,7 + geri 2,6 = 4,3 sn |
| V10 ridge + V9 kalibrasyonu (referans) | 256 | 5,423 | 226,6 | 0,1406 | 0,2791 | ≈ 0,3 sn |
| **V15 yerel düzeltme** (B=8000, λ=10) | 256 | 5,436 | **229,5** | 0,1408 | 0,2689 | 5 blok: ileri 1,0 + geri 1,2 = 2,3 sn |

V12/V10 satırları önceki raporlardan (aynı veri/bölme, yeniden koşulmadı). Unigram PPL 281; trigram 137.
- [Kesin] V15, V12'ye göre H=512'de +0,026 nat (+5,6 PPL), H=256'da +0,013 nat (+2,9 PPL) kötü; top-5 daha düşük.
- [Kesin] **Eski bilgiyi bozuyor.** Seçilen ayarda ilk bloğun eğitim kaybı, ilk adımdan sonra 5,202 (H=512) ve 5,243 (H=256) iken son adımda 5,607 ve 5,647 (**+0,40 nat**; unigram CE 5,64 düzeyine geri dönüyor): her yeni blok önceki bloğun uyumunu siliyor. Doğrulama kaybı (sabit kalibrasyon α=0,25, T=0,01) H=512'de 5,520 → 5,511 → 5,508 → 5,499 → 5,483 (yavaş düşüş), H=256'da 5,512 → 5,533 → 5,525 → 5,539 → 5,505 (monoton değil).
- [Kesin] Seçimler her iki eksende de ızgara kenarında: B=8000 (en büyük), λ=1000 (H=512) / λ=10 (H=256). Büyük blok + büyük λ, yöntemi küresel ridge'e yaklaştırıyor (B=tüm veri, `W₀=0` → V12 A formülü). Yani doğrulamanın tercihi "yerel düzeltmeyi azalt" yönünde; daha büyük B denenmedi (zaman).

## Adım adım kayıp (seçilen ayar, sabit kalibrasyon, 5 adım)
| adım | H=512 doğr. CE | H=512 blok-1 CE | H=256 doğr. CE | H=256 blok-1 CE |
|---|---|---|---|---|
| 1 | 5,5196 | 5,2022 | 5,5122 | 5,2426 |
| 2 | 5,5106 | 5,4779 | 5,5326 | 5,5779 |
| 3 | 5,5075 | 5,5596 | 5,5249 | 5,6101 |
| 4 | 5,4990 | 5,5912 | 5,5390 | 5,6393 |
| 5 | 5,4829 | 5,6067 | 5,5048 | 5,6469 |

## Süre ve RAM
Blok başına (B=8000): ileri `F_tW` + hata ≈ 0,35 sn (H=512) / 0,21 sn (H=256); geri düzeltme (`F_tᵀE`, `F_tᵀF_t`, Cholesky, çözüm) ≈ 0,52 / 0,24 sn. Bir geçiş (5 blok) toplam 4,3 sn (H=512) / 2,3 sn (H=256) — V12'nin ≈ 0,4 sn'sinin ~10 katı, çünkü her blokta m×m Cholesky + V=2001 sağ taraflı çözüm ve `F_tW` çarpımı gerekiyor; birikimli S/R yönteminde bunlar bir kez yapılıyor. Tüm koşu (9 ayar ızgarası + izleme + test modeli): 66,7 sn (H=512), 35,5 sn (H=256); tepe RAM 345 / 300 MB.

## Sınırlar
- Tek tohum, bloklar sıralı (metin içi sıra, karıştırma yok); seçim ızgarası kenarda; γ H=256 için doğrulanmadı.
- "Eski bloklarda unutma" yalnız ilk blok üzerinde ölçüldü (blok-1 eğitim CE).
- Önceki V12 sayıları yeniden koşulmadı.

## Komutlar (gerçek)
Derleme: `g++ -O3 -march=native -fopenmp -o tek_hamle_v15 tek_hamle_v15.cpp`. Çalıştırma: `./tek_hamle_v15 2 512 1 4`, `./tek_hamle_v15 2 256 1 1`; kalibrasyon: `python3 -I metin_kalibrasyon_v15.py cift_loc 2 H 1`. Ölçüm (`manifest.tsv`): H=512: 68,1 sn duvar, 345 MB; H=256: 35,6 sn, 300 MB.

## Ham çıktı (H=512, seçim ve adım izleme)
```
V15 c=2 H=512 tohum=1 gamma=4 V=2001 egitim=39840 dogrulama=4980 test=4980 (4 cekirdek)
  B=1000 lambda=10 ileri=1.64s geri_duzeltme=4.53s  val_CE(ham a=1,T=0.02)=11.8374  val_CE(mini-kalibre)=5.6448
  B=1000 lambda=100 ileri=1.71s geri_duzeltme=4.62s  val_CE(ham a=1,T=0.02)=8.0000  val_CE(mini-kalibre)=5.5624
  B=1000 lambda=1000 ileri=1.67s geri_duzeltme=4.85s  val_CE(ham a=1,T=0.02)=6.7151  val_CE(mini-kalibre)=5.5232
  B=4000 lambda=10 ileri=1.52s geri_duzeltme=2.63s  val_CE(ham a=1,T=0.02)=7.7669  val_CE(mini-kalibre)=5.5496
  B=4000 lambda=100 ileri=1.51s geri_duzeltme=2.57s  val_CE(ham a=1,T=0.02)=6.9951  val_CE(mini-kalibre)=5.5127
  B=4000 lambda=1000 ileri=1.52s geri_duzeltme=2.52s  val_CE(ham a=1,T=0.02)=6.5007  val_CE(mini-kalibre)=5.4943
  B=8000 lambda=10 ileri=1.72s geri_duzeltme=2.65s  val_CE(ham a=1,T=0.02)=7.0838  val_CE(mini-kalibre)=5.5144
  B=8000 lambda=100 ileri=1.67s geri_duzeltme=2.34s  val_CE(ham a=1,T=0.02)=6.7850  val_CE(mini-kalibre)=5.4931
  B=8000 lambda=1000 ileri=1.79s geri_duzeltme=2.58s  val_CE(ham a=1,T=0.02)=6.4690  val_CE(mini-kalibre)=5.4829
SECILEN B=8000 lambda=1000 val_CE(mini-kalibre)=5.4829
ADIM_IZLEME (sabit kalibrasyon a=0.25,T=0.01): adim | dogrulama CE | blok-1 (ilk blok) egitim CE
  adim 1: val_CE=5.5196 blok1_CE=5.2022
  adim 2: val_CE=5.5106 blok1_CE=5.4779
  adim 3: val_CE=5.5075 blok1_CE=5.5596
  adim 4: val_CE=5.4990 blok1_CE=5.5912
  adim 5: val_CE=5.4829 blok1_CE=5.6067
UNUTMA blok-1: adim1 sonrasi 5.2022 -> son 5.6067 (fark +0.4045)
SURE egitim-yalniz: ileri_hesap_toplam=1.729s geri_duzeltme_toplam=2.590s (5 adim, blok basina ileri 0.346s geri 0.518s)
TOPLAM sure=66.7s tepe_RAM=345MB
```
