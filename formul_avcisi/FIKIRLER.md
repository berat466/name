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
