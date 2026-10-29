# PMK1 — kanonik sayısal komut tablosu

**İlke [Kesin]:** Komut ve host tanımlarının tek doğruluk kaynağı `oz/komut/komutlar.pmk` dosyasındaki sayılardır. `src/ops.h`, `src/ops_gen.h`, araçların sözlükleri (`OPNAMES`, `NOPS`, `KIND`, `ARITY`, `OP_*`, `H_*`, `TS_*` …) ve okunabilir görünüm bu sayılardan **üretilir**; tersi geçerli değildir. Okunabilir metin isteğe bağlıdır; düzenlenirse `komut @@yaz` ile sayısal biçime işlenir.

## Araç: `oz/arac/komut.pma` (PMB1 tablosu → ELF)
`stdin` ilk satırı mod seçer (ELF'ler argüman almadığı için):

| Mod | Girdi | Çıktı |
|---|---|---|
| `@@tab` | sayısal tablo | araçların sözlüğü (`doldur` ile `.pma.in` içine işlenir) |
| `@@opsh` | sayısal tablo | C++ için `src/ops_gen.h` (`PM_OPS`, `PM_HOSTS` X-makroları) |
| `@@gor` | sayısal tablo | okunabilir görünüm (düzenlenebilir metin) |
| `@@yaz` | okunabilir görünüm | sayısal tablo (kayıpsız, `gor`'un tersi) |
| `@@kanon` | sayısal tablo | aynı tablo, kanonik yerleşimle (başlık, bölüm başlıkları, kayıt başına satır) |
| `@@dogrula` | sayısal tablo | `1` ya da `0 <kod>` (biçim + araç profili) |

## Sayısal biçim (sürüm 1.0)
Tüm değerler tam sayıdır (`pm liste` ile aynı tür metin: boşlukla ayrılmış ondalık sayılar).

```
1347242801  major  minor  nbolum          # "PMK1"
bolum:  id  bayrak  surum  uzunluk  yuk[uzunluk]        # bayrak bit0 = ZORUNLU
yuk:    kayit_sayisi  kayit...
kayit:  uzunluk(kalan jeton)  alanlar...  ne  (anahtar,deger)*ne  bilinmeyen_kuyruk...
```
| Bölüm | Kayıt alanları |
|---|---|
| 1 KIND (operand türleri) | `id  karakter  bayrak` |
| 2 OP (komutlar) | `opkod  semlen sembol..  adlen ad..  nk  tur_id*nk` |
| 3 HOST (sistem işlevleri) | `id  arguman  adlen ad..` ; ext anahtar 1 = yetenek sınıfı etiketi |

Tür bayrakları (bilgi amaçlı, araçlar henüz yalnızca `id`'yi kullanır): 1 yazmaç okur, 2 yazmaca yazar, 4 doğrudan tamsayı, 8 etiket, 16 işlev, 32 sabit havuzu, 64 pencere tabanı, 128 host kimliği.

## Sürümleme ve uyumluluk politikası [Kesin: aşağıdaki davranışlar `dogrula_komut.sh` ile sınanır]
- **major** değişirse okuyucular tabloyu reddeder. **minor** artışı (yeni ek alanlar/bölümler) kabul edilir.
- **Bilinmeyen bölüm** (bilinmeyen kimlik ya da bilinen kimliğin başka sürümü): bayrak bit0 = 0 ise atlanır, `gor`/`yaz` ile **olduğu gibi korunur**; bit0 = 1 (ZORUNLU) ise tablo reddedilir. Böylece eski araç, yeni özelliği yanlış anlayıp çalışmaz; yok sayabileceği şeyleri yok sayar.
- **Kayıt uzantıları:** `ne` + `(anahtar,değer)` çiftleri ve bilinmeyen kuyruk jetonları okuyucular tarafından atlanır ve korunur (kayıt `uzunluk` alanı ile atlanır).
- **Bölüm sırası** serbesttir (KIND/OP/HOST sırası değişse sözlük aynıdır).
- **Kimlikler açıktır:** komut kimliği kayıtta yazılıdır; biçim yoğun olmasını şart koşmaz. Araç zincirinin **profili** (`@@tab`, `@@opsh`, `@@dogrula`) ek olarak komut/host kimliklerinin 0..n−1 yoğun olmasını ve komut başına en çok 3 operand olmasını ister; bu bir araç koşuludur, biçimin değil.

## Genişletme noktaları
- **Yeni komut** = OP bölümüne bir kayıt (+ ELF için `sablon.txt`'e tek şablon satırı). `dogrula_komut.sh` bölüm 7: `FMIN` (d,f,f) yalnızca bu iki değişiklikle assembler, doğrulayıcı ve ELF üreticide çalıştı.
- **Yeni operand türü** = KIND bölümüne kayıt.
- **Yeni host işlevi** = HOST bölümüne kayıt (RT işlevi ya da şablon gerekir).
- **Ayrılmış bölüm kimlik aralıkları (henüz kullanılmıyor):** 4–15 çekirdek (yürütme anlamı, şablon, doğrulayıcı kuralları, yeniden yazma kuralları); 16–63 yürütme modelleri/donanım arka uçları; 64–127 güvenlik (imza, yetenek haritası, politika); 128–255 uygulama/kullanıcı; ≥ 256 özel.
- **Güvenlik katmanları ayrı olmalıdır [Tasarım ilkesi]:** tablo yalnızca *neyin ifade edilebildiğini* tanımlar. *Kimin neyi yapabileceği* (yetki politikası), imza doğrulama, izolasyon ayrı, sürümlü bölümlerde (64–127) ya da tablo dışı politika dosyalarında tutulur. Host kaydındaki ext anahtar 1 yalnızca etiket (1 metin çıkışı, 2 metin girişi, 3 grafik); hiçbir araç bunu henüz kısıt olarak uygulamaz. Güvenilmeyen programlar için zorunlu doğrulama (`valid`) zincirde atlanamaz (`oz/bag/zincir.sh`: doğrulayıcı reddederse ELF üretilmez); imza, yetki sistemi ve izolasyon **henüz yok**.

## Hâlâ metne ya da elle yazılmış kurala bağlı olanlar [Kesin, ayrıntı OZ_BARINDIRMA.md §3i]
Komut *kimlikleri, adları, operand sayıları/türleri ve host listesi* tablodan geliyor. Komutların *anlamı* ve komutlar arası *ilişkiler* değil.
