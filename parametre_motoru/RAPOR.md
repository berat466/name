# Parametre Motoru — Aşama 1 Raporu

Kapsam: çekirdek şartname + yazma biçimi yükleyicisi + ikili kodlama + doğrulama + yorumlayıcı + testler. Python yok; her şey C++17 (`g++ -O2`). Commit/push yapılmadı.

## Ne yapıldı
| Parça | Dosya | Satır |
|---|---|---|
| Komut tablosu (tek kaynak) | `src/ops.h` | 74 |
| Yazma biçimi → program (ayrıştırıcı) | `src/asm.cpp` | 399 |
| İkili kodlama + doğrulama | `src/ikili.cpp` | 158 |
| Yorumlayıcı (VM) | `src/vm.cpp` | 147 |
| Döküm, komut satırı | `src/dump.cpp`, `src/main.cpp` | 158 |
| **Toplam C++** | | **987 satır** (hedef <3000: tutuyor) |

Komutlar: `pm asm | run | dump | fuzz`. Derleme `make pm`; test `make test` (ASan/UBSan: `make test_san`).

## Doğruluk (ölçüldü)
- **24 test programı, 97 kontrol, hepsi geçti**; normal derleme ve ASan+UBSan derlemesinde. Her program üç yoldan çalıştırıldı ve çıktıları bayt bayt eşit: doğrudan yazma biçimi, ikili, adları atılmış (`--strip`) ikili.
- Beklenen çıktılar **motordan bağımsız** hesaplandı (awk, bc): fib(25)=75025, asal sayısı 25/168/1229, 20! ve 21! sarma değeri (bc), Collatz 111/9232, 3×3 matris çarpımı, Newton√2 ve Leibniz π (awk). Bir kez beklenen değerim yanlış çıktı (Newton, √2'den 1 ulp aşağıda yakınsıyor); awk motorun doğru olduğunu doğruladı, beklenen dosyayı düzelttim.
- Tuzaklar belirli ve test edildi: `DIV0`, `OOB`, `FUEL`, `DEPTH`, `STACK`, `ALLOC` (çıkış kodu 2). Derleme hataları (tanımsız ad, yanlış argüman sayısı, ondalık sabit tamsayı komutta, giriş yok, etiket yok) çıkış kodu 1 ve açıklayıcı mesaj verir.
- Tamsayı kenar durumları test edildi: `INT64_MIN/-1`, `%` işaret kuralı, kaydırma maskesi, taşma sarması, `f2i` doygunluğu, NaN→0, `1/0.0 = inf`.

## Dayanıklılık: bozuk ikili (fuzz)
5 programın ikilisine 20.000'er (ve 2 tanesine ikinci kez 20.000) rastgele bayt bozma/kesme uygulandı, ASan+UBSan altında. **Toplam 140.000 deneme, çökme sıfır.**
| Program | Reddedilen | Geçerli sayılan | Geçerli + tuzak/çalışan |
|---|---|---|---|
| sieve (bss öncesi) | %74,8 | %25,2 | 5 çalıştı, 5035 tuzak |
| sieve (bss sonrası) | %95,4 | %4,6 | 131 çalıştı, 783 tuzak |
| strings | %91,7 | %8,3 | 1527 / 128 |
| matmul | %93,5 | %6,5 | 1098 / 201 |
| sort | %94,5 | %5,5 | 916 / 180 |
| fib | %95,2 | %4,8 | 169 / 775 |

**Sonuç: "her rastgele değişiklik geçerli programdır" hedefi sağlanmadı.** Rastgele bayt bozulmasının %92–95'i reddediliyor (ilk sieve satırındaki %75, 10 KB sıfır baytın bozulmasının zararsız olmasından kaynaklanan bir yapaylıktı; bss sonrası %95). Bu şartnamedeki "her mutasyon geçerli" kolay bir kazanım değil; ayrı bir tasarım ister (ör. operandları modül alarak geçerli aralığa eşlemek; bu da anlamsız programlar üretir).

## Boyut (ölçüldü, `--strip`)
| Program | Kaynak (.pma) | İkili (.pmb) | Komut |
|---|---|---|---|
| fib | 282 B | 98 B | 26 |
| sieve (10.001 sözcüklük alan) | 871 B | 237 B | 66 |
| sort | 621 B | 156 B | 39 |
| strings | 743 B | 259 B | 60 |
| matmul | 675 B | 179 B | 43 |

Bir verimsizlik bulundu ve düzeltildi: `space` ile ayrılan sıfır dolu alan ikilide bayt tutuyordu (sieve 10.237 B); artık `bss` sayısı olarak tutuluyor (237 B). Bu, "yapısı olan yerde parametre sayıyla anlatılır" ilkesinin ilk gerçek örneği.
**Dikkat:** kaynak metne göre küçülme ikili/metin farkıdır, "yerel C++ derlemesinden küçük" anlamına gelmez. Yerel yürütülebilirle boyut karşılaştırması yapılmadı.

## Hız (ölçüldü, 4 çekirdekli sandbox, tek iş parçacığı)
`fib(35)`, özyinelemeli: 283.676.685 komut.
| | Süre | Not |
|---|---|---|
| Motor | **1,03–1,08 s** (≈ 264–276 Mkomut/s) | ~9,5 komut / çağrı |
| Yerel C++ `-O0` | 0,08 s | motor ≈ **13× yavaş** |
| Yerel C++ `-O2` | 0,028 s | motor ≈ **37× yavaş** (derleyici özyinelemeyi açıyor) |

Yani saf yorumlayıcı olarak motor yerel koddan belirgin yavaş. Bu beklenen sonuç; şartnamede "hız kendiliğinden gelmez, özelleştirme (derleme) gerekir" demiştik. **Hız avantajı iddiası yok.**

## Bilerek yapılmayanlar
Şablon/örnekleme katmanı, ilişki katmanı, host arayüzünün çizim/dosya/zaman kısmı, kendini yenileme, Python dönüştürücü, AI üretim testi, yerel C++ ile boyut kıyası. Şartnamede tasarım notu olarak duruyor (bölüm 9–11).

## Bilinen sınırlar
- Komut kümesi 41 komut; `call` her argüman için ek `mov` üretir (fib'de komut/çağrı yüksek). Pencerenin doğrudan hazırlanması (peephole) hız kazandırabilir, denenmedi.
- Sabitler her işlev girişinde yazmaca yüklenir (çağrı başına ek komut). Özyinelemeli küçük işlevlerde maliyetlidir.
- Yazma biçimi satır tabanlı ve basit: ifade yok (`a + b * 2` tek komut olmaz), bu bilinçli.
- Windows/diğer platformlar denenmedi (yalnız Linux g++ 13).
