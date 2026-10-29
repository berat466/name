# Tek hamle V12 raporu — ağırlık üretim formülünü geliştirme (örnek ağırlığı, özellik bazlı düzenlileştirme)

Branch: `claude/formul-avcisi`. Referans: `TEK_HAMLE_V11_RAPOR.md`. Eski kodlar değişmedi (V12 yeni dosyalarda). Kural: gradyan/Adam/backprop/epok yok; tek Cholesky çözümü; ters matris doğrudan hesaplanmadı. Yeni nöron/özellik **eklenmedi**: aynı W1 (veri çiftleri) ve aynı F, yalnız W2 formülü değişti.
Kod: `cpp/tek_hamle_v12.cpp`, `metin_kalibrasyon_v12.py` (V9 kalibrasyonu, tüm formüllere eşit). Veri/bölme V8–V11 ile aynı (Pride and Prejudice ilk 50 000 token; 80/10/10 blok). Ham loglar: `v12_ham/`. 20 dakika sınırı: H=1024 koşusu tamamlandı (aşağıda).

## Açık cevap
**Doğrudan ağırlık hesaplama matematiğini değiştirmek loss'u düşürmedi.** Kalibre perplexity'de ne B (örnek ağırlıklı) ne C (özellik bazlı düzenlileştirme) V10'un ridge'ini (A) geçti.
- [Kesin] **A (referans)**: H=512'de PPL ≈ 217 (3 tohum ort. 217.0) — V10'un sonucu tam γ ızgarasıyla yeniden doğrulandı (V10: 216,8–217,0).
- [Kesin] **B (örnek ağırlıklı ridge)**: ham skorların PPL'ini 521 → 414'e indirdi (kalibrasyonsuz iyileşme) ama V9 kalibrasyonundan sonra **daha kötü**: 3 tohum ort. **228.6** vs A'nın 217.0'ı; top-1 %15,0 → %14,1, top-5 %28,9 → %26,8 (3 tohum ort.).
- [Kesin] **C (varyansa göre Λ)**: A ile pratikte aynı. ref H=512: 217,0 vs 216,8 (+0,2); bag H=512: 222,5 vs 223,1 (−0,6); bag H=1024: 219,3 vs 220,1 (−0,8). Son ikisi tek tohum ve <1 PPL; tohumla doğrulanmadı, kazanç iddiası yok.
- [Kesin] **V11 sonucu tam ızgarada tekrarlanmadı**: V11 (γ∈{1,2}) kimlik torbasını H=512'de 218,1 vs ref 223,2 buldu; tam ızgarada (γ∈{0,5…4}) ref 216,8, bag 223,1 → bag **daha kötü** (H=1024 bag: 220,1; ref H=1024 koşulmadı). [Muhtemel] neden: (γ,λ) seçimi, V8 kuralı gereği **kalibrasyondan önceki** doğrulama CE'sine (α=1) göre yapılıyor; tam ızgara bag için kalibrasyondan sonra daha kötü bir (γ=0,5) seçti. Yani V11'deki kazanç ızgara/seçim kuralı artefaktı olabilir; bu mekanizmayı ayrıca doğrulamadım (kalibrasyon sonrası CE'ye göre seçim yapılmadı).
- Trigram (137) hedefi: ulaşılamadı (en iyi ≈ 217).

## Formüller
- **A**: `W=(FᵀF+λI)⁻¹FᵀT` (256 satırlık FP64 birikimli `FᵀF`, Cholesky).
- **B**: `W=(FᵀDF+λI)⁻¹FᵀDT`, `D=diag(w_i)`, `w_i = clip(norm((n_y/ort_n)^−0,5), 0,25 ; 4)`: n_y eğitimdeki hedef token sayısı; ortalama 1'e normalize, [0,25 ; 4] aralığında sınırlı. Gözlenen ort 0,995, min 0,25, maks 4,0. Doğrulama satırları aynı fonksiyonla ağırlıklandı (`FᵀDF` toplanabilir). Üssü (−0,5) ve sınırı ayarlamadım (zaman).
- **C**: `W=(FᵀF+Λ)⁻¹FᵀT`, `Λ_jj = λ·v_j/ort(v)`, `v_j` = özelliğin eğitim varyansı (`FᵀF`'in köşegen/ortalamasından), bias için `Λ_00 = 10⁻³`. Bu, standardize özellik uzayında düz ridge'e eşdeğer.
- Tam ızgara: γ ∈ {0,5; 1; 2; 4}, λ ∈ {1, 10, 100, 1000}, sıcaklık T 9 değer (V8); kalibrasyon α,T V9 genişletilmiş ızgara (yalnız doğrulama CE); test sonda.

## Sonuçlar — tohum 1
| özellik | H | formül | α=1 ham skor PPL | **kalibre PPL** | test CE | doğr. CE | top-1 | top-5 | seçilen γ / λ | ağırlık üretimi sn |
|---|---|---|---|---|---|---|---|---|---|---|
| ref | 512 | A ridge (referans) | 521.1 | **216.8** | 5.379 | 5.4671 | 0.1492 | 0.2900 | 4 / 1000 | 0.37 |
| ref | 512 | B örnek-ağırlıklı ridge (w∝freq^-0,5, [0,25;4]) | 414.0 | **228.8** | 5.433 | 5.5103 | 0.1398 | 0.2673 | 1 / 1000 | 1.20 |
| ref | 512 | C özellik-varyansına göre Λ_jj=λ·v_j/ort(v) | 520.6 | **217.0** | 5.380 | 5.4661 | 0.1508 | 0.2853 | 4 / 1000 | 0.36 |
| bag | 512 | A ridge (referans) | 517.1 | **223.1** | 5.408 | 5.4963 | 0.1446 | 0.2783 | 0.5 / 100 | 0.35 |
| bag | 512 | B örnek-ağırlıklı ridge (w∝freq^-0,5, [0,25;4]) | 433.6 | **225.1** | 5.416 | 5.4863 | 0.1434 | 0.2763 | 1 / 1000 | 1.12 |
| bag | 512 | C özellik-varyansına göre Λ_jj=λ·v_j/ort(v) | 513.4 | **222.5** | 5.405 | 5.4928 | 0.1444 | 0.2799 | 0.5 / 100 | 0.36 |
| bag | 1024 | A ridge (referans) | 504.1 | **220.1** | 5.394 | 5.4828 | 0.1488 | 0.2839 | 0.5 / 100 | 0.93 |
| bag | 1024 | B örnek-ağırlıklı ridge (w∝freq^-0,5, [0,25;4]) | 437.6 | **232.8** | 5.450 | 5.5233 | 0.1349 | 0.2681 | 0.5 / 1000 | 2.53 |
| bag | 1024 | C özellik-varyansına göre Λ_jj=λ·v_j/ort(v) | 503.9 | **219.3** | 5.391 | 5.4787 | 0.1484 | 0.2851 | 0.5 / 100 | 1.00 |

(Referanslar: unigram PPL 281,1; trigram 137,4. "ham skor PPL" = kalibrasyonsuz softmax(z/T).)

## Tohum doğrulaması (ref, H=512): A vs B
| formül (ref, H=512) | PPL tohum 1 / 2 / 3 | PPL ort. ± std | test CE ort. | top-1 ort. | top-5 ort. | doğr. CE ort. |
|---|---|---|---|---|---|---|
| A ridge (referans) | 216.8 / 216.9 / 217.3 | 217.0 ± 0.2 | 5.380 | 0.1497 | 0.2893 | 5.4732 |
| B örnek-ağırlıklı ridge (w∝freq^-0,5, [0,25;4]) | 228.8 / 228.2 / 228.8 | 228.6 ± 0.3 | 5.432 | 0.1408 | 0.2684 | 5.5089 |

## Neden başarısız oldu (ya da başarılı olmadı)
1. **B** — [Kesin] Ham skorlar daha iyi kalibre (PPL 521→414) çünkü sık tokenlar (the, to, of …) ridge çözümünde baskın; ağırlıklı çözüm skor dağılımını daha düz yapıyor, yani B'nin "kazancı" çoğunlukla unigram benzeri öncül düzeltmesi. V9 kalibrasyonu (unigram karışımı) bu öncülü zaten ekliyor, bu yüzden kazanç kalibrasyondan sonra kayboluyor [Muhtemel; mekanizma ayrıca ölçülmedi]. Ayrıca sık tokenlara düşen ağırlığın azalması, top-1/top-5'i düşürdü (sık tokenların doğru tahmini zayıfladı): ham gerçek tahmin kalitesi arttı **değil**, azaldı.
2. **C** — [Kesin] Varyansa göre λ, A'dan en çok ≈ 1 PPL farklı (yön karışık). [Muhtemel] Neden: `tanh` özelliklerinin varyansları birbirine yakın (aynı γ, aynı giriş ölçeği), Λ_jj ≈ λ·1, yani C ≈ A.
3. **V11'in kazancı** — tam ızgarada tekrarlanmadı; (γ,λ)'nın kalibrasyondan önceki CE ile seçilmesi sonucu etkiliyor (bkz. Açık cevap).
4. [Muhtemel] Asıl sınırlama formül değil özellik bilgisi: ~512–1024 rastgele yön 2001 sınıflı sonraki-kelime için yetersiz; W2'nin düzenlileştirme biçimi bunu değiştirmiyor.

## Süre ve RAM
Ağırlık üretimi (W1 + eğitim+doğrulama istatistiği + 1 Cholesky): A ≈ 0,4 sn, C ≈ 0,4 sn, B ≈ 1,2 sn (H=512; B'de ek ağırlıklı `FᵀDF` geçişi ve satır ölçekleme) — tablodaki "ağırlık üretimi" sütunu. Arama dahil toplam süre tohum başına ≈ 113 sn (3 formül, 48 (γ,λ) noktası, H=512), tepe RAM ≈ 400 MB (H=512, V=2001 sınıf için `Rall` birikimi dahil). Kalibrasyon ≈ 3–4 sn/formül.

## Sınırlar
- Tek metin, 50 000 token; B'nin üssü/sınırı tek değer; C'nin tek biçimi.
- H=1024: yalnız `bag` özelliği (A, B, C) ve tek tohum; `ref` H=1024 ve 3 tohum H=1024 yapılmadı (süre). Tablo bu satırları içermiyorsa süre sınırında tamamlanmamıştır.
- 3 tohum yalnız `ref`, H=512, A ve B için.
- İlk geliştirme koşusunun (ref H=512 tohum 1) ham terminal çıktısı kayda alınmadı; yalnız filtrelenmiş kopyası var (`..._TERMINAL_KOPYA.txt`), skor dökümleri ve kalibrasyon ham çıktıları (`kal.txt`) tam.

## Komutlar (gerçek)
Derleme: `g++ -O3 -march=native -fopenmp -o tek_hamle_v12 tek_hamle_v12.cpp`. Çalıştırma: `./tek_hamle_v12 cift 2 H TOHUM OZELLIK FORMULLER` (OZELLIK ∈ ref|bag, FORMULLER ∈ A,B,C kombinasyonu); kalibrasyon: `python3 -I metin_kalibrasyon_v12.py cift_OZELLIK_FORMUL 2 H TOHUM`. Ölçülen süre/RAM (`v12_ham/v12log/manifest.tsv`; ilk geliştirme koşusu hariç):

| komut | duvar sn | kullanıcı CPU sn | tepe RAM MB |
|---|---|---|---|
| `./tek_hamle_v12 cift 2 512 1 bag ABC` | 107.47 | 358.08 | 403 |
| `./tek_hamle_v12 cift 2 512 2 ref AB` | 70.75 | 236.35 | 361 |
| `./tek_hamle_v12 cift 2 512 3 ref AB` | 70.53 | 238.16 | 361 |
| `./tek_hamle_v12 cift 2 1024 1 bag ABC` | 188.46 | 652.93 | 815 |

## Ham terminal çıktısı örneği (`metin_kalibrasyon_v12.py cift_ref_B 2 512 1`)
```
OZET_V9|unigram|mod=cift_ref_B|c=2|H=512|tohum=1|test_CE=5.6389|PPL=281.15|top1=0.0904|top5=0.2255|mse_olasilik=0.000492
OZET_V9|V8_alpha1_V8_T_izgarasi|mod=cift_ref_B|c=2|H=512|tohum=1|alpha=1.0|T=0.005|val_CE=6.1165|test_CE=6.0259|PPL=414.00|top1=0.1359|top5=0.3120|mse_olasilik=0.000496|olasilik_toplami=1.000000
OZET_V9|V9_sartname_izgarasi|mod=cift_ref_B|c=2|H=512|tohum=1|alpha=0|T=0.25|val_CE=5.6947|test_CE=5.6389|PPL=281.15|top1=0.0904|top5=0.2255|mse_olasilik=0.000492|olasilik_toplami=1.000000
OZET_V9|V9_genisletilmis_izgara|mod=cift_ref_B|c=2|H=512|tohum=1|alpha=0.35|T=0.005|val_CE=5.5103|test_CE=5.4328|PPL=228.80|top1=0.1398|top5=0.2673|mse_olasilik=0.000486|olasilik_toplami=1.000000
```
