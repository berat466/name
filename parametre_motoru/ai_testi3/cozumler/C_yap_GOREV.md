# GÖREV C — Collatz Taraması (sayısal hesaplama)

n = 1, 2, ..., 3000 için Collatz adım sayısını bul: x = n ile başla; x != 1 olduğu sürece, x çiftse x = x / 2, tekse x = 3*x + 1 yap ve adım sayısını 1 artır. St(n) = x'in 1'e ulaşmasına kadarki adım sayısıdır (St(1) = 0).

Yazdırılacaklar (5 sayı, bu sırayla, **tek satırda**, birer boşlukla ayrılmış, sonda satır sonu):
1. St(27)
2. St(n)'in en büyük olduğu n (eşitlikte en küçük n)
3. o en büyük adım sayısı
4. n = 1..3000 için St(n) toplamı
5. St(n) > 100 olan n'lerin sayısı

Sonuçlar **hesaplanmalıdır**; sayıları hazır yazmak (hesabı başka yerde yapıp sabit gömmek) geçersizdir. Doğrulamada uygulamanın yaptığı iş (yakıt/adım sayısı) ölçülecektir.

**Tabloda:** ekran yok (W=0 H=0), kare döngüsü gerekmeyebilir; çıktı `satir` ile. Çalıştırma: `./calistir.sh son.sayi`.

## Teslim ve kurallar (YAPISAL TABLO KİPİ)
- Okuman gereken: `YAPISAL.md` (tablo biçimi), `ornek_toplam.sayi`, `ornek_sahne.sayi`, `paketle.awk`, `calistir.sh`. Teslim: **tek dosya `son.sayi`** (yalnızca sayılar ve `#` açıklamaları; `PMY1` tablosu).
- **Yapısal tabloyu doğrudan sayı olarak yaz.** YASAK: `.pmt` ya da `.pma` metni yazmak/dönüştürmek, `pm asm`, `pm sablon`; sembolik ad/etiketi sayıya çeviren ya da tablo/komut üreten herhangi bir betik/program (awk, bash, C++ ...) yazmak; Python.
- İZİN: `./calistir.sh` ile çalıştırmak; `./pm tablo x.pmy` ile motorun tabloyu nasıl çözdüğünü okumak; `od`; **beklenen sonucu bağımsız hesaplamak için** küçük awk (uygulamayı üretmeyen, yalnızca sayı hesaplayan).
- Depodaki (`/home/user/name/...`) başka dosyaları, özellikle `ai_testi*/`, `uygulamalar/`, `src/` dizinlerini OKUMA; yalnızca bulunduğun kit dizinini kullan.
- Hedef: uygulamayı mümkün olduğunca **az sayıda anlamlı parametreyle** anlat. Binlerce nesneyi tek şablon, tekrarlanan kuralı davranış/işlev olarak ifade et; düşük seviyeli adımları tek tek yazmak yerine tabloda bulunan yapıları (şablon, alan başlatıcıları, olay, toplama, davranış, işlev, döngü, dizi) kullan.
- **İlk denemeyi hiçbir komut çalıştırmadan** yaz (`deneme1.sayi`). Sonra çalıştır; hata varsa düzelt. Her düzeltme turu için yeni dosya adı (`deneme2.sayi` ...); en son çalışanı `son.sayi` olarak kopyala. En çok 12 tur.
- Çalıştırmaları her zaman `./calistir.sh` ile yap (tur sayısı ve hata katmanı kayıtlanır).
- Bitirdiğinde rapor ver: (1) her tur için ne yanlıştı ve nedeni; hata katmanı: **temsil** (yanlış kod/sayaç/yapı sayısı/indeks), **tablo geçerliliği** (yükleyicinin reddi), **algoritma** (çalışıyor ama sonuç yanlış), **ifade gücü** (tabloda istediğini ifade edememe); (2) çözümün kaç sayıdan oluştuğu ve hangi yapıları kullandığın (kaç şablon, davranış, işlev, dizi, döngü); (3) en zor bulduğun noktalar ve tabloda eksik/belirsiz bulduğun yerler; (4) ne kadar emin olduğun.
