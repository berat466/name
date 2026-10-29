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

**Sayısal kipte:** kare tamponu uygulama belleğinde 160*96 sözcüktür (her sözcük `0xRRGGBB`); her karede olayları `poll` ile boşalt (poll 0 dönene kadar), çiz, `present(adres,160,96)` çağır; `present` 1 döndürünce döngüden çık, çıktıyı `puti`/`putc` ile yaz, `halt`. Doğrulama `./pm run son.pmb --girdi OLAYLAR --max-kare 100 --ekran s.ppm` ile yapılır. Senin çalıştırman: `./calistir.sh son.sayi --girdi ornek.girdi --max-kare 100 --ekran s.ppm` (örnek olay dosyası `ornek.girdi`; son karenin görüntüsü için `--ascii 8` de kullanabilirsin).

## Teslim ve kurallar (SAYISAL KİP)
- Okuman gereken: `SAYISAL.md` (sayısal biçim), `ornek.sayi` (çalışan örnek), `paketle.awk`, `calistir.sh`. Teslim: **tek dosya `son.sayi`** (yalnızca sayılar ve `#` açıklamaları).
- **Doğrudan sayı yazacaksın.** Aşağıdakiler YASAK: `pm asm`, `pm sablon`, `.pma`/`.pmt` metni yazmak ya da dönüştürmek; sembolik adı/etiketi sayıya çeviren, atlama ofsetlerini hesaplayan ya da komut üreten herhangi bir betik/program (awk, bash, C++ ... ) yazmak; Python. Ofsetleri, yazmaç numaralarını, komut numaralarını kendin hesapla.
- İZİN VERİLEN: `./calistir.sh` ile çalıştırmak; `./pm dump x.pmb` ile motorun çözdüğü tabloyu okumak; `od`; **beklenen sonucu bağımsız hesaplamak için** küçük awk (uygulamayı üretmeyen, yalnızca sayı hesaplayan).
- Depodaki (`/home/user/name/...`) başka dosyaları, özellikle `ai_testi*/`, `uygulamalar/`, `src/` dizinlerini OKUMA; yalnızca bulunduğun kit dizinini kullan.
- **İlk denemeyi hiçbir komut çalıştırmadan** yaz (`son.sayi` içine ya da `deneme1.sayi` olarak). Sonra çalıştır; hata varsa düzelt. Her düzeltme turu için yeni bir dosya adı (`deneme2.sayi` ...) kullan; en son çalışanı `son.sayi` olarak kopyala. En çok 12 tur.
- Çalıştırmaları her zaman `./calistir.sh` ile yap (tur sayısı ve hata katmanı kayıtlanır).
- Bitirdiğinde rapor ver: (1) her tur için ne yanlıştı ve nedeni; hata katmanı: **sayısal temsil** (yanlış sayı/kodlama/ofset/yazmaç) mı, **dosya geçerliliği** (yükleyici/doğrulayıcı reddi) mi, **algoritma** (çalışıyor ama sonuç yanlış) mı; (2) çözümün kaç sayıdan oluştuğu; (3) en zor bulduğun noktalar; (4) ne kadar emin olduğun.
