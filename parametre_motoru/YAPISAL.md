# Yapısal sayısal parametre tablosu (PMY1) — başvuru

Bir uygulama **yalnızca sayılardan oluşan yapısal bir tablo** olarak anlatılır. Metin, ad, etiket, atlama ofseti, yazmaç numarası yoktur.
Tablo yüklenirken motor onu bir kez doğrular, özelleştirir ve makine koduna çevirir; çalışırken tablo yorumlanmaz. Dosyayı baytlara `paketle.awk` çevirir (yalnızca sayı → bayt; mantık üretmez).

## 1. Yazım
Boşlukla ayrılmış sayılar, `#` satır sonuna kadar açıklama. `N` işaretsiz tamsayı (sayaç, indeks, kod); `zN` işaretli tamsayı (`z-5`, `z0`, `z100`) — hangisinin gerektiği aşağıda belirtilir. Yapı **sayaçlıdır**: bir liste `SAYAÇ, öğe, öğe, ...` biçimindedir.
Çalıştırma: `./calistir.sh dosya.sayi [pm run bayrakları]`. Okuma aracı: `./pm tablo dosya.pmy` tabloyu ağaç olarak okunur basar (motorun tabloyu nasıl çözdüğünü gösterir).

## 2. Dosya düzeni (sırayla)
```
80 77 89 49                  "PMY1"
W H BG KARE                  sahne: W,H piksel (ekran yoksa 0 0); BG arkaplan 0xRRGGBB (onluk sayı); KARE = kare sayısı (0: sınırsız, bitis/present ile biter)
NGLOBAL  NGLOBAL × zDEGER    global tamsayılar (indeks 0..)
NDIZI    NDIZI × (BOYUT NVER NVER×zDEGER)       global diziler (indeks 0..); ilk NVER eleman başlangıç değeri, kalanı 0
NISLEV   NISLEV × (NPARAM NYEREL BLOK)           işlevler (indeks 0..); yerel 0..NPARAM-1 = parametreler; NYEREL >= NPARAM
NDAVR    NDAVR × (NPARAM BLOK)                   davranışlar = parametreli ortak kural kalıbı (indeks 0..)
NSABLON  NSABLON × SABLON                        nesne şablonları (indeks 0..)
NETK     NETK × (A B KOSUL BLOK)                 etkileşimler: A ve B şablon indeksleri
NGIRDI   NGIRDI × (TUS SABLON+1 BLOK)            olaylar: TUS kodu; SABLON+1 = 0 ise genel, değilse o şablonun her örneğine uygulanır
NTOPLA   NTOPLA × (GLOBAL ISLEM SABLON DEGER KOSUL)   toplamalar
BITIS    0  ya da  1 IFADE                       bitiş koşulu (1 ise ardından IFADE)
ILK      BLOK                                    başlangıçta bir kez çalışır
SON      BLOK                                    sonda bir kez çalışır
```
**SABLON** = `ADET NALAN NALAN×ALAN KURALBLOK NCIZIM NCIZIM×CIZIM`
- `ALAN` (başlatıcı, `i` = örnek numarası 0..ADET-1): `1 zC` sabit C · `2 zA zB` A + B·i · `3 M zA zB` A + B·(i % M) (M işaretsiz ≥1) · `4 M zA zB` A + B·(i / M) · `5 zLO zHI zTOHUM` deterministik rastgele LO..HI (dahil).
- `KURALBLOK`: her karede, her örnek için sırayla çalışan komutlar (bölüm 4). Boş olabilir.
- `CIZIM`: `1 x y w h renk` (dolu dikdörtgen; 5 ifade) ya da `2 x y renk` (nokta; 3 ifade). Her ifade o örnek bağlamındadır.
**ISLEM** (toplama): 1 toplam · 2 say · 3 enbuyuk · 4 enkucuk. Her karede, DEGER ve KOSUL o şablonun her örneği için değerlendirilir; KOSUL ≠ 0 olanlar toplanır (`say` DEGER'i kullanmaz ama bir ifade yazılmalıdır). Boş küme: toplam/say = 0, enbuyuk = −2⁶², enkucuk = 2⁶². Sonuç GLOBAL'e yazılır.

## 3. İfadeler (önek biçim, her biri `KOD args...`)
| kod | ad | yazım | anlamı |
|---|---|---|---|
| 1 | sayi | `1 zV` | sabit |
| 2 | alan | `2 F` | bağlamdaki örneğin F. alanı |
| 3 | global | `3 G` | G. global |
| 4 | yerel | `4 L` | L. yerel değişken (0..255) |
| 5 | indis | `5` | bağlamdaki örnek numarası i |
| 6 | kare | `6` | kare sayacı (0 tabanlı: ilk karede 0) |
| 7 | aalan | `7 F` | etkileşimde A örneğinin F. alanı |
| 8 | balan | `8 F` | etkileşimde B örneğinin F. alanı |
| 9 | aindis | `9` | etkileşimde A örnek numarası |
| 10 | bindis | `10` | etkileşimde B örnek numarası |
| 11 | talan | `11 T F IFADE` | T. şablonun IFADE. örneğinin F. alanı (her yerden; sınır denetimli) |
| 12 | dizi | `12 D IFADE` | D. dizinin IFADE. elemanı (sınır denetimli: dışı = OOB tuzağı; sabit indeks yüklenirken denetlenir) |
| 13 | cagri | `13 FN N IFADE×N` | FN. işlevi çağırır, döndürdüğü değer |
| 14 | param | `14 P` | davranışta P. parametre (çağıranın verdiği ifade, yerine konur) |
| 15 | ikili | `15 OP E1 E2` | OP aşağıda |
| 16 | tekli | `16 OP E` | 1 eksi · 2 bit tümleyen · 3 değil (0↔1) |
| 17 | sec | `17 C E1 E2` | C ≠ 0 ise E1, değilse E2 (yalnız seçilen değerlendirilir) |

İkili `OP`: 1 `+` 2 `-` 3 `*` 4 `/` 5 `%` 6 `&` 7 `|` 8 `^` 9 `<<` 10 `>>` (aritmetik) 11 `==` 12 `!=` 13 `<` 14 `<=` 15 `>` 16 `>=` 17 `min` 18 `max` 19 `ve` 20 `veya`. Karşılaştırmalar 0/1 verir. `ve`/`veya` iki tarafı da değerlendirir ve 0 olmayanı "doğru" sayar. Tamsayı 64 bit, taşmada sarar; `/` ve `%` sıfıra doğru kırpar; sıfıra bölme = DIV0 tuzağı.
Atanabilir ifadeler (atama hedefi): `alan`, `aalan`, `balan`, `global`, `yerel`, `dizi`, `talan`; `param` olarak verilen argüman da atanabilir olmalıdır.

## 4. Komutlar ve BLOK
`BLOK = N KOMUT×N`.
| kod | ad | yazım | anlamı |
|---|---|---|---|
| 1 | ata | `1 HEDEF IFADE` | HEDEF = IFADE |
| 2 | eger | `2 KOSUL BLOK_SE BLOK_YOKSA` | KOSUL ≠ 0 ise ilk blok, değilse ikinci (boş `0`) |
| 3 | surde | `3 KOSUL BLOK` | KOSUL ≠ 0 olduğu sürece tekrarla |
| 4 | say | `4 L IFADE_BAS IFADE_SON BLOK` | yerel L, BAS..SON (dahil, +1 adımla; SON bir kez hesaplanır) |
| 5 | cagir | `5 FN N IFADE×N` | işlevi çağır, sonucu at |
| 6 | davran | `6 DV N IFADE×N` | davranışı bu noktaya, parametreleri verilen ifadelerle açar |
| 7 | don | `7 IFADE` | işlevden değer döndür (yalnız işlev içinde); `don` olmadan biten işlev 0 döndürür |
| 8 | yazi | `8 IFADE` | ondalık sayı yazar (ayraçsız) |
| 9 | karakter | `9 IFADE` | bir bayt yazar (32 boşluk, 10 satır sonu) |
| 10 | kir | `10` | en içteki döngüden çık |
| 11 | devam | `11` | en içteki döngünün sonraki turuna geç |
| 12 | satir | `12 N IFADE×N` | N sayıyı boşlukla ayırıp yazar, sonuna satır sonu koyar |

`kir`/`devam` şablon/etkileşim/girdi gövdesinin en üst düzeyinde: "bu örneğin geri kalanını atla".

## 5. Çalışma modeli
1. Alanlar başlatıcılarla doldurulur, dizi başlangıç değerleri yazılır; **ILK** bloğu çalışır.
2. Kare döngüsü vardır eğer sahne (W>0), KARE>0 ya da BITIS varsa. Her karede sırayla: **olaylar** (`poll` ile gelen tüm olaylar sırasıyla; olayın tuş kodu TUS'a eşit olan *her* girdi kaydı tablodaki sırayla çalışır; şablonlu girdi her örnek için) → **şablon kural blokları** (şablon sırasıyla, örnek-öncelikli) → **etkileşimler** (tablodaki sırayla; her (a, b) çifti için KOSUL ≠ 0 ise BLOK) → **toplamalar** (tablo sırasıyla) → **BITIS** (≠ 0 ise döngüden çık; o karede çizim yok) → ekran varsa arkaplanla temizle, tüm şablonların çizimleri (şablon sırasıyla, örnek sırasıyla; sınır dışı kırpılır), kareyi sun. KARE dolunca ya da ekran sunumu 1 dönünce biter.
3. **SON** bloğu çalışır.
Kare döngüsü yoksa yalnızca ILK ve SON çalışır; bu durumda şablon/olay/etkileşim/toplama/çizim tanımlanamaz.
Girdi olayı: `--girdi dosya` (satır: `kare tus`); olay, sunulan kare sayısı olayın karesine ulaşınca teslim edilir.
Yerel değişkenler: işlevlerde NYEREL tane; diğer bloklarda (kural, olay, ILK, SON ...) 0..15. İlk değer 0'dır. Şablon/olay gövdelerinde yereller örnekler arasında korunur (her örnek için sıfırlanmaz); her karede blok işlevi yeniden başlar ve yereller sıfırlanır.
Alan "yazılıyor mu?" otomatik anlaşılır: hiçbir komutta atanmayan alan bellekte tutulmaz (başlatıcı formülden hesaplanır). Bu görünmez bir iyileştirmedir.
Davranış = makro: `14 P` yerine çağıran ifadeyi koyar (her kullanımda yeniden değerlendirilir); davranış gövdesindeki `alan`, `indis` vb. çağıranın bağlamında çözülür. Davranış içinde başka davranış çağrılabilir, özyineleme yasak. İşlevler özyinelemeli olabilir (derinlik sınırı aşılırsa DEPTH/STACK tuzağı).

## 6. Hata iletileri
Tablo hatası `tablo: sayi #N (bayt B): ...` biçiminde N. sayıyı gösterir (başlıktaki `80 77 89 49` sayılmaz; `zN` tek sayıdır). Doğrulama hataları (`tablo: sablon 2 kurallari: ...`) hangi bölümde olduğunu söyler.

## 7. Örnekler
`ornek_toplam.sayi` (işlev + döngü + yazdırma, ekransız) ve `ornek_sahne.sayi` (ekran + şablon + olay + çizim) kitte vardır; her satır açıklamalıdır.

## 8. Sık sorulanlar (AI testinde sorulan noktalar)
- Ekransız uygulamada `KARE=1` kare döngüsünü tam bir kez çalıştırır (olaylar yok, kurallar, etkileşimler ve toplamalar bir kez, sonra SON). Kare döngüsü gerekmiyorsa `KARE=0` ve şablonsuz bırak (yalnız ILK/SON).
- Dizide `NVER=0` geçerlidir (tüm elemanlar 0). Sabit değerler ifade içinde `1 zV` ile yazılır; dizi indeksi de bir ifadedir (`1 zK` ya da başka ifade).
- Toplama, aynı karede şablon kuralları ve etkileşimlerden sonra çalışır; toplama sonucu aynı karede BITIS ve SON tarafından görülür.
- `satir`/`yazi` çıktısı `calistir.sh` ile stdout'a gider.
