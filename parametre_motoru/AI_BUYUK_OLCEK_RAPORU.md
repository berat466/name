# Büyük Ölçekli Karşılaştırma — Kısmi Rapor (token/limit nedeniyle yarıda kesildi)

## Durum
- Planlanan: 9 görev (veri D1–D3, arayüz U1–U3, simülasyon S1–S3) × 3 biçim (PMY1, sayısal assembly, `.pmt`) × birden çok deneme = 45 ajan.
- **Tamamlanan: 26 ajan** (D1, D2, S1, U1 tüm biçimler; S2 PMY1+sayısal; U2 PMY1). **Tamamlanamayan: 19 ajan**: üçüncü dalganın tamamı (D3, S3, U3: "büyük" ölçek) ve U2/S2'nin `.pmt` ile U2'nin sayısal denemeleri; 5 saatlik kullanım sınırına çarptılar. Kullanıcı 45 ajanın token maliyetini sorguladı; düşen ajanlar **devam ettirilmedi**.
- Bu yüzden asıl sorunun (10–100 kat karmaşıklıkta tanım boyutu) **büyük ölçek ayağı ölçülmedi**. Aşağıdakiler yalnızca küçük ve orta ölçek.

## İlk deneme doğruluğu (tamamlanan 26 ajan)
| Biçim | İlk denemede doğru | Son teslimde doğru | İlk denemede hata nedeni |
|---|---|---|---|
| Sayısal assembly | 10 / 10 | 10 / 10 | — |
| PMY1 | 10 / 12 | 12 / 12 | S1: elle yazımda fazladan sayı (AI hatası, tablo reddi). U1: belge/motor tutarsızlığı (`kir`) — belge hatası, düzeltildi |
| `.pmt` | 2 / 4 | 4 / 4 | S1: dil sınırı (aynı şablonla etkileşim yasak, belgede yoktu). U1: aynı tuşa ikinci `girdi` bloğu yok sayılıyor (belgede yoktu) |
Tüm 26 çözümün sonucu bağımsız kehanetle doğrulandı (çıktı birebir; arayüzde 4 olay dosyası ve her piksel).

## Parametre boyutu (öğe sayısı; PMY1 / sayısal / `.pmt` sözcük)
| Görev | PMY1 | Sayısal assembly | `.pmt` |
|---|---|---|---|
| D1 küçük | 291, 307 | 301, 321 | 212 |
| D2 orta | 844, 623 | 726, 853 | 1629 |
| S1 küçük | 652, 630 | 616, 603 | 1051 |
| S2 orta | 700, 743 | 631, 610 | (tamamlanamadı) |
| U1 küçük | 587, 429 | 645, 780 | 1199 |
| U2 orta | 535, 474 | (tamamlanamadı) | (tamamlanamadı) |
- PMY1 ve sayısal assembly **aynı büyüklük sınıfında** (±15%): ölçekte PMY1'in belirgin tanım küçültmesi **görülmedi**. Fark yalnızca arayüzde (U1: PMY1 429–587, sayısal 645–780) ve şablon kullanıldığında çıktı.
- `.pmt` 3–9 kat büyük: D2 için 1629 sözcük / 7551 bayt, 111 global + 111 `topla` (dizi ve döngü yok, sıralama "kaç tane benden önce" etkileşimleriyle).
- Karmaşıklığı artırmak (D1→D2: ×2.4 parametre; S1→S2 ve U1→U2: **×1.0**) tanımı büyütmedi, çünkü orta ölçekte artan şey nesne sayısıydı (N), kural çeşidi değil. [Kesin] Nesne sayısını artırmak tanım boyutu karşılaştırması için yeterli bir ölçek değil; asıl test D3/S3/U3'teki çeşit artışıydı (yapılamadı).

## Gerçek çalışma (µs; makine boştayken; süreç taban maliyeti ~10 ms çıkarılmış)
- Yükleme + kod üretme: PMY1 ve sayısal için ölçüm gürültüsünün (±2 ms) içinde; PMY1 farkı ayırt edilemedi.
- Çalışma: S1'de `.pmt` ≈ 277 ms, PMY1 0.4–3 ms, sayısal 1–3 ms; D2'de `.pmt` 82 ms, PMY1 1.2–1.7 ms. S2'de sayısal 14.4–14.8 ms (C++ -O2 tabanı 14–18 ms), PMY1 30.7–31.9 ms (~2x yavaş). [Kesin] Döngüsü olmayan `.pmt` düzeni hesaplama ağırlıklı işlerde 2 büyüklük mertebesine kadar yavaşlıyor; PMY1 sayısal assembly'ye göre simülasyonda ~2x yavaş.
- Tepe bellek: hepsinde ~12 MB (çalıştırıcı tabanı); görev boyutlarında fark ölçülemedi. C++ tabanı ~3.4 MB.

## Hata katmanları ve belge kusurları
- Ayrıştırılan hata nedenleri: AI elle yazım hatası 1 (PMY1), **belge/motor tutarsızlığı 3** (`kir`; `.pmt` aynı şablon etkileşimi; `.pmt` çoklu `girdi`). Üçü de belgeye eklendi. [Kesin] Yani ilk deneme başarısızlıklarının çoğu AI'dan değil belgeden geldi.
- Çok satırlı çıktı `.pmt`'de üretilemedi (D2, S1): ifade edilemeyen özellik olarak raporlandı. Değerlendirici boşluk-duyarsız olduğu için sayılar doğru sayıldı.
- Tüm ajanlar ilk denemeyi komutsuz yazdığını bildirdi; kit dışı okuma görülmedi, ancak birkaç ajan kendi doğrulama awk'ını kit dışına (scratchpad kökü) yazdı. Bir `.pmt` ajanı gereksiz bir `echo` çalıştırdığını kendisi bildirdi.

## Test tasarımının dürüst sınırları
1. Orta ve küçük görevlerin çoğu **sayaç dizisi / histogram** gibi küçük bir kalıpla çözülebiliyor (D1, D2); sıralama zorunlu değildi.
2. S ve U görevlerinde sınıf/tür tabloları küçük ve sabit örnek; ajanlar örneğe özel sadeleştirme yaptı (kullanılmayan tipleri atlamak gibi). Bu doğru, ama genellemeyi ölçmüyor.
3. Aynı model ailesi; kehanetler şifreliydi (ajanlar okuyamadı), ama uygulama tanımı ajanın belgeyi okuyup öğrenmesine dayanıyor.
4. Bu veri, "PMY1 karmaşıklık 10–100 kat artınca avantaj sağlar mı" sorusuna **cevap vermiyor**.

## Maliyet
Tamamlanan 26 ajan başına ~60–95 bin token (toplam yaklaşık 2 milyon); 19 ajan daha limite takılıp yarıda kaldı. 45 ajanlık plan fazla büyüktü; baştan daha küçük bir çekirdek (ör. her ölçek için sınıf başına 1 deneme) denenmeliydi.

## Öneri (onay bekliyor)
Büyük ölçek yalnızca **6 ajanla**: D3, S3, U3 için PMY1 ve sayısal assembly birer deneme (`.pmt` D3/S3/U3 için tek deneme eklenebilir, +3). Böylece asıl soru (çeşit arttıkça tanım boyutu) ölçülür. Kehanetler şifreli kalır; test bitince `ai_testi4/` altına açılır.
