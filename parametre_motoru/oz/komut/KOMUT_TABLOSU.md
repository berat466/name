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
| 4 PRIM (mikro-işlemler) | `id  adlen ad..  nalan  tur*nalan` ; tür kodları: 1 geçici-girdi (`ti`), 2 geçici-çıktı (`to`), 3 komutun operand yuvası (`op`: 0=a 1=b 2=c), 4 sayı değişmezi (`n`). Ext anahtar 1 = sınıf: 0 saf, 1 bellek/tuzak, 2 denetim akışı, 3 host |
| 5 SEM (komut anlamları) | `opkod  nornek  (islem f0 f1 f2 f3)*nornek` — komutun **mikro-programı**; `islem` PRIM kimliği, alanlar PRIM'in alan türlerine göre geçici (1..8), operand yuvası ya da sayı |

Tür bayrakları (bilgi amaçlı, araçlar henüz yalnızca `id`'yi ve ext anahtar 2'yi kullanır): 1 yazmaç okur, 2 yazmaca yazar, 4 doğrudan tamsayı, 8 etiket, 16 işlev, 32 sabit havuzu, 64 pencere tabanı, 128 host kimliği.

**KIND ext anahtar 2 = tel sınıfı [Kesin: PMB1 çözücü/kodlayıcı bunu kullanır]:** 0 düz (varsayılan), 1 işaretli doğrudan sayı (`i`), 2 göreli etiket (`l`). `coz.pmy`/`yaz.pmy` bu sınıfı, ve komut başına operand sayısını (NOPS), tablodan alır (`@` → `NOPSZ`, `KINDWN`, `KINDWZ`; `oz/uret_ikili.sh`).

## Sürümleme ve uyumluluk politikası [Kesin: aşağıdaki davranışlar `dogrula_komut.sh` ile sınanır]
- **major** değişirse okuyucular tabloyu reddeder. **minor** artışı (yeni ek alanlar/bölümler) kabul edilir.
- **Bilinmeyen bölüm** (bilinmeyen kimlik ya da bilinen kimliğin başka sürümü): bayrak bit0 = 0 ise atlanır, `gor`/`yaz` ile **olduğu gibi korunur**; bit0 = 1 (ZORUNLU) ise tablo reddedilir. Böylece eski araç, yeni özelliği yanlış anlayıp çalışmaz; yok sayabileceği şeyleri yok sayar.
- **Kayıt uzantıları:** `ne` + `(anahtar,değer)` çiftleri ve bilinmeyen kuyruk jetonları okuyucular tarafından atlanır ve korunur (kayıt `uzunluk` alanı ile atlanır).
- **Bölüm sırası** serbesttir (KIND/OP/HOST sırası değişse sözlük aynıdır).
- **Kimlikler açıktır:** komut kimliği kayıtta yazılıdır; biçim yoğun olmasını şart koşmaz. Araç zincirinin **profili** (`@@tab`, `@@opsh`, `@@dogrula`) ek olarak komut/host kimliklerinin 0..n−1 yoğun olmasını ve komut başına en çok 3 operand olmasını ister; bu bir araç koşuludur, biçimin değil.

## Komut anlamları (PRIM + SEM) — davranış da sayıdır [Kesin]
Bir komutun *ne yaptığı*, PRIM bölümündeki küçük **mikro-işlemlerin** SEM bölümündeki sıralı listesidir. Örnek (metin görünümü; asıl biçim sayılardır):
```
sem ADD  rd t1 b ; rd t2 c ; alu_add t3 t1 t2 ; wr a t3      # a = b + c
sem FMIN rd t1 b ; rd t2 c ; flt t3 t1 t2 ; sel t4 t3 t1 t2 ; wr a t4   # a = (b < c) ? b : c
```
`rd tN x` operand `x`'in yazmaç değerini geçici `tN`'ye okur, `wr x tN` yazar, `im` doğrudan sayıyı, `lk` sabit havuzunu okur; kalanlar saf işlemlerdir (`alu_*`, `cmp_*`, `un_*`, `f*`, `i2f`, `f2i`, `sel`), bellek/tuzak (`ld st alloc divs mods ...`), denetim akışı (`br brz brnz call ret halt`) ve `host`.
- Doğrulama (komut aracı): bilinmeyen işlem/komut, alan sayısı, geçicinin tanımdan önce kullanılması, operand yuvasının komutta bulunmaması, yinelenen anlam reddedilir (`dogrula_komut.sh` bölüm 8).
- **Üç yürütücü, aynı anlam:** (1) `oz/sayisal/svm.pmb` tabloyu çalışma anında okuyup mikro-programları doğrudan yürütür (isimle eşleme: kimlikler yeniden numaralansa da çalışır); (2) `gen2` mikro-programı x86'ya indirger (`sablon.txt`'te `p_<işlem>` şablonları); (3) bağımsız referans: elle yazılmış C++ `pm` (`src/anlam.h`, `vm.cpp`). Anlamın C++ ile sayısal tablo arasında eşitliği `anlam_dump.cpp` ile sınanır.
- **Yeni komut, kod yazmadan:** OP kaydı + SEM kaydı (mevcut mikro-işlemlerden). Ne C++ ne x86 şablonu gerekir; svm ve gen2 komutu çalıştırır (`dogrula_komut.sh` bölüm 7, FMIN: NaN/±sonsuz/±0/alt normal dahil 41×41 bit deseni, C++ oracle ile eşit).
- **Yeni mikro-işlem** = PRIM kaydı + her yürütücüde gerçekleştirim (svm'de bir kol, gen2'de `p_<ad>` şablonu). Gerçekleştirimi olmayan yürütücü onu **sessizce yok saymaz, reddeder** (gen2: çıktı yok; svm: hata). Bu bilinçli genişletme noktasıdır.
- `gen2 -6 1` seçeneği, yerleşik x86 şablonu olan komutları da SEM'den üretir (sınama ve iki üretim yolunun eşitliği için). Yerleşik şablon varsa varsayılan olarak o kullanılır (eniyileme yolu); SEM yolu tek düzen, daha yavaş koddur (geçiciler bellekte).

## Genişletme noktaları
- **Yeni komut** = OP kaydı + SEM kaydı (x86 şablonu isteğe bağlı eniyilemedir). `dogrula_komut.sh` bölüm 7: `FMIN` (d,f,f) yalnızca bu iki tablo kaydıyla assembler, doğrulayıcı, svm ve ELF üreticide çalıştı; elle yazılmış `minsd` şablonu eklenince aynı sonuç.
- **Yeni operand türü** = KIND bölümüne kayıt.
- **Yeni host işlevi** = HOST bölümüne kayıt (RT işlevi ya da şablon gerekir).
- **Ayrılmış bölüm kimlik aralıkları (henüz kullanılmıyor):** 4–15 çekirdek (yürütme anlamı, şablon, doğrulayıcı kuralları, yeniden yazma kuralları); 16–63 yürütme modelleri/donanım arka uçları; 64–127 güvenlik (imza, yetenek haritası, politika); 128–255 uygulama/kullanıcı; ≥ 256 özel.
- **Güvenlik katmanları ayrı olmalıdır [Tasarım ilkesi]:** tablo yalnızca *neyin ifade edilebildiğini* tanımlar. *Kimin neyi yapabileceği* (yetki politikası), imza doğrulama, izolasyon ayrı, sürümlü bölümlerde (64–127) ya da tablo dışı politika dosyalarında tutulur. Host kaydındaki ext anahtar 1 yalnızca etiket (1 metin çıkışı, 2 metin girişi, 3 grafik); hiçbir araç bunu henüz kısıt olarak uygulamaz. Güvenilmeyen programlar için zorunlu doğrulama (`valid`) zincirde atlanamaz (`oz/bag/zincir.sh`: doğrulayıcı reddederse ELF üretilmez); imza, yetki sistemi ve izolasyon **henüz yok**.

## Hâlâ metne ya da elle yazılmış kurala bağlı olanlar [Kesin, ayrıntı OZ_BARINDIRMA.md §3i, §3j]
Komut *kimlikleri, adları, operand sayıları/türleri, host listesi ve anlamları (SEM)* tablodan geliyor. Henüz tablo dışı olanlar: komutlar arası *yeniden yazma kuralları* (`add→addi` …), komuta özgü *doğrulayıcı kuralları*, *x86 şablonlarının* kendisi (`sablon.txt` metni), mikro-işlemlerin yürütücülerdeki gerçekleştirimi (her yürütücüde elle) ve host/işletim sistemi işlemleri.
