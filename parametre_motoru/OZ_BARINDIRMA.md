# Öz-barındırma (self-hosting) planı ve ilk adım

**Hedef:** C++ ile kurulan ilk motor, kendi çözücüsünü, doğrulayıcısını, derleyicilerini ve makine kodu üreticisini zamanla **parametre tablolarıyla (PMY1/PMB1)** yeniden üretsin; C++ yalnızca doğrulama ve önyükleme aracı olarak kalsın.
Bu belge: (1) hangi bileşenlerin C++ olduğu ve neye ihtiyaç duyduğu, (2) sıralı geçiş merdiveni, (3) ilk uygulanan adım ve ölçümü.

## 1. Bileşen envanteri (mevcut durum, 3107 C++ satırı + awk)
| Bileşen | Dosya (satır) | İş | PMY1'de ifade edilebilir mi? | Eksik olan |
|---|---|---|---|---|
| Sayı → bayt paketleyici | `ai_testi2/paketle.awk` (awk) | LEB128/zigzag kodlama | **Evet, yapıldı (`oz/paketle.pmy`)** | `oku` (stdin) — eklendi |
| PMB1 çözücü | `ikili.cpp` (213) | bayt → program tablosu | Evet | bayt girdisi (`oku`), kayıt yapısı için diziler (var) |
| Doğrulayıcı | `ikili.cpp` (`validate_program`) | indeks/ofset/pencere denetimi | Evet | — |
| Kesin atama analizi | `ikili.cpp` (`analyze_program`) | veri akışı (bit kümeleri) | Evet, ağır | büyük diziler |
| PMB1 kodlayıcı | `ikili.cpp` (`encode_program`) | tablo → bayt | Evet | — |
| Birleştirici (`.pma`) | `asm.cpp` (520) | metin → tablo | Evet, büyük | metin işleme (`oku` ile) |
| PMY1 özelleştirici | `yapisal.cpp` (611) | ağaç → `.pma` | Evet, büyük | ağaç/yığın veri yapıları (diziyle) |
| `.pmt` özelleştirici | `sablon.cpp` (514) | metin → `.pma` | Evet, büyük | metin işleme |
| Yorumlayıcı (anlam) | `vm.cpp`+`anlam.h` (280) | 55 komutun anlamı | Evet (PMY1'de bir VM döngüsü) | yok; ama yavaşlar (VM üstünde VM) |
| x86-64 kod üretici | `native_x64.cpp` (512) | tablo → makine kodu baytları | Üretim kısmı evet | **`exec`**: üretilen baytları çalıştıran ilkel (yürütülebilir bellek + atlama) |
| Dökümcü | `dump.cpp` (36) | tablo → okunur metin | Evet | — |
| Ana program, komut satırı | `main.cpp` (219) | argüman, dosya, bayrak | Kısmen | dosya/argüman erişimi |
| **Host + ilk yürütücü** | `host.h`, `vm.cpp`, `native_x64.cpp` (mmap/atlama) | G/Ç, bellek, **ilk komutu çalıştıran şey** | **Hayır** | — |

**Düzeltme (kullanıcı kararı):** Önceki sürümde "küçük bir C++ çekirdeği kalıcı olmalı" yazılmıştı; bu yanlış çerçeveydi. Gereken şey bir **başlangıç yürütme mekanizması**dır, ve bunun C++ olması şart değildir. İşletim sistemi, önceden üretilmiş bir makine kodu görüntüsünü (ör. ELF) yükleyip başlatabilir. Nihai hedef: parametre tabanlı araç zinciri kendi makine kodunu üretir (kod üreticisi + bağlayıcı/yükleyici biçimi parametreyle), böylece C++ kaynak koduna ve C++ çalışma zamanına ihtiyaç kalmaz; C++ yalnızca geçiş döneminde önyükleme ve doğrulama aracıdır. Bu yüzden C++ satırlarının "ne kadarının kalacağı" sorulmaz; "sırayla nasıl parametreye taşınacağı" sorulur. Gerekli yeni ilkeller: bellek/dosya/G-Ç için işletim sistemi çağrıları ve üretilen kodun bağımsız bir yürütülebilir olarak yazılması (adım 9–10).

**Durum dürüstlüğü:** Tamamlanan adımlar (paketleyici, kodlayıcı, çözücü) tam öz-barındırma değildir; çünkü bu parametre bileşenleri hâlâ C++ yorumlayıcısı/makine kodu üreticisi üzerinde çalışıyor. Öz-barındırma, zincirin tamamı parametreyle ifade edilip çalıştırma altyapısı da parametre tabanlı üretilen makine kodu olduğunda tamamlanır.

## 2. Geçiş merdiveni (kolaydan zora; her adım C++ sürümüyle bayt-bayt karşılaştırılır)
1. **Paketleyici** (`oz/paketle.pmy`) — *yapıldı (tam 64 bit)*.
2. **PMB1 kodlayıcı + çözücü** — *yapıldı* (bkz. bölüm 3). Doğrulama: depodaki 200+ test programı ve fuzz derlemi; çözücü çıktısı `pm dump` ile, kodlayıcı çıktısı `pm asm` ile bayt-bayt.
3. **Doğrulayıcı**. Doğrulama: fuzz'daki yüz binlerce bozuk `.pmb` için kabul/ret kararı ve ret nedeni C++ ile aynı (diferansiyel).
4. **Dökümcü** (kolay; metin çıktısı `pm dump` ile aynı).
5. **Kesin atama analizi** (`zero_free`): sonuç bir bit; C++ ile aynı.
6. **Birleştirici** (`.pma` → tablo). Doğrulama: tüm `.pma` örnekleri için çıktı `pm asm` ile bayt-bayt.
7. **PMY1 ve `.pmt` özelleştiriciler** (ağaç → `.pma`). Doğrulama: aynı tablo → aynı `.pma`/`.pmb`.
8. **Yorumlayıcı** (PMY1'de VM döngüsü): 203 testi ve 60 uygulamayı koşturup C++ yorumlayıcısıyla aynı çıktı/yakıt/tuzak.
9. **Makine kodu üretici + `exec`**: üretilen bayt dizisi C++ üreticisinin çıktısıyla bayt-bayt aynı olmalı; `exec` tek yeni çekirdek ilkeli.
10. **Sabit nokta:** parametre tabanlı araç zinciri (2–9) kendi tablosunu (`.pmy`) girdi alıp aynı `.pmb`'yi üretmeli; C++ yalnızca çekirdek olarak çalıştırır.

Her adımda **ölçü**: bayt/çıktı eşitliği (otomatik, ajan yok), hız (parametre tabanlı sürüm / C++), tablo boyutu (öğe sayısı) ve C++ satırının ne kadar azaldığı.

## 3. Tamamlanan adımlar (`oz/`)

**Kapsam notu (dürüstlük):** Bu adımlar *tam öz-barındırma değildir*. Üç bileşen artık parametre tablosu (PMY1) olarak yazıldı ve C++ referansıyla bayt-bayt aynı sonuç veriyor; ama bunları çalıştıran altyapı (yorumlayıcı + makine kodu üretici) hâlâ C++. Ayrıca bileşenler arası veri akışı henüz parametre tabanlı bir araç zinciri oluşturmuyor.

**Motora eklenenler (hepsi genel, uygulamaya özel değil):** PMY1 ifadesi `18 oku` (stdin'den bir bayt, bitince −1); C++ tarafında `parse_program` (yükleyicinin biçim çözme yarısı, davranış değişmedi), `pm liste` / `pm yazliste` (PMB1 ⇄ düz sayı listesi, **doğrulamasız**; kodlayıcı/çözücünün C++ referansı), `src/liste.cpp` (~70 satır).

| Bileşen | Dosya | Boyut (PMY1) | Girdi → Çıktı |
|---|---|---|---|
| Paketleyici (tam 64 bit) | `oz/paketle.sayi` → `.pmy` | 438 sayı, 458 B | sayı listesi (metin) → LEB128/zigzag bayt |
| PMB1 **çözücü** | `oz/coz.sayi` → `.pmy` | 1114 sayı, 1147 B | PMB1 bayt → düz sayı listesi (`pm liste` ile aynı) |
| PMB1 **kodlayıcı** | `oz/yaz.sayi` → `.pmy` | 882 sayı, 908 B | düz sayı listesi (mutlak etiket) → PMB1 bayt (`pm yazliste` ile aynı) |

**Doğrulama (otomatik, ajan yok):**
- `oz/dogrula.sh` (paketleyici, 49 denetim): bağımsız C++ referansı `oz/ref_paketle.cpp` (awk'ın 2^53 sınırı yerine tam 64 bit): depodaki 20 `.sayi`, sınır değerleri (0, 2^63−1, 2^63, 2^64−1, z±2^63), 10 rastgele 64 bit dosyası, 16 geçersiz/taşan girdi reddi, kendi kaynağını üretme (sabit nokta).
- `oz/dogrula_ikili.sh` (kodlayıcı + çözücü, **4553 denetim, hepsi geçti**): 81 gerçek PMB1 dosyası (tüm `.pma`, `.pmt`, `.pmy` kaynaklarından, adlı/adsız): çözücü çıktısı = `pm liste`; kodlayıcı çıktısı = orijinal baytlar = `pm yazliste`; **gidiş-dönüş bilgi kaybı yok** (bayt → liste → bayt → liste); 64 bit sınır listeleri (sabit 2^64−1, veri ±2^63, işlenen ±2^31, bellek 2^27); 14 elle yazılmış bozuk girdi (yanlış başlık, bayrak, bellek 0/aşırı, taşan/uzun/yarım LEB128, fazla bayt, bss>bellek, bilinmeyen komut, np>nr…) için C++ ile aynı ret kararı; üç dosyanın **tüm kesik önekleri**; **3200 rastgele bozulmuş kopya** (yaklaşık %22'si geçerli kalıyor): kabul/ret kararı aynı, kabul edilenlerde çıktı bayt-bayt aynı.
- Çekirdek: 203 test + 60 uygulama + 42 yapısal denetim değişmeden geçti.

**Sınırlar / henüz yapılmayanlar [Kesin]:** Çözücü yalnızca *biçimi* çözer (C++ `parse_program` ile aynı kabul/ret); *doğrulama* (`validate_program`: indeks/ofset/pencere denetimleri) ve *analiz* sonraki adım. Girdi 1 MiB ile sınırlı (dizi boyutu). Hatalar mesaj yerine tuzakla (çıkış 2) bildirilir. Hız: küçük dosyalarda süreç tabanı (≈ 10–19 ms) baskın; C++ `pm liste` 4 ms.

## 3b. Fizibilite: PMY1 tablosu bağımsız ELF üretiyor (`oz/elf/`)

**Soru:** Bir parametre tablosu, PMB1 sayı listesinden `pm` ve g++ olmadan çalışan geçerli bir Linux x86-64 ELF üretebilir mi? **Cevap [Kesin]: evet, desteklenen komut altkümesi için.**

**Düzen:** `oz/elf/elf.pmy` (3727 B, PMY1; kaynağı `gen.sh` ile `elf.sayi`) girdi olarak `pm liste` çıktısını (düz sayı listesi) okur, stdout'a ELF yazar. Üretici *sabit makine kodu kopyalamaz*: her PMB1 komutu için işlenen alanlarından (yazmaç, sabit, ofset, atlama hedefi) komut başına sabit boyutlu x86-64 kodu üretir; iki geçişli ofset hesabıyla atlama hedefleri çözülür. Sabit olan yalnızca ortak rutinler (puti/putc/tuzak/çıkış, ≈ 250 B) ve komut şablonlarıdır. ELF: tek RWX `PT_LOAD` @0x400000, 120 B başlık, yazmaçlar bellekte ([rbx+8r]), bellek dizisi r12 tabanlı; INTERP/DYNAMIC yok.

**Desteklenen altküme:** mov/ldi/ldk, tüm tamsayı ALU (+ sabit işlenenli `*i` biçimleri), karşılaştırmalar, jmp/jz/jnz, ret/halt, ld/st (sınır denetimli), host (yazdırma). **Desteklenmeyen (üretici reddeder, hata kodu ≠ 0):** call, alloc, kayan nokta, getc/present, yakıt sınırı. Tuzaklar: `TRAP: DIV0`, `TRAP: OOB` (stderr), çıkış 2.

**Doğrulama (`oz/elf/dogrula_elf.sh`, otomatik, ajan yok):** 92 denetim, hepsi geçti. 68 program `pm run` ile karşılaştırıldı (stdout bayt-bayt, çıkış kodu, tuzak adı): depodaki `tests/ uygulamalar/ bench/` `.pma` dosyaları + `rastgele_pma.awk` ile üretilen 60 rastgele program (60'ının 57'si çıktı üretti, 58 farklı çıktı özeti, 19'u DIV0 tuzağı; yani üretici farklı sayısal programlara farklı doğru davranış veriyor, tek bir sabit ikili değil). 19 program desteklenmeyen altküme dışı olduğu için reddedildi, 1 program yakıt sınırına bağlı olduğu için atlandı. Bağımsızlık: tek `PT_LOAD`, INTERP/DYNAMIC yok, ikili içinde `PMB1`/`libstdc`/`GLIBC` izi yok, `env -i` ile çalışır. `tests/trap_oob.pma` ve `collatz.pma` elle ayrıca doğrulandı.

**Sınırlar [Kesin]:**
- Üretici hâlâ C++ `pm` ile çalıştırılıyor: bu **geçiş kanıtıdır, tam öz-barındırma değil**. Üretilen uygulama `pm`'siz çalışır; üreticinin kendisi henüz değil.
- Üretici *güvenilir/önceden doğrulanmış* tablolar için: doğrulayıcı (`validate_program`) parametrelere taşınana kadar güvenilmeyen girdiden yerel kod üretmemeli (bellek/atlama indeksi denetimi yok).
- Yakıt (fuel) sınırı yok; sonsuz döngü ELF'te sonsuz döner. Tek fonksiyon.
- Test kapsamı: rastgele program üreticisi yalnızca desteklenen altkümeden üretir; stdin her zaman boş (getc desteklenmiyor); OOB tuzağı rastgele kümede çıkmıyor, yalnızca `trap_oob.pma` ile sınandı [Muhtemel yetersiz kapsam].
- Bulunan 3 üretici hatası düzeltildi (başlık çıktısı, `j==2` işlenen yazımı, `sayi` `-` bloğu); bunlar farksal test olmadan fark edilmezdi.

**Sıradaki adım (kullanıcı sırası):** doğrulayıcı + analiz taşındı (§3c); ELF üreticisi genişletildi ve pm'siz çalışıyor (§3d). Sırada: assembler/PMY1 açıcı gibi kaynaktan-tablo üretim araçlarını parametreye taşımak ve motorun kendi yeni sürümünü üretmesi.

## 3c. PMB1 doğrulayıcı + analiz parametre tablosunda (`oz/dogr/`)

**Ne taşındı [Kesin]:** C++ `validate_program` (indeks/ofset/pencere denetimleri) ve `analyze_program` (kesin atama analizi → `zero_free`), ayrıca `parse_program`'ın sayı sınırı denetimleri. Artefakt: `oz/dogr/valid.pmb` (3165 bayt PMB1 tablosu, 16 fonksiyon, 777 komut). Girdi: düz sayı listesi (`oz/coz.pmy` / `pm liste` çıktısı); çıktı: `1 Z` (Z=1: analiz ispatladı) ya da `0 K` (K: ret nedeni kodu).

**Yazım biçimi (dürüstlük):** Doğrulayıcı `valid.pma.in` (+ `tablolar.txt`'den üretilen komut tabloları) metin assembly'sinden `pm asm` ile üretilen PMB1 tablosudur; önceki üç bileşen (PMY1) elle sayı yazılarak yapılmıştı, 786 satırlık veri-akışı analizi için bu pratik değildi. Yani assembler (C++) hâlâ önyükleme aracı; yorumlayıcı da (C++ VM). Kullanılan komutlar: `call`, `ld/st`, `host getc/putc/puti`, tamsayı ALU — `alloc`, kayan nokta yok. Bu, ELF üreticisinin sıradaki genişletmesi (call + getc) tamamlanınca doğrulayıcının kendisinin bağımsız ELF'e çevrilmesine olanak verir [Tahmin: yapılabilir; denenmedi].

**Analiz hakkında düzeltme [Kesin]:** Plandaki "yazmaç/çağrı/bellek/kontrol akışı güvenlik koşulları" ayrımı koda tam oturmuyor: güvenlik denetimleri (yazmaç/atlama/sabit/fonksiyon/host indeksi, çağrı ve host penceresi, `divi/modi` sabiti, son komut, giriş fonksiyonu, veri sığması) **doğrulayıcıda**; C++ `analyze_program` ise yalnızca tek bir bool üretir (`zero_free`: "her yazmaç okunmadan önce her yolda yazılmış mı", VM'in çağrıda yazmaç sıfırlamasını atlaması için bir **eniyileme**). Bellek erişimi (OOB) statik denetlenmez, çalışma zamanı tuzağıdır. İkisi de taşındı; ek bir "bellek analizi" yoktu, yazılmadı.

**Doğrulama (`oz/dogr/dogrula_valid.sh`, otomatik, ajan yok) — tam çalışma: 4882 denetim, hepsi geçti:**
- 77 PMB1 (tüm `.pma/.pmt/.pmy` kaynakları): parametre tablosu = C++ referansı (`pm listedogrula`, yeni; liste biçimini katı çözer + `validate_program` + `analyze_program`).
- 86 elle yazılmış liste, **beklenen sonuç sabit** ("1 1", "1 0" ya da ret) ve hem C++ hem parametre tablosu buna eşit olmak zorunda: her ret kuralı, sınır değerleri (bellek 2^27, giriş 65534, operand int32 sınırları, sabit 2^64−1, veri ±2^63, "-0", `+36`, `--36`, 257 karakterli ad…), çağrı/host penceresi, analiz (dal birleşimi, döngü, ulaşılamayan kod, çağrı sonrası pencere, kullanılmayan fonksiyon).
- ~3000 liste düzeyinde rastgele mutant (belirteç değiştirme/silme/ekleme, satır silme/kopyalama/takas; 38 sınır değerinden seçim): kabul/ret kararı ve `zero_free` aynı (kabul 649, bunlardan `zero_free=0` 162, ret 4148).
- ~1500 bayt mutantı **zincirden** geçirildi: `coz.pmy → valid.pmb` kararı = C++ yükleyicinin (`decode_program`) kararı.
- **Test duyarlılığı (sabotaj):** doğrulayıcı/analize 23 farklı kasıtlı hata sokuldu. İlk fikstür kümesi 8 tanesini kaçırdı (çağrı/host pencere denetimi, host arity, analizde çağrı sonrası temizleme ve çağrı/host argüman okuması, alt fonksiyonda `np>nr`, 257 karakterli ad — fikstürlerimin bir kısmı yanlış kurulmuştu); fikstürler beklenen sonuçla yeniden yazılıp eksikler eklendikten sonra 22'si yakalanıyor. Kalan 1 (giriş indeksi üst sınırı 65534→65535) denk bir hata: `entry<nf` denetimi zaten reddeder, gözlemlenebilir davranış değişmiyor. Not: sabotaj sonrası yeniden yakalama kısa koşuyla (3 mutant) ve yalnızca kaçan 2 hata için yeniden ölçüldü; diğerleri ilk turdaki tam fikstür kümesiyle.
- `oz/boru.sh`: zincir `x.pmb → coz.pmy → valid.pmb → (yalnız "1 …" ise) elf.pmy → ELF`; bozuk bayt, doğrulayıcının reddettiği geçersiz yazmaç ve üreticinin desteklemediği program (`heap.pma`) için ELF **üretilmedi**, `collatz.pma` için üretildi ve çıktı `pm run` ile aynı. Bu, "doğrulayıcı hazır olmadan bilinmeyen tablodan makine kodu üretme" şartının mekanik karşılığıdır.

**Sınırlar [Kesin]:** (1) En çok 524288 sayı (belirteç); referans komut aynı sınırı uygular, `decode_program` uygulamaz. (2) Analizde bit kümesi boyutu M·W ≤ 524288 (C++: 2^24); aşılırsa `zero_free=0` (güvenli yön), kabul kararı değişmez; bu aralık (524288 < M·W ≤ 2^24) test edilmedi. (3) Bayt düzeyinde ad uzunluğu/dosya-boyutu kontrolleri (`nc > n` gibi) listede bulunmayan bilgiye dayandığı için `coz.pmy` tarafında kalır. (4) Hız: 300 bin belirteçli sentetik girdide 0,21 s (C++ referans 0,016 s); 682 belirteçli en büyük gerçek program 27 ms (süreç tabanı baskın). (5) Doğrulayıcı kendisi hâlâ `pm` ile çalışıyor.

**ELF OOB tamamlaması (`oz/elf/dogrula_oob.sh`):** 890 program (ld/st × 5 bellek boyutu × 11 taban × 8 ofset + 10 döngü/geri-sayım), 1640 denetim hepsi geçti; 750 program OOB tuzağına, 140'ı sınır içinde bitti; çıktı, çıkış kodu ve tuzak adı `pm run` ile aynı. Tüm koşular `ulimit -v` + `timeout` altında.

## 3d. Bağımsız ELF üreticisi genişletildi ve kendini yeniden üretti (`oz/elf2/`, `oz/bag/`)

**Yeni üretici `gen2` [Kesin]:** `oz/elf2/gen2.pmb` (4434 bayt PMB1 tablosu, 562 komut, 16 fonksiyon) `pm liste` biçimindeki bir programdan bağımsız Linux x86-64 ELF üretir. Önceki fizibilite üreticisinin (`oz/elf/elf.pmy`, tek fonksiyon) üstüne eklenenler: **çok fonksiyonlu program, `call`/`ret` (pencereli yazmaçlar, çağrıda yerel yazmaçları sıfırlama), `alloc`, `host getc`/`putc`/`puti`, tuzaklar DIV0/OOB/STACK/DEPTH/ALLOC** (C++ VM ile aynı sınırlar: 2^20 sözcük yazmaç yığını, 65536 kare; stderr `TRAP: AD`, çıkış 2). Mantık kodda değil **veri tablosunda**: `sablon.txt` komut başına x86-64 şablonunu tanımlar (bayt + alan yer tutucuları); `sablon.awk` yalnızca bunu sayı dizisine çevirir (`TPL/TPLS`); `gen2.pma.in` şablonları iki geçişte (boyut/konum, sonra çıktı) yorumlayan 560 komutluk döngüdür.

**Yazım biçimi (dürüstlük):** `gen2` `.pma` assembly'sinden `pm asm` ile üretilen PMB1 tablosudur (doğrulayıcıyla aynı gerekçe); `uret.sh` yalnızca komut tablolarını ve eski üreticideki puti/putc bayt bloklarını yerleştirir.

**Doğrulama (`oz/elf2/dogrula_elf2.sh`, otomatik, ajan yok) — tam çalışma: 2846 denetim, hepsi geçti:**
- 482 program `pm run` ile karşılaştırıldı (stdout bayt-bayt, çıkış kodu, tuzak adı; 53'ü tuzakla bitiyor): depodaki tüm `.pma` (boş ve dolu stdin), 400 rastgele çok fonksiyonlu program (çağrı, alloc, getc, döngü, bellek, bölme), elle yazılmış uç durumlar: sonsuz özyineleme (DEPTH), kare büyüklüğüne göre STACK, **eşikte** özyineleme (pm'in geçtiği en büyük derinlik ve +1; yığın sınırında tam eşitlik dahil), alloc tam dolu / 1 fazla / negatif / ikinci alloc, `data`+`space`, getc'de 0/128/255 baytları ve EOF, çağrıda yerel yazmaçların sıfırlanması, çağrı penceresinin üst üste binmesi, karşılıklı özyineleme.
- Desteklenmeyen girdiler (`fadd`, `present`, `putf`, `poll`, `frame`): üretici reddeder, stdout **boş** (kısmi ELF yok). Depodaki 12 program (kayan nokta/sunum kullanan uygulamalar) bu yüzden ELF'e çevrilemiyor.
- OOB: 890 programlık `dogrula_oob.sh` gen2 ile de geçti (1640 denetim).
- **`valid.pmb` (doğrulayıcı) ELF'e çevrildi**; 656 girdide (derlem + rastgele programlar + liste mutantları) `pm run valid.pmb` ile aynı çıktı ve çıkış kodu.
- **Sabit nokta [Kesin]:** `pm run gen2.pmb` → `gen2.elf`; `gen2.elf` kendi listesinden (pm'siz) kendi ELF'ini üretti: 1., 2. ve 3. nesil bayt-bayt aynı (27541 bayt). Not: bu, üretici mantığının doğruluğunu tek başına kanıtlamaz (aynı mantığın kendi çıktısı); doğruluk kanıtı yukarıdaki `pm run` karşılaştırmalarıdır. Sabit nokta, 560 komutluk büyük bir programın yerel kodunun VM ile aynı davrandığının ek kanıtıdır.
- **Test duyarlılığı (sabotaj):** şablonlara 12 kasıtlı hata sokuldu (DEPTH eşiği, STACK karşılaştırması, çağrıda sıfırlama sayısı, `mod -1`, `div -1`, alloc karşılaştırması, `ld` OOB karşılaştırması, `ret` değeri, `sar`↔`shr`, çağrı sonrası pencere geri alma, derinlik sayacı, getc EOF). İlk turda ikisi kaçtı: `ld` için `jae`→`ja` (yalnızca adres tam bellek boyuna eşitse görünür; OOB denetimi gen2'ye uygulanınca yakalandı) ve STACK karşılaştırması `<`→`<=` (yalnızca kalan alan tam çerçeve boyuna eşitse görünür; yazmaç yığınında tam eşitlik kuran test eklenince yakalandı). Sonuçta 12'si de yakalanıyor; `getc EOF` hatası testte asılma/zaman aşımı olarak göründü (çalışma yarıda kesildi, kesin hata sayısı alınmadı). `st` için aynı mutasyon ölçülmedi.

**pm'siz araç zinciri (`oz/bag/`):** `kur.sh` bir kez `pm` kullanır (gen2.elf'i üretmek, PMY1 çözücü/kodlayıcıyı PMB1'e çevirmek); sonrası `gen2.elf` ile: `coz.elf` (16825 B), `yaz.elf` (13171 B), `valid.elf` (21851 B). `zincir.sh x.pmb çıktı.elf` yalnızca bu ELF'leri çağırır: `coz.elf → valid.elf ("1 …" ise) → gen2.elf`; doğrulayıcı reddederse ELF üretilmez. `dogrula_bag.sh` bunu `{timeout, chmod, rm}` dışında hiçbir komutun bulunmadığı bir PATH ile çalıştırır (pm/g++ erişilemez) ve 1216 denetimde (77 program + bayt mutantları) referansla (`pm` yükleyici kararı + `pm run gen2.pmb`) aynı sonucu verir: 103 program üretildi (ELF'ler bayt-bayt referansla aynı, davranış `pm run` ile aynı), 1014 girdi reddedildi.

**Hız ve sınırlar [Kesin]:** `fib(35)`: gen2 ELF 0,345 s, `pm run` yorumlayıcı 0,57 s, C++ `--native` 0,098 s (yazmaçlar bellekte, yazmaç ayırma yok: C++ yerel arka ucundan yaklaşık 3,5 kat yavaş). Yakıt sınırı yok (sonsuz döngü ELF'te sonsuz döner; testler timeout altında). Üretici girdiyi doğrulamaz (güvenilen tablo; `zincir.sh` doğrulayıcıyı önce çalıştırır). Kayan nokta, `present/poll/frame/putf` yok. ELF belleği: 8 MiB yazmaç yığını + `mem_words` × 8 bayt (sanal; en çok ~1 GiB). Üretici 524288 simge / 524288 komutla sınırlı.

**Hâlâ C++ olan [Kesin]:** `pm run` (VM, tüm tabloları çalıştıran önyükleme yorumlayıcısı ve referans), `pm asm` (assembler: doğrulayıcı ve gen2 bu metinden üretildi), `pm yapisal` (PMY1 → PMB1; yalnızca çözücü/kodlayıcıyı ELF'e çevirirken), `pm liste` (referans), derleme ve test araçları. Yani zincirin **çalışma zamanı** C++'sızdır; **kaynaktan üretim** (assembler, PMY1 açıcı) henüz değil.
## 4. Açık sorular ve riskler
- **Çekirdek sınırı:** hangi parçalar C++'ta kalmalı? Öneri: çözücü+doğrulayıcı+yorumlayıcı+host (yaklaşık 800 satır) en küçük gerçekçi çekirdek; native üretici ve özelleştiriciler parametreye taşınır; `exec` yeni ilkeldir.
- **Performans:** VM üstünde çalışan araçlar (özelleştirici, birleştirici) yavaşlar; ama bunlar bir kez çalışan yükleme-zamanı araçları ve makine koduna çevrilince (`--native`) kabul edilebilir olmalı. Ölçülecek.
- **Yazım maliyeti:** elle sayı yazmak hata yapmaya açık (bu adımda 0 hata, ama D1 referansında 3 hata yapmıştım). Büyük bileşenler için (birleştirici, özelleştirici) PMY1'i üreten bir üst düzey kaynak biçimi gerekebilir; bu bir C++/awk aracı olursa önyükleme aracı sayılır ve hedefle çelişmez, ama sabit nokta testiyle sonunda parametre sürümüyle değiştirilmelidir.
- **AI üretimi:** bu bileşenleri sıfırdan AI'ya yazdırmak değil, mevcut C++ davranışını referans alıp otomatik farkla doğrulamak; ajan kullanılmadı.
