# Parametre Motoru — Şartname (v0.1)

Durum: **Aşama 1 uygulandı** (bölüm 1–8, 10). Bölüm 9 ve 11 yalnızca tasarım notudur, henüz kodlanmadı.

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
- **Pencereli çağrı:** `call` çağıranın `w` numaralı yazmacından başlayan pencereyi çağrılanın 0. yazmacı yapar. Argümanlar pencereye kopyalanır, dönüş değeri pencerenin ilk yazmacına yazılır. Çağrılanın parametre dışı yazmaçları sıfırlanır (belirli davranış).
- **Yakıt:** her komut 1 birim yakıt harcar; bitince `TRAP: FUEL`. Böylece her program sonlanır ya da tuzağa düşer.
- **Tuzaklar** (belirli, tanımsız davranış yok): `DIV0`, `OOB`, `STACK`, `DEPTH`, `FUEL`, `ALLOC`.
- Tamsayı: iki tamamlayıcı sarma; `INT64_MIN / -1` = `INT64_MIN`, `% -1` = 0; kaydırma miktarı 63 ile maskelenir. `f2i`: NaN→0, taşma→doygunluk.
- Ondalık: IEEE-754, FMA birleştirme yok, hızlı-matematik yok.

## 3. Komut kümesi (41 komut)
`mov ldi ldk` · `add sub mul div mod and or xor shl shr sar` · `eq ne lt le` (+ `gt ge` sözdizimsel kısaltma) · `neg not` · `fadd fsub fmul fdiv fneg fsqrt feq flt fle` (+ `fgt fge`) · `i2f f2i` · `jmp jz jnz call ret halt` · `ld st alloc` · `host`.
Komut tablosunun tek kaynağı `src/ops.h`; ayrıştırıcı, ikili kodlayıcı, doğrulayıcı, döküm ve VM buradan beslenir.

## 4. Host arayüzü (sistem çağrıları)
Yalnızca 4 işlev: `putc`, `puti`, `putf`, `getc` (stdin; dosya sonu → -1). Çekirdeğe başka alan işlevi gömülmez.
Sonraki aşamada eklenecekler (çizim, girdi olayı, dosya, zaman) aynı `host` komutuyla, küçük ve genel (tampon yaz/oku) tutulacak.

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
Yükleyici ikiliyi çözerken ve çözdükten sonra denetler: başlık, boyut sınırları, komut kodu, yazmaç/etiket/işlev/sabit/host indeksleri, çağrı penceresinin yazmaçlara sığması, her işlevin `jmp/ret/halt` ile bitmesi. Doğrulamayı geçen program VM'i çökertemez; kalan hatalar tuzak olarak bildirilir. (Ölçüm: bölüm 12.)

## 8. Üç koşulun gerilimi ve çözüm yönü
| Koşul | İster | Çatışma |
|---|---|---|
| Kompaktlık | paylaşım, küresel indeks, kanonik numaralama | AI için üretmesi zor |
| Doğrudan üretilebilirlik | yerel, adlı, tipli, tekrarlı, hataya dayanıklı biçim | boyutu büyütür |
| Genel amaçlılık | döngü, özyineleme | "her mutasyon geçerli ve biter" garantisini bozar; yakıtla sınırlanır |

Çözüm yönü (uygulandı): **iki biçim**. Yazma biçimi (.pma) üretilebilirlik için, ikili biçim (.pmb) kompaktlık için; arada tek yönlü, deterministik ve doğrulanabilir dönüşüm.
Not: "her rastgele bayt değişikliği geçerli program verir" **sağlanmamıştır** (bölüm 12). Bu hedef ayrı bir tasarım gerektirir.

## 9. Şablon / örnekleme katmanı — *tasarım notu, kodlanmadı*
Amaç: 10.000 nesneyi döngü+oluşturma işlevi yerine "ortak şablon + tekrar sayısı + değişim kuralı" olarak tutmak.
Taslak: şablon = parametrelenmiş işlev gövdesi benzeri bir tablo; örnekleme = `(şablon, sayı n, kural: alan_i = a + b·i)`. Motor bunu açmadan (tekrar başına komut üretmeden) yürütebilmeli; koşul: kural yalnızca basit afin/tablo bağımlılığı içerir. Kompaktlık kazancı **ölçülmeden** varsayılmaz.

## 10. Kendini yenileme — *yol haritası*
1. Yükleyici/ayrıştırıcının bir alt kümesini bu dilde yaz → eski C++ motorda çalıştır → çıktı bayt-bayt aynı mı (sabit nokta testi).
2. Sonra özelleştirme (parametreden yerel koda) ile yorumlayıcı üstünde yorumlayıcı yavaşlığını azalt.
C++ çekirdek tamamen ortadan kalkmaz (evrensel yürütücü + host arayüzü gerekir).

## 11. Bildirimsel ilişki katmanı — *tasarım notu, kodlanmadı*
Kural/ilişki tabanlı kısım (düzen, kısıt, veri akışı) zorunlu çekirdeğin **üstünde** olacak; saf bildirimsel sistem algoritmik işlerde yavaş kalır. Hangi biçimcilik (veri akışı grafı, yeniden yazma kuralları, ilişkisel sorgu) seçileceği açık sorudur.

## 12. Ölçütler (Aşama 1 sonuçları `RAPOR.md`'de)
bayt boyutu · adım/s · yerel C++ karşılığına oran · rastgele bozulmada geçerlilik oranı · çökme sayısı · (sonra) AI üretim başarı oranı · (sonra) kendini yenileme bayt eşitliği.
