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

**.pmt kipinde çıktı:** `yazdir` ile. Çalıştırma: `./calistir.sh son.pmt`.

## Teslim ve kurallar (.pmt KİPİ)
- Okuman gereken: `DIL.md` (dil başvurusu), `std.pmt`, iki örnek (`ornek_pong.pmt`, `ornek_veri.pmt`). Teslim: **tek dosya `son.pmt`**. Yalnızca `.pmt` dilini kullan; başka program/betik yazma; Python yok.
- Depodaki (`/home/user/name/...`) başka dosyaları, özellikle `ai_testi*/`, `uygulamalar/`, `src/` dizinlerini OKUMA; yalnızca bulunduğun kit dizinini kullan.
- **İlk denemeyi hiçbir komut çalıştırmadan** yaz (`deneme1.pmt`). Sonra çalıştır; hata varsa düzelt. Her düzeltme turu için yeni dosya adı (`deneme2.pmt` ...); en son çalışanı `son.pmt` olarak kopyala. En çok 12 tur.
- Çalıştırmaları her zaman `./calistir.sh dosya.pmt [pm run bayrakları]` ile yap (tur sayısı ve hata katmanı kayıtlanır). Yararlı bayraklar: `--girdi dosya`, `--ekran son.ppm`, `--ascii K`, `--kare-rapor`.
- **Beklenen sonucu bağımsız hesaplamak için** küçük awk kullanabilirsin (uygulamayı üretmeyen).
- Bitirdiğinde rapor ver: (1) her tur için ne yanlıştı ve nedeni; hata katmanı: **dil kullanımı/söz dizimi** mi, **dilin ifade gücü** (istediğini dilde yazamama) mi, **algoritma** mı; (2) çözümün uzunluğu; (3) en zor bulduğun noktalar; (4) ne kadar emin olduğun.
