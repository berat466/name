# Tek hamle V11 raporu — kelime kimliği ve sıralı ilişkiler

Branch: `claude/formul-avcisi`. Referans: `TEK_HAMLE_V10_RAPOR.md`. Eski kodlar değişmedi (V11 yeni dosyalarda). Kural: gradyan/Adam/backprop/epok/RLS yok; tek çözüm `W2=(FᵀF+λI)⁻¹FᵀT`; W1 veri çiftlerinden. 20 dakika sınırı içinde.
Kod: `cpp/tek_hamle_v11.cpp`, `metin_kalibrasyon_v11.py` (V9 kalibrasyonu), `v11_ham/` (ham loglar). Veri/bölme V8/V10 ile aynı (Pride and Prejudice ilk 50 000 token; 80/10/10 blok); bağlam kimlikleri (`c2_*_ctx.i32`) V8 ile aynı konumlardan türetildi.

## Açık cevap
**Kelime kimliği ve sıralı ilişkiler tek hamle ağırlıkların dil öğrenme kapasitesini gerçekten artırdı mı?** — **Hayır, anlamlı biçimde değil.** Kayıp değişimi çok küçük: H=1024'te kimlik torbası −0,006 nat (PPL −1,4 ≈ %0,6; 3 tohumda aynı yön ve büyüklük), H=512'de tek tohumla −0,023 nat (PPL −5,1 ≈ %2,3). Sıralı kimlik ve kelime çifti H=1024'te referanstan **daha kötü** (+0,006 … +0,010 nat). 137'ye (trigram) yaklaşılamadı, 217'nin (V10) belirgin altına da inilemedi.
- [Kesin] En iyi doğrulama CE'si: **H=1024 + kimlik torbası (sırasız one-hot)**. 3 tohumda PPL ortalaması **216.9** vs aynı H'de V10 referansı **218.3** (tohum başına fark ref−bag: +1.4 / +1.4 / +1.6 PPL puanı).
- [Kesin] **Sıralı kimlik (konuma özel bloklar) ve kelime çifti, sırasız kimlik torbasından iyi değil** (aşağıdaki tablo): sıra bilgisini ayrı bloklara koymak ek bir kazanç sağlamadı.
- [Kesin] Hedef: 217'nin altı → yalnız çok az (216,9), 137 → hayır.
- Kalibrasyon V9'daki gibi (`P=(1−α)P_unigram+α softmax(z/T)`, α,T yalnız doğrulamadan). **Gerçek tahmin iyileşmesi** için kalibrasyondan bağımsız ölçüler de verildi: ham skor top-1/top-5 ve ham skor MSE'si.

## Yöntemler (hepsi: W1 = PairDirections(X'), F=[1,tanh(γ(X'W1+b))], W2 ridge)
Yoğun giriş (hepsinde): `[e_{t−2}, e_{t−1}, D]`, `D = e_{t−1} − e_{t−2}` (64'er boyut, K=192; V10 "fark"). Seyrek kimlik blokları (yoğun one-hot **oluşturulmadı**: örnek başına en çok 3 seyrek satır indisi, `W1` satırları toplanır):
- `ref`: kimlik yok (V10 referansı).
- `bag`: iki önceki kelimenin kimliği tek 2001'lik blokta (sırasız).
- `seq`: t−1 ve t−2 için ayrı 2001'lik bloklar (konuma özel).
- `pair`: (id_{t−2}, id_{t−1}) çiftinin hash'i, 4096 kova.
- `seqpair`: seq + pair.
`W1` sütunu: hedefleri farklı iki eğitim satırının X' farkı (yoğun fark + seyrek ±1), toplam normla bölünür (birim norm); bias `N(0,σ_b²)`, σ_b = çiftlerin orta-nokta bias'larının std'si. **Ölçek kontrolü**: her blok kabaca birim normlu (gömme birim norm, one-hot norm 1, D norm ≈ 1,4); ayrı bir blok ölçek ayarı yapılmadı (zaman). H = 512 ve 1024.
Hiperparametre ızgarası (yalnız doğrulama CE): γ ∈ {1, 2}, λ ∈ {1, 10, 100, 1000}, sıcaklık/α/T V9 ile aynı. **Not:** V10'daki ızgara γ ∈ {0,5, 1, 2, 4} idi; zaman için {1,2}'ye kısaltıldı. Bu yüzden V11'in `ref` satırı (H=512: PPL 223,2) V10'un `fark` sonucundan (216,8) farklı: V10 muhtemelen γ=0,5 seçmişti [Muhtemel; ayrıca doğrulanmadı]. **Bu rapordaki tüm karşılaştırmalar V11 içinde aynı ızgarayla yapıldı**; V10 sayısıyla doğrudan kıyas yanıltıcı olur.

## Tohum 1 — tüm yöntemler
| H | özellik | **PPL (kalibre)** | test CE | doğr. CE | top-1 | top-5 | ham skor PPL (α=1) | ham skor MSE | V10-ref'e göre CE değişimi (aynı H) | ağırlık üretimi sn | toplam sn (ayar dahil) | tepe RAM MB |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 512 | V10 referansı (gömme + fark) | **223.2** | 5.408 | 5.5008 | 0.1430 | 0.2821 | 533.8 | 0.000482 | +0.000 | 0.64 | 19.6 | 299 |
| 512 | + kimlik torbası (sırasız one-hot, 2001) | **218.1** | 5.385 | 5.4772 | 0.1482 | 0.2859 | 512.2 | 0.000481 | -0.023 | 0.73 | 18.9 | 300 |
| 512 | + sıralı kimlik (t−1 ve t−2 ayrı one-hot blokları) | **219.6** | 5.392 | 5.4826 | 0.1482 | 0.2859 | 522.8 | 0.000481 | -0.016 | 0.62 | 18.7 | 304 |
| 512 | + kelime çifti (hash 4096 kova) | **225.3** | 5.418 | 5.5092 | 0.1446 | 0.2797 | 540.1 | 0.000483 | +0.009 | 0.56 | 18.6 | 305 |
| 512 | + sıralı kimlik + kelime çifti | **219.7** | 5.392 | 5.4822 | 0.1504 | 0.2845 | 521.1 | 0.000481 | -0.016 | 0.73 | 19.2 | 312 |
| 1024 | V10 referansı (gömme + fark) | **218.3** | 5.386 | 5.4760 | 0.1492 | 0.2886 | 527.0 | 0.000481 | +0.000 | 1.96 | 37.4 | 561 |
| 1024 | + kimlik torbası (sırasız one-hot, 2001) | **216.9** | 5.379 | 5.4679 | 0.1508 | 0.2898 | 513.9 | 0.000480 | -0.006 | 1.64 | 35.8 | 562 |
| 1024 | + sıralı kimlik (t−1 ve t−2 ayrı one-hot blokları) | **219.6** | 5.392 | 5.4821 | 0.1466 | 0.2873 | 529.2 | 0.000481 | +0.006 | 1.62 | 35.8 | 570 |
| 1024 | + kelime çifti (hash 4096 kova) | **220.3** | 5.395 | 5.4850 | 0.1474 | 0.2863 | 529.7 | 0.000482 | +0.009 | 1.75 | 37.0 | 570 |
| 1024 | + sıralı kimlik + kelime çifti | **220.5** | 5.396 | 5.4856 | 0.1474 | 0.2851 | 529.8 | 0.000481 | +0.010 | 1.75 | 37.5 | 585 |

(Referanslar: unigram PPL 281,1; trigram 137,4; V10 raporu: H=512 fark 216,8–217,0. "Ham skor PPL (α=1)" = kalibrasyonsuz saf softmax; "ham skor MSE" = `mean((z−onehot)²)`, hep-sıfır tabanı 0,000500.)

## Tohum doğrulaması (en iyi yöntem ve referans, H=1024, 3 tohum)
| özellik (H=1024) | PPL tohum 1 / 2 / 3 | PPL ort. ± std | test CE ort. | top-1 ort. | top-5 ort. | doğr. CE ort. | α / T |
|---|---|---|---|---|---|---|---|
| ref | 218.3 / 218.3 / 218.4 | 218.3 ± 0.0 | 5.386 | 0.1505 | 0.2903 | 5.4785 | 0.25/0.01, 0.25/0.01, 0.25/0.01 |
| bag | 216.9 / 216.9 / 216.8 | 216.9 ± 0.1 | 5.379 | 0.1519 | 0.2897 | 5.4687 | 0.2/0.01, 0.2/0.01, 0.2/0.01 |

## Okuma
1. [Kesin] Kimlik torbası (bag) doğrulama CE'sini en çok düşüren yöntem (H=1024: 5,4679 vs ref 5,4760); 3 tohumda PPL 216,9 vs 218,3 (fark 1,4–1,6, tohum std'sinden büyük ama pratikte küçük). Büyük bir sıçrama yok. H=512'de bag −5,1 PPL, ama o `ref` satırı kısaltılmış γ ızgarasıyla (üstteki not) zayıf kaldığı için kazanç şişmiş olabilir [Muhtemel].
2. [Kesin] Ham (kalibrasyonsuz) skorların kalitesi: top-1 %14,1–15,0, top-5 %32,9–34,7 (V10'da %13,8–15,0 / %32,6–33,8): aynı H'de referansa göre kimlik özellikleri (bag) sıralama yeteneğini top-1'de ≈ 0,0–0,5, top-5'te ≈ 0,7–1,2 puan artırdı; bu, perplexity'deki küçük kazançla uyumlu. Yani değişim kalibrasyondan değil, ham skorlardan da (kısmen) geliyor ama çok küçük.
3. [Kesin] H=512 → 1024: referansta 223,2 → 218,3 (daha çok özellik yardım etti); kimlikli yöntemlerde kazanç daha az. [Muhtemel] Kimlik bilgisi (2001 sınıf, 8–16K seyrek boyut) 512–1024 rastgele yön ile ancak zayıf kodlanıyor: her yön birkaç kelimenin kimliğine bağlı, tek bir yön bir kelimeyi çözmüyor.
4. [Kesin] 137'ye yakınlık yok: ≈ 217 vs 137. [Muhtemel] Trigram, bağlamın tam kimliğini sayılarla saklıyor; tek hamle modelinde kimlik, ≈ 1000 rastgele hiperdüzlemden geçen yumuşak bir kodlama.
5. Süre/RAM: ağırlık üretimi H=512'de ≈ 0,6–0,7 sn, H=1024'te ≈ 1,6–2,0 sn; ayar dahil 19 / 36–37 sn (γ ızgarası yarıya kısaldığı için V10'dan düşük). Tepe RAM 299–312 MB (H=512), 561–585 MB (H=1024); seyrek bloklar ek RAM getirmedi (W1 `KT×H`: ≈ 8,2K × 1024 × 4 B ≈ 33 MB). Veri hazırlama (bağlam kimlik dosyaları) birkaç saniye (Python), toplam süreye dahil değil.

## Sınırlar
- Tek metin (50 000 token), test 4 980 örnek; yalnız tek bir kimlik ölçek ayarı; γ ızgarası kısaltıldı (yukarıdaki not).
- 3 tohum yalnız en iyi yöntem + referans, H=1024; öbür yöntemler tek tohum.
- H=2048 ve 4 kelimelik bağlam denenmedi (süre).
- Test yalnız sonda; hiçbir hiperparametre testle seçilmedi.

## Komutlar (gerçek)
Derleme: `g++ -O3 -march=native -fopenmp -o tek_hamle_v11 tek_hamle_v11.cpp`. Çalıştırma: `./tek_hamle_v11 cift 2 H TOHUM OZELLIK` (OZELLIK ∈ ref|bag|seq|pair|seqpair); kalibrasyon: `python3 -I metin_kalibrasyon_v11.py cift_OZELLIK 2 H TOHUM`. Ölçülen süreler/RAM (`v11_ham/v11log/manifest.tsv`):

| komut | duvar sn | kullanıcı CPU sn | tepe RAM MB |
|---|---|---|---|
| `./tek_hamle_v11 cift 2 512 1 seq` | 18.77 | 64.95 | 304 |
| `./tek_hamle_v11 cift 2 512 1 ref` | 19.66 | 67.80 | 299 |
| `./tek_hamle_v11 cift 2 512 1 bag` | 19.01 | 65.89 | 300 |
| `./tek_hamle_v11 cift 2 512 1 pair` | 18.69 | 64.47 | 305 |
| `./tek_hamle_v11 cift 2 512 1 seqpair` | 19.31 | 67.09 | 312 |
| `./tek_hamle_v11 cift 2 1024 1 ref` | 37.44 | 130.89 | 561 |
| `./tek_hamle_v11 cift 2 1024 1 bag` | 35.83 | 126.05 | 562 |
| `./tek_hamle_v11 cift 2 1024 1 seq` | 35.89 | 127.14 | 570 |
| `./tek_hamle_v11 cift 2 1024 1 pair` | 37.02 | 130.51 | 570 |
| `./tek_hamle_v11 cift 2 1024 1 seqpair` | 37.55 | 132.61 | 585 |
| `./tek_hamle_v11 cift 2 1024 2 bag` | 35.88 | 127.07 | 561 |
| `./tek_hamle_v11 cift 2 1024 2 ref` | 38.07 | 134.05 | 561 |
| `./tek_hamle_v11 cift 2 1024 3 bag` | 35.87 | 126.71 | 562 |
| `./tek_hamle_v11 cift 2 1024 3 ref` | 35.95 | 128.11 | 561 |

## Ham terminal çıktısı örneği (`metin_kalibrasyon_v11.py cift_bag 2 1024 1`)
```
OZET_V9|unigram|mod=cift_bag|c=2|H=1024|tohum=1|test_CE=5.6389|PPL=281.15|top1=0.0904|top5=0.2255|mse_olasilik=0.000492
OZET_V9|V8_alpha1_V8_T_izgarasi|mod=cift_bag|c=2|H=1024|tohum=1|alpha=1.0|T=0.02|val_CE=6.3495|test_CE=6.2421|PPL=513.92|top1=0.1502|top5=0.3468|mse_olasilik=0.000502|olasilik_toplami=1.000000
OZET_V9|V9_sartname_izgarasi|mod=cift_bag|c=2|H=1024|tohum=1|alpha=0|T=0.25|val_CE=5.6947|test_CE=5.6389|PPL=281.15|top1=0.0904|top5=0.2255|mse_olasilik=0.000492|olasilik_toplami=1.000000
OZET_V9|V9_genisletilmis_izgara|mod=cift_bag|c=2|H=1024|tohum=1|alpha=0.2|T=0.01|val_CE=5.4679|test_CE=5.3794|PPL=216.89|top1=0.1508|top5=0.2898|mse_olasilik=0.000482|olasilik_toplami=1.000000
CPU_SURE|cift_bag_c2_H1024_s1|kalibrasyon_cpu_sn=3.15|duvar_sn=2.98|RAM_MB=341
```
