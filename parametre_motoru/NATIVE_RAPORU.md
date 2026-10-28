# Anlam Tablosu + x86-64 Makine Kodu Yürütücüsü — Rapor

İstenen: ilk teslim = anlam tablosu + ondan üretilen yorumlayıcı + x86-64 üretici, yorumlayıcıyla birebir aynı çıktı ve hız ölçümü. Yakıt kuralı (çağrı + geri atlama + dış işlem başına) kullanıcı onayıyla uygulandı.

## Yapılanlar
| Parça | Dosya | Satır |
|---|---|---|
| **Anlam tablosu**: 44 komutun anlamı, 21 mikro-işlemle, tek yerde | `src/anlam.h` | 169 |
| Yorumlayıcı: işleyiciler bu tablodan üretilir (INTERP), ondalık komutlar elle | `src/vm.cpp` | 88 (önceden 147) |
| **x86-64 üretici ve yürütücü**: şablonlar aynı tablodan (DATA) | `src/native_x64.cpp` | 253 |
| Host işlevleri (iki yürütücü AYNI kodu kullanır) | `src/host.h` | 13 |
| Toplam C++ | | **1578** (hedef <3000: tutuyor) |

- **Yakıt kuralı değişti:** artık kontrol noktaları: her `call`, her alınan geri atlama, her `host`; her biri 1 yakıt. Komut sayısına bağlı değil. Şartname bölüm 2'de.
- `pm run dosya --native` makine koduyla çalıştırır; `--native-strict` desteklenmiyorsa hata verir (çıkış kodu 3). `--stats` harcanan yakıtı da basar. `make pm_steps` komut sayacı ekler (`-DPM_STEPS`).
- Makine kodu: RW olarak yazılır, sonra `mprotect` ile RX yapılır. Ayrı bir makine yığını ayrılır (çerçeve sınırına göre boyutlanmış).

## Doğruluk: iki yürütücü gerçekten aynı mı?
- **133 kontrol geçti**, normal ve ASan+UBSan derlemesinde.
- **21 test programı** hem yorumlayıcıda hem makine kodunda çalıştırıldı ve **çıktı, çıkış kodu, tuzak türü ve harcanan yakıt** bire bir aynı çıktı. Tuzaklar dahil: `DIV0`, `OOB`, `FUEL`, `DEPTH`, `STACK`, `ALLOC`. Bir program (`float.pma`, ondalık komut) makine koduna çevrilemediği için atlandı.
- Bu karşılaştırma bir hata yakaladı: yakıt tuzağında `sub r13,1` borcu yakıt sayacını taşırıp raporlanan yakıtı 1001 yaptı (doğrusu 1000). Düzelttim; test artık yakalıyor.
- **Farklılık fuzz'ı:** bozuk ama doğrulamayı geçen programlar iki yürütücüde çalıştırıldı (`pm fuzz dosya N tohum --diff`). 12 programdan 360.000 deneme, doğrulamayı geçen ve karşılaştırılan **~22.000 program: 0 fark, 0 çökme.** ASan+UBSan altında ek 24.000 deneme (1.369 karşılaştırma): 0 fark.
- Sınır: fuzz bozulmuş küçük programları kapsar. Ondalık komutlar ve çok büyük programlar bu karşılaştırmada yok.

## Hız (en iyi 5 koşu, s; yürütme süresi)
| İş yükü | Yorumlayıcı | Makine kodu | Hızlanma | Yakıt (ikisi de) |
|---|---|---|---|---|
| fib35 | 0,546 | **0,099** | **5,5×** | 29.860.705 |
| loop_sum | 0,801 | **0,210** | 3,8× | 100.000.001 |
| memsum | 0,690 | **0,156** | 4,4× | 51.000.052 |
| sieve10m (bellek) | 0,678 | **0,366** | 1,8× | 42.853.217 |
| floatloop | 0,389 | — | desteklenmiyor (ondalık) | 20.000.002 |

Karşılaştırma için (önceki ölçüm oturumundan, aynı makine): yerel C++ `fib35` `-O0` 0,077 s / `-O2` 0,026 s; `loop_sum` 0,301 / 0,031; `memsum` 0,172 / 0,022; `sieve10m` 0,483 / 0,345.
- Makine kodu `fib35`'te yerel `-O0`'a ≈1,3× yakın, `-O2`'den ≈3,8× yavaş. Önceki "AOT'yi g++ ile derleme" denemesinin (0,14 s) ve yorumlayıcının (0,65 s) altında.
- `loop_sum`/`memsum`'da yerel `-O0`'dan hızlı ya da eşit, `-O2`'nin 7–10× gerisinde. Beklenen: şu an her komut yazmacı bellekten okuyup yazıyor (yazmaç dağıtımı yok), derleyicinin döngü iyileştirmeleri yok.
- **Yorumlayıcının kendisi de hızlandı** (yeni yakıt kuralı + tablodan üretim), önceki commit'e göre: fib35 −%9,9, loop_sum −%14,8, sieve10m −%23,2, floatloop −%11,4, memsum −%12,8. Kabul koşulu "hız kaybı <%3" sağlandı (kazanç var). Makine gürültüsü ±%5; yönün hepsi aynı.

## Dürüst sınırlar
- **Yalnızca Linux x86-64.** Diğer platformlarda `--native` yorumlayıcıya düşer. ARM ve Windows üreticisi yok.
- **Ondalık komutlar yok** (SSE2 gerekir). Ondalık içeren program makine koduna çevrilemez, yorumlayıcıda çalışır.
- **Tuzak konumu** (işlev/komut) raporlanmaz; yalnızca tür ve yakıt.
- Üretici her çalıştırmada kodu yeniden üretir (kaydedilen önbellek yok). Süre küçük programlarda ihmal edilebilir; büyük programlarda ölçülmedi.
- **Güvenlik:** makine kodu doğrulanmış programdan ve tablodan üretilir; bellek/bölme/yığın/yakıt denetimleri kodda duruyor. Yine de üretici bir hataysa bellek güvenliği bozulabilir; bunun savunması farklılık fuzz'ı ve ASan, kanıt değil.
- **Tablo derleme anında kullanılıyor.** Komut anlamlarını çalışma anında yükleyip değiştirme (kademe 2) yok.
- Gerçek uygulama (ekran, oyun) ölçülmedi; ekran/girdi kapısı henüz yok.

## Karar bekleyenler / sıradaki adım
1. **Yazmaç dağıtımı** (VM yazmaçlarını işlev içinde CPU yazmaçlarında tutmak): en büyük tek hız kazancı, `loop_sum`/`memsum`'u `-O0` altına, `-O2`'ye yaklaştırır. Karmaşıklığı: çağrı penceresi ve tuzak çıkışlarıyla etkileşimi.
2. **Ondalık komutlar** (tabloya SSE2 ile ekleme): `floatloop` makine koduyla çalışsın.
3. **Ekran/girdi kapısı (host ABI)** ve **şablon deneyi**: önceki öneri; makine kodu yolu hazır olduğu için host çağrı sözleşmesi artık sabit (`rdi=pencere, rsi=bağlam, edx=kimlik`).
