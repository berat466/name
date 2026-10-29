# TEK HAMLE V30 — 4 F + kelime kimliği formülünün optimizasyonu

Dal `claude/formul-avcisi` · kod `cpp/tek_hamle_v30.cpp` (V29 kopyası) · ham loglar/komutlar `v30_ham/`. H=512 (4×128), yalnız eğitim protokolü, V26 doğrusal okuma (α,ε yalnız doğrulama). Bütün seçimler **doğrulama PPL/CE ile**; test seçime girmedi. Formül V29'dakiyle aynı (ortak ridge, `F=[F₁..F₄]`, `X*=[X,ID(w_{t−2}),ID(w_{t−1})]`, F₂..F₄ girdisinde X* ve önceki F'ler); yalnız λ ve kimlik ölçek çarpanı `s_ID` değişti. `s_ID`: kimlik bloklarının hem F₁ (RMS_X/√nid × s_ID) hem F₂..F₄ (1/√nid × s_ID) girdi ölçeğini çarpar; ölçek temeli yalnız eğitimden.

## 1. Deney A — λ taraması (tohum 1, γ₁=1, γ₂=0,5, s_ID=1; ortak ridge, Y hedefi)

| λ | eğitim PPL | doğrulama PPL | (bilgi) test PPL | Top-1 / Top-5 | eğitim–doğr. CE farkı |
|---|---|---|---|---|---|
| 0.1 | 70.2 | 177.3 | 168.8 | 15.5% / 34.9% | 0.926 |
| 1 | 70.6 | 171.4 | 163.8 | 15.4% / 35.2% | 0.886 |
| 3 | 72.0 | 169.1 | 162.1 | 15.0% / 35.2% | 0.854 |
| 5 | 75.0 | 168.5 | 161.2 | 14.9% / 35.1% | 0.809 |
| 10 ← doğrulamada en iyi | 80.0 | 168.4 | 160.7 | 15.1% / 34.9% | 0.745 |
| 15 | 83.3 | 168.5 | 160.7 | 15.0% / 34.5% | 0.705 |
| 20 | 85.8 | 168.6 | 160.9 | 14.9% / 34.4% | 0.676 |
| 30 | 86.2 | 168.8 | 161.1 | 14.8% / 34.4% | 0.672 |
| 100 | 99.2 | 170.5 | 162.9 | 14.1% / 33.4% | 0.541 |
| 300 | 110.3 | 173.8 | 165.3 | 13.7% / 32.5% | 0.455 |

Seçilen λ=10 (ızgara içinde, kenar değil); ızgara {0,1; 1; 10; 30; 100; 300} + daraltma {3; 5; 15; 20} (λ=10–20 aralığında doğrulama farkı ≤0,2 PPL: anlamlı kazanç yok).

## 2. Deney B — kimlik ölçek çarpanı s_ID (tohum 1; her s_ID için komşu λ'lar)

| s_ID | λ | eğitim PPL | doğrulama PPL | (bilgi) test PPL | Top-1 / Top-5 | eğitim–doğr. CE farkı |
|---|---|---|---|---|---|---|
| 0.5 | 5 | 78.5 | 172.4 | 164.8 | 14.8% / 34.5% | 0.786 |
| 0.5 | 10 | 83.7 | 172.6 | 164.7 | 14.6% / 34.2% | 0.724 |
| 0.5 | 20 | 89.6 | 173.0 | 164.9 | 14.7% / 33.9% | 0.658 |
| 1 | 5 | 75.0 | 168.5 | 161.2 | 14.9% / 35.1% | 0.809 |
| 1 | 10 | 80.0 | 168.4 | 160.7 | 15.1% / 34.9% | 0.745 |
| 1 | 15 | 83.3 | 168.5 | 160.7 | 15.0% / 34.5% | 0.705 |
| 1 | 20 | 85.8 | 168.6 | 160.9 | 14.9% / 34.4% | 0.676 |
| 2 | 5 | 73.8 | 165.4 | 158.1 | 15.1% / 35.1% | 0.807 |
| 2 | 10 | 78.6 | 164.7 | 157.3 | 15.0% / 34.9% | 0.740 |
| 2 | 20 | 81.0 | 164.6 | 157.1 | 14.9% / 34.4% | 0.708 |
| 4 | 5 | 74.9 | 163.0 | 155.1 | 15.0% / 34.8% | 0.778 |
| 4 | 10 | 76.4 | 161.9 | 154.2 | 14.8% / 34.5% | 0.751 |
| 4 | 20 | 81.9 | 161.4 | 154.0 | 14.6% / 34.4% | 0.678 |
| 8 | 10 | 77.9 | 160.6 | 152.7 | 14.5% / 34.4% | 0.724 |
| 8 | 20 | 83.4 | 160.6 | 153.0 | 14.3% / 34.1% | 0.655 |
| 8 | 40 | 86.9 | 161.8 | 153.9 | 14.0% / 33.4% | 0.621 |
| 16 | 10 | 79.2 | 161.1 | 152.8 | 14.3% / 34.3% | 0.710 |
| 16 | 20 | 84.8 | 161.7 | 153.3 | 13.9% / 33.9% | 0.645 |
| 16 | 40 | 88.5 | 163.6 | 154.6 | 13.6% / 33.4% | 0.614 |

Grid {0,5; 1; 2; 4} içinde doğrulama PPL monoton düştü ve s_ID=4 **ızgara kenarındaydı**; bu yüzden ızgara 8 ve 16 ile genişletildi (görev ızgarasının dışında, aynı kuralla). Doğrulama optimumu s_ID=8 (λ=10 ve 20 eşit: 160,6); s_ID=16 daha kötü (161,1) → optimum ızgara içinde. Eşitlikte daha çok düzenlileştirilmiş **λ=20** seçildi (eğitim–doğrulama farkı daha küçük).

## 3. Doğrulama: seçilen konfigürasyon (s_ID=8, λ=20), 3 tohum, H=512

| tohum | γ₁ | test PPL (Y) | test CE | Top-1 / Top-5 | eğitim PPL | doğr. PPL | eğitim–doğr. CE farkı | Q açıklanan varyans (test) | Q-hedefli test PPL |
|---|---|---|---|---|---|---|---|---|---|
| 1 | 1 | **153.0** | 5.0302 | 14.3% / 34.1% | 83 | 161 | 0.655 | 0.306 | 156.2 |
| 2 | 2 | **155.0** | 5.0436 | 14.4% / 33.9% | 82 | 164 | 0.692 | 0.320 | 158.1 |
| 3 | 1 | **152.1** | 5.0248 | 14.6% / 34.0% | 83 | 162 | 0.670 | 0.313 | 156.0 |
| **ort.** | | **153.4** | 5.0329 | 14.4% / 34.0% | 83 | 162 | 0.672 | 0.313 | 156.7 |

## 4. Adil mimari karşılaştırması (H=512 toplam özellik boyutu, doğrusal okuma)

| sistem | test PPL | Top-1 / Top-5 | kaynak / protokol |
|---|---|---|---|
| Tek F (512+1) | 182,5 | 14,8% / 33,3% | V26–V28 3 tohum ortalaması (bu koşuda yeniden ölçülmedi) |
| 4 F, kimliksiz, ortak ridge | 176,5 (3 tohum) / **175.3** (tohum 1, bu programda yeniden ölçüldü) | 14.5% / 33.4% (tohum 1) | V28/V29 + yeniden ölçüm |
| 4 F + sıralı kimlik (V29 ayarı s_ID=1, λ=1) | 166,3 (3 tohum) / **163.8** (tohum 1, yeniden ölçüldü) | 15.4% / 35.2% (tohum 1) | V29 + yeniden ölçüm |
| **V30 optimize (s_ID=8, λ=20), 3 tohum** | **153.4** | 14.4% / 34.0% | bu rapor, bölüm 3 |
| Yalnız kimlik (F yok, ridge, λ=1, tohum 1) | 152.2 (W_Y) / 138.9 (W_Q) | 15.8% / 35.2% | V26c `E` modu yeniden koşuldu; **n-gram tablosu benzeri, genelleme başarısı sayılmaz** |
| Trigram (n-gram) | 138,9 | 16,0% / 35,8% | V24/V25 |

| Yalnız kimlik (F yok, ridge, **λ=10**, tohum 1) | 135,7 (W_Y) / 137,5 (W_Q) | 16,3% / 35,4% (W_Y) | V26c `E` modu, H=1024 koşusunun C0 satırı (kimlik-yalnız modelde F yok, H'den bağımsız; aynı doğrusal okuma, doğrulamayla). **n-gram tablosu benzeri** |

**Önemli:** kimlik-yalnız ridge λ'ya duyarlı (λ=1: 152,2; λ=10: 135,7). Ayarlanmış kimlik-yalnız model (135,7) V30'dan (153,4) **18 PPL daha iyi** ve trigrama (138,9) eşit/yakın. Yani dört F'nin matematiksel özellikleri, kimlik bilgisini seyrelterek bu n-gram benzeri tabanın **altında** kalıyor; V30'un F'li yapı olarak kazancı V29'a göre büyük, ama ham kimlik tablosunu geçmiyor. [Kesin: tohum 1 karşılaştırması]

## 5. Görülmüş / görülmemiş bağlamlar (test; tohum 1; Y hedefi, ortak ridge)

Test satırlarının %68,1'inde (t−2,t−1) trigram bağlamı eğitimde görülmüş (3 392 satır), %31,9'unda yalnız bigram bağlamı görülmüş (1 588 satır). Trigram n-gram: görülen 4,928 / yalnız bigram 4,945 nat (V24).

| model | trigram bağlamı GÖRÜLDÜ: CE (PPL) / Top-1 | yalnız bigram görüldü (trigram bağlamı görülmedi): CE (PPL) / Top-1 |
|---|---|---|
| 4F kimliksiz (V28 ayarı) | 5.1690 (175.7) / 14.4% | 5.1610 (174.3) / 14.7% |
| 4F + kimlik, V29 ayarı (s_ID=1, λ=1) | 5.1021 (164.4) / 15.4% | 5.0915 (162.6) / 15.4% |
| **V30** (s_ID=8, λ=20) | 5.0437 (155.0) / 14.4% | 5.0012 (148.6) / 14.2% |

## 6. H=1024 (4×256), tohum 1 (zaman kaldığı için tek koşu; γ₁=4, s_ID=8, λ=20 — H=1024 için ayrıca seçilmedi)

Test PPL (Y) **156.9** (Top-1 15.1% / Top-5 34.9%), eğitim / doğr. PPL 69 / 164, eğitim–doğr. CE farkı 0.865; Q açıklanan varyans (test) 0.401. Karşılaştırma: V28 H=1024 ortak ridge 183,4 (3 tohum), tek F 192,2. Bu H=1024 sonucu seçilmiş hiperparametre içermez ve tek tohumdur.

## 7. Sonuç

- **3 tohum, H=512, doğrulamayla seçilmiş (s_ID=8, λ=20): test PPL 153.4** (V29: 166,3; V28: 176,5; tek F: 182,5). Kazanç 166,3'e göre 12.9 PPL; bunun kaynağı öncelikle s_ID (λ tek başına: bölüm 1, sID=1 iken λ=1→10 test 163,8→160,7 yani ≈3 PPL, s_ID=1→8: ≈8 PPL, tohum 1).
- [Kesin] λ optimumu ızgara içinde (10–20); s_ID optimumu ızgara dışına (8) uzandı — kimlik katkısının V29 ölçeğinde (1) **hafife alındığı** görülüyor. Daha büyük s_ID doğrulamada tekrar kötüleşiyor (16).
- [Kesin] Hedef "trigram 138,9'a yaklaşma" **sağlanmadı**: V30 ≈ 15 PPL üstünde; yalnız-kimlik ridge (F yok) V25'te ≈137 verdi, yani F'li model kimlik bilgisini seyreltiyor (tanh, ortak λ).
- [Kesin] V30'un Q açıklanan varyansı (0,313) V29'dan (0,433) **düştü**, çünkü seçim Y hedefine göre yapıldı (λ=20, s_ID=8); Y için iyi olan ayar trigram dağılımını daha az doğrusal olarak yeniden üretiyor. Q-hedefli PPL ise 156,7. Açıklanan varyans ile PPL burada da birlikte hareket etmiyor.
- **Kimlik n-gram belleğidir:** s_ID büyüdükçe model kimlik (sayım) bilgisine daha çok yaslanıyor ve Top-1 düşüyor (bölüm 2: s_ID 1→8, λ=10: Top-1 15,1→14,5%) ama PPL düşüyor; bu ezber/sayım bilgisinin daha iyi kullanılmasıdır, bağımsız genelleme değildir.

## 8. Sınırlamalar

- Seçimler (λ, s_ID) yalnız H=512, tohum 1 doğrulamasında; diğer tohumlar ve H=1024 aynı değerlerle koşuldu. γ₁, γ₂ değişmedi (V29: γ₁ V17, γ₂=0,5).
- Üç hedefe (Y/Q) ayrı seçim yapılmadı; seçim Y'ye göre. Q sonuçları öğretici aktarımıdır, bağımsız öğrenme değil.
- Kimlik ölçeği yalnız tek çarpan; F₁ ve F₂..F₄ girdisi için ayrı çarpanlar, X bloğu için ayrı ölçek denenmedi. Kimlik çiftleri (hash) V29'da yardım etmedi, bu çalışmada tekrarlanmadı.
- Süre: λ-sweep aynı özellikler üzerinde çok λ için tek Gram ile yapıldı (λ başına ≈ 3 s çözüm + değerlendirme). Ağırlık üretim süresi/RAM aşağıda.
- V30 ana mimariye eklenmedi.

## 9. Süre ve bellek

| komut | wall s | user CPU s | RAM MB |
|---|---|---|---|
| `./tek_hamle_v30 2 512 1 1 0.1,1,10,30,100,300 0.5 BK 1 Y` | 26.00 | 45.96 | 870 |
| `./tek_hamle_v30 2 512 1 1 3,5,15,20 0.5 BK 1 Y` | 21.31 | 32.64 | 870 |
| `./tek_hamle_v30 2 512 1 1 5,10,20 0.5 BK 0.5 Y` | 13.40 | 24.22 | 870 |
| `./tek_hamle_v30 2 512 1 1 5,10,20 0.5 BK 2 Y` | 14.48 | 25.09 | 870 |
| `./tek_hamle_v30 2 512 1 1 5,10,20 0.5 BK 4 Y` | 14.19 | 25.26 | 870 |
| `./tek_hamle_v30 2 512 1 1 10,20,40 0.5 BK 8 Y` | 18.86 | 25.87 | 870 |
| `./tek_hamle_v30 2 512 1 1 10,20,40 0.5 BK 16 Y` | 13.83 | 25.98 | 870 |
| `./tek_hamle_v26c 2 512 1 1 1 E` | 21.17 | 64.45 | 1011 |
| `./tek_hamle_v30 2 512 1 1 20 0.5 BK 8 YQ` | 14.17 | 25.61 | 870 |
| `./tek_hamle_v30 2 512 2 2 20 0.5 BK 8 YQ` | 13.86 | 26.05 | 870 |
| `./tek_hamle_v30 2 512 3 1 20 0.5 BK 8 YQ` | 14.03 | 26.78 | 870 |
| `./tek_hamle_v30 2 512 1 1 1 0.5 ABK 1 YQ` | 27.91 | 53.03 | 904 |
| `./tek_hamle_v30 2 1024 1 4 20 0.5 BK 8 YQ` | 20.81 | 51.23 | 1022 |

Cholesky + çözüm süresi (513×513 sistem, V=2001 sağ taraf; H=512 tohum 1): 0.109 s; kimlik dahil F üretimi + Gram + RHS: `ZINCIR` satırlarında (v30_ham).

