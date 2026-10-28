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

**Çıkarım [Kesin]:** Hiçbir parametre sistemi kendini sıfırdan çalıştıramaz; en az bir **çekirdek** (ilk PMB'yi çalıştıran yürütücü + G/Ç + yürütülebilir bellek erişimi) bir kez dışarıdan gelmelidir. Gerçekçi son durum: küçük bir C++ çekirdeği (`PMB1` yükle + doğrula + yürüt + host + `exec`), geri kalanı (birleştirici, özelleştiriciler, makine kodu üreticisi, analizler, dökümcü) PMB1/PMY1 programları. C++ kaynak koduna gerek kalmadan **yeni sürüm** üretmek, bu parametre tabanlı araçların kendi çıktılarını (`.pmb`) yeniden üretmesiyle doğrulanır (sabit nokta testi).

## 2. Geçiş merdiveni (kolaydan zora; her adım C++ sürümüyle bayt-bayt karşılaştırılır)
1. **Paketleyici** (`oz/paketle.pmy`) — *yapıldı*.
2. **PMB1 kodlayıcı + çözücü** (tablo ⇄ bayt). Doğrulama: depodaki 200+ test programı ve fuzz derlemi; çözücü çıktısı `pm dump` ile, kodlayıcı çıktısı `pm asm` ile bayt-bayt.
3. **Doğrulayıcı**. Doğrulama: fuzz'daki yüz binlerce bozuk `.pmb` için kabul/ret kararı ve ret nedeni C++ ile aynı (diferansiyel).
4. **Dökümcü** (kolay; metin çıktısı `pm dump` ile aynı).
5. **Kesin atama analizi** (`zero_free`): sonuç bir bit; C++ ile aynı.
6. **Birleştirici** (`.pma` → tablo). Doğrulama: tüm `.pma` örnekleri için çıktı `pm asm` ile bayt-bayt.
7. **PMY1 ve `.pmt` özelleştiriciler** (ağaç → `.pma`). Doğrulama: aynı tablo → aynı `.pma`/`.pmb`.
8. **Yorumlayıcı** (PMY1'de VM döngüsü): 203 testi ve 60 uygulamayı koşturup C++ yorumlayıcısıyla aynı çıktı/yakıt/tuzak.
9. **Makine kodu üretici + `exec`**: üretilen bayt dizisi C++ üreticisinin çıktısıyla bayt-bayt aynı olmalı; `exec` tek yeni çekirdek ilkeli.
10. **Sabit nokta:** parametre tabanlı araç zinciri (2–9) kendi tablosunu (`.pmy`) girdi alıp aynı `.pmb`'yi üretmeli; C++ yalnızca çekirdek olarak çalıştırır.

Her adımda **ölçü**: bayt/çıktı eşitliği (otomatik, ajan yok), hız (parametre tabanlı sürüm / C++), tablo boyutu (öğe sayısı) ve C++ satırının ne kadar azaldığı.

## 3. Adım 1: sonuç (`oz/`)
- Motora **tek genel ilkel eklendi**: PMY1 ifadesi `18 oku` (stdin'den bir bayt, bitince −1). Uygulamaya özel hiçbir şey eklenmedi.
- `oz/paketle.sayi` → `oz/paketle.pmy` (322 bayt, 303 sayı civarı): sayı listesini LEB128/zigzag baytlara çeviren tek ILK bloğu. Önyükleme olarak ilk `.pmy` baytları awk ile üretildi; sonra **kendini yeniden üretiyor**.
- Doğrulama (`oz/dogrula.sh`, 38 denetim, hepsi geçti): depodaki 20 gerçek `.sayi` dosyası bayt-bayt aynı; paketleyici kendi kaynağını paketlediğinde kendisini veriyor (sabit nokta); 8 rastgele girdi (negatif, 4 milyara kadar sayılar, yorumlar, sekme/boşluk) aynı; 8 geçersiz girdi her iki sürümde de reddediliyor (awk çıkış 1, parametre sürümü DIV0 tuzağı ile çıkış 2).
- Hız: 34 KB girdide parametre sürümü 11 ms (süreç tabanı dahil), awk 4 ms; aynı mertebede.
- Sınırlar [Kesin]: hata iletisi yerine tuzak veriyor (PMY1'de stderr yok); sayılar 64 bit tam (awk 2^53'e kadar tam), yani parametre sürümü daha kesin.

## 4. Açık sorular ve riskler
- **Çekirdek sınırı:** hangi parçalar C++'ta kalmalı? Öneri: çözücü+doğrulayıcı+yorumlayıcı+host (yaklaşık 800 satır) en küçük gerçekçi çekirdek; native üretici ve özelleştiriciler parametreye taşınır; `exec` yeni ilkeldir.
- **Performans:** VM üstünde çalışan araçlar (özelleştirici, birleştirici) yavaşlar; ama bunlar bir kez çalışan yükleme-zamanı araçları ve makine koduna çevrilince (`--native`) kabul edilebilir olmalı. Ölçülecek.
- **Yazım maliyeti:** elle sayı yazmak hata yapmaya açık (bu adımda 0 hata, ama D1 referansında 3 hata yapmıştım). Büyük bileşenler için (birleştirici, özelleştirici) PMY1'i üreten bir üst düzey kaynak biçimi gerekebilir; bu bir C++/awk aracı olursa önyükleme aracı sayılır ve hedefle çelişmez, ama sabit nokta testiyle sonunda parametre sürümüyle değiştirilmelidir.
- **AI üretimi:** bu bileşenleri sıfırdan AI'ya yazdırmak değil, mevcut C++ davranışını referans alıp otomatik farkla doğrulamak; ajan kullanılmadı.
