# Şablon / Örnekleme Katmanı — Rapor

Soru: uygulamaları, uygulamaya özel komut yazmadan, **yalnızca yapısal parametrelerle** (şablon, örnekleme, davranış, ilişki, olay) tarif edebilir miyiz; hız ve makine kodu yolunu koruyarak, ve bu normal bir döngüden gerçekten daha az tanımla mı?

## Ne yapıldı
- **`.pmt` dili** (`uygulamalar/sablon/DIL.md`): şablon + örnekleme üreteçleri (`sabit/dogrusal/mod/bol/rastgele`), davranış makroları, kural dili (`hedef = ifade eger koşul`), ilişkiler (`etkilesim A B`), olaylar (`girdi`), toplama (`topla`), çıkış (`yazdir`).
- **Özelleştirici** (`src/sablon.cpp`, 514 satır C++): tanımı `.pma`'ya çevirir, mevcut ayrıştırıcı ve **makine kodu üreticisinden** geçer. Yorumlayıcı katmanı yok; kural tabloları çalışma anında çözülmüyor. Toplam C++ 2.469 satır (hedef <3.000).
- **Davranışlar veri:** `std.pmt` (20 satır, 5 davranış) kural dilinin makrolarıdır: `hareket`, `yercekimi`, `sek`, `sekalt`, `sinirla`. Uygulama davranışı adıyla seçer.
- **Sanal alan:** hiçbir kuralla yazılmayan alan bellekte tutulmaz, üreteç formülünden hesaplanır.
- **Beş uygulama, uygulamaya özel komut yok:** `pong.pmt`, `parcacik.pmt` (10.000 örnek), `tugla.pmt` (izgara üreteci, 50 tuğla), `kovala.pmt` (başka şablonun alanına bakan kural, toplama, çarpışma), `veri.pmt` (grafiksiz veri işleme).

## Doğruluk
- **Pong eşdeğerliği (en güçlü kanıt):** `pong.pmt` (hiç komut yok) ile elle yazılmış `pong.pma`, 400 karede, betikli girdiyle **kare zinciri ve son kare hash'i bire bir aynı** (`84d106c16f05c7c3`). Aynı şekilde `parcacik.pmt` = elle yazılmış döngü sürümü (`f7f7b219f192a44f`).
- **Bağımsız awk benzetimleri (ayrı yazıldı, aynı kurallar), hepsi tuttu:** parçacık sahnesi (**57.600 bayt**, 10.000 örnek × 120 kare), tuğla kırma son durumu, kovalamaca son durumu, Pong, veri işleme toplamları (`5284079 717 9900 8 2642`).
- **Yorumlayıcı = makine kodu**, üç yazmaç kipinde (kare zinciri, çıktı, yakıt) ve `.pmb` = kaynak: 9 uygulama.
- Hatalı tanımlar anlaşılır mesajla reddedilir (tanımsız hedef, sonlanmayan uygulama, bilinmeyen davranış).
- Farklılık fuzz'ı, üretilmiş uygulamalarla: 10 seri × 25.000 deneme, ~21.000 geçerli program, **0 fark, 0 çökme**.
- Toplam: **203 test kontrolü + 60 uygulama kontrolü**, normal ve ASan/UBSan derlemesinde geçti.

## Ölçüm: tanım miktarı, artefakt, bellek, süre
Not: "normal bir döngü de 10.000 nesneyi kısa yazar" uyarısını ciddiye aldım: aynı uygulamaların **elle yazılmış** sürümleriyle karşılaştırdım.

**Tanım miktarı** (yorum ve boş satırlar sayılmadı; `std.pmt` paylaşımlı: 20 satır, 102 sözcük, 516 bayt)
| Uygulama | Şablon tanımı | Elle yazılmış | Fark |
|---|---|---|---|
| Pong | 50 satır / 240 sözcük / 1.039 B | 125 satır / 423 sözcük / 2.001 B | −%43 sözcük, −%48 bayt |
| Parçacık (10.000) | 18 satır / 93 sözcük / 411 B | 89 satır / 338 sözcük / 1.516 B | −%72 sözcük, −%73 bayt |
Kütüphane dahil sayılırsa fark küçülür: Pong 342 sözcük (−%19), parçacık 195 sözcük (−%42). Kütüphane **bir kez** yazılıp tüm uygulamalarda paylaşılıyor. Yeni uygulamalar yalnızca tanım: `tugla.pmt` 52 satır / 265 sözcük, `kovala.pmt` 53 satır / 309 sözcük, `veri.pmt` 19 satır / 92 sözcük.

**Derlenmiş parametre dosyası** (`--strip`) ve bellek
| Uygulama | Şablondan | Elle | Not |
|---|---|---|---|
| Pong | 1.548 B | 504 B | **3,1× büyük** |
| Parçacık | 853 B | 456 B | **1,9× büyük** |
- Alan belleği: parçacıkta **fark yok** (dört alanın hepsi dinamik/rastgele, 40.000 sözcük iki sürümde de). Sanal alanın kazancı **tuğlada**: 205 alan sözcüğünün 150'si (%73) bellekte hiç yer tutmuyor.
- Tepe bellek hepsinde ≈ 12,3–12,5 MB (sabit yazmaç yığını baskın): bellek açısından fark ölçülmedi.

**Süre** (yürütme, en iyi 5 koşu)
| Uygulama | Yorumlayıcı | Makine kodu | Yakıt |
|---|---|---|---|
| pong.pmt | 33,8 ms | 14,3 ms (≈28.000 kare/s) | 1.960.259 |
| pong.pma (elle) | 35,2 ms | 13,9 ms | 1.938.932 (**+%1,1**) |
| parcacik.pmt | 187 ms | 32,1 ms (≈3.700 kare/s) | 5.914.601 |
| parcacik_elle.pma | 123 ms | 25,5 ms | 4.754.240 (**+%24**) |
| tugla.pmt (600 kare) | 66 ms | 20,9 ms (≈28.700 kare/s) | 3.648.701 |
| veri.pmt (2000 satır) | 5,9 ms | 5,5 ms | 12.017 |

## Dürüst değerlendirme
1. **Tanım gerçekten kısa, ama ürün büyük.** Şablon tanımı elle yazılmışın yarısı ile dörtte biri arasında; derlenmiş dosya ise 1,9–3,1× büyük, çünkü özelleştirici genel döngü ve çizim yardımcılarını da üretiyor. "Az parametreyle tarif" hedefi için kazanç var; "küçük dağıtım dosyası" hedefi için kayıp var.
2. **Hız maliyeti iş yüküne bağlı.** Pong'da ölçülemeyecek kadar küçük (+%3 süre, +%1 yakıt); parçacıkta %26 yavaş (çizimde nokta başına işlev çağrısı, genel adres hesabı). Elle yazılmış döngüyü geçmiyoruz; yakınlaşıyoruz.
3. **"Normal döngü de kısa yazar" doğru.** Gerçek fark kısa yazmakta değil, **değiştirme maliyetinde ve alan-bağımsızlıkta**: aynı kural diliyle oyun (`pong`, `tugla`, `kovala`), grafik (`parcacik`) ve grafiksiz veri işleme (`veri`) yazıldı, hiçbirinde uygulamaya özel komut yok. Kısa döngü ise her biri için baştan yazılıyor.
4. **Davranış bağlantısı çalışıyor:** `davran sek x vx 0 79` gibi seçimlerle davranışlar birleşiyor; `kovala`'da kural başka şablonun alanına bakıyor (`oyuncu.x`), ilişkiler ve olaylar tablolarla tanımlı.
5. **"Kodsuz" hâlâ tam değil:** davranış makroları kural dilinin metin değiştirmesi, özelleştirici ise C++. Uygulama yazarı komut yazmıyor, ama sistem yine de bir kod üretiyor ve ben bu sistemi yazdım.

## Sınırlar
- **Genel amaçlı hesaplama değil:** kurallarda döngü/özyineleme yok, ifadede tek ikili işlem, toplama işlemleri sabit dört (toplam/say/en büyük/en küçük); sıralama, birleştirme (join), metin yok.
- **Etkileşim O(|A|·|B|):** uzamsal dizin yok. 10.000×10.000 çift bu sürümde denenmedi ve pratik değil.
- **3B ve arayüz bileşenleri yapılmadı** (kullanıcının hedeflediği örnekler arasındalardı). Veri işleme kısmı genel amaçlılığın yalnızca bir göstergesi.
- Sıra anlamı örnek-öncelikli; etkileşimde bir çiftin eylemi sonraki çiftlerin koşulunu etkiler (belgelendi).
- Assembler hataları üretilmiş `.pma` satır numarasını gösterir, tanım satırını değil (`--pma` ile bakılır).
- Gerçek pencere yok; AI'nın parametre üretmesi henüz denenmedi (bir sonraki adım).

## Bir sonraki adım (senin kararın): doğrudan AI uygulama üretimi
Başvuru dokümanı hazır (`uygulamalar/sablon/DIL.md`, 60 satır). Deney: bir dil modeline yalnızca bu doküman + bir iki örnek tanım verilip yeni bir uygulama (ör. "uzay gemisi, meteorlar, skor") tanımlatılır; ölçülen: ilk denemede geçerli mi, kaç düzeltme turu, çalıştırınca istenen davranış mı.
