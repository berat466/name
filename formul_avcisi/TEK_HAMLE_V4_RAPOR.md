# Tek hamle V4 raporu — büyüyen ağırlıklar + kalıcı hafıza

Branch: `claude/formul-avcisi`. Referans: `TEK_HAMLE_V3_RAPOR.md`. V3 dosyaları değişmedi.
Kural: gradyan yok, Adam yok, epok yok; tek çözüm `W = (S + λI)⁻¹ R` (Cholesky). Delta karışmadı.
Kod: `cpp/tek_hamle_v4.cpp`. Ham loglar ve komutlar: `v4_ham/` (her koşunun ham stdout'u + `manifest.tsv`: komut, duvar süresi, CPU süresi, tepe RAM).
Ortam: Intel Xeon 2,80 GHz, 4 çekirdek (AVX-512), 16 GB, g++ 13.3.0 `-O3 -march=native -fopenmp` (`v4_ham/ortam.txt`).

## Kurulum
- Sınıf-artımlı MNIST, 5 görev × 2 sınıf: {0,1},{2,3},{4,5},{6,7},{8,9}; 10 çıkışlı tek model. Eğitim = 55k fit satırı, test = 10k. (5k doğrulama satırı bu deneyde kullanılmadı.)
- W1 yalnız ilk görevin fit verisinden (H₀=1024 yön); görev t≥2'de yalnız o görevin verisinden +256 yön; son H=2048. Yön = iki örneğin farkı (birim norm), bias = N(0, σ_b²) (V3'ün kazanan yöntemi `cyon_rbias`; σ_b o bloktaki çiftlerin orta-nokta bias'larının std'si). Gelecek görevlerin verisi W1'e girmedi (V4'ün `v3_*` referansı hariç, aşağıda).
- Kolon düzeni: 0 = bias, 1..H = özellikler; büyüme sona eklenir, eski S,R indeksleri değişmez.
- Hiperparametre: seçim yapılmadı. Ana ayar γ=0,125, λ=1 (V3'te doğrulamayla seçilen değer). Duyarlılık: γ=0,25, λ=10. İkisi de tam raporlandı.
- 3 tohum (1,2,3). Tablolar 3 tohum ortalaması.
- **Kayıp** = MSE: ortalama `(z−t)²` (örnek ve 10 çıkış üzerinden; t = one-hot). Eski = önceki görevlerin sınıfları, yeni = o adımın görevi, toplam = görülen tüm sınıflar. Unutma = (t. adımdan sonra eski görev i'nin test MSE'si) − (görev i'yi öğrendikten hemen sonraki test MSE'si), i<t üzerinden ortalama.

## Yöntemler (hepsi aynı ölçülerle)
- **E1** sabit W1 + birikimli kesin hafıza: W1 görev-1 verisinden, H=1024 sabit; `S += FᵀF`, `R += FᵀT`. **E1b**: aynı, H=2048 sabit (büyüyen sonla aynı boyut; kontrol).
- **E2a sıfır dolgu**: özellik uzayı büyüyünce eski S,R yeni boyuta sıfırla doldurulur (çapraz blok ve yeni-yeni blokta eski verinin payı = 0 varsayımı), yeni görevin kesin istatistiği eklenir, ortak çözülür.
- **E2b donuk + kalıntı**: eski W donuk; yalnız yeni 256 satır, yeni görevin istatistiğiyle `W_yeni = (S_nn+λI)⁻¹(R_n − S_no W_eski)` çözülür.
- **E3a/E3b karma**: sıfır dolgu + eski verinin eksik blokları (çapraz ve yeni-yeni) tampondan (sınıf başına 20 / 100 ham örnek, eski görevlerden, ağırlık = sınıf fit sayısı/K) TAHMİN edilir; eski-eski blok saklanan kesin değer.
- **E3c/E3d saf tampon**: eski verinin TÜM istatistiği tampondan tahmin edilir (pozitif yarı-belirli), yeni görev kesin.
- **TAVAN (bilgi, uygulanamaz)**: görülen tüm görevlerin ham verisiyle aynı özelliklerin kesin ortak çözümü. Özelliklerin kendisinin kalitesini ayırır.
- **E4 V3 referansı**: W1 TÜM sınıfların fit verisinden (gelecek görevleri görmüş), H=1024 ve 2048, birikimli kesin hafıza (μ=1).

## Ana sonuç — görev 5 sonrası (γ=0,125, λ=1)
| yöntem | H | test MSE eski | test MSE yeni | test MSE toplam | eğitim MSE toplam | unutma (sonraki−önceki, test MSE) | doğr. eski % | doğr. yeni % | doğr. toplam % | güncelleme sn | λ_kullanılan (maks) | Cholesky başarısız |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| E1  sabit W1 (görev-1), H=1024, birikimli kesin S,R | 1024 | 0.0162 | 0.0215 | 0.0173 | 0.0172 | 0.0091 | 95.7 | 93.6 | 95.3 | 0.10 | 1 | 0/3 |
| E1b sabit W1 (görev-1), H=2048 (kontrol) | 2048 | 0.0129 | 0.0174 | 0.0138 | 0.0132 | 0.0072 | 96.8 | 95.1 | 96.5 | 0.61 | 1 | 0/3 |
| E2a büyüyen W1, sıfır dolgu | 2048 | 0.1899 | 0.0045 | 0.1531 | 0.1522 | 0.1878 | 0.0 | 98.9 | 19.6 | 0.65 | 1 | 0/3 |
| E2b büyüyen W1, donuk eski W + kalıntı | 2048 | 0.2119 | 0.0081 | 0.1715 | 0.1699 | 0.2079 | 0.0 | 98.2 | 19.5 | 0.01 | 1 | 0/3 |
| E3a büyüyen W1, karma (kesin eski blok + tampon 20/sınıf) | 2048 | 0.1000 | 0.1000 | 0.1000 | 0.1000 | 0.0317 | 12.2 | 0.0 | 9.8 | 1.96 | -1 | 3/3 |
| E3b büyüyen W1, karma, tampon 100/sınıf | 2048 | 0.0433 | 0.0592 | 0.0465 | 0.0473 | 0.0180 | 84.3 | 72.3 | 81.9 | 2.07 | 100000 | 0/3 |
| E3c büyüyen W1, saf tampon 20/sınıf | 2048 | 0.0541 | 0.0061 | 0.0446 | 0.0457 | 0.0521 | 67.8 | 99.2 | 74.1 | 0.62 | 1 | 0/3 |
| E3d büyüyen W1, saf tampon 100/sınıf | 2048 | 0.0312 | 0.0122 | 0.0274 | 0.0271 | 0.0273 | 90.1 | 98.2 | 91.7 | 0.59 | 1 | 0/3 |
| (bilgi) büyüyen W1, TAVAN: görülen tüm veriyle kesin ortak çözüm — uygulanamaz | 2048 | 0.0111 | 0.0148 | 0.0118 | 0.0112 | 0.0056 | 97.4 | 95.9 | 97.1 | 6.94 | 1 | 0/3 |
| E4  V3 referansı, H=1024 (W1 tüm sınıflardan) | 1024 | 0.0131 | 0.0175 | 0.0140 | 0.0139 | 0.0073 | 96.8 | 94.8 | 96.4 | 0.09 | 1 | 0/3 |
| E4b V3 referansı, H=2048 | 2048 | 0.0104 | 0.0139 | 0.0111 | 0.0104 | 0.0057 | 97.6 | 96.3 | 97.3 | 0.64 | 1 | 0/3 |

(H sütunu özellik sayısı. Tablolar `v4_ham/` ham loglarından `v4_ham/analiz.py` ile üretildi.)

### Toplam test MSE'nin görev boyunca seyri (γ=0,125, λ=1)
| yöntem | t=1 | t=2 | t=3 | t=4 | t=5 |
|---|---|---|---|---|---|
| SABIT_1024 | 0.0002 | 0.0044 | 0.0085 | 0.0120 | 0.0173 |
| SABIT_2048 | 0.0002 | 0.0035 | 0.0066 | 0.0095 | 0.0138 |
| BUYUME_SIFIR | 0.0002 | 0.0981 | 0.1356 | 0.1330 | 0.1531 |
| BUYUME_DONUK | 0.0002 | 0.1141 | 0.1554 | 0.1486 | 0.1715 |
| BUYUME_ORNEK20 | 0.0002 | 0.0221 | 0.0342 | 0.2709 | 0.1000 |
| BUYUME_ORNEK100 | 0.0002 | 0.0193 | 0.0334 | 0.0386 | 0.0465 |
| BUYUME_TAMPON20 | 0.0002 | 0.0167 | 0.0283 | 0.0364 | 0.0446 |
| BUYUME_TAMPON100 | 0.0002 | 0.0065 | 0.0146 | 0.0207 | 0.0274 |
| BUYUME_TAVAN | 0.0002 | 0.0038 | 0.0066 | 0.0086 | 0.0118 |
| V3_1024 | 0.0002 | 0.0039 | 0.0071 | 0.0097 | 0.0140 |
| V3_2048 | 0.0002 | 0.0031 | 0.0055 | 0.0077 | 0.0111 |

### Eski görev test MSE'si
| yöntem | t=1 | t=2 | t=3 | t=4 | t=5 |
|---|---|---|---|---|---|
| SABIT_1024 | -1.0000 | 0.0024 | 0.0079 | 0.0118 | 0.0162 |
| SABIT_2048 | -1.0000 | 0.0020 | 0.0063 | 0.0093 | 0.0129 |
| BUYUME_SIFIR | -1.0000 | 0.1893 | 0.1955 | 0.1761 | 0.1899 |
| BUYUME_DONUK | -1.0000 | 0.2186 | 0.2233 | 0.1964 | 0.2119 |
| BUYUME_ORNEK20 | -1.0000 | 0.0166 | 0.0306 | 0.2932 | 0.1000 |
| BUYUME_ORNEK100 | -1.0000 | 0.0142 | 0.0288 | 0.0398 | 0.0433 |
| BUYUME_TAMPON20 | -1.0000 | 0.0293 | 0.0399 | 0.0476 | 0.0541 |
| BUYUME_TAMPON100 | -1.0000 | 0.0084 | 0.0187 | 0.0255 | 0.0312 |
| BUYUME_TAVAN | -1.0000 | 0.0022 | 0.0062 | 0.0084 | 0.0111 |
| V3_1024 | -1.0000 | 0.0023 | 0.0066 | 0.0096 | 0.0131 |
| V3_2048 | -1.0000 | 0.0018 | 0.0051 | 0.0075 | 0.0104 |

### Yeni görev test MSE'si
| yöntem | t=1 | t=2 | t=3 | t=4 | t=5 |
|---|---|---|---|---|---|
| SABIT_1024 | 0.0002 | 0.0064 | 0.0097 | 0.0126 | 0.0215 |
| SABIT_2048 | 0.0002 | 0.0051 | 0.0075 | 0.0101 | 0.0174 |
| BUYUME_SIFIR | 0.0002 | 0.0036 | 0.0027 | 0.0018 | 0.0045 |
| BUYUME_DONUK | 0.0002 | 0.0058 | 0.0048 | 0.0035 | 0.0081 |
| BUYUME_ORNEK20 | 0.0002 | 0.0277 | 0.0422 | 0.2031 | 0.1000 |
| BUYUME_ORNEK100 | 0.0002 | 0.0247 | 0.0437 | 0.0348 | 0.0592 |
| BUYUME_TAMPON20 | 0.0002 | 0.0037 | 0.0028 | 0.0026 | 0.0061 |
| BUYUME_TAMPON100 | 0.0002 | 0.0044 | 0.0053 | 0.0061 | 0.0122 |
| BUYUME_TAVAN | 0.0002 | 0.0054 | 0.0075 | 0.0090 | 0.0148 |
| V3_1024 | 0.0002 | 0.0055 | 0.0081 | 0.0101 | 0.0175 |
| V3_2048 | 0.0002 | 0.0044 | 0.0063 | 0.0081 | 0.0139 |

### Eğitim MSE'si (görülen sınıflar)
| yöntem | t=1 | t=2 | t=3 | t=4 | t=5 |
|---|---|---|---|---|---|
| SABIT_1024 | 0.0002 | 0.0045 | 0.0085 | 0.0119 | 0.0172 |
| SABIT_2048 | 0.0001 | 0.0034 | 0.0064 | 0.0089 | 0.0132 |
| BUYUME_SIFIR | 0.0002 | 0.0972 | 0.1345 | 0.1307 | 0.1522 |
| BUYUME_DONUK | 0.0002 | 0.1128 | 0.1538 | 0.1457 | 0.1699 |
| BUYUME_ORNEK20 | 0.0002 | 0.0229 | 0.0355 | 0.2618 | 0.1000 |
| BUYUME_ORNEK100 | 0.0002 | 0.0203 | 0.0345 | 0.0394 | 0.0473 |
| BUYUME_TAMPON20 | 0.0002 | 0.0173 | 0.0294 | 0.0371 | 0.0457 |
| BUYUME_TAMPON100 | 0.0002 | 0.0069 | 0.0151 | 0.0207 | 0.0271 |
| BUYUME_TAVAN | 0.0002 | 0.0038 | 0.0064 | 0.0081 | 0.0112 |
| V3_1024 | 0.0002 | 0.0038 | 0.0071 | 0.0095 | 0.0139 |
| V3_2048 | 0.0001 | 0.0026 | 0.0051 | 0.0070 | 0.0104 |

## Duyarlılık ayarı (γ=0,25, λ=10) — görev 5 sonrası
| yöntem | H | test MSE eski | test MSE yeni | test MSE toplam | eğitim MSE toplam | unutma (sonraki−önceki, test MSE) | doğr. eski % | doğr. yeni % | doğr. toplam % | güncelleme sn | λ_kullanılan (maks) | Cholesky başarısız |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| E1  sabit W1 (görev-1), H=1024, birikimli kesin S,R | 1024 | 0.0172 | 0.0232 | 0.0184 | 0.0182 | 0.0096 | 95.2 | 92.6 | 94.7 | 0.10 | 10 | 0/3 |
| E1b sabit W1 (görev-1), H=2048 (kontrol) | 2048 | 0.0138 | 0.0190 | 0.0148 | 0.0141 | 0.0077 | 96.3 | 93.9 | 95.9 | 0.63 | 10 | 0/3 |
| E2a büyüyen W1, sıfır dolgu | 2048 | 0.1870 | 0.0050 | 0.1509 | 0.1503 | 0.1847 | 0.0 | 98.8 | 19.6 | 0.64 | 10 | 0/3 |
| E2b büyüyen W1, donuk eski W + kalıntı | 2048 | 0.2230 | 0.0112 | 0.1810 | 0.1796 | 0.2182 | 0.0 | 98.1 | 19.5 | 0.01 | 10 | 0/3 |
| E3a büyüyen W1, karma (kesin eski blok + tampon 20/sınıf) | 2048 | 0.0533 | 0.0688 | 0.0564 | 0.0570 | 0.0227 | 79.8 | 60.3 | 76.0 | 2.21 | 1e+06 | 0/3 |
| E3b büyüyen W1, karma, tampon 100/sınıf | 2048 | 0.0372 | 0.0512 | 0.0400 | 0.0409 | 0.0170 | 86.5 | 77.9 | 84.8 | 1.78 | 100000 | 0/3 |
| E3c büyüyen W1, saf tampon 20/sınıf | 2048 | 0.0526 | 0.0063 | 0.0434 | 0.0444 | 0.0507 | 67.6 | 99.2 | 73.8 | 0.63 | 10 | 0/3 |
| E3d büyüyen W1, saf tampon 100/sınıf | 2048 | 0.0301 | 0.0126 | 0.0266 | 0.0263 | 0.0262 | 90.1 | 98.0 | 91.6 | 0.61 | 10 | 0/3 |
| (bilgi) büyüyen W1, TAVAN: görülen tüm veriyle kesin ortak çözüm — uygulanamaz | 2048 | 0.0113 | 0.0155 | 0.0121 | 0.0115 | 0.0058 | 97.2 | 95.6 | 96.9 | 6.41 | 10 | 0/3 |
| E4  V3 referansı, H=1024 (W1 tüm sınıflardan) | 1024 | 0.0132 | 0.0181 | 0.0142 | 0.0141 | 0.0073 | 96.6 | 94.5 | 96.2 | 0.10 | 10 | 0/3 |
| E4b V3 referansı, H=2048 | 2048 | 0.0105 | 0.0145 | 0.0113 | 0.0106 | 0.0058 | 97.5 | 95.9 | 97.2 | 0.62 | 10 | 0/3 |

Toplam test MSE seyri (γ=0,25, λ=10):

| yöntem | t=1 | t=2 | t=3 | t=4 | t=5 |
|---|---|---|---|---|---|
| SABIT_1024 | 0.0002 | 0.0045 | 0.0089 | 0.0128 | 0.0184 |
| SABIT_2048 | 0.0002 | 0.0037 | 0.0071 | 0.0102 | 0.0148 |
| BUYUME_SIFIR | 0.0002 | 0.0920 | 0.1336 | 0.1333 | 0.1509 |
| BUYUME_DONUK | 0.0002 | 0.1153 | 0.1604 | 0.1576 | 0.1810 |
| BUYUME_ORNEK20 | 0.0002 | 0.0168 | 0.0457 | 0.0497 | 0.0564 |
| BUYUME_ORNEK100 | 0.0002 | 0.0159 | 0.0262 | 0.0303 | 0.0400 |
| BUYUME_TAMPON20 | 0.0002 | 0.0154 | 0.0281 | 0.0350 | 0.0434 |
| BUYUME_TAMPON100 | 0.0002 | 0.0060 | 0.0139 | 0.0201 | 0.0266 |
| BUYUME_TAVAN | 0.0002 | 0.0038 | 0.0067 | 0.0088 | 0.0121 |
| V3_1024 | 0.0002 | 0.0039 | 0.0071 | 0.0098 | 0.0142 |
| V3_2048 | 0.0002 | 0.0031 | 0.0056 | 0.0077 | 0.0113 |

## Okuma — ne ölçüldü
1. [Kesin] **Özellik büyütmenin kendisi zarar vermiyor; zarar eksik istatistik yaklaşımından geliyor.** Aynı büyüyen özelliklerle tüm veriyle kesin çözüm (TAVAN) görev 5'te toplam test MSE 0,0118; W1'i tüm sınıflardan kuran V3 (H=2048) 0,0111. Yani yalnız ilk görevden/görev başına kurulan yönler V3'e yakın kalitede.
2. [Kesin] **En iyi kayıp koruması sabit W1 + kesin birikimli hafızada**: E1b (H=2048, görev-1 verisi) 0,0138, E1 (H=1024) 0,0173; V3 H=1024 0,0140. Bunlar kesin istatistik toplamıdır (V3'teki μ=1 denkliği); eksik çapraz blok yok.
3. [Kesin] **Büyüyen W1 + yaklaşık hafıza kaybı belirgin artırıyor.** Görev 5 toplam test MSE: sıfır dolgu 0,153, donuk 0,172, karma-20 0,100 (Cholesky başarısız, W=0), karma-100 0,047, saf tampon-20 0,045, saf tampon-100 0,027. Aynı özellik boyutunda (2048) sabit W1'e (0,0138) göre en iyi yaklaşım (saf tampon-100) 2,0 kat, TAVAN'a göre 2,3 kat kötü.
4. [Kesin] **Hata ilk büyümede (görev 2) başlıyor**, yaklaşık yöntemlerin hepsinde: toplam test MSE görev 2'de TAVAN'ın (0,0038) 1,5 katını aşan tüm büyüyen-yaklaşık yöntemler: sıfır dolgu 0,098, donuk 0,114, karma-20 0,022, karma-100 0,019, tampon-20 0,017, tampon-100 0,0065. Sabit W1 ve V3 hiçbir adımda 1,5 katı aşmadı (γ=0,125/λ=1).
5. [Kesin] **Sıfır dolgu ve donuk, eski görevi tamamen kaybediyor**: eski görev test MSE ≈ 0,19–0,21 (görev 5), eski doğruluk %0. Yeni sütunların yeni veride büyük ağırlık alması, eski örneklerde de ateşlendiği halde eski istatistikte karşılığı olmaması bunun nedeni gibi görünüyor ([Muhtemel], bu mekanizmayı ayrıca ölçmedim).
6. [Kesin] **Karma tampon pozitif yarı-belirliliği bozuyor.** Kesin eski blok + tampondan tahmin edilen bloklar karışınca matris belirsizleşiyor; Cholesky λ=1'de geçmiyor, λ'nın 1e5'e kadar ×10 artırılmasıyla ancak geçiyor (görev 2'den itibaren λ=1e5 kullanıldı = ölçülen kayıp ağır düzenlemeyle elde edildi). Karma-20 γ=0,125'te görev 4'te 1 tohumda, görev 5'te 3 tohumda hiç çözülemedi (ağırlıklar sıfır, MSE 0,100 = hep sıfır tahmini). Duyarlılık ayarında (λ=10 başlangıç, 1e6'ya kadar) çözüldü ama kayıp yine yüksek (0,056). Saf tampon (matris belirsizleşmiyor) hiç hata vermedi.
7. [Kesin] Tampon boyutu etkisi (saf tampon, görev 5 toplam test MSE): 20/sınıf 0,0446, 100/sınıf 0,0274 (TAVAN 0,0118). 100/sınıf = toplam 1000 ham örnek ≈ fit verisinin %1,8'i.
8. [Kesin] **Unutma (sonraki−önceki) görev 5**: sabit W1 H=2048 0,0072; V3 H=2048 0,0057; TAVAN 0,0056; saf tampon-100 0,0273; saf tampon-20 0,0521; karma-100 0,0180; sıfır dolgu 0,188; donuk 0,208. Kesin birikimli yöntemlerdeki ~0,006–0,009 gerçek unutma değil: görülen sınıf sayısı arttıkça ortak çözümün de MSE'si artıyor (TAVAN da 0,0056). Bu tabanın üstündeki fark yaklaşımın maliyeti.
9. Doğruluk yardımcı metrik: görev 5 toplam doğruluk E1b %96,5, V3 H=2048 %97,3, TAVAN %97,1, saf tampon-100 %91,7, saf tampon-20 %74,1, karma-100 %81,9, sıfır dolgu %19,6, donuk %19,5 (tablo yukarıda).
10. Duyarlılık ayarı (γ=0,25, λ=10) aynı sıralamayı veriyor (E1b 0,0148, TAVAN 0,0121, V3-2048 0,0113, saf tampon-100 0,0266, karma-100 0,0400, sıfır dolgu 0,151). Ayar seçimi yapılmadı; iki ayarın sonucu da verildi.

## Süre ve RAM
- Güncelleme süresi (sütun "güncelleme sn", görev 5): kesin birikimli E1 0,10 sn, E1b 0,6 sn; sıfır dolgu 0,65 sn, donuk 0,01 sn, karma ~2 sn, saf tampon ~0,6 sn (yalnız kendi çözümü; tampon özellik/istatistik hesabı karma satırının süresine dahil, saf tamponla paylaşılır, saf tampon için gerçek güncelleme maliyeti ≈ karma süresi), TAVAN 6,5 sn. Yeni görev için kesin istatistik hesabı (tüm yöntemler ortak) görev 5'te ≈ 1,2 sn.
- Süre/RAM ölçümleri tepe RAM: `buyume` süreci (7 yöntem aynı süreçte) 510 MB; `sabit1024` ve `v3_1024` 243 MB; `sabit2048` ve `v3_2048` 314 MB. Yöntemlerin ayrı RAM'i `buyume` içinde ayrılamaz. Duvar süreleri: buyume ≈ 46 sn, sabit1024 ≈ 8,4 sn, sabit2048 ≈ 20 sn, v3_1024 ≈ 9,4 sn, v3_2048 ≈ 22 sn (manifest).
- Yöntemin gerçek durum maliyeti: kesin birikimli yöntemde tek S (m×m) + R; tampon yöntemlerinde ek olarak 20 veya 100 ham örnek/sınıf (200 / 1000 örnek).

## Doğrulama
- Aynı komut ve tohumla (`./tek_hamle_v4 buyume 1 0.125 1`) ikinci bağımsız koşu: tüm ADIM ve GOREV satırları (süre alanları hariç) **birebir aynı** (140/140 satır). Ham: `v4_ham/tekrar_buyume_g0.125_l1_s1.txt`, `v4_ham/manifest_tekrar.tsv`.
- Hata ve düzeltme: `v3_*` ailesinin ilk koşusunda H, ad ayrıştırmasındaki hata yüzünden 3 (m=4) olarak alındı (çok kısa süre, anlamsız sonuç). Hata görüldü, düzeltildi (`H = atoi(aile.substr(3))`) ve `v3_*` koşuları yeniden yapıldı. Hatalı koşuların ham logları silinmedi: `v4_ham/hatali_v3_H3/`. Tablolardaki V3 satırları düzeltilmiş koşulara ait. Diğer aileler (`sabit*`, `buyume`) etkilenmedi.

## Komutlar (gerçek, `v4_ham/manifest.tsv`)
Her satır bir koşu; tohum 1,2,3 × (γ,λ) ∈ {(0,125; 1), (0,25; 10)} × aile ∈ {buyume, sabit1024, sabit2048, v3_1024, v3_2048}:

| komut | duvar sn | kullanıcı CPU sn | tepe RAM MB |
|---|---|---|---|
| `./tek_hamle_v4 buyume 1 0.125 1` | 47.36 | 173.07 | 510 |
| `./tek_hamle_v4 sabit1024 1 0.125 1` | 8.33 | 31.10 | 243 |
| `./tek_hamle_v4 sabit2048 1 0.125 1` | 19.65 | 73.86 | 314 |
| `./tek_hamle_v4 buyume 2 0.125 1` | 46.09 | 169.44 | 510 |
| `./tek_hamle_v4 sabit1024 2 0.125 1` | 8.44 | 31.59 | 243 |
| `./tek_hamle_v4 sabit2048 2 0.125 1` | 20.76 | 77.56 | 314 |
| `./tek_hamle_v4 buyume 3 0.125 1` | 45.81 | 168.55 | 510 |
| `./tek_hamle_v4 sabit1024 3 0.125 1` | 8.44 | 31.72 | 243 |
| `./tek_hamle_v4 sabit2048 3 0.125 1` | 20.03 | 74.94 | 314 |
| `./tek_hamle_v4 buyume 1 0.25 10` | 46.36 | 169.37 | 510 |
| `./tek_hamle_v4 sabit1024 1 0.25 10` | 8.71 | 32.45 | 243 |
| `./tek_hamle_v4 sabit2048 1 0.25 10` | 20.65 | 77.40 | 314 |
| `./tek_hamle_v4 buyume 2 0.25 10` | 45.68 | 167.82 | 510 |
| `./tek_hamle_v4 sabit1024 2 0.25 10` | 8.50 | 31.90 | 243 |
| `./tek_hamle_v4 sabit2048 2 0.25 10` | 20.08 | 75.32 | 314 |
| `./tek_hamle_v4 buyume 3 0.25 10` | 45.85 | 168.32 | 510 |
| `./tek_hamle_v4 sabit1024 3 0.25 10` | 9.09 | 33.97 | 243 |
| `./tek_hamle_v4 sabit2048 3 0.25 10` | 20.62 | 77.29 | 314 |
| `./tek_hamle_v4 v3_1024 1 0.125 1` | 9.36 | 35.19 | 243 |
| `./tek_hamle_v4 v3_2048 1 0.125 1` | 22.10 | 82.93 | 314 |
| `./tek_hamle_v4 v3_1024 2 0.125 1` | 9.58 | 35.96 | 243 |
| `./tek_hamle_v4 v3_2048 2 0.125 1` | 22.63 | 85.28 | 314 |
| `./tek_hamle_v4 v3_1024 3 0.125 1` | 9.45 | 35.68 | 243 |
| `./tek_hamle_v4 v3_2048 3 0.125 1` | 21.39 | 80.85 | 314 |
| `./tek_hamle_v4 v3_1024 1 0.25 10` | 9.30 | 35.16 | 243 |
| `./tek_hamle_v4 v3_2048 1 0.25 10` | 22.40 | 84.29 | 314 |
| `./tek_hamle_v4 v3_1024 2 0.25 10` | 9.44 | 35.62 | 243 |
| `./tek_hamle_v4 v3_2048 2 0.25 10` | 22.34 | 84.07 | 314 |
| `./tek_hamle_v4 v3_1024 3 0.25 10` | 9.54 | 35.94 | 243 |
| `./tek_hamle_v4 v3_2048 3 0.25 10` | 22.21 | 83.90 | 314 |

## Ham terminal çıktısı örneği (`./tek_hamle_v4 buyume 1 0.125 1`, seçilmiş satırlar)
```
V4 aile=buyume tohum=1 gamma=0.125 lambda=1 (4 cekirdek)
ADIM|buyume|BUYUME_SIFIR|1|0.125|1|1|1025|-1.000000|0.000217|0.000217|0.132290|-1.000000|0.000168|0.000168|-1.0000|0.9991|0.9991|0.518|0.145|0.647|1|0
ADIM|buyume|BUYUME_TAMPON100|1|0.125|1|1|1025|-1.000000|0.000217|0.000217|0.132290|-1.000000|0.000168|0.000168|-1.0000|0.9991|0.9991|0.518|0.096|0.647|1|0
ADIM|buyume|BUYUME_SIFIR|1|0.125|1|2|1281|0.186005|0.003638|0.096423|0.139995|0.183006|0.003682|0.095549|0.0000|0.9936|0.4881|0.621|0.189|1.317|1|0
ADIM|buyume|BUYUME_TAMPON100|1|0.125|1|2|1281|0.008394|0.004525|0.006494|0.098086|0.009477|0.004343|0.006973|0.9730|0.9946|0.9836|0.621|0.169|1.317|1|0
ADIM|buyume|BUYUME_SIFIR|1|0.125|1|3|1537|0.194300|0.002769|0.134786|0.150164|0.192731|0.002479|0.133385|0.0014|0.9979|0.3111|0.828|0.303|2.246|1|0
ADIM|buyume|BUYUME_TAMPON100|1|0.125|1|3|1537|0.018283|0.005639|0.014354|0.070192|0.019495|0.004638|0.014861|0.9343|0.9963|0.9536|0.828|0.310|2.246|1|0
ADIM|buyume|BUYUME_SIFIR|1|0.125|1|4|1793|0.176208|0.001820|0.133008|0.142501|0.174457|0.001372|0.130818|0.0003|0.9970|0.2472|0.982|0.467|2.832|1|0
ADIM|buyume|BUYUME_TAMPON100|1|0.125|1|4|1793|0.024976|0.006499|0.020399|0.047056|0.025987|0.004786|0.020642|0.9249|0.9894|0.9409|0.982|0.484|2.832|1|0
ADIM|buyume|BUYUME_SIFIR|1|0.125|1|5|2049|0.189882|0.004494|0.153119|0.153119|0.188284|0.003793|0.151980|0.0000|0.9869|0.1957|1.170|0.643|3.682|1|0
ADIM|buyume|BUYUME_TAMPON100|1|0.125|1|5|2049|0.031124|0.012518|0.027434|0.027434|0.031483|0.009852|0.027226|0.9017|0.9829|0.9178|1.170|0.589|3.682|1|0
OZET_ISLEM|buyume|1|toplam_sn=47.2|tepe_RAM_MB=510|cholesky_yeniden=42
```
(Tüm satırlar: `v4_ham/buyume_g0.125_l1_s1.txt`. Satır biçimi: `ADIM|aile|yöntem|tohum|γ|λ|t|m|test_MSE_eski|test_MSE_yeni|test_MSE_görülen|test_MSE_10sınıf|eğitim_MSE_eski|eğitim_MSE_yeni|eğitim_MSE_görülen|doğr_eski|doğr_yeni|doğr_görülen|yeni_istatistik_sn|yöntem_sn|değerlendirme_sn|λ_kullanılan|cholesky_başarısız`.)

## Başarısız veya zayıf sonuçlar (gizlenmedi)
- Sıfır dolgu, donuk, karma-20: toplam test MSE 0,10–0,17 (hiç öğrenmemiş modele yakın: sınıf-artımlı hep-sıfır tahmini 0,100), görev 2'den itibaren.
- Karma tampon: Cholesky görev 4-5'te çözülemedi (λ=1 ayarı, karma-20).
- Hiçbir yaklaşık büyüyen yöntem, aynı boyuttaki sabit-W1 kesin birikimli yöntemi (E1b) geçemedi.

## İyileştirme fikirleri (ölçülmedi, yalnız öneri)
- Kesin eski blok + tahmin edilen bloklar yerine Schur tamamlaması veya pozitif yarı-belirli projeksiyonla birleşik matrisi PSD'ye zorlamak (karma tampon hatası).
- Çapraz istatistik için ham örnek yerine eski verinin girdi uzayında sıkıştırılmış özeti (sınıf ortalaması + düşük rank kovaryans) saklayıp yeni özelliklerin eski veri üzerindeki momentlerini ondan hesaplamak. Eski veri olmadan bir tahmin olacağı için ayrıca doğrulanmalı.
- Büyütmeyi görev sırasında değil, önceden sabit havuzdan "açarak" yapmak: ölçülen en iyi yöntem (E1b) zaten bu mantıkta; H₀=2048'i görev-1'den kurup sabit tutmak büyütmeden iyi.
- Tampon örneklerini rastgele yerine sınıf içinde çeşitliliği artıracak biçimde (k-medoid) seçmek.
