# Tek hamlede ağırlık üretimi V2 — ilk matrisi de veriden hesapla

Kural: gradyan yok, Adam yok, epok yok. C++ (`cpp/tek_hamle_v2.cpp`), eski dosyalar değişmedi.
Veri: MNIST 60k eğitim / 10k test (önceki raporlarla aynı ön işleme). Eğitimin 55k'sı fit, 5k'sı doğrulama.
Test verisi yalnız sonda, seçilen ayarla BİR kez kullanıldı. Çift seçimi yalnız fit kümesinden.

## Algoritma
```
Her özellik j için: farklı sınıftan iki eğitim örneği (xa, xb)
  w_j = (xa - xb) / (||xa - xb|| + eps)          b_j = -(xa + xb)ᵀ w_j / 2
F  = [ tanh( γ (X W1 + b1) ), 1 ]
W2 = (FᵀF + λI)⁻¹ FᵀT      → ters alınmadı, Cholesky ile S·W2 = R çözüldü
```
Referans ("rastgele"): aynı kod, aynı ızgara, W1 ~ N(0,1/d), b1 ~ N(0,0.5²).
γ ve λ doğrulama kümesinde seçildi (λ ∈ {1,10,100,1000}).

## Sonuçlar (test doğruluğu, 3 tohum, aynı makine, 4 çekirdek)
| H | yöntem | test doğruluğu | tek model süresi* | tepe RAM |
|---|---|---|---|---|
| 1024 | rastgele ELM | %93,3 / 93,5 / 93,3 | ~2,9 sn | 273 MB |
| 1024 | **çift (V2)** | **%96,1 / 96,0 / 96,0** | ~2,9 sn | 273 MB |
| 2048 | rastgele ELM | %95,5 / 95,5 / 95,6 | ~8,4 sn | 430 MB |
| 2048 | **çift (V2)** | **%97,0 / 97,3 / 97,3** | ~8,0 sn | 429 MB |
| 4096 (tek tohum) | rastgele ELM | %96,76 | 31,2 sn | 1028 MB |
| 4096 (tek tohum) | **çift (V2)** | **%97,90** | 28,6 sn | 1028 MB |

\* Tek model süresi: W1/b1 üretimi + 60k örnek için FᵀF, FᵀT + bir Cholesky; ayar aramasını İÇERMEZ.
Ayar aramasıyla toplam süre (16 nokta + son çözüm + test): H=1024 ~16 sn, H=2048 ~45 sn, H=4096 ~200 sn.

Çift yöntemi için γ ızgarası: istenen {0.5,1,2,4} ile çift YALNIZ alt uçta (0.5) en iyi çıktı
(H=1024 %95,4, H=2048 %96,6). Alt uç kenar olduğu için ızgarayı {0.06, 0.125, 0.25, 0.5}'e kaydırdım: optimum
0.125 (0.06'da düşüyor, 0.03'te çöküyor), yani optimum artık içeride. Yukarıdaki çift satırları bu genişletilmiş ızgarayla.
Rastgele ELM de aynı alt uç kontrolünden geçti ({0.125..1}): optimum γ=1, değişmedi. H=4096 rastgele yalnız
varsayılan ızgarayla koşuldu (seçilen γ=0.5, alt uçta; küçük bir ek kazanç olasılığı var).

## Karşılaştırma (aynı makine, önceki numpy ölçümleri)
| yöntem | test | süre |
|---|---|---|
| Adam MLP 256 gizli, 10 epok | %97,46 | 18 sn |
| Adam MLP 1024 gizli, 10 epok | %97,82 | 41 sn |
| Tek hamle ELM (numpy) H=4096 / 8192 | %96,8 / 97,7 | 14,6 / 55 sn |
| **V2 çift H=4096 (C++)** | **%97,90** | **28,6 sn** |

## Okuma
- [Kesin] Aynı özellik sayısında ilk matrisi çiftlerden hesaplamak rastgeleye göre +%2,7 (H=1024), +%1,7 (H=2048), +%1,1 (H=4096, tek tohum) puan kazandırıyor. Üç tohumda yön aynı, fark tohum farkından büyük.
- [Kesin] V2 H=2048 (%97,2) rastgele H=4096'yı (%96,8) geçiyor: aynı doğruluğa yarı özellikle, yaklaşık 3,5x daha kısa sürede (8 sn vs 29–31 sn) varılıyor. Soruya cevap: daha az özellikle yüksek doğruluk, evet.
- [Kesin] W1'i hesaplamak 0,01 sn sürüyor, bedava. Maliyet FᵀF'de kalıyor (n·H² + H³/3), RAM H² ile büyüyor, bu değişmedi.
- [Muhtemel] V2 H=4096 %97,90, Adam (1024 gizli) %97,82'nin üstünde ve tek model süresi (28,6 sn) 41 sn'den kısa. UYARI: Adam ölçümü eski numpy kodundan, ayar aramasız tek koşu; V2'nin 28,6 sn'sine ayar aramasız süre; ayar aramasıyla toplam 199 sn. Adil karşılaştırma için Adam'ı aynı C++ ortamında ayar aramasıyla koşmak gerek, yapılmadı.
- [Kesin] Sınır: tek tohumda 4096; 8192 ve üzeri denenmedi; bellek 4096'da 1 GB, 8192'de ~4 GB.
- [Tahmin] Çiftler küçük γ'da neredeyse doğrusal bir sınıf-ayırıcı özellik veriyor (iki sınıf ortasından geçen hiperdüzlem), bu rastgele yönlerden daha bilgilendirici. Mekanizma ayrı test edilmedi (çift yerine aynı sınıftan çift, yön katsayısı rastgele gibi kontroller yok).

## Dosyalar
`cpp/tek_hamle_v2.cpp`, `cpp/mnist_hazirla.py`. Derleme: `g++ -O3 -march=native -fopenmp -o tek_hamle_v2 tek_hamle_v2.cpp`.
Çalıştırma: `python cpp/mnist_hazirla.py; ./tek_hamle_v2 cift|rastgele H [tohum] [klasör] [γ listesi a,b,c,d]`.
