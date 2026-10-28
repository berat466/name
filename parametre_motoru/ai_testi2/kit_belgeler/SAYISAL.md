# Parametre Motoru — sayısal biçim (PMB1) başvurusu

Motor, bir uygulamayı **sayı dizisi** olarak okur. Bu belge o sayı dizisinin anlamını anlatır. Senin işin doğrudan bu sayıları yazmaktır.
`paketle.awk` yalnızca sayıları bayta çevirir (aşağıda); komutları, adları, etiketleri bilmez, mantık üretmez.

## 1. Yazım kuralları (`.sayi` dosyası)
- Boşlukla ayrılmış sayılar; `#` satır sonuna kadar açıklamadır.
- `N` (ör. `300`) işaretsiz tamsayı → LEB128. `zN` (ör. `z-3`, `z10`) işaretli tamsayı → zigzag + LEB128. Hangi alanın hangisi olduğu aşağıda yazılıdır.
- Çalıştırmak için: `./calistir.sh dosya.sayi [pm run bayrakları]`.

## 2. Dosya düzeni (sırayla)
```
80 77 66 49        "PMB1" (başlık)
BAYRAK             0 (ad bölümü yok; hep 0 yaz)
BELLEK             işaretsiz: uygulama belleği, sözcük sayısı (1 .. 134217728)
GIRIS              işaretsiz: giriş fonksiyonunun indeksi
NSABIT             işaretsiz: sabit havuzu uzunluğu, ardından NSABIT adet işaretsiz 64 bit sözcük
NVERI              işaretsiz: veri uzunluğu, ardından NVERI adet z-işaretli sözcük (bellekte 1. adresten başlar)
BSS                işaretsiz: sıfır dolu alan (veriden sonra)
NFONK              işaretsiz: fonksiyon sayısı, ardından her fonksiyon:
    NPARAM NYAZMAC NKOMUT            hepsi işaretsiz (NPARAM <= NYAZMAC)
    NKOMUT adet komut: KOD işlenen1 işlenen2 ...   (KOD tek sayıdır; işlenen sayısı ve türü §3 tablosunda)
```
Koşullar: `BELLEK >= 1 + NVERI + BSS`; giriş fonksiyonu parametre almaz; her fonksiyon `jmp`, `ret` veya `halt` ile **bitmeli**; fonksiyon boş olamaz.

## 3. Komutlar (55; kod = numara)
İşlenen türleri: **d** hedef yazmaç (işaretsiz yazmaç numarası) · **r** tamsayı kaynak yazmaç (işaretsiz yazmaç numarası; sayısal dosyada sabit yok) · **f** ondalık kaynak yazmaç · **s** herhangi kaynak yazmaç · **i** doğrudan işaretli tamsayı (`z` ile yazılır, 32 bit sınırı) · **k** sabit havuzu indeksi (işaretsiz) · **l** atlama hedefi: `z(HEDEF_KOMUT_NO - BU_KOMUTUN_NO)`, yani **göreli** (işaretli, `z` ile) · **F** fonksiyon indeksi (işaretsiz) · **b** pencere taban yazmacı (işaretsiz) · **h** host işlev kimliği (işaretsiz).
Komut numaraları fonksiyon içinde 0'dan başlar.

| kod | komut | işlenenler | anlamı |
|---|---|---|---|
| 0 | mov | ds | d = s |
| 1 | ldi | di | d = i (tamsayı sabit) |
| 2 | ldk | dk | d = sabit havuzu[k] (64 bit ham sözcük) |
| 3 | add | drr | d = r1 + r2 |
| 4 | sub | drr | d = r1 - r2 |
| 5 | mul | drr | d = r1 * r2 |
| 6 | div | drr | d = r1 / r2 (sıfıra doğru kırpar; 0'a bölme = tuzak DIV0) |
| 7 | mod | drr | d = r1 % r2 (işaret r1'i izler; 0 = DIV0) |
| 8 | and | drr | bit VE |
| 9 | or | drr | bit VEYA |
| 10 | xor | drr | bit ÖZEL VEYA |
| 11 | shl | drr | d = r1 << (r2 & 63) |
| 12 | shr | drr | mantıksal sağa kaydır |
| 13 | sar | drr | aritmetik sağa kaydır |
| 14 | eq | drr | d = (r1 == r2) ? 1 : 0 |
| 15 | ne | drr | d = (r1 != r2) |
| 16 | lt | drr | d = (r1 < r2) işaretli |
| 17 | le | drr | d = (r1 <= r2) işaretli |
| 18 | neg | dr | d = -r |
| 19 | not | dr | d = ~r (bit tümleyen) |
| 20 | fadd | dff | ondalık + |
| 21 | fsub | dff | ondalık - |
| 22 | fmul | dff | ondalık * |
| 23 | fdiv | dff | ondalık / |
| 24 | fneg | df | ondalık - |
| 25 | fsqrt | df | ondalık karekök |
| 26 | feq | dff | ondalık == |
| 27 | flt | dff | ondalık < |
| 28 | fle | dff | ondalık <= |
| 29 | i2f | dr | tamsayı → ondalık |
| 30 | f2i | df | ondalık → tamsayı |
| 31 | jmp | l | koşulsuz atla (l) |
| 32 | jz | rl | r == 0 ise atla |
| 33 | jnz | rl | r != 0 ise atla |
| 34 | call | Fb | F: fonksiyon indeksi, b: pencere tabanı (§4) |
| 35 | ret | s | s yazmacını döndür (§4) |
| 36 | halt | - | programı bitir |
| 37 | ld | dri | d = bellek[r + i]  (adres sözcük cinsinden; sınır dışı = OOB) |
| 38 | st | sri | bellek[r + i] = s  (ilk işlenen kaynak değerdir) |
| 39 | alloc | dr | d = r adet sıfırlı yeni sözcüğün başlangıç adresi (yığın yalnızca büyür) |
| 40 | host | hb | h: host işlevi, b: pencere tabanı (§5) |
| 41 | addi | dri | d = r + i |
| 42 | muli | dri | d = r * i |
| 43 | andi | dri | d = r & i |
| 44 | shli | dri | d = r << i |
| 45 | shri | dri | mantıksal sağa |
| 46 | sari | dri | aritmetik sağa |
| 47 | divi | dri | d = r / i (i 0 ya da -1 olamaz) |
| 48 | modi | dri | d = r % i (i 0 ya da -1 olamaz) |
| 49 | eqi | dri | d = (r == i) |
| 50 | nei | dri | d = (r != i) |
| 51 | lti | dri | d = (r < i) |
| 52 | lei | dri | d = (r <= i) |
| 53 | gti | dri | d = (r > i) |
| 54 | gei | dri | d = (r >= i) |

Yazmaç numaraları `0 .. NYAZMAC-1` aralığında olmalı. Yazmaçlar 64 bit sözcüktür (tamsayı ya da IEEE ondalık; tür komutta belli). Tamsayı işlemleri iki tamamlayıcı sarar.

## 4. Çağrı ve dönüş
`call F, b`: çağrılan fonksiyonun yazmaçları, çağıranın `b` numaralı yazmacından başlayarak **aynı yığın alanıdır** (çağrılanın 0. yazmacı = çağıranın `b`. yazmacı). Parametreleri çağıran önce `b, b+1, ...` yazmaçlarına koyar. Çağrılan `ret s` ile s yazmacının değerini döndürür; değer çağıranın `b` yazmacına yazılır. Çağrıdan sonra `b` ve üstündeki yazmaçların eski değerleri **kaybolur**. Doğrulama: `b + max(1, NPARAM_çağrılan) <= NYAZMAC_çağıran`. Çağrılan fonksiyonun parametre dışı yazmaçları sıfırdır. Giriş fonksiyonu `halt` ile bitirir.

## 5. Host işlevleri (`host h, b`: argümanlar `b, b+1, ...` yazmaçlarında, sonuç `b` yazmacına yazılır)
| h | ad | argümanlar | sonuç |
|---|---|---|---|
| 0 | putc | karakter kodu (r[b]) | 0 — bir bayt yazar |
| 1 | puti | tamsayı (r[b]) | 0 — ondalık yazıyla yazar (`-12`), **ayraç/satır sonu eklemez** |
| 2 | putf | ondalık | 0 |
| 3 | getc | — | stdin'den bir bayt; dosya sonu −1 |
| 4 | present | r[b]=bellek adresi, r[b+1]=genişlik, r[b+2]=yükseklik | 0 devam, 1 çık (kare sınırı doldu), −1 geçersiz argüman. Bellekteki `genişlik×yükseklik` sözcüğü (her biri `0xRRGGBB`, satır sırasıyla) bir kare olarak sunar. 1<=w,h<=4096 ve alan bellek içinde olmalı |
| 5 | poll | — | sıradaki girdi olayı: `(1<<32) + tuş_kodu`; olay yoksa 0. Olaylar kare numarasıyla verilir: `frame()` olayın karesine ulaşınca ya da geçince teslim edilir; `poll` bir çağrıda en çok bir olay verir |
| 6 | frame | — | şimdiye kadar sunulan kare sayısı |

Her `host` çağrısı, her `call` ve her alınan geri atlama 1 yakıt harcar (varsayılan yakıt 4×10⁹). Çalışma tuzakları: DIV0, OOB (bellek sınır dışı), STACK, DEPTH, FUEL, ALLOC. Bellek adresi 0 kullanılmaz; veri adres 1'den başlar, sonra `bss`, sonra `alloc` yığını. Bellek sıfırla başlar.

## 6. Çalıştırma
`./calistir.sh dosya.sayi` sayıları baytlara çevirir (`paketle.awk`), `./pm run` ile çalıştırır ve sonucu `kayit.log` dosyasına ekler (hata katmanı: paketleme / ikili-doğrulama / çalışma tuzağı / çalıştı). Ek bayraklar olduğu gibi `pm run`'a geçer: `--girdi olaylar.txt` (satır: `kare kod`), `--max-kare N` (N. `present`'ten sonra 1 döndürür), `--ekran son.ppm` (son kare), `--ascii K` (son kareyi KxK bloklarla '#'/'.' basar), `--stats`.
Okuma aracı: `./pm dump dosya.pmb` motorun çözdüğü tabloyu okunabilir basar (ürettiğin `.pmb` dosyasının kaydı `calistir.sh` tarafından `dosya.pmb` olarak bırakılır).

## 7. Örnek (1..10 toplamı; `ornek.sayi`)
Bu dosya `ornek.sayi` olarak kitte vardır; her komut ve alan açıklamalıdır. Çalıştır: `./calistir.sh ornek.sayi` → `55`.
