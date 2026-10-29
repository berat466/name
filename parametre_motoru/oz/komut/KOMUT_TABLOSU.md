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
| 6 PSEUDO (takma komutlar) | `kimlik(0..255)  symlen sym..  adlen ad..  operand_sayisi` — doğrudan kodlanmayan mnemonikler (`gt ge fgt fge`); kimlik/sembol/ad komutlar ve birbirleriyle çakışamaz |
| 7 REWRITE (yeniden yazma kuralları) | `kaynak_turu(0 komut,1 takma)  kaynak_kimligi  nvaka  vaka*` ; vaka = `koşul alt üst neylem (eylem arg)*neylem` ; koşul 1: c doğrudan sayı ve alt≤değer≤üst · 2: c sayı değil, b doğrudan sayı ve alt≤değer≤üst · 3: her zaman ; eylem 1: komutu arg yap · 2: b,c takas · 3: c doğrudan sayısını eksi yap. Vakalar sırayla denenir, ilk eşleşen uygulanır |
| 8 TOKEN (şablon simgeleri) | `kod(256..1023)  adlen ad..  sınıf` — şablon simge sözlüğü: 256+ imm32 (`R_A`, `I_C`, `MEMW` …), 300+ göreli hedef (`REL_*`), 320 `IMM8C`, 400+ mikro-işlem alan simgeleri (`TMP_Fk OPR_Fk OPV_Fk KAD_Fk LIT_Fk REL_TFk`). Simgelerin *anlamını* `gen2` uygular (genişletme noktası); tablo yalnızca ad↔kod verir |
| 9 TEMPLATE (x86 şablonları) | `tür(0 komut,1 özel,2 işlem)  başvuru  adlen ad..  ntok(-1 tanımsız)  simge*` — simge < 256 ise doğrudan bayt, değilse TOKEN kodu. Tür 0: başvuru = komut kimliği; tür 1: sıra (0..) + ad (`host_putc`, `prologue`, `jmp_back` …); tür 2: PRIM kimliği (mikro-işlem şablonu). Yuva numarası: komut → kimlik, özel → `OPCOUNT + sıra`, işlem → `OPCOUNT + özel_sayısı + kimlik` |
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

## Yeniden yazma kuralları ve doğrulayıcı özellikleri [Kesin]
- **Assembler (`asm.pma.in`)** `add→addi`, `sub x,imm→addi x,-imm`, `lt/le` ayna takasları, `gt/ge/fgt/fge` takma komutları, `div/mod→divi/modi` (0 ve −1 hariç) kurallarını **tablodan** okur (`RWOFF/RWDAT/PSNAMES/PSNOPS`); `rewrite` işlevinde komut numarası yok. C++ `pm asm` (elle yazılmış kurallar) ile bayt-bayt eşitlik `oz/asm/dogrula_kural.sh` ve `dogrula_asm.sh` ile sınanır.
- **Doğrulayıcı (`valid.pma.in`)** komuta özgü bilgiyi tablodaki **SEM'den türetilmiş** dizilerden alır: `OPFLOW` (0 sıradan, 1 durdurur: `ret/halt`, 2 koşulsuz atlama: `br`, 3 koşullu atlama: `brz/brnz`), `OPTGT` (hedef operand yuvası), `OPCALL/OPCS0/OPCS1` (işlev ya da host çağrısı ve operand yuvaları), `OPDIVSL` (`divn/modn` bölenini besleyen anlık operand yuvası: 0 ve −1 yasak), `KFLAGS` (KIND bayrakları: okur/yazar/anlık/etiket/işlev/sabit/host). Dosyada `@OP_*@` kalmadı.
- **Sonuç:** yeni bir atlama komutu (`GOTO l`, SEM: `br a`) ve yeni bir anlık komut + kuralı (`XORI`, `xor→xori`) yalnızca tablo kayıtlarıyla assembler'da, doğrulayıcıda ve svm'de çalıştı (`dogrula_komut.sh` bölüm 9).
- **Sınırlar:** kural ve takma komut bölümlerinin **okunabilir görünümü sayısal** (`gor` onları `section 6 …` / `section 7 …` ham sayı satırı olarak verir; kayıpsızdır ama komut kimlikleri sembole çevrilmez, kimlik değiştiren metin düzenlemesi kuralların hedeflerini kendiliğinden izlemez). Denetim akışı komutları (`br`, `brz`, `call`, `ret`, `host`) için **x86 mikro-işlem şablonu yok**: bunların ELF üretimi yerleşik şablonlarla yapılır; yeni bir atlama komutu ELF üretecinde henüz çalışmaz (svm'de çalışır). Doğrulayıcının akış analizi yalnızca `br/brz/brnz/ret/halt/call/host` sözcük dağarcığını tanır; yeni denetim akışı mikro-işlemi eklenirse `komut.pma`'daki `semflow` ve doğrulayıcı güncellenmelidir (bilinçli genişletme noktası).

## Araçlar sayısal nesnedir (PMO1) ve metinsiz bağlanır [Kesin]
Komut tablosuna bağlı üç araç (`asm`, `valid`, `gen2`) artık **sayısal nesne** olarak tutulur: `oz/nesne/{asm,valid,gen2}.pmo`. Derleme zinciri bunlardan şöyle üretir (assembler, `doldur`, `.pma.in` metni **kullanılmaz**):
```
{ komut @@tab  [+ statik.sozluk + RTL (rt.pmb listesi)] ; @@NESNE ; X.pmo }  --baglayici-->  pm liste  --yaz.elf / pm yazliste-->  X.pmb
```
- **Biçim (hepsi sayı):** `1347243825 1 nanahtar nbagil` · `nanahtar` satır `tür(0 skaler,1 dizi) taban adlen ad..` · `nbagil` satır `konum anahtar katsayı` · ardından taslak liste. Skaler yer tutucu = tek sayı (`S1+no`; `sub` yeniden yazması yüzünden eksilmiş de görülebilir), dizi yer tutucusu = tek sözcük (`S2+no·65536`, tabloda genişler). **Bağıl kayıt:** dizi uzunluğu/ skaler değer değişince yerleşimi etkilenen sayılar (veri adresleri, veri sözcük sayısı …); bağlayıcı `değer += katsayı·(gerçek − taban)` uygular. Uzunluk sınırı yoktur (kapasite tahsis edilmez).
- **Doğruluk:** bağlayıcı çıktısı, aynı tablo için metin yolunun (`doldur` + assembler) çıktısıyla **bayt-bayt aynıdır**: mevcut tablo, ad değişikliği, yeni komut/anlam/kural/takma komut için (`oz/nesne/dogrula_nesne.sh`).
- **Metin isteğe bağlı ön yüzdür:** `oz/nesne/ice_aktar.sh` bir `.pma.in` kaynağını nesneye çevirir (bağıl kayıtları, kaynağı farklı dizi uzunluklarıyla yeniden derleyip karşılaştırarak bulur; assembler'a dokunmaz); `oz/nesne/metinden.sh` üç aracın nesnesini, `.pmb`'sini ve `.pma` görünümünü metinden yeniden üretir. Uyumluluk kapısı: metinden ice aktarılan nesne depodakiyle aynı olmalıdır.
- **Sınır (bilinçli ve yüksek sesle reddedilir):** bir skaler yer tutucu `space` boyutunu (yerleşimi) belirleyemez; ice aktarma bunu reddeder.
- **Oluşturulan araç takımı:** `oz/arac/baglayici.pma/.pmb` (ELF olarak zincirde); `oz/bag/yenile.sh` yalnızca sayısal girdileri kopyalar (`komutlar.pmk`, `*.pmo`, `*.pmb`, `rt.pmb`, `statik.sozluk`, `paketle.pmy`, `*.sayi.in`) ve ESKİ ELF takımıyla yeni takımı üretir; çalışma ağacında hiç `.pma/.pma.in/fmt.txt` bulunmadığı test edilir.

- **Okunabilir görünüm ve düzenleme (PMV):** nesne `oz/nesne/duzenle.sh gor` ile okunabilir metne (komut adları, sembolik etiketler, işlev adları, `&DIZI`/`~DIZI`/`&bölge` adresleri) çevrilir ve `yaz` ile **aynı sayılara** döner (bayt-bayt kimlik). Böylece asm/valid/gen2 algoritmaları `.pma` kaynağına dönmeden sayısal kaynaktan düzenlenir; ayrıntı ve hata kodları: `oz/nesne/GORUNUM.md`.

## x86 şablonları sayıdır [Kesin]
- `komut @@tab` `TPLS/TPL/TS_*` sözlüğünü doğrudan tablodan üretir; `sablonc` ve `sablon.txt` **kaldırıldı**. Okunabilir görünümde şablonlar `tpl op ADD : 48 8B 83 R_B …`, `tpl sp 7 prologue : …`, `tpl prim alu_add : …` satırlarıdır (iki hanelik büyük onaltılık bayt ya da simge adı; makro yok, açılmış biçim). `gor`/`yaz` kayıpsız. Şablon düzenlemek = görünümde satırı değiştirip `komut @@yaz`.
- **Geri atlama (yakıt) varyantları** tabloda: özel şablonlar `jmp_back jz_back jnz_back` (komutlar) ve `br_back brz_back brnz_back` (işlemler); adlandırma kuralı `<ad>_back` ile `OPBACK`/`PBACK` dizileri üretilir. `gen2` artık `OP_JMP..OP_JNZ` aralığını bilmez: atlama sınıfını (`OPFLOW`, SEM'den) ve hedef yuvasını (`OPTGT`) kullanır.
- **Denetim akışı mikro-işlemleri (`br brz brnz`) için x86 şablonu var**: yalnızca SEM ile tanımlı yeni bir atlama komutu (`GOTO l`, SEM `br a`) ELF üreticide de çalışır, geri atlamalarda yakıt sayılır (`dogrula_komut.sh` bölüm 9). `call`, `ret`, `halt`, `host` için mikro-işlem şablonu **yok** (çerçeve kurulumu, host/RT çağrısı özel); bu komutlar yerleşik şablonlarla üretilir.

## Genişletme noktaları
- **Yeni komut** = OP kaydı + SEM kaydı (x86 şablonu isteğe bağlı eniyilemedir). `dogrula_komut.sh` bölüm 7: `FMIN` (d,f,f) yalnızca bu iki tablo kaydıyla assembler, doğrulayıcı, svm ve ELF üreticide çalıştı; elle yazılmış `minsd` şablonu eklenince aynı sonuç.
- **Yeni operand türü** = KIND bölümüne kayıt.
- **Yeni host işlevi** = HOST bölümüne kayıt (RT işlevi ya da şablon gerekir).
- **Ayrılmış bölüm kimlik aralıkları (henüz kullanılmıyor):** 4–15 çekirdek (yürütme anlamı, şablon, doğrulayıcı kuralları, yeniden yazma kuralları); 16–63 yürütme modelleri/donanım arka uçları; 64–127 güvenlik (imza, yetenek haritası, politika); 128–255 uygulama/kullanıcı; ≥ 256 özel.
- **Güvenlik katmanları ayrı olmalıdır [Tasarım ilkesi]:** tablo yalnızca *neyin ifade edilebildiğini* tanımlar. *Kimin neyi yapabileceği* (yetki politikası), imza doğrulama, izolasyon ayrı, sürümlü bölümlerde (64–127) ya da tablo dışı politika dosyalarında tutulur. Host kaydındaki ext anahtar 1 yalnızca etiket (1 metin çıkışı, 2 metin girişi, 3 grafik); hiçbir araç bunu henüz kısıt olarak uygulamaz. Güvenilmeyen programlar için zorunlu doğrulama (`valid`) zincirde atlanamaz (`oz/bag/zincir.sh`: doğrulayıcı reddederse ELF üretilmez); imza, yetki sistemi ve izolasyon **henüz yok**.

## Hâlâ metne ya da elle yazılmış kurala bağlı olanlar [Kesin, ayrıntı OZ_BARINDIRMA.md §3i, §3j]
Komut *kimlikleri, adları, operand sayıları/türleri, host listesi, anlamları (SEM), yeniden yazma kuralları, takma komutlar, doğrulayıcı özellikleri (SEM'den türetilir) ve x86 şablonları (simge sözlüğüyle)* tablodan geliyor. Henüz tablo dışı olanlar: mikro-işlemlerin ve şablon simgelerinin *anlamının uygulanışı* (her yürütücüde/`gen2` içinde elle), `call/ret/halt/host` mikro-işlem şablonları, host/işletim sistemi işlemleri, araç kaynakları (`.pma`), `fmt.txt` mnemonikleri, güvenlik katmanları.
