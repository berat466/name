# Kod Üretimi Optimizasyonu — Rapor

Kapsam: yalnızca **üretilen programın kendisi**. Yürütme yöntemi (yorumlayıcı) ve mimari büyütülmedi; tek ISA büyümesi 14 doğrudan-operand komutu. Karşılaştırma: eski motor (`deney_profil/eski_src` = commit 8eb302b) ↔ yeni motor, aynı kaynak programlar. Ham çıktı: `deney_profil/kod_uretimi_sonuc.txt`, tekrar üretmek için `deney_profil/kod_uretimi_karsilastir.sh`.

## Yapılanlar
| İş | Nasıl | Nerede |
|---|---|---|
| Gereksiz sabit yüklemeleri | 32 bite sığan sabit son operandsa doğrudan-operand komutu (`addi lti eqi shli divi ...`), sabit solda ise operand/yön çevrilir; `mov d,k`, `ret k`, çağrı argümanı `k` doğrudan yüklenir | `asm.cpp` (`rewriteImm`, `build`) |
| Argüman kopyaları | tek yazılıp tek okunan geçici doğrudan çağrı penceresine yazılır; `mov x,W`+`mov W,x` ve okunmayan çağrı sonucu silinir; kendine kopya silinir | `asm.cpp` (`peephole`) |
| Çağrı kuralı / sıfırlama | **kesin atama analizi**: ispat varsa çağrıda yazmaç sıfırlama atlanır; yoksa korunur. İkili biçim ve anlam değişmez | `ikili.cpp` (`analyze_program`), `vm.cpp` |
| Yazmaç yeniden kullanımı, ortak alt ifade | **Yapılmadı.** Çağrı ağırlıklı yükte yazmaç sayısı zaten %33 düştü; ortak alt ifade için ölçülmüş bir fazlalık görmedim |  |

## Doğruluk (davranış korundu mu?)
- **112 kontrol geçti**, normal ve ASan+UBSan derlemesinde (önceki 97 + yeni 15). Beklenen çıktılar bağımsız hesaplanmıştı (awk/bc); yeni testlerin beklenenini elle hesaplayıp **eski motorla da çapraz doğruladım** (eski motor aynı çıktıyı veriyor).
- Yeni testler: `opt_imm` (her doğrudan-operand biçimi, sabit solda/sağda, 32 bit dışı, `INT32_MIN`, data adresi), `opt_calls` (argüman kopyası tehlikeleri: çift okunan geçici, araya giren çağrı, araya giren etiket, kullanılmayan sonuç, zincir, döngü içi, özyineleme), `opt_zero`.
- **`opt_zero` ayırt edici:** okunmadan önce yazılmayan yazmaç içerir. Gerçek motor `6 / 1` verir ve `sifirlama-atlama=hayir` raporlar. Sıfırlamayı koşulsuz atlayan bir deneme kopyası yanlış `6 / 6` verdi. Yani analiz gereksiz değil ve koruması çalışıyor.
- 27 programdan **26'sında ispat başarılı**, yalnızca bilerek yazılan `opt_zero` başarısız.
- 80.000 bozuk ikili daha (yeni komutlarla), ASan+UBSan altında **çökme yok**. `divi/modi` sabiti 0/-1 doğrulayıcıda reddediliyor (aksi hâlde SIGFPE olurdu).

## Sonuç: yalnızca fib35'e bakmadan
**A) Yapısal (kesin sayım)**
| Program | Çalışan komut | Statik komut | Yazmaç | İkili boyut |
|---|---|---|---|---|
| fib35 (çağrı) | 283,7M → 194,1M (**−%31,6**) | 26 → 18 | 15 → 10 | 98 → 74 B (−%24) |
| opt_calls | 613 → 474 (−%22,7) | 170 → 129 | 79 → 60 | 560 → 437 B (−%22) |
| gcd | 60 → 50 (−%16,7) | 34 → 26 | 18 → 12 | 121 → 97 B (−%20) |
| fact | 242 → 230 (−%5,0) | 33 → 23 | 12 → 7 | 117 → 87 B (−%26) |
| strings | 365 → 350 (−%4,1) | 60 → 48 | 31 → 21 | 259 → 223 B (−%14) |
| sieve (küçük) | 256.665 → 256.652 (≈0) | 66 → 55 | 23 → 17 | 237 → 204 B (−%14) |
| sort / matmul / heap / collatz | −%0,4 … −%1,3 | −%7 … −%16 | −%25 … −%46 | −%6 … −%13 |
| loop_sum / sieve10m / floatloop / memsum | **≈ 0** | −%10 … −%20 | −%21 … −%50 | −%8 … −%15 |

**B) Süre ve bellek (büyük iş yükleri)**
| İş yükü | Yürütme eski → yeni | Toplam (süreç başı → sonu) | Tepe bellek |
|---|---|---|---|
| fib35 | 0,996 → 0,649 s (**−%35**) | 1,036 → 0,687 s (−%34) | 13,8 MB → 13,8 MB |
| loop_sum | 0,989 → 0,969 (−%2, gürültü) | 0,948 → 0,997 (+%5, gürültü) | aynı |
| sieve10m | 0,949 → 0,969 (+%2, gürültü) | 1,050 → 0,985 (−%6, gürültü) | 92,3 MB, aynı |
| floatloop | 0,431 → 0,431 | 0,439 → 0,455 (gürültü) | aynı |
| memsum | 0,878 → 0,836 (−%5, gürültü) | 0,845 → 0,866 (gürültü) | aynı |

Notlar:
- Bu koşuda makine daha gürültülüydü (`loop_sum` önceki ölçümde 0,77 s, burada 0,97 s). ±5–6% altındaki farkları **sıfır** sayıyorum.
- **fib35 kazancı iki kaynaktan** (ayrı ölçtüm): yalnızca yeni kod üretimi (sıfırlama açık) ≈ **−%25…28** (1,00 → 0,73–0,77 s); ek olarak sıfırlama atlama → toplam ≈ **−%33** (0,66–0,67 s). Komutun %31,6 azalması süreyi %26 azalttı; yani komut başına maliyet biraz yükseldi (kalan komutlar ortalama olarak daha pahalı: `call/ret` payı arttı).
- **Bellek kullanımı değişmedi** (sabit 8 MB yazmaç yığını + çerçeve tablosu + program belleği baskın). Değişen şey çerçeve boyutu: fib'de çerçeve 10 → 8 sözcük (özyinelemede yığın %20 daha az dolar).
- **Döngü ağırlıklı yükte kazanç yok.** O kod bu ISA'da zaten minimaldi (sabitler döngü dışına alınmıştı). Orada maliyet dağıtım ve `lt`+`jz/jnz` çiftleri (komutların %25–29'u); birleşik "karşılaştır-ve-atla" komutu yaklaşık %14–16 komut azaltırdı ama ISA büyütür, **yapmadım**.

## Sonuçların yorumu
1. Kod üretimi optimizasyonu **çağrı ağırlıklı** programlarda gerçek (komut −%17…−%32, süre −%35), saf döngü ve bellek programlarında etkisiz. Eski ölçümdeki "%42 gereksiz komut" yalnızca fib35 için doğruydu; genel bir oran değil.
2. Aynı sadeleşme gelecekteki AOT'ye de yarar: daha az kopya, daha az yazmaç, sıfırlama yok.
3. Kalan fark (yerel C++ `-O2`'ye ≈ 25×) hâlâ yürütme yönteminden geliyor; bu rapor onu çözmüyor.

## Karar bekleyenler (şartname kararı, ben değiştirmedim)
1. **Yakıt tanımı:** şu an "çalışan komut". Optimizasyonla aynı program daha az yakıt harcıyor (fib35 −%31,6): yakıt değeri derleyici sürümüne bağlı hâle geldi. Seçenekler: (a) böyle bırak ve belgele, (b) "çağrı + geri atlama" başına say (AOT ile de tutarlı olur). AOT aşamasından önce karar gerekir.
2. **Birleşik karşılaştır-ve-atla komutları:** döngülerde ≈ −%15 komut, ISA'ya +6 komut. İster misin?
3. Hesaplanmış `goto` dağıtımı (ISA değişmez, yalnızca VM içi): önceki ölçümde fib/sieve/memsum için %3–19. Henüz uygulanmadı.

## Sıradaki adım
AOT'yi ikinci yürütme yöntemi yapmak. Öncesinde yukarıdaki 1. karar (yakıt) gerekir; çünkü AOT'de komut bazlı sayaç pahalı, geri-atlama bazlı sayaç ucuz. Mevcut deney AOT aracında tuzak konumu ve yığın taşması hâlâ yorumlayıcıyla aynı değil.
