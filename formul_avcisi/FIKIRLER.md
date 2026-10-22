# Fikir defteri (test edilecekler)

Her fikir: tek cumle, nasil test edilir, durum.

1. **Dolambacli hesap (asiri git, geri don).** Hedef 2+2 iken once 2+45'e gidip geri dusurmek.
   Benzer: Nesterov, Lookahead, extragradient, catapult fazi, Strassen'in iptal edilen ek terimleri.
   Test: optim_suite'e `g2` (ileri noktadaki gradyan) degiskeni ekle; maliyet 2 gradyan sayilir,
   Adam'a da 2x adim hakki. Durum: TASLAK; kullanici (a) agirliklarda mi (b) hesap ici mi netlestirecek.
2. **Nokta/cizgi kodlari.** Parametrelere {., -} dizisi kod vermek (bijective base-2). Sorun: ayirici
   gerekir; sadece adres ise kazanc yok; deger temsiliyse kod defteri kuantizasyonu. Durum: TASLAK.
3. **Delta (ΔW tahmincisi).** Ayri proje (kullanicinin bilgisayarinda). Hiz kazanci yok; deger = kalici
   bellek hipotezi. Durum: ASAMA 1 kapali, genel egitim bekliyor.
4. **Kalan aday formul:** `tanh(m/(sqrt(v)+v))` gercek veri/GPU'da dogrulanacak.
5. **Capa / eski degeri formule vermek (kullanici fikri).** TEST EDILDI: `Adam + 0.1*(w - capa)` ardisik gorevlerde
   Adam'dan %53 iyi. Siradaki: gorev siniri bilinmiyorsa capa = agirligin yavas hareketli ortalamasi (EMA); replay ve
   gercek EWC ile karsilastirma; gercek veri.
6. **Kucuk temsilde hesap (kullanici).** 7B parametreyi ~7M sayiyla/kucuk basamakla temsil et; hesabi kucukte yap, kural/algoritma
   (Delta benzeri) sonucu buyuge cevirsin (1M+5M=6M -> 6 milyar). Iki okuma: (A) sayilari kucuk gostermek = nicemleme/blok olcek
   (toplama dogrusal, carpmada olcek karesi; kayip: alt basamaklar); (B) sayi ADEDINI azaltmak = altuzay/dusuk rutbe/hiper-ag.
   Kilit: acmadan kucuk uzayda hesap yapilabiliyor mu (dogrusalda evet, aktivasyonda hayir). Test: rakam agi, 8/4/3/2 bit + olcek ve
   dusuk rutbe/hash ile dogruluk-sikistirma egrisi. Durum: TASLAK.
7. **Hafiza gibi / tek-hamle ogrenme (yapildi).** Bkz README: ELM/kNN/istatistik biriktirme. Acik: zor veride sinir, bellek Hf^2.
8. **FRIDAY guncelleyici (taslak).** Adimda K aday guncelleme uret, 2-3 adim ileri kafada oynat, en iyisini uygula; maliyet-esit karsilastir.
