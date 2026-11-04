# GÖREV B — Ayar Paneli (etkileşimli arayüz davranışı)

Dört çubuktan oluşan bir ayar paneli yaz. Klavye olaylarıyla yönetilir.

**Ekran:** 160x96 piksel, arka plan `0x101018`. **Tam 100 kare** çalışır, sonra durur.
**Durum:** seçili çubuk `sec` (0..3, başlangıç 0) ve dört değer v[0..3] (her biri 0..20). Başlangıç değerleri: v[0]=4, v[1]=8, v[2]=12, v[3]=16.
**Çizim:** s = 0..3 çubuğu için dolu dikdörtgen: x aralığı [8, 8 + 7*v[s]), y aralığı [8 + 22*s, 8 + 22*s + 14) (yarı açık; v[s] = 0 ise hiçbir şey çizilmez). Renk: s == sec ise `0xFFC040`, değilse `0x4080C0`. Çubuklar örtüşmez. Diğer pikseller arka plan rengindedir.
**Tuşlar** (olay `tuş kodu` ile gelir; her karede o karede teslim edilen **tüm** olaylar, geliş sırasıyla, çizimden önce işlenir):
- 1: seçimi yukarı al: sec = (sec + 3) % 4
- 2: seçimi aşağı al: sec = (sec + 1) % 4
- 3: v[sec] = en çok 20 olmak üzere 1 artır
- 4: v[sec] = en az 0 olmak üzere 1 azalt
- 5: tüm v değerlerini 0 yap (sec değişmez)
- başka her tuş: yok sayılır
**Çıktı:** 100. kare sunulduktan sonra tek satır: `sec v0 v1 v2 v3` (boşlukla ayrılmış, sonda satır sonu).

Olaylar `ornek.girdi` gibi bir dosyadan gelir (satır: `kare tuş`; bir olay, sunulan kare sayısı olayın karesine ulaşınca teslim edilir). Doğrulama **başka olay dosyalarıyla** yapılacaktır; yani uygulama gerçekten tuşlara göre davranmalıdır. Başarı ölçütü: çıktı satırı ve son karedeki her pikselin rengi.

**.pmt kipinde:** `sahne 160 96` ve `kare 100` kullan; çıktı `yazdir` ile. Çalıştırma: `./calistir.sh son.pmt --girdi ornek.girdi --ekran s.ppm` (`--ascii 8` de olur).

## Teslim ve kurallar (.pmt KİPİ)
- Okuman gereken: `DIL.md` (dil başvurusu), `std.pmt`, iki örnek (`ornek_pong.pmt`, `ornek_veri.pmt`). Teslim: **tek dosya `son.pmt`**. Yalnızca `.pmt` dilini kullan; başka program/betik yazma; Python yok.
- Depodaki (`/home/user/name/...`) başka dosyaları, özellikle `ai_testi*/`, `uygulamalar/`, `src/` dizinlerini OKUMA; yalnızca bulunduğun kit dizinini kullan.
- **İlk denemeyi hiçbir komut çalıştırmadan** yaz (`deneme1.pmt`). Sonra çalıştır; hata varsa düzelt. Her düzeltme turu için yeni dosya adı (`deneme2.pmt` ...); en son çalışanı `son.pmt` olarak kopyala. En çok 12 tur.
- Çalıştırmaları her zaman `./calistir.sh dosya.pmt [pm run bayrakları]` ile yap (tur sayısı ve hata katmanı kayıtlanır). Yararlı bayraklar: `--girdi dosya`, `--ekran son.ppm`, `--ascii K`, `--kare-rapor`.
- **Beklenen sonucu bağımsız hesaplamak için** küçük awk kullanabilirsin (uygulamayı üretmeyen).
- Bitirdiğinde rapor ver: (1) her tur için ne yanlıştı ve nedeni; hata katmanı: **dil kullanımı/söz dizimi** mi, **dilin ifade gücü** (istediğini dilde yazamama) mi, **algoritma** mı; (2) çözümün uzunluğu; (3) en zor bulduğun noktalar; (4) ne kadar emin olduğun.
