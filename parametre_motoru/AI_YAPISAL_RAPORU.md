# Yapısal Sayısal Parametre Sistemi (PMY1) ve İlk AI Testi — Rapor

## 1. Ne yapıldı
- Yeni yazım biçimi **PMY1**: metin yok, yalnızca sayılar. Nesne şablonları, alan başlatıcıları, davranışlar, işlevler (özyineleme dahil), global diziler, etkileşimler, olaylar, toplamalar, koşul/döngü/çağrı komutları ve ifade ağaçları, önek ve sayaçlı kayıtlarla. Ofset, etiket, yazmaç numarası yok. Başvuru: `YAPISAL.md`.
- Yükleyici (`src/yapisal.cpp`, 611 satır) tabloyu çözer, doğrular, **bir kez** özelleştirip mevcut hattan geçirir: tablo → `.pma` → ikili tablo → x86-64 makine kodu. Çalışırken tablo yorumlanmaz; mevcut makine kodu üreticisi ve yorumlayıcı değişmedi. Yeni komutlar: `pm yapisal`, `pm tablo` (okunabilir ağaç), `pm run x.pmy`.
- Uygulamaya özel C++ eklenmedi. Çekirdekte tek değişiklik: yükleyiciye `PMY1` tanıma, iki yeni alt komut, fuzz'ın `.pmy` kabulü.

## 2. Doğrulama (kesin)
- Referans tablolarım (histogram, panel, Collatz, özyinelemeli fib + dizi sıralama) bağımsız awk kehanetleriyle birebir tuttu. Panel görevi, 3 farklı olay dosyası + örnek için çıktı ve **her piksel** düzeyinde.
- Yorumlayıcı = makine kodu (3 yazmaç-terfi kipi) = `.pmb`; çıktı, ekran zinciri ve yakıt aynı. `ai_testi3/kontrol3.sh`: 42 denetim geçti. Mevcut çekirdek testleri aynen geçti (203 + 60).
- Bozuk tablo dayanıklılığı: ASan/UBSan'lı yapıda 60 000 rastgele bozulmuş tablo (3 uygulama), çökme yok; geçerli kalanlar güvenle çalıştı veya deterministik tuzakla bitti. Yorumlayıcı/makine kodu farkı: 536 mutant üzerinde 0.

## 3. AI testi: doğrudan yapısal tablo (3 görev, 3 bağlamsız ajan)
Aynı A/B/C görevleri ve aynı gizli kehanetler (önceki testle birebir karşılaştırma için). Üç ajan da ilk komutu çalıştırmadan önce tabloyu yazdı. Araç çağrıları tarandı: kit dışına çıkma, `.pma`/`.pmt`, `pm asm`/`pm sablon`, Python, kod üreten betik yok.

| Görev | İlk deneme | Tur | Hata katmanı |
|---|---|---|---|
| A histogram | doğru | 1 | yok |
| B panel (3 gizli olay dosyası + örnek, piksel dahil) | doğru | 1 | yok |
| C Collatz | doğru | 1 | yok |

### Üç yöntemin karşılaştırması (aynı görev, aynı doğrular, hepsi doğru)
| Görev | Yöntem | Parametre dosyası (bayt) | Yazılan öğe sayısı | Derlenmiş .pmb | Yakıt | Yorumlayıcı / native (ms) | Toplam işlem, native (ms) | Bellek (KiB) |
|---|---|---|---|---|---|---|---|---|
| A | `.pmt` | 917 (metin) | 228 sözcük | 1270 | 56 039 | 6.7 / 4.9 | 10.9 | 12 400 |
| A | sayısal assembly | 214 | 205 sayı | 214 | 4 030 | 6.5 / 6.0 | 10.9 | 11 912 |
| A | **yapısal tablo** | **178** | **171 sayı** | 796 | 24 033 | 5.9 / 4.4 | 10.4 | 12 204 |
| B | `.pmt` | 787 (metin) | 195 sözcük | 1111 | 1 578 568 | 37.7 / 11.7 | 18.4 | 12 480 |
| B | sayısal assembly | 365 | 348 sayı | 365 | 1 553 039 | 22.6 / 16.8 | 19.9 | 12 412 |
| B | **yapısal tablo** | **168** | **159 sayı** | 782 | 1 576 070 | 31.2 / 12.2 | 17.7 | 12 492 |
| C | `.pmt` | 697 (metin) | 170 sözcük | 896 | 4 540 738 | 135.3 / 14.9 | 21.0 | 12 224 |
| C | sayısal assembly | 181 | 179 sayı | 181 | 218 072 | 10.5 / 5.2 | 9.5 | 11 896 |
| C | **yapısal tablo** | **151** | **149 sayı** | 267 | 221 188 | 10.3 / 5.5 | 10.6 | 12 224 |

Okuma:
- **Parametre sayısı** (öğe): yapısal tablo her üç görevde en az (171/159/149); sayısal assembly'ye göre %17 / %54 / %17 az. En büyük kazanç B'de (şablon + ortak ifade + olay): assembly'nin 348 sayısına karşı 159.
- **Parametre dosyası boyutu**: tablo 178/168/151 B; assembly'nin `.pmb`'si 214/365/181 B; `.pmt`'nin derlenmiş hali 1270/1111/896 B (çok daha büyük çünkü şablon açılımı ve genel kural kodu içerir).
- **Derlenmiş `.pmb`**: tablodan üretilen kod assembly'den büyük (796/782/267 B'a karşı 214/365/181 B). Yani küçülen şey *girdi* tablosu, çalışan kod değil; çalışan kodu özelleştirici üretiyor ve genel amaçlı olduğu için biraz şişkin. [Kesin]
- **Yakıt ≠ gerçek süre** (senin uyarın doğrulandı): C'de `.pmt` 21x fazla yakıt harcıyor ama native süre 2.9x, yorumlayıcı süre 13x. A'da tablo assembly'den 6x fazla yakıt harcıyor ama native 4.4 ms'ye karşı 6.0 ms. Karar için süre, yakıt değil.
- **Gerçek süre**: native hep 5–18 ms; çoğu başlatma (Native "toplam işlem" ≈ 10–21 ms, `.pmy` yüklemesi + özelleştirme + makine kodu üretimi dahil). B'de tablo en hızlı (12.2 ms'ye karşı assembly 16.8: assembly her karede tüm 15 360 pikseli yeniden yazıyor, tablo yalnızca dikdörtgenleri çiziyor [Tahmin]).
- **Bellek**: hepsi ~12 MB; fark ölçülebilir değil, bu ölçekte bellek çalıştırıcı tabanı tarafından domine ediliyor. [Kesin]
- Gürültü: ms ölçümleri ±1 ms oynuyor (en iyi-3 değeri).

### Hangi aşamada sorun yaşandı?
| Aşama | Sonuç |
|---|---|
| Sayısal temsil üretimi (kod/sayaç/yapı/indeks) | 0 hata (3/3) |
| Tablo geçerliliği (yükleyici reddi) | 0 ret |
| Algoritmik doğruluk | 0 hata |
| İfade gücü (istediğini tabloda yazamama) | 0; A ajanı "başlatıcılar yalnızca doğrusal/mod/böl" notunu düştü (kural bloğunda kare ile çözdü) |
| Belge belirsizliği (ajanların not ettiği) | 3 küçük nokta; `YAPISAL.md` §8'e eklendi |
[Kesin] Hata çıkmadığı için katmanlar yine ayırt edilemedi.

## 4. En önemli bulgu — "yeniden kullanım" hedefi
- C ajanı **şablon, davranış, toplama kullanmadı**: tek işlev + tek döngülü klasik program yazdı. Bu görev buna izin veriyordu ve en kısa yol buydu. A ve B ajanları şablon/toplama/olay kullandı (B: tek şablon, 5 olay, çizimde `indis` ifadeleri; davranış yok — `min`/`max` ile tek atama yazdı).
- Yani sistem ortak yapıları destekliyor, ama AI görev gerektirmedikçe onları seçmiyor. [Kesin] Davranış (makro) ve işlev hiç kullanılmadı: ajanlar bu görevlerde onlara ihtiyaç duymadı. "Az sayıda anlamlı parametreyle ortak kural" hedefi, ancak bu yapıları gerektiren görevlerle (çok varlıklı, tekrarlayan davranışlı, iç içe işlevli) sınanabilir.
- Üç görevde tablo, sayısal assembly'den ancak %17–54 küçük. 200 / 2 000 / 20 000 komutluk bir uygulamada farkın büyümesi **beklenti**; ölçülmedi. [Tahmin]

## 5. Dürüst değerlendirme
1. Görevler hâlâ küçük (tek döngü / tek şablon). 3/3 ilk deneme, sistemin çalıştığını ve AI'nın bu biçimi belgeden öğrenebildiğini gösteriyor; tavanı göstermiyor.
2. `YAPISAL.md` çok açık: her kod, her bağlam kuralı yazılı, iki açıklamalı örnek var. Belgesiz sonuç bilinmiyor.
3. Aynı model ailesi, aynı kuralları yazan taraf.
4. İzolasyon yine kısmen güvene dayanıyor: bu kez A/B/C'nin kehanetleri ve önceki çözümleri depoda açıktı (şifreli değildi). Ajanların tüm dosya erişimleri tarandı ve yalnızca kit dosyaları görüldü. Sonuç yine de "bağımsız doğrulama" için kusurlu: yeni, şifreli kehanetli görevlerle tekrar etmek gerek.
5. Bu hâlâ klasik programlama mantığının sayılarla anlatılması mı? Kısmen. İşlev/döngü/koşul düzeyinde evet (C buna indi); şablon/olay/toplama düzeyinde hayır (A, B). Fark: AI yazmaç, ofset, adres saymıyor; yapı bildirimsel olduğunda parametre sayısı 2x'e kadar küçülüyor.
6. Sınırlar: yalnızca tamsayı, nesne "canlı" bayrağı yok, davranışlar çağrı-adıyla (makro) açılır, dizi/şablon indeksi çalışma zamanında denetlenir (bedeli ölçülmedi).

## 6. Sonraki adım (onay bekliyor)
Kullanıcının planı: büyük ve zor genel amaçlılık/dayanıklılık testi. Önerim, görevleri **ortak yapıyı zorlayacak** biçimde seçmek ve tabloyu `.pmt` + assembly ile aynı kıstaslarla kıyaslamak: çok varlıklı (≥ 50 tür davranış), iç içe işlev + dizi + özyineleme, kullanıcı etkileşimi; yeni, şifreli kehanetlerle; ve ajan başına birden çok bağımsız deneme (başarı oranı için).
