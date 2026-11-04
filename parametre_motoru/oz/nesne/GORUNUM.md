# PMO1 okunabilir görünüm (PMV) ve sayısal düzenleme

**İlke [Kesin]:** asıl kaynak sayısal nesnedir (`oz/nesne/*.pmo`: `asm`, `valid`, `gen2`). Okunabilir metin **isteğe bağlı görünümdür**, kayıpsızdır ve derleme zinciri onu kullanmaz:

```
sayısal parametreler (.pmo) --gor--> okunabilir görünüm (.pmv) --düzenle--> --yaz--> güncellenmiş sayılar (.pmo)
        --bağlayıcı (+ komut tablosu)--> PMB1 (.pmb) --gen2--> yeni makine kodu (ELF)
```

Eski C++ ya da `.pma/.pma.in` kaynağına dönmeden, motorun kendi algoritması (assembler, doğrulayıcı, makine kodu üretici) okunup değiştirilebilir. `gor → yaz` her nesnede **bayt-bayt kimliktir** (üç depo nesnesinde ve rastgele nesnelerde sınanır).

## Araçlar

| Dosya | Görev |
|---|---|
| `oz/arac/gorunum.pma` → `gorunum.pmb` (ELF'e çevrilebilir) | `@@gor` (sayılar → metin), `@@yaz` (metin → sayılar), `@@adlar` (metinden yan dosya) |
| `oz/nesne/duzenle.sh` | kullanıcı arayüzü: `gor`, `yaz`, `uygula`, `elf` |
| `oz/nesne/*.adlar` | yan dosya (sidecar): işlev adları ve bss bölgeleri; **sayısal nesnenin parçası değildir** |
| `oz/nesne/ref/pmo_ref.py` | bağımsız referans (Python): aynı görünüm, aynı nesne, aynı hata kodu |
| `oz/nesne/ref/rastgele_pmv.awk`, `mutpmv.awk` | test üreteçleri (rastgele görünüm, bozulmuş görünüm) |

```
oz/nesne/duzenle.sh gor    valid.pmo valid.pmv     # sayılar → okunabilir metin (valid.adlar varsa adlarla)
$EDITOR valid.pmv                                  # algoritmayı düzenle
oz/nesne/duzenle.sh uygula valid.pmv valid         # sayılar yazılır, valid.pmb yeniden üretilir (hata olursa eski dosyalar geri konur)
oz/nesne/duzenle.sh elf    valid /tmp/valid.elf    # yeni makine kodu (bağımsız ELF)
```
Ortam: `PM` (pm yolu) ya da `ARAC` (bağımsız `.elf` araçları dizini; `pm` gerekmez); `OPT` (komut tablosu).

## Görünüm biçimi

Satır tabanlı; `;` satır sonuna kadar yorumdur; ayırıcılar boşluk, sekme ve virgüldür. Sayılar **kanonik onluk** olmalıdır (`007`, `-0`, `+5` reddedilir; bu sayede `gor → yaz` kimlik kalır).

```
pmo 1
flags 0                       ; PMB1 BAYRAK (1 = işlev adları PMB1 içinde)
memory 12582912
entry main                    ; işlev adı ya da #indeks
key ARITY array               ; anahtarlar (sıra = anahtar numarası); scalar | array
key OPCOUNT scalar
const 9223372036854775808 ... ; işaretsiz 64 bit; $ANAHTAR de olur
data
  array OPNAMES               ; dizi başlığı (bağlayıcı bu yere dizinin elemanlarını koyar)
  words 109 101 ...           ; işaretli 64 bit sözcükler, satırda en çok 16; $ANAHTAR / -$ANAHTAR olur
bss 10312192
space SRC 0                   ; bss bölgeleri (yan dosyadan): ad, bss tabanına göre ofset
func rewrite 1 37             ; ad, parametre sayısı, yazmaç sayısı
  ldi r32, &G
  addi r1, r0, &IRMN
L11:                          ; etiketler sembolik: araya komut eklenince atlamalar kendiliğinden düzelir
  gei r5, r2, $OPCOUNT
  jz r5, L20
  call err, 33                ; işlev çağrısı ad ile
end
```
Komut adları, operand sayıları ve operand türleri **komut tablosundan** (`komut @@tab`: `OPNAMES`, `NOPS`, `KIND`, `KFLAGS`) okunur; tabloya yeni komut eklenince görünüm onu tanır (sınanır: `xori`).

### Operand sözdizimi (tür bayrağına göre)
| Tür (KFLAGS) | Yazım |
|---|---|
| yazmaç (1/2) | `r<n>` |
| doğrudan sayı (4) | `123`, `-5`, `$ANAHTAR`, `-$ANAHTAR`, adres ifadeleri (aşağıda) |
| etiket (8) | `L<n>` / kullanıcı adı; sınır dışı hedef `@n` |
| işlev (16) | işlev adı; sınır dışı `#n` |
| sabit havuzu (32) | `c<n>` |
| host (128) | host adı (`putc`, `puti`, …, tablodan) ya da sayı |
| diğer | düz sayı |

### Adres ifadeleri (bağlayıcıya görünür; dizi uzunlukları değişince adresler kendiliğinden kayar)
PMO1 nesnesinde bir adres `taslak değer + (dizi uzunluğu − 1) · katsayı` toplamıdır ve **bağıl kayıtla** işaretlidir. Görünüm bunu simgesel yazar (bellek adresi 1'den başlar: veri indeksi `i` → adres `i+1`):

| Yazım | Anlam |
|---|---|
| `&ANAHTAR[+n]` | dizinin başlangıcı (+ n sözcük) |
| `~ANAHTAR[+n]` | dizinin **hemen sonrası** (+ n) — veri sözcükleri, dizinin ardından gelen bölge |
| `&BÖLGE[+n]` | bss bölgesi (yan dosyadaki `space` adı) |
| `&bss[+n]` | bss tabanı (bölge adı yoksa) |
| `n{ANAHTAR*katsayı\|…}` | ham biçim: yukarıdakilere uymayan her bağıl kayıt **kayıpsız** burada yazılır |

Her ifade tek biçimde yazılır; `gor` en okunaklısını seçer, `yaz` hepsini aynı sayılara çevirir.

## Hata kodları (`HATA: kod` + DIV0 tuzağı; hiçbir hatada kısmi çıktı yazılmaz)
- **gor (sayı → metin):** 2 sözlük/giriş biçimi · 3 bozuk/kesik sayı · 4 sihirli sayı/sürüm · 5 sınır (4096 anahtar, 65536 bağıl kayıt, 524288 sayı, 4096 işlev) · 6 anahtar türü/tabanı · 7 anahtar adı · 8 bağıl kayıt geçersiz/yanlış yerde · 9 bağıl kayıt skaler anahtarda ya da yer tutucu değerinde/tekrarda · 10 yer tutucu desteklenmeyen alanda · 11 dizi başlığı tekrarı · 12 liste yapısı/aralık · 13 PMB1 işlev adı geçersiz · 14 yan dosya geçersiz.
- **yaz (metin → sayı):** 30 sayı biçimi · 31 sayı aralığı · 40 beklenen satır yok · 41–44 başlık satırları · 45 `key` · 46 `$ANAHTAR` geçersiz · 47 sayı yer tutucu değeriyle çakışıyor · 48 `array` · 49 `bss` · 50 `func` başlığı · 51 `end` · 52 işlevlerden sonra artık satır · 53 işlev adı tekrarı · 54 `entry` · 55 `&`/`~` başvurusu · 56 ham bağıl kayıt · 57 doğrudan sayı biçimi · 58 etiket · 59 bilinmeyen komut · 60 operand sayısı · 61 yazmaç · 62 etiket yok · 63 işlev yok · 64 sabit havuzu biçimi · 65 operand int32 dışı · 66 `space` · 67 işlev adı > 256 (flags 1) · 5 sınır.

## Dürüst sınırlar [Kesin]
- **Adresler yalnızca bağıl kayıtlıysa simgeseldir.** İlk dizi başlığından önceki bölgeye (başlıktan önceki sözcükler, ilk dizinin kendisi) yapılan başvurularda nesne bağıl kayıt taşımaz; bu adresler düz sayı olarak kalır (`ldi r0, 3`). Başlıktan sonraki her şey (diğer diziler, sonraki sözcükler, bss) simgeseldir.
- **Yazmaç adları yok.** Yazmaçlar numaradır (`r17`); atama sırası kaynak adlarını taşımaz. Yan dosya yalnızca işlev adlarını ve bss bölgelerini taşır. (Genişletme: yan dosyaya `reg` satırları eklenip yorum olarak basılabilir; derlemeye etkisi olmaz.)
- **Skaler anahtarlar yalnızca `$ANAHTAR` / `-$ANAHTAR` olarak** (tam yer tutucu değeri) desteklenir; `$ANAHTAR+3` gibi toplamlar ve skaler bağıl kayıtlar nesne biçiminde zaten üretilmez, gelirse reddedilir (kod 9/10).
- Veri başlığı her dizi anahtarı için **en çok bir kez**; sentinel çakışması (düz sayı yer tutucu aralığına düşerse) reddedilir (bağlayıcı da onu yer tutucu sanırdı).
- Yan dosya (`.adlar`) kaynak metinden (`ice_aktar.sh`) üretilir; elle düzenlenmiş nesnelerde `yaz` onu görünümden yeniden çıkarır (işlev ve bölge satırları). Bir işlev eklenirse görünüme `func` yazılır, adı yan dosyaya otomatik girer.
- `memory`/`bss` > 2^63 gibi uç değerlerde araç işaretli karşılaştırma yapar (gerçek nesnelerde yok).
