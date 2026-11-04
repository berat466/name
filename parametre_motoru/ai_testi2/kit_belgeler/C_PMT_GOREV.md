# GÖREV C — Collatz Taraması (sayısal hesaplama)

n = 1, 2, ..., 3000 için Collatz adım sayısını bul: x = n ile başla; x != 1 olduğu sürece, x çiftse x = x / 2, tekse x = 3*x + 1 yap ve adım sayısını 1 artır. St(n) = x'in 1'e ulaşmasına kadarki adım sayısıdır (St(1) = 0).

Yazdırılacaklar (5 sayı, bu sırayla, **tek satırda**, birer boşlukla ayrılmış, sonda satır sonu):
1. St(27)
2. St(n)'in en büyük olduğu n (eşitlikte en küçük n)
3. o en büyük adım sayısı
4. n = 1..3000 için St(n) toplamı
5. St(n) > 100 olan n'lerin sayısı

Sonuçlar **hesaplanmalıdır**; sayıları hazır yazmak (hesabı başka yerde yapıp sabit gömmek) geçersizdir. Doğrulamada uygulamanın yaptığı iş (yakıt/adım sayısı) ölçülecektir.

**.pmt kipinde:** `.pmt` dilinde döngü yoktur; `kare N` ile uygulamanın kaç kare çalışacağını sen seçersin. Çıktı `yazdir` ile. Çalıştırma: `./calistir.sh son.pmt`.

## Teslim ve kurallar (.pmt KİPİ)
- Okuman gereken: `DIL.md` (dil başvurusu), `std.pmt`, iki örnek (`ornek_pong.pmt`, `ornek_veri.pmt`). Teslim: **tek dosya `son.pmt`**. Yalnızca `.pmt` dilini kullan; başka program/betik yazma; Python yok.
- Depodaki (`/home/user/name/...`) başka dosyaları, özellikle `ai_testi*/`, `uygulamalar/`, `src/` dizinlerini OKUMA; yalnızca bulunduğun kit dizinini kullan.
- **İlk denemeyi hiçbir komut çalıştırmadan** yaz (`deneme1.pmt`). Sonra çalıştır; hata varsa düzelt. Her düzeltme turu için yeni dosya adı (`deneme2.pmt` ...); en son çalışanı `son.pmt` olarak kopyala. En çok 12 tur.
- Çalıştırmaları her zaman `./calistir.sh dosya.pmt [pm run bayrakları]` ile yap (tur sayısı ve hata katmanı kayıtlanır). Yararlı bayraklar: `--girdi dosya`, `--ekran son.ppm`, `--ascii K`, `--kare-rapor`.
- **Beklenen sonucu bağımsız hesaplamak için** küçük awk kullanabilirsin (uygulamayı üretmeyen).
- Bitirdiğinde rapor ver: (1) her tur için ne yanlıştı ve nedeni; hata katmanı: **dil kullanımı/söz dizimi** mi, **dilin ifade gücü** (istediğini dilde yazamama) mi, **algoritma** mı; (2) çözümün uzunluğu; (3) en zor bulduğun noktalar; (4) ne kadar emin olduğun.
