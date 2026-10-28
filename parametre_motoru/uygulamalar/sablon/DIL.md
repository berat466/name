# .pmt — Şablon / Örnekleme Dili (başvuru)

Bir uygulamayı **yalnızca tanımla** anlatırsın; komut yazılmaz. `pm run uygulama.pmt` (ya da `pm sablon uygulama.pmt -o x.pmb`) çalıştırır. Satır tabanlı, `;` yorum, adlar yalnızca ASCII (`a-z A-Z 0-9 _`).

## Genel
```
ekle std.pmt                  ; davranış kütüphanesini dahil et
sahne G Y                     ; ekran (kare çıkışı); yoksa grafiksiz uygulama
kare N                        ; N kare sonra dur (sahne yoksa zorunlu)
arkaplan RENK                 ; 0xRRGGBB
global AD [= DEGER]           ; tamsayı global (kare: yerleşik sayaç)
```
## Şablon ve örnekleme (tekrarlı yapı)
```
sablon AD adet N              ; N örnek; tanım N'den bağımsız boyuttadır
  alan AD sabit C             ; ilk değer: sabit
  alan AD dogrusal A B        ;           A + B*i
  alan AD mod M A B           ;           A + B*(i % M)        (ızgara sütunu)
  alan AD bol M A B           ;           A + B*(i / M)        (ızgara satırı)
  alan AD rastgele LO HI TOHUM;           LO..HI deterministik rastgele
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
OPERAND: tamsayı (0x.. olur) | alan | global | i (örnek no) | kare | a.alan / b.alan (etkileşimde) | SABLON.alan (0. örnek)
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
Kurallarda döngü/özyineleme yok; ifadede tek ikili işlem; etkileşim O(|A|·|B|); yalnızca 4 toplama işlemi; 3B, ses, metin, arayüz bileşenleri yok.
