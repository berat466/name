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

## 4. Açık sorular ve riskler
- **Çekirdek sınırı:** hangi parçalar C++'ta kalmalı? Öneri: çözücü+doğrulayıcı+yorumlayıcı+host (yaklaşık 800 satır) en küçük gerçekçi çekirdek; native üretici ve özelleştiriciler parametreye taşınır; `exec` yeni ilkeldir.
- **Performans:** VM üstünde çalışan araçlar (özelleştirici, birleştirici) yavaşlar; ama bunlar bir kez çalışan yükleme-zamanı araçları ve makine koduna çevrilince (`--native`) kabul edilebilir olmalı. Ölçülecek.
- **Yazım maliyeti:** elle sayı yazmak hata yapmaya açık (bu adımda 0 hata, ama D1 referansında 3 hata yapmıştım). Büyük bileşenler için (birleştirici, özelleştirici) PMY1'i üreten bir üst düzey kaynak biçimi gerekebilir; bu bir C++/awk aracı olursa önyükleme aracı sayılır ve hedefle çelişmez, ama sabit nokta testiyle sonunda parametre sürümüyle değiştirilmelidir.
- **AI üretimi:** bu bileşenleri sıfırdan AI'ya yazdırmak değil, mevcut C++ davranışını referans alıp otomatik farkla doğrulamak; ajan kullanılmadı.
