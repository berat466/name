# Tek hamle V13 raporu — tek geri yankı ile W1 düzeltme

Branch: `claude/formul-avcisi`. Referans: `TEK_HAMLE_V12_RAPOR.md`. V8–V12 dosyaları değişmedi (V13 yeni dosyada). Süre: 10 dk sınırı içinde.
Kod: `cpp/tek_hamle_v13.cpp`, `metin_kalibrasyon_v13.py` (V9 kalibrasyonunun V13 skor dizini kopyası). Ham loglar: `v13_ham/v13log/`.
Kurulum: aynı 50 000 token, aynı bölme; H=512, tohum 1; V12'nin en iyi (γ,λ) = (4, 1000) (özellik `ref`, formül A). Eğitim/doğrulama/test: 39 840 / 4 980 / 4 980 örnek.

## Algoritma (tek geri geçiş; döngü yok)
`F=tanh(γXW₁+b)`; `W₂=(FᵀF+λI)⁻¹FᵀT` (Cholesky). `E=FW₂−T`; `G₁=γ·Xᵀ[(E·W₂,hᵀ)⊙(1−F_h²)]` (F_h, W₂,h = bias hariç). `W₁'=W₁−ηG₁`, `F'=tanh(γXW₁'+b)`, `W₂'=(F'ᵀF'+λI)⁻¹F'ᵀT`. Gradyan ölçeği: `η = r·‖W₁‖_F/‖G₁‖_F`, r ∈ {0 (referans), 0,01, 0,03, 0,10} (r = göreli adım). G₁ yalnız bir kez, ilk (eğitim-yalnız) W₂'den hesaplandı; her aday için W₂' tek Cholesky çözümüyle yeniden üretildi. Adam/RLS/çok adım yok.
Seçim ve kalibrasyon: V9 kalibrasyonu (α,T, genişletilmiş ızgara) yalnız doğrulamada; aday seçimi de **kalibre doğrulama CE'si** ile; test yalnız sonda.

## Sonuçlar (tohum 1)
| aday | göreli adım ‖ΔW1‖/‖W1‖ | α=1 ham skor PPL | doğr. CE (kalibre, seçim ölçütü) | **test CE** | **test PPL** | top-1 | top-5 | test CE farkı (η=0'a göre) |
|---|---|---|---|---|---|---|---|---|
| e0 | 0 (referans) | 521.1 | 5.4671 | 5.3791 | 216.82 | 0.1492 | 0.2900 | +0.0000 |
| e1 | 0,01 | 527.6 | 5.4666 | 5.3789 | 216.79 | 0.1494 | 0.2902 | -0.0002 |
| e2 | 0,03 | 541.1 | 5.4658 | 5.3805 | 217.14 | 0.1500 | 0.2861 | +0.0014 |
| e3 | 0,10 | 576.5 | 5.4645 | 5.3793 | 216.86 | 0.1534 | 0.2886 | +0.0002 |

(Referanslar: V12 A: PPL 216,8; unigram 281,1; trigram 137,4. η=0 satırı V12'nin sonucunu birebir tekrarlıyor: PPL 216,82, test CE 5,3791.)
Doğrulama CE'sine göre seçilen aday: **e3** (r=0,10). Test CE farkı: +0.0002 nat.

## Açık cevap
**Tek geri yankı, klasik uzun eğitim döngüsüne girmeden loss'u anlamlı biçimde düşürmedi.** Test CE 5,3791 → 5,3789 / 5,3805 / 5,3793 (η=0 → r=0,01 / 0,03 / 0,10): fark ≤ 0,0014 nat, işaret karışık; PPL 216,8–217,1. Doğrulama CE'si adım büyüdükçe çok küçük düştü (5,4671 → 5,4645, −0,0026) ama bu test kaybına yansımadı.
- [Kesin] **Ham (kalibrasyonsuz) skorlar kötüleşti**: α=1 PPL 521 → 528 → 541 → 577 (adım büyüdükçe). Düzeltme W₂'yi çıktıdan uzaklaştırıyor: skorların ölçeği/öncülü bozuluyor, V9 kalibrasyonu bunu telafi ediyor.
- [Kesin] **Sıralama yeteneği çok küçük arttı**: ham top-1 %15,02 → %15,44 (r=0,10), top-5 %33,71 → %34,36; kalibre top-1 %14,92 → %15,34, top-5 %29,00 → %28,86. top-1'deki +0,4 puan ≈ 20 örnek (test 4 980 örnek, standart hata ≈ ±0,5 puan): istatistiksel olarak ayırt edilemez, tek tohum.
- [Muhtemel] Neden etkisiz: tek bir geri geçişten gelen küçük adım (‖ΔW₁‖ ≤ %10) F'yi az değiştiriyor; W₂ zaten yeniden çözüldüğü için kayıp neredeyse aynı kalıyor. Büyük adım (r=0,10) ham skorları bozuyor ama kalibre kayıp değişmiyor. Daha büyük adımlar denenmedi.

## Süre ve RAM (H=512, tek aday için)
- İlk W₁ üretimi ≈ 0,001 sn; ilk W₂ (özellik + istatistik + Cholesky) ≈ 0,55–0,65 sn (V12: ≈ 0,4 sn; ölçüm farkı: burada özellik hesabı ve eğitim-yalnız çözüm dahil, ayar araması hariç).
- **Geri yankı ≈ 2,7–3,0 sn** (n·m·V ≈ 4×10¹⁰ çarpma-toplama iki kez: `F·W₂` ve `E·W₂,hᵀ`).
- İkinci W₂ (eğitim-yalnız) ≈ 0,6–1,1 sn.
- **Toplam ağırlık üretimi (W₁ + ilk W₂ + geri + ikinci W₂) ≈ 4,0–4,8 sn**, V12'nin ≈ 0,4–0,6 sn'sinin yaklaşık 8–10 katı; kazanç yok → hızlanma yok.
- W₁ değişimi: göreli ‖ΔW₁‖/‖W₁‖ = 0,0100 / 0,0300 / 0,1000 (tasarım gereği); ‖G₁‖ = 79,0, ‖W₁(yoğun)‖ = 22,6.
- Tepe RAM: 380 MB (süreç; 4 aday için skor dökümleri dahil); koşunun toplamı ≈ 12 sn (4 aday + dökümler).

## Sınırlar
- Tek tohum, H=512, tek (γ,λ); yalnız üç adım büyüklüğü; G₁ yalnız ilk modelden.
- Gradyan, 1/n ölçeklemesiz toplam üzerinden (adım gradyan normuna göre ölçeklendiği için önemsiz); `tanh` türevi γ ile (formüle uygun). Özellikler bu deneyde `ref` (yoğun gömme + fark), kimlik özelliği yok.
- Gradyan doğruluğu türevle (sonlu fark) ayrıca doğrulanmadı.
- Test yalnız sonda; hiçbir seçim testle yapılmadı.

## Komutlar (gerçek)
Derleme: `g++ -O3 -march=native -fopenmp -o tek_hamle_v13 tek_hamle_v13.cpp`. Çalıştırma: `./tek_hamle_v13 cift 2 512 1 4 1000`; kalibrasyon: `python3 -I metin_kalibrasyon_v13.py cift_eE 2 512 1` (E=0..3). Ölçüm (`manifest.tsv`): `./tek_hamle_v13 cift 2 512 1 4 1000` → wall_s=9.21, tepe RAM 380 MB.

## Ham çıktı (`v13_c2_H512_s1.txt`)
```
V13 c=2 H=512 tohum=1 gamma=4 lambda=1000 V=2001 K=192 egitim=39840 dogrulama=4980 test=4980 (4 cekirdek)
ilk hesap (F+W2) = 0.551s ; geri yanki = 2.673s ; ||G1||=79.04 ||W1_yogun||=22.63
ADAY e0 goreli_adim=0.00 eta=0 dW1_goreli(gercek)=0.0000 ikinci_egitim_W2(train)=1.116s son_model(train+val)=0.250s
ADAY e1 goreli_adim=0.01 eta=0.002863 dW1_goreli(gercek)=0.0100 ikinci_egitim_W2(train)=0.595s son_model(train+val)=0.139s
ADAY e2 goreli_adim=0.03 eta=0.008588 dW1_goreli(gercek)=0.0300 ikinci_egitim_W2(train)=0.857s son_model(train+val)=0.295s
ADAY e3 goreli_adim=0.10 eta=0.02863 dW1_goreli(gercek)=0.1000 ikinci_egitim_W2(train)=0.737s son_model(train+val)=0.100s
AGIRLIK_URETIM (W1 + ilk W2 + geri yanki + ikinci W2, egitim-yalniz, tek aday): W1=0.001s ilk=0.551s geri=2.673s ikinci~0.551s
TOPLAM sure=9.2s tepe_RAM=380MB
```
