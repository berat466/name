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

**Tabloda:** ekran 160x96, KARE = 100; çıktı `satir` ile. Çalıştırma: `./calistir.sh son.sayi --girdi ornek.girdi --ekran s.ppm` (örnek olay dosyası `ornek.girdi`; `--ascii 8` de olur).

## Teslim ve kurallar (YAPISAL TABLO KİPİ)
- Okuman gereken: `YAPISAL.md` (tablo biçimi), `ornek_toplam.sayi`, `ornek_sahne.sayi`, `paketle.awk`, `calistir.sh`. Teslim: **tek dosya `son.sayi`** (yalnızca sayılar ve `#` açıklamaları; `PMY1` tablosu).
- **Yapısal tabloyu doğrudan sayı olarak yaz.** YASAK: `.pmt` ya da `.pma` metni yazmak/dönüştürmek, `pm asm`, `pm sablon`; sembolik ad/etiketi sayıya çeviren ya da tablo/komut üreten herhangi bir betik/program (awk, bash, C++ ...) yazmak; Python.
- İZİN: `./calistir.sh` ile çalıştırmak; `./pm tablo x.pmy` ile motorun tabloyu nasıl çözdüğünü okumak; `od`; **beklenen sonucu bağımsız hesaplamak için** küçük awk (uygulamayı üretmeyen, yalnızca sayı hesaplayan).
- Depodaki (`/home/user/name/...`) başka dosyaları, özellikle `ai_testi*/`, `uygulamalar/`, `src/` dizinlerini OKUMA; yalnızca bulunduğun kit dizinini kullan.
- Hedef: uygulamayı mümkün olduğunca **az sayıda anlamlı parametreyle** anlat. Binlerce nesneyi tek şablon, tekrarlanan kuralı davranış/işlev olarak ifade et; düşük seviyeli adımları tek tek yazmak yerine tabloda bulunan yapıları (şablon, alan başlatıcıları, olay, toplama, davranış, işlev, döngü, dizi) kullan.
- **İlk denemeyi hiçbir komut çalıştırmadan** yaz (`deneme1.sayi`). Sonra çalıştır; hata varsa düzelt. Her düzeltme turu için yeni dosya adı (`deneme2.sayi` ...); en son çalışanı `son.sayi` olarak kopyala. En çok 12 tur.
- Çalıştırmaları her zaman `./calistir.sh` ile yap (tur sayısı ve hata katmanı kayıtlanır).
- Bitirdiğinde rapor ver: (1) her tur için ne yanlıştı ve nedeni; hata katmanı: **temsil** (yanlış kod/sayaç/yapı sayısı/indeks), **tablo geçerliliği** (yükleyicinin reddi), **algoritma** (çalışıyor ama sonuç yanlış), **ifade gücü** (tabloda istediğini ifade edememe); (2) çözümün kaç sayıdan oluştuğu ve hangi yapıları kullandığın (kaç şablon, davranış, işlev, dizi, döngü); (3) en zor bulduğun noktalar ve tabloda eksik/belirsiz bulduğun yerler; (4) ne kadar emin olduğun.
