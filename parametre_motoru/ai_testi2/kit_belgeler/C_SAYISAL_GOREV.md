# GÖREV C — Collatz Taraması (sayısal hesaplama)

n = 1, 2, ..., 3000 için Collatz adım sayısını bul: x = n ile başla; x != 1 olduğu sürece, x çiftse x = x / 2, tekse x = 3*x + 1 yap ve adım sayısını 1 artır. St(n) = x'in 1'e ulaşmasına kadarki adım sayısıdır (St(1) = 0).

Yazdırılacaklar (5 sayı, bu sırayla, **tek satırda**, birer boşlukla ayrılmış, sonda satır sonu):
1. St(27)
2. St(n)'in en büyük olduğu n (eşitlikte en küçük n)
3. o en büyük adım sayısı
4. n = 1..3000 için St(n) toplamı
5. St(n) > 100 olan n'lerin sayısı

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
