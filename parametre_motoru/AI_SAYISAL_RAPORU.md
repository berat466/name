# Doğrudan Sayısal Parametre Testi — Rapor

## Düzen
- Karşılaştırılan iki yol, **aynı üç görev**, her biri için ayrı, bağlamsız bir ajan (toplam 6):
  - Sayısal kip: AI → doğrudan sayı dizisi → `paketle.awk` (yalnızca LEB128/zigzag bayt paketleyici, mantık üretmez) → `pm`.
  - .pmt kipi: AI → `.pmt` metni → şablon dönüştürücü → `pm`.
- Görevler (hepsi yeni, oyun değil): **A** veri analizi (4000 okumanın histogramı + toplam/min/max, 12 çıktı), **B** etkileşimli arayüz (4 çubuklu ayar paneli, klavye olayları, 100 kare, piksel doğrulaması), **C** sayısal hesaplama (3000 sayının Collatz taraması).
- Yalıtım: bağımsız awk kehanetleri ve referans çözümler **şifreli arşivdeydi** (AES-256, anahtar ajanlara verilmedi); test bitince açıldı (`ai_testi2/gizli_dogrular/`). Önceki testte kehanetler düz dosyaydı; bu fark önemlidir.
- Doğrulama: çıktı birebir + (B için) son karenin her pikseli, **ajanın hiç görmediği 3 olay dosyası** + gösterilen örnek (4/4). A ve C'de sabit gömmeye karşı yakıt eşiği (A ≥ 3000, C ≥ 50000).
- Kural denetimi: altı ajanın tüm araç çağrıları tarandı. Kit dışına çıkma, `pm asm`/`pm sablon`, `.pma`, Python, kod üreten betik **yok**. Yazılan tek dosyalar kendi deneme dosyaları ve test olay girdileri; awk yalnızca beklenen sonucu hesaplamak için kullanıldı.
- Ajan notu: ilk deneme hiçbir komut çalıştırılmadan yazıldı (altısı da öyle bildirdi; ilk komut kayıtları ve araç çağrı sırası ile tutarlı).

## Sonuç
| Görev | Kip | İlk deneme | Tur | .pmb (bayt) | Tanım | Yakıt | Yorumlayıcı / native (ms) |
|---|---|---|---|---|---|---|---|
| A histogram | sayısal | **doğru** | 1 | **214** | 205 sayı, 1188 bayt, 57 komut | 4 030 | 5.3 / 4.3 |
| A histogram | .pmt | **doğru** | 1 | 1270 | 228 sözcük, 41 satır, 917 bayt | 56 039 | 6.4 / 4.8 |
| B panel | sayısal | **doğru** (4/4 olay dosyası, piksel dahil) | 1 | **365** | 348 sayı, 1652 bayt, 97 komut | 1 553 039 | 31.3 / 16.7 |
| B panel | .pmt | **doğru** (4/4) | 1 | 1111 | 195 sözcük, 42 satır, 787 bayt | 1 578 568 | 30.0 / 11.5 |
| C Collatz | sayısal | **doğru** | 1 | **181** | 179 sayı, 991 bayt, 52 komut | 218 072 | 8.6 / 5.4 |
| C Collatz | .pmt | **doğru** | 1 | 896 | 170 sözcük, 30 satır, 697 bayt | 4 540 738 | 115.9 / 14.5 |

Başarı oranı: sayısal kip **3/3 ilk denemede, 0 düzeltme**; .pmt kipi **3/3 ilk denemede, 0 düzeltme**.
(Ajan süreleri: sayısal 55 / 112 / 65 sn, .pmt 43 / 73 / 48 sn; belirteç kullanımı ikisinde de 57–69 bin.)

## Hangi aşamada sorun yaşandı? (istenen ayrı rapor)
| Aşama | Sayısal kip | .pmt kipi |
|---|---|---|
| Sayısal temsil üretimi (yanlış kodlama/ofset/yazmaç; dil söz dizimi) | 0 hata | 0 hata |
| Dosya geçerliliği (paketleme/yükleyici/doğrulayıcı reddi) | 0 ret | 0 ret |
| Algoritmik doğruluk (çalışır ama sonuç yanlış) | 0 | 0 |
| Çalışma tuzağı | 0 | 0 |
| Sabit gömme | 0 (yakıt eşiği geçildi) | 0 |

[Kesin] `kayit.log` dosyalarında tüm çalıştırmalar `CALISTI`. Bu test, hata katmanlarını **ayırt edemedi**, çünkü hata olmadı. "AI hangi aşamada tıkanır" sorusu bu görev düzeyinde cevapsız kaldı; yalnızca "bu düzeyde tıkanmıyor" kanıtlandı.
Ajanların kendi bildirdiği zor noktalar (hata doğurmadı): elle göreli atlama ofseti sayma (A: 7, B: 20'den fazla geri atlama), `z` (zigzag) alanlarını doğru işaretleme, `poll` sonucundan tuş kodunu ayıklamak (`andi` 32 bit sınırı yüzünden `shli 32` + `shri 32`), host penceresi için ayrı yazmaç ayırmak, bss'in 1. adresten başlaması.

## Doğrudan karşılaştırma — ne diyor, ne demiyor
- **Çalıştırılan parametre dosyası:** sayısal kip 3–6 kat küçük (181–365 B'a karşı 896–1270 B). [Kesin] Ama bu karşılaştırma adil değil: .pmt'nin .pmb'si şablon açılımını ve genel amaçlı kural kodunu içerir; sayısal çözüm göreve özel döngüdür.
- **Yazılan metin:** sayısal kip daha **büyük** (B'de 1652 B'a karşı 787 B; A'da 1188'e 917; C'de 991'e 697). [Kesin] Yani AI'nın ürettiği şey .pmt'den kompakt değil; küçülen yalnızca derlenmiş çıktı.
- **Yakıt:** A'da 14x, C'de 21x az; B'de eşit (çizim ve sunum baskın). Nedeni: .pmt'de döngü yok, adım = kare, her örnek ayrı kural uygulaması. [Muhtemel] Yorumlayıcıda C 116 ms'ye karşı 8.6 ms.
- **Native kipte:** A ve C'de sayısal kip yine daha hızlı (C: 5.4 ms'ye karşı 14.5), B'de .pmt biraz daha hızlı (11.5'e karşı 16.7; sayısal çözüm her karede tüm 15360 pikseli yeniden yazıyor, derleyicinin ürettiği hali yalnızca dikdörtgenleri çiziyor). Küçük süreler başlatma gürültüsüne yakın. [Tahmin] ölçümler ±1 ms oynuyor.

## Dürüst değerlendirme
1. **Test, tavanı bulamadı.** [Kesin] Altı denemenin altısı ilk seferde doğru; görevler (tek döngü, sayaç dizisi, piksel doldurma) elle makine kodu yazılabilecek ölçekteydi. "AI doğrudan sayı üretebilir mi" için evet; "ne kadar karmaşığa kadar" için bilgi yok. En çok bilgi verecek sonraki deney, bu ölçeğin 5–10 katı bir görev (ör. sıralama + medyan, iç içe fonksiyon çağrıları, özyineleme) olurdu.
2. **Yardımcı iskele güçlüydü.** [Kesin] `SAYISAL.md` her komut kodunu, işlenen türünü ve host kimliklerini veriyor; `ornek.sayi` tam açıklamalı çalışan bir örnek. Bu belge olmadan sonuç farklı çıkardı. Ayrıca `calistir.sh` ve `pm dump` hata geri bildirimi sağlıyordu (kullanılmaya gerek kalmadı).
3. **Aynı model ailesi, aynı betik yazarı.** [Kesin] Ajanlar, belgeyi yazan modelin ailesinden; belge de onların okuyacağı biçimde yazılmış.
4. **A ve C'de genelleme sınırlı.** Veri sabit olduğu için "sayı hesaplanıyor" kanıtı yakıt eşiğine dayanıyor (sayısal A: 4030 yakıt = 4000 döngü + 30 host çağrısı, hesaplandığı belli). B'de ise gizli olay dosyalarıyla gerçek genelleme ölçüldü (4/4).
5. **Asıl soru hâlâ açık.** [Kesin] Bu, yeni bir parametre paradigması kanıtı **değil**. AI'nın yazdığı şey, yazmaç numaraları ve atlama ofsetleriyle sayısal assembly'dir (kullanıcı notundaki ayrım doğru). Kompakt, yeniden kullanılabilir yapı ölçütünde sayısal çözüm, .pmt'ye göre metin olarak büyük, ve yeniden kullanılabilir hiçbir bileşen içermiyor (her görev baştan döngü).

## Çıkarım ve öneri
- [Kesin] Sayısal biçim, AI tarafından belgeyle üretilebilir; doğrulayıcı ve paketleyici katmanı hiç hata vermedi.
- [Muhtemel] Sonraki adım, kullanıcının belirttiği gibi **şablonların sayısal temsili**: `.pmt` dilinin tablosunu doğrudan sayı tablosu olarak (alan türü, kural kodu, ilişki tablosu) tanımlayıp AI'nın bunu üretmesini ölçmek; karşılaştırma ölçütü yazılan sayı sayısı ve kullanılan bileşenlerin yeniden kullanımı olmalı.
- [Muhtemel] Önce bir "tavan" görevi (yukarıdaki 5–10x ölçek) eklenmeli; yoksa "6/6" bilgisi zayıf kalır.
- Optimizasyon (parametre küçültme, hız) bu testin parçası değil; başlatılmadı.

Tüm veriler: `ai_testi2/` (çözümler `cozumler/`, ham kayıtlar `kayitlar/`, görev metinleri `kit_belgeler/`, doğrular `gizli_dogrular/`, değerlendirici `degerlendir2.sh`, çıktılar `SONUCLAR.txt`).
