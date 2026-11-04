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

**Durum:** planın 6 adımı tamamlandı (§3c doğrulayıcı+analiz, §3d üretici, §3e assembler + motorun kendini yeniden üretmesi); ardından PMY1 dönüştürücüsü ve awk/sed üretim adımları parametreye taşındı, uçtan uca PMY1 → ELF bağımsız üretim sınandı (§3f). Kalan: `pm run` ikincil referans olarak kalır; ondalık işlemler, SSE ve grafik sonraki yetenek aşaması.

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

**Hız ve sınırlar [Kesin]:** `fib(35)`: gen2 ELF 0,345 s, `pm run` yorumlayıcı 0,57 s, C++ `--native` 0,098 s (yazmaçlar bellekte, yazmaç ayırma yok: C++ yerel arka ucundan yaklaşık 3,5 kat yavaş). Yakıt sınırı gen2 v1’de yoktu; v2’de eklendi (§3e). Üretici girdiyi doğrulamaz (güvenilen tablo; `zincir.sh` doğrulayıcıyı önce çalıştırır). Kayan nokta, `present/poll/frame/putf` yok. ELF belleği: 8 MiB yazmaç yığını + `mem_words` × 8 bayt (sanal; en çok ~1 GiB). Üretici 524288 simge / 524288 komutla sınırlı.

**Hâlâ C++ olan [Kesin]:** `pm run` (VM, tüm tabloları çalıştıran önyükleme yorumlayıcısı ve referans), `pm asm` (assembler: doğrulayıcı ve gen2 bu metinden üretildi), `pm yapisal` (PMY1 → PMB1; yalnızca çözücü/kodlayıcıyı ELF'e çevirirken), `pm liste` (referans), derleme ve test araçları. Yani zincirin **çalışma zamanı** C++'sızdır; **kaynaktan üretim** (assembler, PMY1 açıcı) henüz değil.
## 3e. Altıncı aşama: assembler parametre tablosunda; motor kendi yeni sürümünü üretti (`oz/asm/`, `oz/bag/`)

**Assembler `oz/asm/asm.pmb` [Kesin]:** C++ `assemble()` (src/asm.cpp, 520 satır) ile aynı algoritmayı yürüten 14121 baytlık PMB1 tablosu (3570 komut, 62 fonksiyon). stdin `.pma` metni, stdout `pm asm --strip` ile **bayt bayt aynı** PMB1. Etiketler, dolaylı operand dönüşümü (`rewriteImm`), sabit yazmaçları, çağrı pencereleri ve üç kurallı gözetleme deliği (peephole) C++ ile aynı sırayla işlenir; sabit havuzu sırası bile aynıdır. Hata: stdout'a `HATA satir N: kod` yazar ve DIV0 tuzağıyla (çıkış 2) biter.

**Doğrulama (`oz/asm/dogrula_asm.sh`, otomatik, ajan yok) — tam çalışma 2069 denetim, hepsi geçti:** 70 kaynak (depodaki `.pma` + `.pmt`/`.pmy`'den üretilenler), 400 rastgele program (iki üreteç), 70 elle yazılmış uç durum (her `rewriteImm` kuralı, `sub x,y,-2^31`, karakter/dize/`\xHH` veri, hex ve 64 bit taşma, space/data çakışmaları, giriş fonksiyonu, `memory` sığmama, ad çakışmaları, CRLF, yorum içinde `;`, etiket+komut aynı satır…) ve 1500 kaynak mutantı: **629'unda C++ ile aynı üretim, 1403'ünde aynı ret kararı**. Ayrıca kendi kaynağını (`asm.pma`) ve `gen2.pma`/`valid.pma` kaynaklarını derlemesi C++ ile aynı; `asm.pmb(asm.pma) == asm.pmb` (sabit nokta); ELF'e çevrilmiş sürümü (`asm.elf`, 95 KB) de aynı çıktıyı verir.
- **Test duyarlılığı (sabotaj):** 13 kasıtlı hata denendi (gözetleme deliği A/B/C kuralları, `sub` INT32_MIN, kaydırma/bölme/`ge` dönüşümleri, sabit tekilleştirme, karakter sabiti yorum kesme, sondaki etiket, hex taşma sınırı). 9'u ilk turda, `hex_limit` ise hex taşma vakaları eklenince yakalandı. Yakalanamayan: gözetleme deliği B kuralındaki `q>=M` sınır denetimi (bu yolun gözlemlenebilir bir farkı bulunamadı). İki mutasyon (`lit_dedupe`, `mkint_neg`) sed deseni uygulanamadığı için **ölçülmedi**.
- **Sınırlar [Kesin]:** ondalık sabitler (`2.0`) desteklenmez (parametre tablosunda `strtod` yok) → hata kodu 90; depoda 4 kaynak (`float.pma`, `float2.pma`, birkaç türetilmiş) bu yüzden C++ ile farklı davranır ve sayaçta "bilinen fark" olarak ayrıdır. `validate_program`/`analyze_program` assembler içinde çalışmaz (çıktı `valid.pmb` ile doğrulanır); yalnızca "veri belleğe sığmıyor" ve "giriş fonksiyonu parametre almamalı" burada da denetlenir. Kaynak en çok 1 MiB, fonksiyon başına 65536 deyim; adlar atılmış ikili üretir (`--strip`).

**Üretici v2: yakıt (FUEL) sınırı eklendi [Kesin].** Önceki sürümde ELF'te yakıt sınırı yoktu. Şablon tablosuna (`sablon.txt`) ve üreticiye yeni özellik olarak: `r14` yakıt sayacı (varsayılan 4e9 = `pm run` ile aynı); her `call`, her `host` ve her **geriye doğru alınan** atlamada kontrol (`jmp/jz/jnz` hedef ≤ komut indeksi → `*_back` şablonları); yakıt 0 ise `TRAP: FUEL`, çıkış 2. Test için girdi başına `-1 N` öneki yakıtı N yapar (geçerli listeler `-1` ile başlayamaz). `dogrula_elf2.sh` §1f: 45 depo programı × 4 yakıt değeri + 100 rastgele program + elle yazılmış döngü/özyineleme/host/ileri-atlama programları, her biri `pm run --fuel N` ile **aynı stdout, çıkış kodu ve tuzak adı** (tuzağın düştüğü nokta dahil; FUEL/DEPTH önceliği dahil). 251'lik küçük turda ve tam turda (2478 denetim) hepsi geçti.

**Yakıt şablonlarının sabotaj denemesi:** 6 kasıtlı hata (jz geri atlama sıçrama uzunluğu, `jnz` geri atlama koşulu ters, `call`/`host puti`/`host getc`/geri `jmp` yakıt kontrolü eksik) hepsi yakalandı (9–75 test hatası).

**Motor kendi yeni sürümünü üretti (`oz/bag/yenile.sh`) [Kesin]:** `yenile.sh ESKI YENI` yalnızca eski araç takımını (`asm.elf`, `gen2.elf`, `coz.elf`, `valid.elf`) kullanır; PATH'inde `pm`, `g++` ve Python olmayan bir ortamda (yalnız bash/awk/coreutils) çalışır: kaynak → `.pma` (awk/bash biçim dönüşümü) → `asm.elf` → PMB1 → `coz.elf` ile liste → `valid.elf` onayı (`1 …` şart) → eski `gen2.elf` ile ara nesil ELF → **yeni `gen2.elf` ile tüm araçların son nesli** → sabit nokta (son `gen2.elf` kendi listesinden kendini bayt bayt üretir). v1 (yakıtsız, 27541 B) → v2 (yakıtlı, 31133 B) yenilemesi 0,7 saniye sürdü; çıkan beş ELF (gen2, asm, coz, yaz, valid) `pm` ile kurulan takımla **bayt bayt aynı**.
- **A → B → A turu (`dogrula_yenile.sh`, 45 denetim, hepsi geçti):** E2 (yakıtlı) → v1 kaynaklarından E1 (yakıtsız) → tekrar mevcut kaynaklardan E2b; E2b'nin beş ELF'i E2 ile aynı; E1.gen2.elf, v1 kaynaklarından `pm` ile kurulanla aynı; yeni takım `-1 N` önekini işler; yeni takımla `zincir.sh` ve `asm.elf` çalışır.
- **Yeni takımla tam paketler (TAM=1):** yenilenmiş **yerel** `gen2.elf` ile `dogrula_elf2.sh` 2478 denetim, kalan 0; yenilenmiş `asm.elf` ile `dogrula_asm.sh` 673 denetim, kalan 0.
- v1 kaynakları `oz/bag/nesil_v1/` altında (yalnızca `sablon.txt` ve `gen2.pma.in`; diğer dosyalar ortak).

**Hâlâ parametre olmayan / C++ olan [Kesin]:** (1) `pm run` yorumlayıcısı: ilk kurulum (`kur.sh`) ve tüm testlerin referansı; (2) `pm yapisal` (PMY1 → PMB1): çözücü/kodlayıcının PMB1 hâli `oz/coz.pmb`, `oz/yaz.pmb` olarak depoda tutuluyor (bu iki dosya C++ ile üretildi, sonra yenilemelerde yalnızca onlar kullanılır); (3) **awk/bash yapıştırıcı**: `sablon.awk`, `tablolar.sh`, `uret.sh` komut tablolarını ve şablonları sayıya çevirir (yalnızca biçim dönüşümü ama parametre tablosu değil); (4) araçların **kaynağı** assembly metnidir (`*.pma.in`); (5) test düzeneği (`dogrula_*.sh`, `pm` referansı). Yani döngü artık kapalı: motor kendi araçlarını kaynaktan, **C++ olmadan**, yeniden üretebilir; ama kaynaktan-tabloya yolun ilk başlatılması ve doğruluk ölçütü hâlâ C++ referanstır.
## 3f. PMY1 dönüştürücü ve üretim araçları parametre tablosunda; uçtan uca bağımsız üretim (`oz/yap/`, `oz/arac/`, `oz/bag/`)

Bu bölüm kullanıcı kararının dört maddesini kapatır: (1) `pm yapisal` parametreye taşındı, (2) `awk/sed/od` tabanlı üretim adımları parametreye taşındı, (3) yeni bir PMY1 tanımından yalnızca bağımsız araçlarla ELF üretildi, (4) C++ referans korunarak karşılaştırıldı. `pm run` yorumlayıcısı bilerek **silinmedi**: test referansı ve ilk kurulum aracı olarak duruyor.

**PMY1 özelleştirici `oz/yap/yap.pmb` [Kesin]:** `src/yapisal.cpp`'nin (612 satır) `yapisal_to_pma` işini yapan 20250 baytlık PMB1 tablosu (4277 komut, 54 fonksiyon). PMY1 baytlarını çözer (ifade/komut ağacı, şablonlar, alan başlatıcıları, çizim, etkileşim, girdi, toplama), davranışları makro olarak açar (parametre yerine koyma ve derinlik denetimi dahil), yazılan alanları işaretler ve `.pma` metnini üretir. Çıktısı `pm yapisal --pma` ile **bayt bayt aynıdır**. Metin biçimleri (`fmt.txt`) veridir: `%o` operand, `%n` sayı, `%A` değişken liste, `%P` parametre listesi. `.pma` → `asm.elf` → PMB1 zinciri `pm yapisal -o --strip` ile aynı baytları verir.
- **Doğrulama (`oz/yap/dogrula_yap.sh`) — tam çalışma 2365 denetim, hepsi geçti:** 39 depo tablosu/sayı listesi; 300 rastgele **geçerli** model (`rastgele_pmy.awk`: bağlam duyarlı üreteç; 200 örneğin 173'ü C++ tarafından kabul edildi ve davranış, çizim, etkileşim, girdi, toplama, bitiş, kir/devam, sec, dizi, talan, çağrı, rastgele alan vb. hepsini içeriyor); **76 sınır değerli model** (`sinir_modelleri.sh`: ifade derinliği 199–203, blok derinliği 99–102, davranış zinciri 15–18, sahne 4096/4097, dizi boyutu 2^24±1, şablon örnek sayısı 10^7±1, şablon sayısı 1000/1001, işlev sayısı 5000/5001, parametre/yerel sınırları, kod aralıkları, fazla/kesik bayt…); 1500 bayt mutantı. Kabul/ret kararı ve kabul edilince metin ile PMB1 aynı (zincirde 391 kabul, 1542 ret). `yap.elf` (gen2 ile üretilmiş, 170 KB) aynı çıktıyı verir.
- **Test duyarlılığı (sabotaj):** 8 sınır sabotajı (derinlik 200/100/16, `nl` 257, sahne 4096, şablon 1000, dizi 2^24, örnek 10^7) ilk turda **kaçtı** (testlerde sınır değer yoktu); sınır modelleri eklenince 8'i de yakalandı. Diğer 7 sabotaj ilk turda yakalandı (davranış argüman sayısı, rastgele alan sabiti, toplama başlangıcı, arka plan alanı, prolog `np`, minimum bellek, kare karşılaştırması: 12–104 test hatası). İki mutasyon sed deseni uygulanamadığı için ölçülmedi.
- **Sınırlar [Kesin]:** girdi ≤ 1 MiB; ifade düğümü 262144, komut düğümü 131072 (davranış açılımı dahil); çıktı ≤ 2,2 M karakter; sayaç tavanları: global 65536, dizi 4096, işlev 8192 (C++ zaten >5000'i reddeder), davranış 4096, şablon 1024 (C++ >1000'i reddeder), alan/çizim toplamı 65536, başlangıç değeri 131072. C++ bunlardan büyük sayıları dosya boyutuna kadar kabul eder; bu aralıkta davranış farkı vardır (test edilmedi).

**Üretim araçları (awk/sed/od yerine) `oz/arac/` [Kesin]:** dört küçük PMB1 programı. `doldur` (yer tutucu doldurma: sözlük + `@@SABLON` + şablon, `@ANAHTAR@` → değer, bilinmeyen anahtar → ret), `opstab` (`src/ops.h` → `OPNAMES/NOPS/KIND/OPCOUNT` sözlüğü), `fmtc` (`fmt.txt` → `STO/STRT` + `F_AD` kimlikleri), `sablonc` (x86-64 şablon bildirimi `sablon.txt` → `TPLS/TPL`). `asm/`, `elf2/`, `dogr/` ve `yap/` altındaki `uret.sh` betikleri artık yalnızca bu araçları, `cat` ve assembler'ı çağırır; `awk`, `sed`, `od`, `tr`, `grep` yok. Üretilen `asm.pmb`, `gen2.pmb`, `valid.pmb` eski awk yoluyla üretilenlerle **bayt bayt aynı** çıktı.
- **Doğrulama (`oz/arac/dogrula_arac.sh`) — 1508 denetim, hepsi geçti (666'sı aynı ret kararı):** eski awk/sed uygulamaları `oz/arac/ref/` altında REFERANS olarak (yalnızca test oracle'ı) tutulur; araçlar gerçek girdilerde ve rastgele varyantlarda (rastgele `ops.h`, rastgele biçim tabloları, rastgele sözlük/şablonlar, rastgele x86 şablon bildirimleri, bilinmeyen simge/ad hataları) referansla aynı çıktıyı/ret'i verir. Bilinçli fark: yinelenen şablon tanımını `sablonc` reddeder, referans awk sessizce üzerine yazardı.
- Statik veri (`oz/elf2/statik.sozluk`: ELF mesajları, `puti/putc` makine kodu baytları) bir kez `elf/T.txt`'den çıkarılıp sabit veri olarak depoya kondu.

**Uçtan uca bağımsız üretim [Kesin]:** `oz/bag/zincir.sh girdi.(pmy|pmb) cikti.elf`: `yap.elf → asm.elf → coz.elf → valid.elf → gen2.elf`; PATH'inde yalnızca 13 coreutils komutu olan bir ortamda (pm, g++, awk, sed, od, tr, grep yok) çalışır; yapı reddedilirse ya da doğrulayıcı reddederse ELF üretilmez.
- `oz/bag/dogrula_pmy2elf.sh` — **397 denetim, hepsi geçti:** 39 depo tablosu, 2 elle yazılmış metin uygulaması (beklenen çıktı elle hesaplandı: `51` ve `0 1 4 9 16`) ve 300 rastgele model; 87 ELF üretildi ve davranışı `pm run tanim.pmy` ile (stdout + çıkış kodu) aynı; üretilen PMB1, `pm yapisal -o` ile aynı; 247 girdi reddedildi (C++ de reddediyor ya da ekran/girdi ana makinesi gerektiriyor: gen2 henüz `present/poll/frame` desteklemiyor); 13 girdi sonsuz döngü/çıktı nedeniyle atlandı. **Not:** ekranlı PMY1 uygulamaları (grafik) bu zincirle ELF'e çevrilemez; bu, "ondalık ve grafik" yetenek tamamlama aşamasına bırakıldı.
- **Motorun bir sonraki sürümünü üretmesi (`yenile.sh`, `dogrula_yenile.sh` — 51 denetim, hepsi geçti):** `yenile.sh ESKI YENI` yalnızca eski ELF takımıyla (10 ELF: gen2, asm, valid, yap, doldur, opstab, fmtc, sablonc, coz, yaz) `src/ops.h`, assembly kaynakları, `sablon.txt`, `fmt.txt` ve **PMY1 kaynaklarından** (`oz/coz.pmy`, `oz/yaz.pmy`: artık `coz.pmb/yaz.pmb` ikilileri depoda tutulmuyor) tüm takımı yeniden üretir; ara nesil → son nesil → sabit nokta. PATH'te yalnızca bash ve 12 coreutils komutu varken 10 ELF **1,2 saniyede** üretildi ve `pm` ile kurulan takımla **bayt bayt aynı**. A → B → A turu (yakıtlı → yakıtsız → yakıtlı) 10 ELF'in hepsi için tuttu. Yeni (yenilenmiş) yerel takımla tam paketler: ELF üretici 2478, assembler 678, yapısal özelleştirici 837, zincir 674 denetim, hepsi geçti.

**Hâlâ parametre olmayan [Kesin]:** (1) `pm run` yorumlayıcısı: ilk kurulum (`kur.sh`: `pm` ile gen2.elf'i üretir) ve her testin referansı; bilerek korunuyor. (2) Test düzeneği hâlâ `awk/sed/g++/bash` kullanır (rastgele program/model üreteçleri, mutasyon aracı `mutasyon.cpp`, `.sayi → .pmy` paketleyici `ai_testi3/paketle.awk`, referans uygulamalar); bunlar yalnızca doğrulama aracıdır, üretim zincirinde değildir. (3) Araçların kaynağı assembly metnidir (`*.pma(.in)`), PMY1 değil; `ops.h` hâlâ C++ başlığıdır (komut tablolarının tek kaynağı). (4) `kur.sh` depodaki `.pmb` tablolarına (asm, gen2, valid, yap, arac) güvenir; bunlar `yenile.sh` ile kaynaktan yeniden üretilebilir ama ilk üretim `pm asm` idi. Ondalık işlemler, SSE ve grafik: sonraki aşama.
## 3g. Yedinci aşama (1/5): `present`, `poll`, `frame` bağımsız ELF'te (`oz/elf2/rt.pma`, `dogrula_grafik.sh`)

**Hedef (kullanıcı kararı) [Kesin]:** gerçek pencere değil; mevcut sanal çerçeve tamponu davranışını bağımsız ELF'te birebir üretmek (`src/host.h`: FNV tabanlı kare özeti ve zincir, olay kuyruğu, `--max-kare`, `--kare-rapor`, `--ekran`, `--ascii`).

**Tasarım [Kesin]:** çalışma zamanı kütüphanesi elle makine kodu değil, `.pma` olarak yazıldı (`oz/elf2/rt.pma`, 10 fonksiyon, 340 komut). `uret.sh` onu PMB1 listesine çevirip üreticinin sözlüğüne (`RTL`, `RTLN`) koyar; `gen2` bunu uygulama fonksiyonlarının sonuna **ayrıcalıklı fonksiyon** olarak ekler:
- Uygulamadaki `host present/poll/frame` komutu, RT fonksiyonlarına normal bir `call` olarak derlenir (argüman ve sonuç kuralı aynı).
- Ayrıcalıklı fonksiyonlarda `ld/st` şablonları (`ldp/stp`) uygulama belleğinin altındaki 64K sözcüklük RT bölgesini (durum bloğu, olaylar, çıkış tamponu) ve uygulama belleğinin sonrasındaki kare kopyasını da kapsar; çağrı ve atlamalar yakıtsızdır; `host present` ayrıcalıklı fonksiyonda `write(fd, ptr, n)` sistem çağrısıdır (`host_sysw`).
- Her çıkış ve tuzak yolu `exitc`/`exitc_rt` üzerinden geçer: önce RT çıkış raporu (stderr kare raporu, fd 3'e son kare PPM, stdout'a ASCII), sonra tuzak mesajı. Bu sıra `pm run` ile aynıdır.
- RT yalnızca uygulama `present/poll/frame` kullanıyorsa ya da rapor bayrağı istenmişse eklenir; eklenmezse ELF, RT'siz yolu kullanır (grafiksiz programlar için ek bellek ya da kod yok). RT'li ELF'te bellek 2 kat + 512 KiB sanal alan ister.
- **Seçenekler üretim anında** stdin ön ekiyle ELF'e gömülür (yakıt gibi): `-1 yakıt`, `-2 n (kare kod)*n` olaylar (≤30000; çağıran kareye göre kararlı sıralar), `-3 azami_kare`, `-4 ascii_K`, `-5 bayrak` (1 kare raporu, 2 son kare PPM → fd 3). **ELF çalışma anında argüman ya da olay dosyası okumaz [Kesin]**; bu bilerek bırakılmış bir sınırdır.

**Doğrulama [Kesin]:**
- `oz/elf2/dogrula_grafik.sh`: 97 karşılaştırma (her biri seçenekli + seçeneksiz ELF), hepsi `pm run` ile aynı: stdout, çıkış kodu, kare raporu (kare sayısı, boyut, zincir, son özet), son kare PPM, ASCII, tuzak adı. Kapsam: hayat, sahne, pong (olaylı), tuğla, kovala, parçacık, veri; `grafik/` altında elle yazılmış uç durumlar (geçersiz `present` argümanları, olay zamanlaması/sırasız/bozuk satır, boyut değişimi, piksel üst bitleri, 640×480, OOB/DIV0/DEPTH tuzağı kirli yığınla, `halt`, yalnızca `frame`, yalnızca `poll`, grafiksiz tuzak).
- **Sabotaj (`sabotaj_grafik.sh`) [Kesin]:** 23 mutant; ilk turda 17'si yakalandı, 6'sı kaçtı. Kaçanların çözümlemesi: 3'ü gözlenebilir davranışı değiştirmeyen **eşdeğer mutant** (`ptr==n` zaten alan denetimiyle reddedilir; çıkış tamponu eşiği 32768→32769 yalnızca uygulamanın 0. sözcüğüne taşar; piksel maskesi 24→25 bit — tüm tüketiciler baytı zaten maskeler), 3'ü **gerçek test boşluğu** (seçeneksiz ELF yolu hiç denenmiyordu: `tarama_frame`, `tarama_poll`; grafiksiz tuzak yolu `exitc` denenmiyordu). Üçü için test eklendi, alt küme çalıştırmalarında yakalandı. Betikte kalan 20 mutantın 20'si yakalanıyor, ama **tek bir temiz tam tur** henüz koşulmadı (ilk tam tur 12 dk).
- Mevcut takımlar: `dogrula_elf2.sh` 2767 geçti (kalan 3 = "present/poll/frame reddedilmeli" diye yazılmış eski testler; artık yanlış oldukları için kaldırıldı); `dogrula_arac.sh` (sablonc, genişletilmiş şablon listesiyle) 758 geçti, kalan 0.
- Zincir testleri (yeni `gen2` ile): `dogrula_pmy2elf.sh` 211 geçti, kalan 0 (67'si grafik seçenekli karşılaştırma; 19 model `pm run` zaman aşımı/abort nedeniyle atlandı); `dogrula_yenile.sh` 51 geçti, kalan 0 (A→B→A turu, pm'siz yeniden üretim); `dogrula_bag.sh` 1244 geçti, kalan 0 (ilk koşuda 134 hata: `zincir.sh`e eklediğim `cat` testin 3 komutluk PATH'inde yoktu; düzeltildi). `dogrula_asm/yap/valid.sh` bu değişiklikten sonra yeniden koşulmadı.
- **Ret sınıflandırması (`oz/bag/siniflandir_ret.sh`) [Kesin]:** 332 PMY1 girdisinden önce 85'i ELF üretiyordu; şimdi 292'si üretiyor. Kalan 40'ı C++ `pm yapisal` zincirinde de geçersiz (18 yapı/bağlam, 22 assembler reddi). Yani eski 247 retten 207'si yalnızca üretici eksiğiydi ve kapandı.

**Bilinen sınırlar / ölçülmemiş [Kesin]:** (1) _[Düzeltme]_ İlk yazımda "yakıt muhasebesi pm ile birebir değil" demiştim; bu **ölçülmemiş bir iddiaydı**. Yakıt kuralı `anlam.h`'de zaten tek ve aynı (CALL, alınan geri atlama, HOST = 1 yakıt); `dogrula_grafik.sh` bölüm 3 şimdi 7 programda 9 yakıt değerinde (63 durum) tuzak noktasındaki kare sayısını ve çıktıyı `pm` ile karşılaştırır: hepsi aynı. (2) Çalışma zamanı performansı (PPM yazımı bayt bayt `wb` çağrısıdır) ölçülmedi. (3) Gerçek pencere/girdi yok. (4) Seçenekler üretim anında gömülür, ELF çalışma anında argüman/olay dosyası okumaz.

## 3h. Yedinci aşama (2/5): ondalık işlemler (SSE2), `putf`, assembler'da ondalık sabit

**ELF'te ondalık komutlar [Kesin]:** `fadd fsub fmul fdiv fneg fsqrt feq flt fle i2f f2i` için SSE2 şablonları (`sablon.txt`). `f2i` C++ ile aynı kuralı uygular (NaN → 0, taşma → INT64_MAX/MIN). `oz/elf2/dogrula_float.sh` **bit düzeyinde** karşılaştırır (yazmaç değeri tamsayı olarak basılır, `putf`'e bağlı değildir): 41 özel değer (±0, ±∞, farklı yüklü NaN'lar, alt normaller, 2^63 sınırı…) × 7 ikili işlem × tekli işlemler + 5 tohumlu 30000 rastgele desen. Sonuç: hepsi `pm run` ile aynı (NaN yükü dahil).
**`putf` (`%.15g`) [Kesin]:** `rt.pma` içinde, tam sayılarla ve büyük sayı (N/D) aritmetiğiyle yazıldı: değer `mant·2^e` olarak ölçeklenir, 15 basamak çıkarılır, kalan yarım-eşitlikte çifte yuvarlanır; `%g` biçimi (üstel/sabit eşiği, `e+05`, sondaki sıfırların atılması, `-0`, `inf`, `nan`/`-nan`) yeniden uygulanır. Doğrulama: özel değerler, 10^k ve 2^k kuvvetleri (±330/±1100), 4 tohumlu 12000 rastgele değer (ham desen, ondalık, tam yarım-eşitlik `n+0.5`, ikili kesir) `pm` (`snprintf`) ile aynı. `tests/float.pma` ve `float2.pma` beklenen çıktıyı birebir verir.
**Assembler'da ondalık sabit [Kesin]:** `asm.pma.in` içindeki `fbits` (büyük sayı, en yakın-çifte yuvarlama = `strtod`). Önceki "bilinen fark" kapandı: `dogrula_ondalik.sh` ve `dogrula_asm.sh` rastgele + sınır sabitlerde (alt normal/taşma sınırları, 2^53±1 yarım-eşitlik, 800+ basamaklı yapışkan kuyruk, 800+ tam sayı basamağı) PMB1'i C++ `pm asm` ile **bayt-bayt** karşılaştırır.
**Sabotaj:** ELF ondalık şablonları 12/12 yakalandı. `putf` mutantlarında eşdeğer olan iki mutant (`sifir_kirp`, `tahmin`) çıkarıldı; assembler ondalık sabotajında eşdeğer olan `drop_sinir` ve (ilk biçimiyle) `tasima_m` düzeltildi/çıkarıldı. Düzeltilmiş `tasima_m` (46 hata) ve `putf_kare` (5 hata) yakalandı; assembler ondalık sabotajında kalan eşdeğer olmayan tüm mutantlar yakalandı (16/16 + düzeltilmiş 1).

## 3i. Yedinci aşama (3/5): kanonik sayısal komut tablosu (`oz/komut/`) — komut tanımlarında `ops.h` artık otorite değil

**Karar (kullanıcı):** sayısal parametreler tek doğruluk kaynağı; okunabilir metin isteğe bağlı görünüm; güvenlik katmanları ayrı ve sonradan eklenebilir; yetenek sınırlanmayacak; AI doğrudan sayı üretebilecek. Ayrıntılı biçim ve uyumluluk: `oz/komut/KOMUT_TABLOSU.md`.

### Envanter: komut bilgisinin tek doğruluk kaynakları (başlangıç durumu) [Kesin]
| Bilgi | Önce nerede tanımlıydı | Şimdi |
|---|---|---|
| komut kimliği, mnemonik, operand sayısı/türleri | `src/ops.h` (X-makro) | **`oz/komut/komutlar.pmk`** (sayılar) |
| host listesi (ad, kimlik, argüman sayısı) | `ops.h` enum + `ikili.cpp` + `asm.pma.in` (`ARITY`) + `valid.pma.in` | tablo (HOST bölümü); hepsi üretilir |
| araç sözlükleri (`OPNAMES`, `NOPS`, `KIND`) | `opstab` ← `ops.h` metni | `komut @@tab` ← sayısal tablo |
| şablon anahtarı | mnemonik metni | komut **sembolü** (mnemonik değişse şablon etkilenmez) |
| komut sayısı/kimlikleri araç kaynaklarında | 55, 34, 40, 62, 81 … literal sayılar | `@OPCOUNT@`, `@OP_CALL@`, `@NAMECOUNT@`, `@HOSTCOUNT@`, `@TS_*@` (şablon yuvaları `sablonc`'tan), `@H_*@` |
| PMB1 çözücü/kodlayıcı: komut başına operand sayısı ve operand tel sınıfı | `coz.sayi`/`yaz.sayi` içinde elle yazılmış 55'lik dizi ve `kind(op,j)` işlevi (op numaralarıyla) | tablodan: `NOPSZ`, `KINDW` (KIND ext anahtar 2); `oz/uret_ikili.sh` |

### Yapılan değişiklikler [Kesin]
- **PMK1 biçimi:** sürümlü, bölümlü (KIND/OP/HOST), kayıt uzunluklu, `ext` (anahtar,değer) ve bilinmeyen-kuyruk jetonlarını koruyan sayısal biçim; bilinmeyen isteğe bağlı bölüm korunur, zorunlu olanı reddedilir; major farkı reddedilir, minor kabul. Ayrılmış bölüm aralıkları: yürütme anlamı/şablon/kurallar, arka uçlar, **güvenlik (64–127)**, uygulama, özel.
- **Araç `oz/arac/komut.pma`** (9–10 KB PMB1; ELF): `@@tab @@opsh @@gor @@yaz @@kanon @@dogrula`. Okunabilir görünüm düzenlenip `@@yaz` ile sayısal tabloya **kayıpsız** işlenir.
- **C++ tarafı:** `src/ops.h` komut tanımı içermez; `src/ops_gen.h` tablodan üretilir (`PM_OPS`, `PM_HOSTS`); `kHostInfo` da oradan. `pm` yeniden derlendi: `tests/run_tests.sh` 203 geçti, 0 kalan; 33 kaynakta `pm asm` çıktısı eski `pm` ile bayt-bayt aynı.
- **Araç zinciri:** `opstab` yerine `komut @@tab`; `sablonc` tabloyu okur; `uret.sh` (asm/valid/gen2), `kur.sh`, `yenile.sh` tabloyu kullanır; `paketle.elf` (PMY1 paketleyici) araç takımına eklendi.
- **Eşdeğerlik:** tablodan üretilen `asm.pmb`, `valid.pmb` (ve önceki sürümde `gen2.pmb`) depodakiyle **bayt-bayt** aynı; `sablonc` çıktısı eski `ops.h`+awk oracle'ı ile aynı; sözlük eski `opstab` ile aynı; tablodan üretilen `coz.pmy/yaz.pmy` işlevsel olarak eşdeğer (`dogrula_ikili.sh` 3757 denetim, 0 kalan).

### Doğrulama (kullanıcının 6 maddesi) [Kesin; sayılar son koşudan]
1. **Metinsiz üretim:** `ops.h` okunmadan tablodan asm/valid/gen2/coz/yaz yeniden üretilir ve depodakilerle aynıdır (`dogrula_komut.sh` bölüm 3).
2. **Sayı değişikliği davranışa yansır:** tabloda `add` → `toplam` yapılınca üretilen assembler `toplam`'ı kabul eder, `add`'i reddeder, **aynı** PMB1 ve aynı ELF davranışı çıkar (bölüm 6).
3. **Okunabilir metin ↔ sayı kayıpsız:** gerçek tablo, v1 tablosu ve 200 rastgele tablo (seyrek/yoğun kimlik, bilinmeyen bölümler, ext/kuyruk, karışık bölüm sırası) için sayı → metin → sayı özdeş (bölüm 4).
4. **Genişletilebilirlik:** tabloya yeni komut `FMIN d,f,f` + tek şablon satırı eklenince assembler, çözücü, doğrulayıcı ve ELF üretici **kaynak kodu değiştirilmeden** yeni komutu işler (`fmin` 1.5 ve −3 doğru basıldı); yeniden üretilen C++ başlığıyla `pm` de derlenir (bölüm 7).
5. **Önceki doğruluk ve öz-yeniden üretim:** `dogrula_elf2` (2787 geçti, 0 kalan), `float`, `grafik` (257), `valid` (5662), `ikili` (3757), `bag` (1256), `pmy2elf` (205), `yenile` (A→B→A turu, 53 denetim, 12 ELF; ESKI takımla pm olmadan, yalnız bash+cat+cp… PATH’inde) — bkz. aşağıdaki son koşu tablosu.
6. **Sürümleme/uyumluluk:** major/minor, bölüm sürümü + zorunlu bayrağı, `ext`/kuyruk, bölüm sırası bağımsızlığı testle sınanır (bölüm 5).

### Sabotaj (testlerin gerçekten yakaladığı) [Kesin]
`sabotaj_komut.sh`: `komut.pma` içinde 20 mutant (sihirli sayı, sürüm, zorunlu bölüm, yinelenen kimlik/sembol, tür başvurusu, `gor` ext kaybı, `yaz` kuyruk kaybı, `tab`/`opsh` biçimi, profil koşulları, karakter denetimleri, bölüm sonu…) → **20/20 yakalandı**. Test yazarken görülen boşluk: "okunabilir görünüm bilinen satırları içeriyor mu" testi sonradan eklendi (`opaque_gor` mutantı ilk taslakta kaçardı).

### Hâlâ metne / elle yazılmış kurala bağlı olanlar (dürüst liste) [Kesin]
Komutun *kimliği, adı, operand sayısı/türleri, tel sınıfı, host listesi* artık sayıdır. Aşağıdakiler **henüz değil**:
1. **Komut anlamı (yürütme) — §3j ile sayıya taşındı (komut→mikro-işlem düzeyinde); mikro-işlemlerin kendisi hâlâ elle:** C++ `anlam.h` (55 `SEM_*` makrosu, mikro-işlem listeleri), `vm.cpp` (ondalık işlemler elle), `native_x64.cpp`, `host.h` (host anlamı). Yeni komutun `pm run` ile çalışması için bunlar elle yazılır (`FMIN` testinde `pm` yalnızca liste/asm için kullanıldı, yürütmeyi ELF yaptı).
2. **Assembler yeniden yazma kuralları:** `asm.pma.in` `rewrite` işlevi: anlık-biçim eşlemeleri (`add→addi` …), karşılaştırma takasları, takma komutlar (`gt ge fgt fge`) **komut numaralarıyla ve aralarındaki aritmetik ilişkilerle** (`m+33`, `m+41`…) kodlanmış (~33 literal). Yeni komutun anlık biçimi/yer değiştirme kuralı tabloda tanımlanamıyor.
3. **Doğrulayıcı kuralları:** `valid.pma.in` komut başına özel durumlar (`call ret halt jmp jz jnz divi modi …`) sembolik ama komut-özel; genel bir "kural bölümü" yok.
4. **ELF üretici:** komutların x86-64 şablonları **metin** (`sablon.txt`, 83 satır, baytlar onaltılık); şablon dili simgeleri (`R_A`, `REL_*`…) `sablonc.pma` kodunda sabit; `gen2` özel işlemleri (host→RT çağrısı, `ld/st` ayrıcalık, geri-atlama yakıtı) komut sembollerine göre elle yazılmış; RT işlev sırası (`RTF_*`) `statik.sozluk`'ta elle.
5. **PMY1 → .pma dönüştürücü:** `yap/fmt.txt` içinde ~157 yerde mnemonik **metni** (komut adı değişirse çıktı bozulur).
6. **Tüm `.pma` kaynakları** (uygulamalar, testler, `rt.pma`, araçlar) mnemonik adlarıyla yazılmış; adı değiştirilen komutun eski adıyla yazılmış kaynakları yeni assembler reddeder (yalnızca sayısal PMB1/liste biçimleri etkilenmez).
7. **Araçların kendi tanımı hâlâ metin:** `*.pma`/`*.pma.in` ve `sablon.txt` **elle yazılan kaynaktır**, `.pmb` ondan üretilir. İlkenin ("sayısal parametreler kanonik kaynak") araç kaynaklarına uygulanması — yani PMB1/liste kanonik, `.pma` görünüm — **yapılmadı**; bu bir sonraki büyük adımdır.
8. **Güvenlik katmanları:** imza, yetki politikası, izolasyon, kullanıcıya özel çeşitlendirme **yok**. Yapılan yalnızca zemin: ayrılmış bölüm kimlikleri, host yetenek etiketi (ext 1, uygulanmıyor), zorunlu doğrulayıcının atlanamaması (`zincir.sh`).
9. **Test düzeneği** hâlâ awk/sed/od içerir (referans oracle'lar bilerek).

### Önerilen sıra (sonraki adımlar, ana sırayı bozmadan)
(a) Yeniden yazma kuralları ve doğrulayıcı kurallarını tabloya (ayrılmış bölüm 4–15) taşı → bir komutun anlık biçimi/takası/doğrulama kuralı sayıyla tanımlanır; (b) x86 şablonlarını sayısal bölüme taşı (şablon dili kendi sayısal tablosu); (c) `fmt.txt`'te mnemonikleri sembolle yaz (`@M_ADD@`); (d) komut *anlamını* (mikro-işlem listeleri) sayısal bölüme taşı ve C++ `anlam.h`'yi ondan üret; (e) araç kaynaklarının kanonik biçimini PMB1/liste yap, `.pma` görünümü `dump`+ad yan tablosundan üret.


## 3j. Yedinci aşama (4/5): komut anlamları sayıya taşındı (`PRIM` + `SEM` bölümleri, `svm`, mikro-işlem şablonları)

**Karar (kullanıcı):** komutların yalnızca *adları* değil *ne yaptıkları* da sayısal parametre olacak; eski C++ motor bağımsız doğrulama referansı olarak kalacak; yeni anlam tanımı "C++ üreten başka bir şablon" olmayacak, işleyen mantık gerçekten sayıyla ifade edilecek.

### Yapılan [Kesin]
- **PMK1 bölüm 4 PRIM, bölüm 5 SEM** (`oz/komut/KOMUT_TABLOSU.md`): 47 mikro-işlem (okuma/yazma, ALU, karşılaştırma, bölme, bellek, denetim akışı, host, ondalık, `sel`) ve 55 komutun mikro-programı. `src/anlam.h`'deki (C++) mikro-programlar `anlam_dump.cpp` ile dökülüp tablodaki `sem` satırlarıyla **satır satır eşit** bulundu (`dogrula_komut.sh` bölüm 8). `komut` aracı bölümleri doğrular (bilinmeyen işlem, geçicinin tanımdan önce kullanılması, operand yuvası, yinelenen anlam …), `gor`/`yaz` ile kayıpsız metin görünümü, `tab` ile sözlük (`PR_*`, `PRCLS`, `PRNF`, `SEMOFF`, `SEMDAT`) üretir.
- **Sayısal yürütücü `svm`** (`oz/sayisal/svm.pma`, 188 geçti / 0 hata): tabloyu **çalışma anında** okur, her komutu SEM mikro-programından yürütür; kimlik değil işlem *adı* ile eşler (yeniden numaralama güvenli). C++ `pm run` ile karşılaştırma (`dogrula_svm.sh`): depodaki tüm programlar, yakıt sınırlı koşular (tuzak noktası dahil), 2 rastgele program üreteci, ondalık özel değerler (NaN, ±sonsuz, ±0, alt normal; bit düzeyinde), grafik (kare raporu, PPM) — **188 geçti, 0 kalan** (2 program zaman aşımı nedeniyle atlandı).
- **Yerel kod üretimi SEM'den:** `sablon.txt`'te 40 mikro-işlem şablonu (`p_<işlem>`); `gen2` bir komutun yerleşik x86 şablonu yoksa SEM'den üretir, `-6 1` seçeneğiyle şablonu olan komutlar için de zorlar. Geçiciler `[rsp-8t]` (kırmızı bölge), alan simgeleri `TMP_Fk/OPR_Fk/OPV_Fk/KAD_Fk/LIT_Fk`. Zorlanmış mikro yolu ile yerleşik şablon yolu aynı sonuçları verir: `dogrula_elf2.sh` zorlanmış (`GENPRE="-6 1"`, sabit nokta dahil) ve normal koşu; `dogrula_mikro.sh` (ondalık özel değerler × tüm ondalık komutlar, tamsayı sınır değerleri × tüm ikili/anlık/bölme komutları, sıfıra bölme, 40 rastgele program).
- **FMIN, yalnızca iki tablo kaydıyla** (`op 55 FMIN fmin d,f,f` + `sem FMIN rd t1 b ; rd t2 c ; flt t3 t1 t2 ; sel t4 t3 t1 t2 ; wr a t4`): assembler, doğrulayıcı, çözücü, `svm` ve ELF üretici **C++ ve x86 şablonu yazılmadan** çalıştı. Bağımsız oracle (elle yazılmış C++ `(x<y)?x:y`) ile 41×41 bit deseni (NaN yükleri, ±sonsuz, ±0, alt normal, 2^63 sınırı dahil): `svm` = ELF(yalnız-SEM) = ELF(elle yazılmış `minsd` şablonu) = ELF(`-6 1`) = oracle. Yeni işlem (`xyz`) tanımlandığında yürütücü bilmediği işlemi **reddeder** (gen2 çıktı vermez, svm uydurma sonuç vermez).

### Dürüst sınırlar [Kesin]
- Mikro-işlemlerin **kendi anlamı** hâlâ her yürütücüde elle yazılmış (C++ `anlam.h`/`vm.cpp`, svm'de bir kol, gen2'de bir x86 şablonu). Sayısal olan, *komutların bu işlemlerden nasıl oluştuğu*. Yeni komut kod gerektirmez; yeni **işlem** gerektirir (bilinçli genişletme noktası).
- C++ `pm` hâlâ kendi (elle yazılmış) `anlam.h`'sini kullanır: `pm run` tabloya eklenen yeni komutu (FMIN) çalıştırmaz; referans rolü korunur, yeni komutlar için oracle testte ayrıca yazılır.
- Mikro-işlem şablonları geçicileri bellekte tutar (kayıt tahsisi yok): yerleşik şablonlardan yavaş/uzun kod; eniyileme sonraki katmanın işidir.
- Yeniden yazma kuralları, doğrulayıcı kuralları ve x86 şablon metni hâlâ elle (aşağıdaki §3i listesi geçerli; 1. madde artık kapandı).
- Test düzeneği hâlâ awk/sed/g++ (oracle) kullanır.


## 3k. Yedinci aşama (5/5, ilk kısım): yeniden yazma kuralları ve doğrulayıcı kuralları sayıya taşındı

**Karar (kullanıcı):** sıra: komut anlamları → yeniden yazma kuralları → doğrulayıcı kuralları → x86 şablonları → araç kaynakları → tam bağımsızlık testi. Bu bölüm 2. ve 3. adımları kapsar.

### Yapılan [Kesin]
- **PMK1 bölüm 6 PSEUDO ve 7 REWRITE** (biçim: `oz/komut/KOMUT_TABLOSU.md`). `asm.pma.in` içindeki ~33 sabit komut numaralı `rewrite` ve `gt/ge/fgt/fge` için ayrı anahtar sözcük tabloları kaldırıldı; yerine kural tablosunu yorumlayan genel `rewrite` ve tablodan gelen takma komut adları kondu. 19 kural (anlık biçimler, ayna takasları, takma komutlar, bölme sınırları).
- **Doğrulayıcı kuralları SEM'den türetilir:** `komut @@tab` her komutun mikro-programından `OPFLOW/OPTGT/OPCALL/OPCS0/OPCS1/OPDIVSL` ve KIND bayraklarından `KFLAGS` üretir; `valid.pma.in` artık hiçbir komut numarası içermez (`31/35/36`, `DIVI/MODI`, `CALL`, `HOST`… yok). Host sayısı literal `7` yerine `@HOSTCOUNT@`.
- **Genişletme kanıtı:** yalnızca tablo kayıtlarıyla `XORI` + `xor→xori` kuralı (assembler iki `xori` üretti, doğrulayıcı kabul etti, ELF ve svm doğru sonuç verdi) ve `GOTO` (SEM `br a`; assembler, doğrulayıcı, svm).

### Doğrulama [Kesin; sayılar bu aşamanın koşularından]
- `dogrula_asm` 2251/0 (C++ ile ayni üretim 793), `dogrula_kural` (yeni: 24 komut × 4 operand deseni × sınır değerleri) 352/0; `dogrula_valid` 5662/0 (C++ referansıyla); `dogrula_komut` 121/0 (PSEUDO/REWRITE olumsuz örnekleri hata koduyla birlikte, bölüm 9 genişletme).
- **Sabotaj:** kural sayıları 19/19, komut aracı yeni kod (vpseudo/vrw/emit/semflow/kflags) 14/14, doğrulayıcı 13/13. Sabotaj betiğinin taban koşusunun eksikliği sonradan bulundu: ilk turda tüm mutantlar "25 hata" ile yakalanmış göründü (kopyada eksik dosyalar yüzünden taban bile başarısızdı); taban koşusu eklenip yeniden koşuldu, mutantlar tek bir hatayla, doğru nedenle yakalandı.

### Dürüst sınırlar [Kesin]
- Kural/takma komut bölümlerinin okunabilir görünümü ham sayı (kayıpsız, ama okunaklı değil; sembolik görünüm yapılmadı).
- Denetim akışı komutları için x86 mikro-işlem şablonu yok: yeni atlama komutu svm'de çalışır, ELF üreticide çalışmaz (bir sonraki adım olan x86 şablonlarının sayısallaştırılması bunu kapsayacak).
- Doğrulayıcının akış analizi `br/brz/brnz/ret/halt/call/host` sözcük dağarcığını tanır (mikro-işlem vokabüleri); yeni denetim akışı mikro-işlemi araç koduna da eklenmelidir.
- Hâlâ metin: `sablon.txt`, sablonc'ın simge sözlüğü, `fmt.txt` mnemonikleri, tüm araç `.pma` kaynakları; C++ `pm` kendi kurallarıyla bağımsız referanstır.


## 3l. Yedinci aşama (5/5, ikinci kısım): x86 şablonları sayıya taşındı; `sablonc` ve `sablon.txt` kaldırıldı

**Karar (kullanıcı):** sıra: anlamlar → yeniden yazma/doğrulayıcı kuralları → **x86 şablonları** → araç kaynakları → tam bağımsızlık testi. Bu bölüm 3. adımı kapsar.

### Yapılan [Kesin]
- **PMK1 bölüm 8 TOKEN, bölüm 9 TEMPLATE** (biçim: `oz/komut/KOMUT_TABLOSU.md`). Şablon bayt dizileri ve simge sözlüğü (ad↔kod) tablodadır: 126 şablon (komut 54, özel 29, mikro-işlem 43), 75 simge. `komut @@tab` `TPLS/TPL/TS_*` sözlüğünü doğrudan üretir; `gen2` yalnızca tablo + `statik.sozluk` + `rt.pma` ile kurulur. `sablonc.pma/.pmb`, `ref/sablon_ref.awk` ve `oz/elf2/sablon.txt` silindi (toplu araç sayısı 12 → 11).
- **Okunabilir görünüm kayıpsız:** `komut @@gor`/`@@yaz` şablonları `tpl op ADD : 48 8B 83 R_B …`, `tpl sp 7 prologue : …`, `tpl prim alu_add : …` ve `token 256 R_A 1` satırlarıyla gösterir. Makrolar (`LDA(b)` …) açılmış biçimde (artık görünümün parçası değil).
- **Geçiş:** eski `sablonc` çıktısı `oz/komut/gecis/sablon_to_pmk.py` ile bir kez tabloya çevrildi; tablodan üretilen `gen2.pmb` ile eski yolla üretilen `gen2.pmb`'nin ELF çıktıları (collatz, pong, hayat) aynı; `dogrula_elf2` 2787/0 (hem normal hem `-6 1` zorlanmış).
- **Denetim akışı mikro-işlemleri** (`br brz brnz`) için x86 şablonları ve geri-atlama (yakıt) varyantları tabloda (`br_back brz_back brnz_back`; `<ad>_back` adlandırması ile `OPBACK`/`PBACK` türetilir). `gen2`'den sabit kodlu `OP_JMP..OP_JNZ` aralığı kalktı; atlama sınıfı SEM'den (`OPFLOW`, `OPTGT`). Sonuç: yalnızca SEM ile tanımlı yeni atlama komutu `GOTO` ELF üreticide de çalışır ve yakıtı doğru sayar (yakıt 1,3,6,9,100000 için pm'in standart `jmp` programıyla aynı tuzak noktası).
- Testler/sabotaj betikleri tabloya uyarlandı: şablon sabotajı artık okunabilir görünümü bozup `yaz` ile tabloya işler (float, grafik, mikro).
- `dogrula_yenile` v1 nesli: eski `nesil_v1/sablon.txt` yerine `nesil_v1/sablonlar.txt` (v1 şablonlarının görünümü) mevcut tabloyla birleştirilir.

### Doğrulama [Kesin]
- `dogrula_komut` ek olarak: TOKEN/TEMPLATE olumsuz örnekleri hata koduyla (simge kodu/adı yinelenen, bayt gibi görünen ad, bilinmeyen simge/komut/işlem, yinelenen şablon, yoğun olmayan özel sıra …), GOTO ELF'i + yakıtlı döngü, `TS_*` yuvaları. Komut aracı sabotajı: yeni kod (vtoken/vtpl/emit_tpl/gor/yaz/hexd) için 15 mutantın 15'i yakalandı.
- Mikro-işlem şablonu sabotajı: denetim akışı mutantları (koşul ters, değer yuvası, geri-atlama yakıtı, geri koşul/hedef) 7/7 yakalandı; ilk turda 3 mutant kaçtı → testler eklendi (ileri `jmp`, geriye `jz`, geçici kimliği ≠ yuva olan `JZX`). Bir mutant (`br` için `REL_TF1`) **eşdeğer**: `br`'nin yalnız 1. alanı anlamlı, diğer alanlar 0 olduğundan `REL_TF1` aynı yuvaya (a) işaret eder.

### Dürüst sınırlar [Kesin]
- Şablon *simgelerinin anlamı* (`R_A`, `REL_*`, `TMP_Fk`… ne hesaplar) hâlâ `gen2.pma.in` içinde elle; tablo yalnızca ad↔kod verir. Yeni simge = gen2'ye de kod.
- `call`, `ret`, `halt`, `host` için mikro-işlem şablonu yok (çerçeve kurulumu, host/RT çağrısı özel): yeni bir çağrı benzeri komut ELF'te çalışmaz.
- `gen2` hâlâ komut sembollerine göre elle yazılmış özel işlemler içerir (host→RT çağrısı, ayrıcalıklı `ld/st`, `HOST` kolu); `RTF_*` sırası `statik.sozluk`'ta.
- Araç kaynakları (`.pma`, `.pma.in`, `fmt.txt`, `statik.sozluk`) hâlâ metin; PMB1/liste kanonik değil.
- Şablonlar bellek geçicili mikro-işlem yolu yerleşik şablonlardan uzun/yavaş; eniyileme yok.


## 3m. Yedinci aşama (5/5, üçüncü kısım): araçlar sayısal nesne (PMO1); metinsiz üretim ve yalnızca sayılarla genişletme

**Karar (kullanıcı):** sıra son adımlar: araç kaynakları sayısal asıl kaynak, `.pma/.pma.in` isteğe bağlı görünüm; ardından tam bağımsızlık ve genişletilebilirlik testi.

### Yapılan [Kesin]
- **PMO1 nesne biçimi + bağlayıcı** (`oz/nesne/`, `oz/arac/baglayici.pma`): tablo-bağımlı üç araç (`asm`, `valid`, `gen2`) sayısal nesnedir; çıktı metin yolundan **bayt-bayt aynı** (ayrıntı: `oz/komut/KOMUT_TABLOSU.md`). Kapasite sınırı yok; yerleşim bağıl kayıtlarla ayarlanır.
- `uret.sh` betikleri (asm, dogr, elf2) artık nesneyi bağlar; `elf2/rt.pmb` de sayısal kaynak (rt.pma yalnızca metin ön yüzü). `oz/nesne/metinden.sh` ve `ice_aktar.sh` metinden yeniden üretir (isteğe bağlı).
- `yenile.sh` yalnızca sayısal dosyaları çalışma ağacına koyar. `dogrula_yenile`: A→B→A turu 59 geçti, 0 kalan; çalışma ağaçlarında **0 metin kaynağı**.
- **Yalnızca sayılarla genişletme (nihai test):** tabloya `XORI` + `xor→xori` kuralı, `GOTO` (SEM `br a`) ve `FMIN` (SEM) eklenir; **ESKİ ELF takımı** bunlardan YENİ takımı üretir (assembler, doğrulayıcı, ELF üretici, çözücü/kodlayıcı, bağlayıcı …); yeni takımın ELF araçları pm/awk/sed olmadan yeni komutlu programı derler, doğrular, ELF'e çevirir ve program doğru çalışır (çıktı `1005 1007 -1.5 4` beklenenle aynı); eski takım aynı programı reddeder.

### Doğrulama [Kesin]
- `dogrula_nesne` 37/0 (uyumluluk kapısı, nesneden üretim = depodaki, 4 değiştirilmiş tabloda bayt-bayt, iki dizili küçük örnek, 6 bozuk girdi hata koduyla reddedilir, belirlenimcilik).
- Bağlayıcı sabotajı: 14 mutant + 3 nesne-bozma = 17/17 yakalandı; 4 mutant bilerek yok (yerleşim etkisi olmayan skaler yol ve geçersiz nesnelerde ulaşılamayan koşullar, `sabotaj_nesne.sh` başında açıklandı). Test düzeneğinde iki hata bulundu ve düzeltildi: boru hattı alt kabuğunda sayılmayan olumsuz örnekler (sayaç ana kabuğa dönmüyordu) ve `sabotaj_grafik`'in eskimiş iki deseni (`tarama_frame/poll`, yer tutucuya geçince uygulanamaz hâle gelmişti).
- Güncellenen sabotaj betikleri (asm ondalık, doğrulayıcı, float, grafik) yeni nesne yoluyla duman testinden geçti.

### Dürüst sınırlar [Kesin]
- **Metin hâlâ depoda:** `.pma.in`/`.pma` kaynakları (yorumlar ve adlarla) duruyor; ayrıca `ice_aktar` onlardan nesne üretir ve kapı testi ikisinin eşitliğini sınar. Yani **iki kopya** var, tek doğruluk kaynağı **nesne**; metin kopyası insan düzenleme kolaylığı için. Bir araç yalnızca sayılarla düzenlenirse metin kopyası eskir (kapı testi bunu yakalar).
- Nesneyi doğrudan sayılarla düzenlemek: tablo tanımlarıyla (komut/anlam/kural/şablon) mümkün ve test edildi; **program mantığını** (asm'nin kendi algoritması) sayılarla düzenlemek kâğıt üzerinde mümkün ama kullanışlı değil (yorumsuz, adsız); görünüm olarak dökücü bu aşamada yapılmadı (**§3n'de yapıldı**).
- `coz/yaz`: sayısal sablonlardan (.sayi.in) PMY1 üretilir; PMB1'e dönüşümde yap + assembler ara metni (üretilmiş, insan metni değil) kullanılır. `yap.pmb` ve `komut/doldur/opstab/fmtc/baglayici.pmb` depodaki sayısal ikililerdir; `fmt.txt` mnemonikleri hâlâ metin.
- svm ve C++ `pm` hâlâ elle yazılmış anlam kodu içerir (bağımsız referans olarak kalır); mikro-işlem ve şablon-simge uygulamaları elle; `call/ret/halt/host` mikro-işlem şablonu yok; güvenlik katmanları (imza, yetki, izolasyon) yok.


## 3n. Yedinci aşama sonrası: hızlı test sistemi ve PMO1 sayısal düzenleme (okunabilir görünüm)

**Karar (kullanıcı):** önce testleri hızlandır, ardından sayısal parametreleri düzenlemeyi kolaylaştır. Akış: *asıl kaynak sayısal parametreler (.pmo) → isteğe bağlı okunabilir görünüm ve düzenleme → güncellenmiş sayısal program → yeni makine kodu.* Bunun ardından (sıra): `call/ret/host` mikro-işlemleri, sonra güvenlik katmanları.

### 1. Hızlı test sistemi (`oz/hizli.sh`) [Kesin]
- **Ne yapar:** değişen dosyaları (git: çalışma ağacı + izlenmeyenler; `--son`, `--since REF`) alanlara eşler ve yalnızca ilgili paketleri **azaltılmış örnek sayısıyla**, **paralel** (çekirdek sayısı kadar) çalıştırır. Örnek: `oz/asm/*` değişince `asm kural ondalik`; `src/*` (bağımsız C++ referans) ya da `komutlar.pmk` değişince neredeyse hepsi. Belge dosyaları test gerektirmez.
- **Kritik denetimler her çalıştırmada, kapatılamaz:** ana test kosucusu (`tests/`), PMB1 doğrulayıcı (reddetme/güvenlik kapısı), mikro-işlem yerel üretimi (bellek sınırı, tuzak, denetim akışı, pm ile bit-bit), araç/sözlük tazeliği.
- **Büyük aşamalardan sonra:** `oz/hizli.sh --tam` (her betik kendi varsayılan, tam argümanlarıyla), `--bag` (yavaş ELF/kendini yeniden üretme zincirleri), `--sabotaj` (test-testi betikleri). `--liste` yalnızca planı yazar.
- **Mevcut test sistemine dokunmaz:** betikleri kendi argümanlarıyla çağırır; hızlı mod = aynı karşılaştırma yöntemi, daha az rastgele/mutasyon örneği. Ölçüm (4 çekirdek, paralel): hızlı modun **tüm paketleri ≈ 115 sn**, tek alan değişikliği ≈ 35–60 sn (kritik denetimler dahil); **tam koşu (`--tam --bag`, 21 paket) 953 sn, hepsi geçti** (assembler 2252, ELF üretici gen2 2787, doğrulayıcı 5662, PMB1 kodlayıcı/çözücü 3757, pm'siz zincir 1256, PMO1 görünüm 803 denetim …). Yani her değişiklikte tam koşu yerine yaklaşık 8–25 kat daha kısa koşu; büyük aşama sonunda tam koşu.
- **Kendi denetimi** `oz/dogrula_hizli.sh` (51 geçti): dosya→alan eşlemeleri (gereksiz ve eksik alan), kritik denetimlerin hiçbir değişiklikte eksilmemesi, bozuk `pm` ile hızlı sistemin hata bildirmesi ve çıkış kodu.
- **Bulunan zayıflık:** `dogrula_ondalik.sh` bozuk bir `pm` ile **boş geçiyordu** (iki taraf da hep reddedince "geçti"); artık hiç kabul edilmeyen koşuyu hata sayar.

### 2. PMO1 okunabilir görünüm ve sayısal düzenleme [Kesin]
- **Araçlar:** `oz/arac/gorunum.pma/.pmb` (PMA; bağımsız ELF'e çevrilir ve zincire dahildir: `kur.sh`, `yenile.sh`), `oz/nesne/duzenle.sh` (`gor`, `yaz`, `uygula`, `elf`), yan dosyalar `oz/nesne/{asm,valid,gen2}.adlar` (işlev adları + bss bölgeleri; nesnenin parçası değil). Biçim, hata kodları ve sınırlar: `oz/nesne/GORUNUM.md`.
- **Görünüm:** komut adları, operand sayıları ve türleri komut **tablosundan** okunur (yeni komut otomatik tanınır); etiketler sembolik (araya komut eklenince atlamalar kendiliğinden düzelir); işlev çağrıları ad ile; bellek adresleri bağıl kayıtlardan `&DİZİ`, `~DİZİ`, `&bölge+n` olarak; kayıpsız ham biçim `n{DİZİ*k|…}`. `gor → yaz` **bayt-bayt kimlik**tir (üç depo nesnesi ve rastgele nesneler).
- **Uçtan uca düzenleme (kopya ağaçta, `dogrula_gorunum.sh` bölüm 5):**
  - **valid:** yeni kural — *bir işlev 100'den fazla komut içeremez (ret kodu 99)* — görünüme beş satır (komutlar + yeni etiket) eklenerek; sayıya döner, bağlayıcı yeni `valid.pmb`'yi üretir; 101 komutlu program `0 99` ile reddedilir, 100'lüğü ve 12 rastgele program C++ referansıyla aynı sonucu alır.
  - **gen2 (makine kodu üretici):** varsayılan kod üretim yolu değiştirildi (mikro-işlem yolu artık varsayılan; `main`'e iki komut + bir yazmaç): aynı girdiden **farklı makine kodu**, eski aracın `-6 1` çıktısıyla bayt-bayt aynı; üretilen `fib` ve `asm` ELF'leri doğru çalışır.
  - **asm:** yorum karakteri `;` → `#` (sözcük analizindeki tek karşılaştırma); eski assembler `#` yorumunu reddeder, yenisi kabul eder ve `;` artık yorum değildir.
  - **yeni tablo komutu** (`xori`): eski tabloda görünüm reddeder, yeni tabloda kabul eder; `uygula` başarısız üretimde eski nesne/yan dosya/`.pmb`'yi geri koyar.
- **Doğrulama:** bağımsız Python referansı (`oz/nesne/ref/pmo_ref.py`, ayrı yazılmış; aynı görünüm, aynı sayılar, aynı hata kodu) ile PMA aracı **bayt-bayt aynı**. Geliştirme koşuları: 782 rastgele görünüm (yaz / gor sidecar'lı ve sidecar'sız / adlar / gor→yaz kimliği; bunların 274'ü hatasız derlendi, geri kalanı bilerek hatalı üretimdi ve aynı hata koduyla reddedildi) ve 150 bozulmuş görünüm: **0 fark**. Test varsayılanı 120 rastgele + 120 bozulmuş görünüm; ayrıca 49 görünüm→sayı, 31 sayı→görünüm ve 10 yan dosya olumsuz/sınır örneği beklenen `HATA` koduyla; üç depo nesnesinde gor/yaz/adlar kimliği; `gorunum.pmb` gen2 ile ELF'e çevrilip aynı çıktıları verir ve `pm` olmadan (`env -i`, yalnızca ELF araçları) `duzenle.sh gor/yaz` aynı sonucu üretir.
- **Sabotaj (`sabotaj_gorunum.sh`):** `gorunum.pma`'ya 60 tek-noktalı mutant (sabit/ofset/karşılaştırma/ayırıcı değişiklikleri, doğrulama kapılarının çıkarılması) → **60/60 yakalandı**; 2 mutant bilerek yok (aynı hata kodunu veren ikinci kapı: bağıl kayıt tüketim sayacı; boş anahtar adı). Test düzeneğinde iki hata bulundu ve düzeltildi: olumsuz örnekler boru hattı alt kabuğunda koşup sayaca yansımıyordu (ilk sabotajda 14 mutant "kaçtı" göründü; sayaçlar düzeltilince gerçek boşluklar — sınır değerleri, yalnız sıfırdan oluşan satır, işlev sonu etiketi, ≥2^63 sözcük, yinelenen işlev adı — yeni testlerle kapatıldı) ve bir mutant regex'i geri izlemede takıldı.

### Dürüst sınırlar [Kesin]
- **İlk dizi başlığından önceki adresler düz sayıdır** (nesne orada bağıl kayıt taşımaz): `ldi r0, 3` bir adres olabilir ama görünüm bunu sayıdan ayıramaz. Diğer tüm adresler (sonraki diziler, sonraki sözcükler, bss) simgeseldir.
- **Yazmaç adları yok** (numara: `r17`); algoritmayı okumak için yan dosyada yalnızca işlev adları ve bölgeler var. Veri ve sabit havuzu içine yeni dizi eklemek mümkündür (`array`), ama eski adres düz sayıyla yazılmışsa (yukarıdaki durum) kaymaz.
- Görünüm düzenlemesi algoritmayı *anlamayı* kolaylaştırır, doğrulamaz: değişiklikten sonra yine ilgili test paketleri (`oz/hizli.sh`) çalıştırılmalıdır; düzenleme araçları yanlış programı da sayıya çevirir (yalnızca biçim denetimi yapar).
- Metin kopyaları (`.pma.in/.pma`) hâlâ depoda ve kapı testleri onları nesneyle eşitliyor; bir nesne doğrudan görünümden düzenlenirse metin kopyası eskir (bilinen, belgelendi).
- **Sıradaki:** `call/ret/halt/host` mikro-işlem şablonları (şu an yalnızca sabit şablonlar), sonra güvenlik katmanları (imza, yetki, izolasyon — ayrı, sürümlü katmanlar; bu aşamada yok).


## 3o. Ana hedef düzeltmesi ve 1.399 maddelik plan (2026-10-31)
**Karar (kullanıcı):** projenin ana ürünü kendi bağımsız Sayısal Parametre Motorumuzdur (PMB1/PMY1/PMK1/PMO1); kanonik kaynak sayısal parametrelerdir. C++/Python/Rust/Java/C#/JavaScript motorun yerine geçmez, yalnızca desteklenen **dış diller** olarak uyumluluk ve aktarım katmanıdır (mevcut uygulamaları ve kütüphaneleri sayısal mimariye aktarmak, farklı dillerin işlevlerini tek sayısal bütünde birleştirmek, davranışı korumak). Yeni zorunlu metinsel dil yok. Öncelik: (1) sayısal motorun yetenekleri, (2) dış dillerle/kütüphanelerle uyumluluk, (3) donanım uyarlaması ve gelişmiş hesaplama. Plan: `PLAN_1399_MADDE.md` (41 bölüm, 1.399 madde), bölümler sırayla, her bölüm tam bitince durulur. Önceki "C++ kaynak dili kapsam dışı" ifadesi bu kararla düzeltildi: dış dil ön yüzleri kapsam içidir ama ikinci öncelikli uyumluluk katmanıdır.

## 4. Açık sorular ve riskler
- **Çekirdek sınırı:** hangi parçalar C++'ta kalmalı? Öneri: çözücü+doğrulayıcı+yorumlayıcı+host (yaklaşık 800 satır) en küçük gerçekçi çekirdek; native üretici ve özelleştiriciler parametreye taşınır; `exec` yeni ilkeldir.
- **Performans:** VM üstünde çalışan araçlar (özelleştirici, birleştirici) yavaşlar; ama bunlar bir kez çalışan yükleme-zamanı araçları ve makine koduna çevrilince (`--native`) kabul edilebilir olmalı. Ölçülecek.
- **Yazım maliyeti:** elle sayı yazmak hata yapmaya açık (bu adımda 0 hata, ama D1 referansında 3 hata yapmıştım). Büyük bileşenler için (birleştirici, özelleştirici) PMY1'i üreten bir üst düzey kaynak biçimi gerekebilir; bu bir C++/awk aracı olursa önyükleme aracı sayılır ve hedefle çelişmez, ama sabit nokta testiyle sonunda parametre sürümüyle değiştirilmelidir.
- **AI üretimi:** bu bileşenleri sıfırdan AI'ya yazdırmak değil, mevcut C++ davranışını referans alıp otomatik farkla doğrulamak; ajan kullanılmadı.
