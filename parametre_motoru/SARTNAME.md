# Parametre Motoru — Şartname (v0.1)

Durum: **uygulanan: bölüm 1–9, 13–16** (çekirdek, anlam tablosu, makine kodu, ekran/girdi kapısı, şablon katmanı). Bölüm 10–12 yol haritası / tasarım notu, henüz kodlanmadı.

## 0. Amaç ve ilkeler
- Motor sıradan bir C++ programıdır. Sinir ağı, öğrenilmiş ağırlık, vektör uzayı yoktur.
- Uygulama, motorun okuduğu **sayı tablosu**dur (parametre). Dağıtılan/çalışan uygulamada kaynak kod yoktur.
- Tek motor, farklı parametreler, farklı uygulamalar. Alana özel işlev motora gömülmez.
- Üç koşul birlikte aranır: genel amaçlılık, kompaktlık, doğrudan üretilebilirlik. Bunlar birbirini çeker (bölüm 8).

## 1. Değer ve bellek modeli
- Tek değer türü: 64 bit sözcük. Tamsayı (int64) ya da IEEE-754 double olarak yorumlanır; tür yazmaçta değil **komutta** belirlidir (`add` tamsayı, `fadd` ondalık).
- Bellek: düz sözcük dizisi. Adres 0 kullanılmaz (null kuralı). Adres 1'den itibaren sırayla: `data` sözcükleri, `bss` (sıfır dolu alan), sonra `alloc` yığını (yalnızca büyür, serbest bırakma yok).
- Her `ld/st` çalışma anında sınır denetimi görür (`TRAP: OOB`).
- Yazmaçlar: işlev başına numaralı yazmaçlar, ayrı bir yazmaç yığınında. Adresi alınamaz.

## 2. Yürütme modeli
- Yazmaç makinesi. Komut: `op` + en çok 3 operand.
- **Pencereli çağrı:** `call` çağıranın `w` numaralı yazmacından başlayan pencereyi çağrılanın 0. yazmacı yapar. Argümanlar pencereye kopyalanır, dönüş değeri pencerenin ilk yazmacına yazılır. Anlam: çağrılanın parametre dışı yazmaçları sıfırdır. Yükleyici **kesin atama** ispatlarsa (hiçbir yazmaç yazılmadan okunmuyor; çağrıdan sonra pencere ve ötesi tanımsız sayılır) VM bu sıfırlamayı atlar, çünkü gözlemlenemez; ispat yoksa sıfırlama korunur (bölüm 13).
- **Yakıt (kontrol noktaları):** yakıt komut sayısına bağlı değildir. Kontrol noktaları: her `call`, her alınan **geri atlama** (hedef ≤ mevcut komut) ve her `host`. Her biri 1 yakıt harcar; yakıt 0 iken kontrol noktasına gelinirse `TRAP: FUEL`. İleri atlamalar ve düz komutlar yakıt harcamaz; sonlanma garantisi korunur (sonsuz yürütme ya sonsuz çağrı ya sonsuz geri atlama içerir). Yorumlayıcı ve makine kodu yürütücüsü bu kuralı aynı uygular ve aynı yakıt değerini üretir (testle doğrulanır). Optimizasyonlar çağrı ve geri atlama sayısını değiştirmemelidir.
- **Tuzaklar** (belirli, tanımsız davranış yok): `DIV0`, `OOB`, `STACK`, `DEPTH`, `FUEL`, `ALLOC`.
- Tamsayı: iki tamamlayıcı sarma; `INT64_MIN / -1` = `INT64_MIN`, `% -1` = 0; kaydırma miktarı 63 ile maskelenir. `f2i`: NaN→0, taşma→doygunluk.
- Ondalık: IEEE-754, FMA birleştirme yok, hızlı-matematik yok.

## 3. Komut kümesi (55 komut)
`mov ldi ldk` · `add sub mul div mod and or xor shl shr sar` · `eq ne lt le` (+ `gt ge` sözdizimsel kısaltma) · `neg not` · `fadd fsub fmul fdiv fneg fsqrt feq flt fle` (+ `fgt fge`) · `i2f f2i` · `jmp jz jnz call ret halt` · `ld st alloc` · `host` · doğrudan-operand biçimleri (32 bit işaretli sabit son operandda): `addi muli andi shli shri sari divi modi eqi nei lti lei gti gei` (14; `divi/modi` sabiti 0 ve -1 olamaz).
Komut tablosunun tek kaynağı `src/ops.h`; ayrıştırıcı, ikili kodlayıcı, doğrulayıcı, döküm ve VM buradan beslenir.

## 4. Host arayüzü (sistem çağrıları)
7 işlev; hepsi aynı `host` komutuyla çağrılır (argümanlar pencere yazmaçlarında, sonuç pencerenin ilk yazmacında). Yorumlayıcı ve makine kodu **aynı** C++ kodunu (`src/host.h`) çağırır. Her `host` çağrısı 1 yakıt harcar.
- `putc(c)`, `puti(n)`, `putf(x)`, `getc()` (stdin; dosya sonu → -1).
- `present(ptr, w, h)`: uygulamanın belleğinde `ptr`'den başlayan w×h sözcüğü (her biri `0xRRGGBB`) kare olarak sunar. Sonuç: 0 devam, 1 çık (kare sınırı doldu ya da çıkış istendi), −1 geçersiz argüman (sunum yapılmaz; w, h ∈ [1, 4096] ve bölge uygulama belleği içinde olmalı).
- `poll()`: sıradaki girdi olayı `(1<<32) | tuş_kodu`, yoksa 0. Olaylar kare numarasıyla verilir (`--girdi dosya`, satır: `kare kod`); olay, sunulan kare sayısı olayın karesine ulaşınca teslim edilir. Bu, girdiyi deterministik ve yeniden üretilebilir yapar.
- `frame()`: şimdiye kadar sunulan kare sayısı.
Ana bilgisayar tarafı şimdilik **sanal çerçeve tamponu**: son kare PPM (`--ekran`), ASCII (`--ascii K`) ya da kare zinciri özeti (`--kare-rapor`) olarak alınır; `--max-kare N` N kare sonra `present` 1 döndürür. **Gerçek pencere (SDL/X11) bu arayüzün arkasına takılabilir, ama yapılmadı ve denenmedi** (sandbox'ta ekran yok). Ses, fare, dosya, ağ, zaman yok.

## 5. Yazma biçimi (.pma) — insan ve AI için
Satır tabanlı, adlı yazmaç, etiket, yorum `;`.
```
data ad = 1, 2, "metin", 0      ; veri sözcükleri
space ad N                      ; sıfır dolu alan (ikilide yalnızca sayı)
memory N                        ; bellek sözcük sayısı
entry ad                        ; varsayılan: main
func ad(p1, p2) { ... }
etiket:   komut ...             ; etiket ve komut aynı satırda olabilir
call [d,] ad, arg...  host [d,] ad, arg...
```
Sabitler (tamsayı `42`, `0x1f`, `'A'`; ondalık `2.5`, `1e-3`) her operandda yazılabilir; yükleyici bunları işlev girişinde yazmaca taşır. Tür denetimi: tamsayı komutta ondalık sabit (ve tersi) hatadır. Tanımsız ad, yanlış argüman sayısı, bulunamayan etiket/giriş derleme hatasıdır. Yazma biçimi kasıtlı olarak tanıdık (assembler benzeri, adlı, yerel) tutulmuştur: hazır dil modelleri bunu sayı tablosundan çok daha güvenilir üretir.

## 6. Kanonik ikili biçim (PMB1)
`"PMB1"`, bayrak, bellek, giriş, sabit havuzu, veri, bss, işlevler (`parametre, yazmaç, komut sayısı`, komutlar), isteğe bağlı ad bölümü (`--strip` ile atılır). Tüm tamsayılar LEB128; işaretli değerler zigzag; atlama hedefleri **göreli**. Aynı program her zaman aynı baytları üretir (kanonik).

## 7. Doğrulama
Yükleyici ikiliyi çözerken ve çözdükten sonra denetler (doğrudan-operand `divi/modi` sabiti 0/-1 ise reddedilir): başlık, boyut sınırları, komut kodu, yazmaç/etiket/işlev/sabit/host indeksleri, çağrı penceresinin yazmaçlara sığması, her işlevin `jmp/ret/halt` ile bitmesi. Doğrulamayı geçen program VM'i çökertemez; kalan hatalar tuzak olarak bildirilir. (Ölçüm: bölüm 12.)

## 8. Üç koşulun gerilimi ve çözüm yönü
| Koşul | İster | Çatışma |
|---|---|---|
| Kompaktlık | paylaşım, küresel indeks, kanonik numaralama | AI için üretmesi zor |
| Doğrudan üretilebilirlik | yerel, adlı, tipli, tekrarlı, hataya dayanıklı biçim | boyutu büyütür |
| Genel amaçlılık | döngü, özyineleme | "her mutasyon geçerli ve biter" garantisini bozar; yakıtla sınırlanır |

Çözüm yönü (uygulandı): **iki biçim**. Yazma biçimi (.pma) üretilebilirlik için, ikili biçim (.pmb) kompaktlık için; arada tek yönlü, deterministik ve doğrulanabilir dönüşüm.
Not: "her rastgele bayt değişikliği geçerli program verir" **sağlanmamıştır** (bölüm 12). Bu hedef ayrı bir tasarım gerektirir.

## 9. Şablon / örnekleme katmanı — *uygulandı, bölüm 16*
Amaç: 10.000 nesneyi döngü+oluşturma işlevi yerine "ortak şablon + tekrar sayısı + değişim kuralı" olarak tutmak.
Taslak: şablon = parametrelenmiş işlev gövdesi benzeri bir tablo; örnekleme = `(şablon, sayı n, kural: alan_i = a + b·i)`. Motor bunu açmadan (tekrar başına komut üretmeden) yürütebilmeli; koşul: kural yalnızca basit afin/tablo bağımlılığı içerir. Kompaktlık kazancı **ölçülmeden** varsayılmaz.

## 10. Kendini yenileme — *yol haritası*
1. Yükleyici/ayrıştırıcının bir alt kümesini bu dilde yaz → eski C++ motorda çalıştır → çıktı bayt-bayt aynı mı (sabit nokta testi).
2. Sonra özelleştirme (parametreden yerel koda) ile yorumlayıcı üstünde yorumlayıcı yavaşlığını azalt.
C++ çekirdek tamamen ortadan kalkmaz (evrensel yürütücü + host arayüzü gerekir).

## 11. Bildirimsel ilişki katmanı — *tasarım notu, kodlanmadı*
Kural/ilişki tabanlı kısım (düzen, kısıt, veri akışı) zorunlu çekirdeğin **üstünde** olacak; saf bildirimsel sistem algoritmik işlerde yavaş kalır. Hangi biçimcilik (veri akışı grafı, yeniden yazma kuralları, ilişkisel sorgu) seçileceği açık sorudur.

## 12. Ölçütler (Aşama 1 sonuçları `RAPOR.md`, kod üretimi `KOD_URETIMI_RAPORU.md`)
bayt boyutu · adım/s · yerel C++ karşılığına oran · rastgele bozulmada geçerlilik oranı · çökme sayısı · (sonra) AI üretim başarı oranı · (sonra) kendini yenileme bayt eşitliği.

## 13. Kod üretimi sadeleştirmeleri (uygulandı)
Yükleyici/ayrıştırıcı, davranışı değiştirmeden şunları yapar (ölçüm: `KOD_URETIMI_RAPORU.md`):
1. **Doğrudan-operand:** son operand (ya da değişmeli işlemde ilk operand) 32 bite sığan sabitse/data adresiyse `addi muli andi eqi nei lti lei gti gei shli shri sari divi modi` kullanılır; `sub x, k` → `addi x, -k`; `lt/le/gt/ge` sabit solda ise yön çevrilir. 32 bit dışı sabit ya da `div/mod` 0/-1 yazmaç biçiminde kalır.
2. **Sabit yükleme:** `mov d, k`, `ret k`, `call/host` argümanı `k` doğrudan yüklenir (ön hazırlık yazmacı üretilmez). Diğer sabitler (ör. ondalık, `st` değeri) işlev girişinde bir kez yazmaca yüklenir.
3. **Çağrı penceresi:** tek yazılıp tek okunan, hemen çağrı argümanına kopyalanan geçici doğrudan pencereye yazılır; `mov x, W` ardından `mov W, x` ve hiç okunmayan çağrı sonucu kopyası silinir; kendine kopya silinir.
4. **Sıfırlama atlama:** kesin atama analizi (`analyze_program`): bütün işlevlerde her yazmaç okunmadan önce her yoldan yazılmışsa `zero_free` olur. Bu bayrak **ikiliye yazılmaz**, yüklenirken hesaplanır; ikili biçim ve anlam değişmez.
Yapılmayanlar (ölçümle gerekçe çıkmadı ya da mimariyi büyütür): ortak alt ifade, sabit katlama, yazmaç yeniden kullanımı/renklendirme, karşılaştır-ve-atla birleşik komutu.

## 14. Anlam tablosu (uygulandı: `src/anlam.h`)
**55 komutun tamamının** anlamı (tamsayı, ondalık, karşılaştırma, bellek, atlama, çağrı, host) küçük bir **mikro-işlem dilinde** tek yerde tanımlıdır: `RD IM LK ALU CMP UN DIVS MODS DIVN MODN WR LD ST BR BRZ BRNZ CALLM RETM HALTM HOSTM ALLOCM FALU FUN FCMP CVTIF CVTFI`. Aynı tanımdan iki ayrı genişletme üretilir:
- **INTERP** (derleme anında): yorumlayıcının işleyicileri (`vm.cpp`).
- **DATA** (veri tablosu): makine kodu üretecinin sablonları (`native_x64.cpp`).
Tabloda olmayan komut kalmadı (ondalık komutlar da tabloda: `fadd fsub fmul fdiv fneg fsqrt feq flt fle i2f f2i`).
Sınır: tablo **derleme anında** kullanılır. Komut anlamları çalışma anında yüklenip değiştirilemez (kademe 2 / 3, yapılmadı). Tanım dili çalışma anında yüklenebilecek biçimde (küçük, tam tanımlı, tuzakları belli) tasarlandı.

## 15. Makine kodu yürütücüsü (uygulandı: `src/native_x64.cpp`, yalnızca Linux x86-64)
- Program tablosu, anlam tablosundaki şablonlarla x86-64 makine koduna çevrilir ve doğrudan işlemcide çalışır (`pm run --native`). C++ yorumlayıcı yürütme yolunda yoktur; yalnızca yükleyici, host işlevleri ve üretici C++'tır.
- Yorumlayıcıyla **aynı** anlam, tuzak ve yakıt kuralları: çıktı, çıkış kodu, tuzak türü ve harcanan yakıt bire bir aynı olmak zorundadır (testle doğrulanır). Yorumlayıcı **referanstır**; makine kodu yolu onunla farklı çıkarsa hata makine kodundadır.
- Sınır denetimleri korunur: bellek sınırı, sıfıra bölme, yığın/derinlik, yakıt kontrol noktaları, `alloc` sınırı.
- Üretilen kod yazılabilir-yürütülebilir değildir: önce yazılır, sonra `mprotect` ile yalnızca okunur+yürütülür yapılır. Makine yığını ayrı ayrılır ve `max_frames` ile sınırlıdır.
- Bilinen farklar: tuzak konumu (işlev/komut) raporlanmaz; yalnızca tür ve yakıt. Desteklenmeyen platformda `--native` yorumlayıcıya düşer (`--native-strict` hata verir, çıkış kodu 3).
- **Yazmaç dağıtımı:** program "kesin atama" ispatlıysa (`zero_free`), bir işlevdeki sık kullanılan VM yazmaçları (en çok 8) CPU yazmaçlarında tutulur. Yalnızca işlevin en küçük `call/host` tabanının altındaki yazmaçlar adaydır; çağrı penceresi hep bellekte kalır. İşlev kullandığı havuz yazmaçlarını girişte saklar, çıkışta geri yükler. Eşik: ağırlıklı kullanım (döngü içi ×8) ≥ `native_promote` (varsayılan 4; `--native-promote N`, 0 = hepsi, büyük = hiç).
- **Karşılaştır-ve-atla birleştirmesi:** bir karşılaştırma komutunun ardından hemen `jz/jnz` gelir ve sonuç başka yerde okunmuyorsa tek `cmp`+`jcc` üretilir, sonuç yazmacı yazılmaz. Bu da yalnızca `zero_free` programlarda yapılır: ispat yoksa çağrı penceresi üst üste binen programlarda çağıran, çağrılanın yazmacını okuyabilir (fuzz bunu buldu; `tests/regress/`).
- Kural: **yazmaca alma ve birleştirme, bir çağrıdan sonra pencere yazmaçlarının okunmadığı ispatına dayanır.** İspat yoksa bellekte tutulan (basit) şablonlar kullanılır; anlam değişmez.
- **Ondalık (SSE2):** `addsd subsd mulsd divsd sqrtsd ucomisd cvtsi2sd cvttsd2si`; değerler 64 bit kalıp olarak tamsayı yazmaçlarında/bellekte durur, işlem sırasında xmm0/xmm1'e `movq` ile alınır. `f2i` yorumlayıcıyla aynı: NaN→0, ≥2^63→INT64_MAX, ≤−2^63→INT64_MIN, aksi halde sıfıra kırpma; NaN karşılaştırmaları yanlış.
- **Host çağrısı kuralı:** C++ yardımcısı caller-saved yazmaçları (`rsi rdi r8–r11`) bozar. Yazmaca alma kullanılan bir programda bu altı yazmaç **her** `host` çağrısında saklanır/geri yüklenir; çünkü çağıran zincirinde bir üst işlev değerini orada tutuyor olabilir (kendi yazmaç kullanmayan bir işlev bile). Fuzz bu hatayı buldu (`tests/regress/host_caller_saved_*`).
- **Belirsiz kalan:** iki *farklı* NaN içeren ondalık işlemde sonucun NaN işareti/yükü belirsizdir (x86 ilk operandı seçer; yorumlayıcıda derleyici değişmeli işlemin operand sırasını değiştirebilir). Fuzz'da fark gözlenmedi ama kanıtlanmış değil.

## 16. Şablon / örnekleme katmanı (uygulandı: `src/sablon.cpp`, dil başvurusu `uygulamalar/sablon/DIL.md`)
- Bölüm 9'daki fikir hayata geçti, ama **yorumlayıcı değil özelleştirici** olarak: bildirimsel uygulama tanımı (`.pmt`: şablonlar, örnekleme üreteçleri, davranış makroları, ilişki/olay/toplama tabloları) yazma biçimine (`.pma`) çevrilir, sonra mevcut ayrıştırıcı ve makine kodu üreticisinden geçer. Böylece yürütme hızı yorumlayıcı katmanı eklemeden korunur ve tanım boyutu örnek sayısından bağımsızdır (10.000 örnek = tek döngü).
- **Davranışlar veri:** kural dili küçüktür (`hedef = ifade eger koşul`); `hareket`, `sek`, `yercekimi`, `sinirla` gibi davranışlar bu kuralların parametreli makrolarıdır ve `std.pmt` kütüphanesinde tanımlıdır. Uygulama davranışı adıyla seçer, kuralları birleştirir.
- **Genel amaçlılık:** aynı dil oyun, grafik ve veri işleme (toplam, sayı, en büyük/küçük) için çalışır; ekrana özgü kısım (`sahne`, `ciz`) isteğe bağlıdır.
- **Sanal alanlar:** hiçbir kuralla yazılmayan alan (sabit/doğrusal/mod/bol üreteçli) bellekte tutulmaz; formülden hesaplanır.
- **Anlam:** bir şablonun kuralları her örnek için sırayla uygulanır (örnek-öncelikli); etkileşim A×B çiftlerini (A dış, B iç döngü) sırayla gezer; kare sırası DIL.md'de.
- **Sınırlar:** kurallarda döngü/özyineleme yok, ifadede tek ikili işlem, etkileşim O(|A|·|B|), toplama işlemleri sabit dört; 3B, metin, arayüz bileşenleri yok. Özelleştirici C++'tır (514 satır), uygulamanın kendisi değildir; kendini yenileme (bölüm 10) için ileride tanım dili üzerinden yeniden yazılabilir.
- Doğrulama: `uygulamalar/kontrol.sh` (bağımsız awk benzetimleri, elle yazılmış sürümlerle kare zinciri eşitliği) ve `SABLON_RAPORU.md`.

## 17. Yapısal sayısal parametre tablosu (uygulandı: `src/yapisal.cpp`, başvuru `YAPISAL.md`)
Üçüncü yazım biçimi: metinsiz, yalnızca sayılardan oluşan yapısal tablo (`PMY1`). Nesne şablonları (alan başlatıcıları: sabit/doğrusal/mod/böl/rastgele), davranışlar (parametreli ortak kural kalıbı), işlevler (özyineleme), global diziler, etkileşimler, olaylar, toplamalar, koşul/döngü/çağrı komutları ve ifade ağaçları **önek, sayaçlı kayıtlarla** anlatılır; ofset, etiket, yazmaç numarası yoktur. Yükleyici tabloyu çözer, doğrular, **bir kez** özelleştirip aynı `.pma` → ikili tablo → makine kodu hattından geçirir (çalışırken tablo yorumlanmaz). `pm run x.pmy`, `pm yapisal x.pmy -o x.pmb`, `pm tablo x.pmy` (okunabilir ağaç). Uygulamaya özel hiçbir kod yoktur. Sınırlar: yalnızca tamsayı, nesne "canlı" bayrağı yok, dizi/şablon indeksi çalışma zamanında denetlenir (dışı = OOB). Denetim: `ai_testi3/kontrol3.sh`, bozuk tablo fuzz'ı (`pm fuzz x.pmy N --diff`).
