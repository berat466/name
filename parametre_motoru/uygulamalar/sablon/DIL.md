# .pmt — Şablon / Örnekleme Dili (başvuru)

Bir uygulamayı **yalnızca tanımla** anlatırsın; komut yazılmaz. `pm run uygulama.pmt` (ya da `pm sablon uygulama.pmt -o x.pmb`) çalıştırır. Satır tabanlı, `;` yorum, adlar yalnızca ASCII (`a-z A-Z 0-9 _`).

## Genel
```
ekle std.pmt                  ; davranış kütüphanesini dahil et
sahne G Y                     ; ekran (kare çıkışı); yoksa grafiksiz uygulama
kare N                        ; N kare sonra dur (sahne yoksa zorunlu)
arkaplan RENK                 ; 0xRRGGBB
global AD [= DEGER]           ; tamsayı global (DEGER negatif olabilir)
; yerleşik `kare` sayacı 0 tabanlıdır: ilk karede 0, bitişte yazdir ile okunursa çalışılan kare sayısı. `kare N` (dur komutu) ile karıştırma.
```
## Şablon ve örnekleme (tekrarlı yapı)
```
sablon AD adet N              ; N örnek; tanım N'den bağımsız boyuttadır
  alan AD sabit C             ; ilk değer: sabit
  alan AD dogrusal A B        ;           A + B*i
  alan AD mod M A B           ;           A + B*(i % M)        (ızgara sütunu)
  alan AD bol M A B           ;           A + B*(i / M)        (ızgara satırı)
  alan AD rastgele LO HI TOHUM;           LO..HI (ikisi de dahil) deterministik rastgele
  davran DAVRANIS arg...      ; kütüphane davranışını bu şablona bağla
  kural HEDEF = IFADE [eger KOSUL]   ; örnek başına, sırayla uygulanır
  ciz dikdortgen X, Y, G, Y, RENK    ; ya da: ciz nokta X, Y, RENK    (argümanlar ifade)
son
```
Hiçbir kuralla **yazılmayan** alan bellekte tutulmaz (formülden hesaplanır). `canli` yerleşik alandır (1/0); kural `canli = 0` yazarsa örnek ölür (çizilmez, kurallara girmez).
## İfade ve koşul
```
IFADE : OPERAND | OPERAND OP OPERAND | min OPERAND OPERAND | max OPERAND OPERAND
OP    : + - * / % & | ^ << >>        (>> aritmetik; / ve % sıfıra doğru kırpar; sıfıra bölme = tuzak)
OPERAND: tamsayı (negatif ve 0x.. olur; -8 geçerli) | alan | global | i (örnek no) | kare | a.alan / b.alan (etkileşimde) | SABLON.alan (0. örnek)
KOSUL : IFADE KARSILASTIRMA IFADE [ve IFADE KARSILASTIRMA IFADE ...]     KARSILASTIRMA: < > <= >= == !=
```
## Davranış (makro)
```
davranis AD(p1, p2, ...)      ; kural satırları; parametre adları metin olarak yerine konur
  kural ...
son
```
`std.pmt`: `hareket(konum,hiz)`, `yercekimi(hiz,g)`, `sek(konum,hiz,alt,ust)`, `sekalt(konum,hiz,alt)`, `sinirla(konum,alt,ust)`.
## İlişki, olay, toplama, çıkış
```
etkilesim A B [eger KOSUL]    ; A'nın her canlı örneği × B'nin her canlı örneği; kurallarda a.alan, b.alan, global
  kural a.alan = ... [eger ...]
son
girdi KOD [SABLON]            ; KOD tuşu gelince kurallar (SABLON verilirse tüm örneklerine) uygulanır
  kural ...
son
topla GLOBAL toplam|say|enbuyuk|enkucuk SABLON [IFADE] [eger KOSUL]   ; her karede yeniden hesaplanır
bitis eger KOSUL              ; koşul doğru olunca dur
yazdir IFADE, IFADE, ...      ; bitişte yazdır
```
## Karede sıra
1) girdi olayları (poll ile gelen tümü) → 2) şablonların kuralları (bildirim sırasıyla, örnek-öncelikli) → 3) etkileşimler (bildirim sırasıyla) → 4) topla → 5) bitis → 6) çizim (şablon sırasıyla) ve sunum.
## Sınırlar (bilinçli)
Kurallarda döngü/özyineleme yok; `etkilesim A B` için A ve B **farklı** şablonlar olmalı (bir şablonun kendi örnekleri arasında etkileşim için aynı alanları taşıyan ikinci bir şablon gerekir); `yazdir` yalnız bitişte, tek satırda çıktı verir; ifadede tek ikili işlem; etkileşim O(|A|·|B|); toplama işlemi 4 çeşit (toplam, say, enbuyuk, enkucuk); 3B, ses, metin, arayüz bileşenleri yok.
## Ayrıntılar (AI testinde sorulan noktalar)
- Aynı alana birden çok `kural` yazılabilir; bildirim sırasıyla uygulanır, her biri öncekinin sonucunu görür. İki işlemli ifade (ör. `(x+37)%152`) iki kural satırıyla yazılır; ara değer ara alana yazılabilir.
- `girdi` kuralları karede en önce çalışır (şablon kurallarından, sınırlamalardan önce).
- `etkilesim` koşulu her (a,b) çifti için bir kez, kuralların başında değerlendirilir; kurallar a/b alanlarını değiştirse de çift için koşul yeniden bakılmaz. Etkileşim B şablonuna bakmasa bile bir B gerekir.
- `topla`: GLOBAL önceden bildirilmiş olmalı; her karede baştan hesaplanıp yazılır. Boş küme: toplam/say = 0, enbuyuk = -2^62, enkucuk = 2^62.
- `bitis` karenin sonunda, çizimden önce değerlendirilir; bitiş karesinde ekran güncellenmez. `yazdir` içinde `SABLON.alan` ve global kullanılabilir.
- `pm run ... --ascii K`: son kareden her KxK bloğun sol-üst pikselini '#' (parlak) / '.' olarak yazar.
- **Aynı tuş kodu için birden çok `girdi` bloğu yazılırsa yalnızca ilki çalışır** (sonrakiler sessizce yok sayılır); aynı tuşun tüm işini tek bloğa topla.
