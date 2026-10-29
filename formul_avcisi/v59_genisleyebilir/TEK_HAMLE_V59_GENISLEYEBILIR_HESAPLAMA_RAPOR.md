# V59: 2.000 → 5.000 → 10.000 kelime — durum: KISMİ (A tamam; B/devam eden hesap uygulanamadı)

## Veri (ortak, kayıtlı)
Project Gutenberg (kamu malı): pg1342 Pride and Prejudice + pg11 Alice + pg2701 Moby Dick (indirme: gutenberg.org). 500-token bloklar, rng(0), %80/%10/%10; egitim 295 998 örnek (~297 k token), eğitimde 10 000+ farklı kelime. Kelime kimlikleri eğitimden frekansla **sabit**, V yalnız eşik (id≥V → <unk>); PPMI-SVD gömmesi eğitimden **bir kez** (V=10001 üzerinde), üç boyut aynı X'i kullanır. Sızıntı yok, sahte kelime yok. Eğitim <unk> oranı: 14,9 % / 7,2 % / 2,7 %. **Bu veri V54/V58 verisinden farklı**: V58 sayıları (9,8/21,8/33,8 s) ile doğrudan kıyaslanmaz; aşağıdaki A koşuları yeni ortak veride, aynı VM oturumunda, V54 ikilisiyle (`tek_hamle_v54`, LEAN+FAST, V53_PMIN=2 hafif ayar, RAM için) alındı.

## A — sıfırdan üretim (V54 hızlı yolu, tek çekirdek/tek havuz/tek analitik çözüm)
| | 2.000 | 5.000 | 10.000 |
|---|---|---|---|
| Model hazır süresi | 13,2 s | 21,3 s | 65,1 s |
| Tepe RAM | 1,55 GB | 3,02 GB | 6,55 GB |
| Val CE / PPL (yalnız kendi sözlüğünde) | 4,504 / 90,3 | 5,079 / 160,7 | 5,442 / 230,9 |
| C bloğu (mc) | 29 272 | 37 193 | 42 651 |
| K-tablo + kapı (f2) | 2,3 s | 10,2 s | **50,4 s** |
| 6-tablo kNN grafı (k1; V×V×V sgemm) | 0,9 s | 6,6 s | 39,4 s |
| Grup çözümü Y0/X (g45) | 1,0 s | 2,0 s | 2,4 s |
| U: Y0d kurulum + dgemm | 0,8 s | 2,1 s | 3,2 s |
| Schur W_F + W_C (g10+g11) | 0,3 s | 0,8 s | 1,9 s |
| Rezidu denetimi | 1,2e-13 | 3,7e-13 | 1,5e-13 (hepsi KABUL) |

## B — devam eden hesap: UYGULANMADI
15 dakikada V54'ün tek parça C++ yürütme düzenine doğrulanabilir bir artımlı genişletme yazamadım; doğrulanmamış yolu eklemedim. A ve B eşitliği, "yeniden kullanılan/yeni hesap" süreleri **ölçülmedi** (tabloda yok). Ölçülmüş olan ve B'nin dayanacağı yapı:
- Girdi X sabit olduğundan F=tanh(γXW+b), Gram AFF ve RF'nin F-kısmı V'den **bağımsız** → yeniden kullanılabilir (hazırlık f0−f2: 4,6 / 3,4 / 3,1 s, boyutla büyümüyor).
- V büyürken yalnız hedef/bağlam kimliği yeni aralığa düşen örneklerin slot katkıları değişir: 2001→5001'de örneklerin **%21,6**'sı, 5001→10001'de **%12,8**'i (tahmin: blok sınırı ihmal edildi). Yani A_CC/A_CF/R_C sayımlarında ~%13–22 oranında güncelleme yeterli olur, tamamını sıfırdan saymak gerekmez [Muhtemel; uygulanmadı].
- Q/OOF tabloları ve kNN grafı (Cn sayımlarına bağlı) genişlemede **değişir** → onlar yeniden/artımlı güncellenmeli; körü körüne korunamaz.

## Sorulara cevaplar
1. **V58 neden yavaştı?** [Kesin] Süre büyük ölçüde K-tabloları/kapı aşamasında (f2): 6 tablo × V×V×V yoğun sgemm (kosinüs kNN grafı, `v51_graph_cn`), V³ ile büyüyor. Gerisi (grup çözümü, Schur, W) 10.000'de bile toplam ~8 s. V58'de de (V=6722) k1 12,4 s / U 7,4 s ağırlıktaydı.
2. **V54 hızlı yolları geniş sözlükte aynen çalışıyor mu?** Evet, aynı ikili/yol (LEAN+FAST, AVX-512 grup çözücü, vektör tanhf, K-graf, kapı tablosu); kullanılan fallback yok (rezidu denetimi her boyutta kabul). Ama **K-graf yoğun V³ olarak kalıyor** — V54'te 2001'de ucuzdu, ölçekle hızla baskın oldu; bu "kaybolan verimlilik" asıl darboğaz ve düzeltilmedi. Aday düzeltme: Cn satırları çok seyrek (satır başına ≤ birkaç yüz sıfır olmayan), Nn·Nnᵀ'yi seyrek-seyrek çarpımla yapıp top-k'yı yalnız ortak bağlamı olan çiftlerden seçmek; sonuç topolojisi değişebileceğinden doğrulanmadan uygulanmadı. U için yoğun Y0d+dgemm de V ile büyüyor (3,2 s); V57'nin seyrek U yolu (`V53H_U=2`) bunun adayı.
3. **5.000 kelime:** 21,3 s (aynı VM oturumu, ortak veri; 2.000 için 13,2 s).
4. **10.000 kelime:** 65,1 s, 6,55 GB.
5. **Yeniden kullanılan oran:** ölçülmedi (B yok). Yukarıdaki F/Gram kısmı V-bağımsız; örneklerin %78–87'si 2001→5001→10001 genişlemesinde slot kimliğini korur.
6. **Tek çekirdek, ortak havuz, analitik çözüm:** Evet; yeni motor yok, backprop/replay/iteratif eğitim yok, ayrı AI yok.
7. **Doğruluk ve eşitlik:** Her boyutta rezidu ≤3,7e-13, val CE sonlu; A–B ağırlık eşitliği **doğrulanamadı** (B yok). Hangi V54 optimizasyonu korunmadı: hiçbiri kapatılmadı, ama K-graf V³ ve yoğun U V ile ölçeklenmiyor.

## Yetişmeyenler
Artımlı (B) motor, A/B ağırlık eşitliği, yeniden kullanılan/yeni süre ölçümleri, seyrek K-graf. Önerilen sıra: (1) seyrek kNN grafı (10.000'de ~%60 kazanç adayı), (2) seyrek U, (3) sayım güncellemesi ile artımlı genişletme.
