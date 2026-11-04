# GÖREV A — Okuma Histogramı (veri analizi)

4000 okumadan oluşan bir veri kümesini analiz eden bir uygulama yaz. Okuma i (i = 0, 1, ..., 3999) için değer:

    v(i) = (i*i*7 + i*13) % 1009 - 500        (tamsayı aritmetik; % ve - önceliği: önce mod, sonra 500 çıkarılır)

Her okumanın **kutusu**: b = (v + 500) / 127  (tamsayı bölme; sonuç 0..7).

Hesaplanıp yazdırılacaklar (12 sayı, bu sırayla, **tek satırda**, birer boşlukla ayrılmış, sonda satır sonu):
1. c0 c1 ... c7: her kutudaki okuma sayısı (8 sayı)
2. v değerlerinin toplamı
3. en küçük v
4. en büyük v
5. i % 3 == 0 olan ve v > 0 olan okumaların v toplamı

Sonuçlar **hesaplanmalıdır**; sayıları hazır yazmak (hesabı başka yerde yapıp sabit gömmek) geçersizdir. Doğrulamada uygulamanın yaptığı iş (yakıt/adım sayısı) ölçülecektir.

**Sayısal kipte çıktı:** `puti` ve `putc` ile yaz (puti ayraç eklemez; boşluk için `putc` 32, satır sonu için `putc` 10). Çalıştırma: `./calistir.sh son.sayi`. Bellek ve yakıt serbest.

## Teslim ve kurallar (SAYISAL KİP)
- Okuman gereken: `SAYISAL.md` (sayısal biçim), `ornek.sayi` (çalışan örnek), `paketle.awk`, `calistir.sh`. Teslim: **tek dosya `son.sayi`** (yalnızca sayılar ve `#` açıklamaları).
- **Doğrudan sayı yazacaksın.** Aşağıdakiler YASAK: `pm asm`, `pm sablon`, `.pma`/`.pmt` metni yazmak ya da dönüştürmek; sembolik adı/etiketi sayıya çeviren, atlama ofsetlerini hesaplayan ya da komut üreten herhangi bir betik/program (awk, bash, C++ ... ) yazmak; Python. Ofsetleri, yazmaç numaralarını, komut numaralarını kendin hesapla.
- İZİN VERİLEN: `./calistir.sh` ile çalıştırmak; `./pm dump x.pmb` ile motorun çözdüğü tabloyu okumak; `od`; **beklenen sonucu bağımsız hesaplamak için** küçük awk (uygulamayı üretmeyen, yalnızca sayı hesaplayan).
- Depodaki (`/home/user/name/...`) başka dosyaları, özellikle `ai_testi*/`, `uygulamalar/`, `src/` dizinlerini OKUMA; yalnızca bulunduğun kit dizinini kullan.
- **İlk denemeyi hiçbir komut çalıştırmadan** yaz (`son.sayi` içine ya da `deneme1.sayi` olarak). Sonra çalıştır; hata varsa düzelt. Her düzeltme turu için yeni bir dosya adı (`deneme2.sayi` ...) kullan; en son çalışanı `son.sayi` olarak kopyala. En çok 12 tur.
- Çalıştırmaları her zaman `./calistir.sh` ile yap (tur sayısı ve hata katmanı kayıtlanır).
- Bitirdiğinde rapor ver: (1) her tur için ne yanlıştı ve nedeni; hata katmanı: **sayısal temsil** (yanlış sayı/kodlama/ofset/yazmaç) mı, **dosya geçerliliği** (yükleyici/doğrulayıcı reddi) mi, **algoritma** (çalışıyor ama sonuç yanlış) mı; (2) çözümün kaç sayıdan oluştuğu; (3) en zor bulduğun noktalar; (4) ne kadar emin olduğun.
