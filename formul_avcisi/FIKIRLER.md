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
9. **Tek-hamle ogrenme MNIST'te (yapildi, olumsuz).** Hiz avantaji yalniz kucuk/kolay veride; MNIST'te esit dogrulukta klasikten yavas
   (ELM 8192 oz. 55 sn vs Adam 18-41 sn). Melez de esit. Kalan deger: unutmasiz ardisik ogrenme, katman-katman kapali form.
10. **Gradyansiz iki katman (kullanici: once 2. katmani coz, sonra 1. katmani ona gore) — yapildi, olumsuz/kismi.** `mnist_iki_katman_cozum.py`.
    MNIST test dogrulugu (60k/10k): (a) saf hedef-yayilim (W1'i pseudo-inverse hedefe coz): H=256 ~%87.5, H=1024 ~%92, tur arttikca DUSER.
    (b) duzeltme surumu: %89 / %93.7, yine tur arttikca duser. (c) dogru delta1 yonu + (XtX+lam I)^-1 on kosullu adim + cikis katmani ridge ile
    kapali form: H=256 %88.1->%93.3 (11 sn), H=1024 %93.7->%95.6 (43 sn). Referans Adam: %97.5 @18 sn (H=256), %97.8 @41 sn (H=1024).
    Sonuc: "2. katman once, sonra 1." zaten geri yayilimin yapisi; gradyan yonunu cikarinca kaybediyor, yonu tutup adimi kapali formla
    on kosullayinca yavas ama duzenli iyilesiyor; Adam'i gecmiyor.
11. **Derinlik testi (gradyansiz katman-katman, L=1..4 gizli, H=256, MNIST) — olumsuz.** `mnist_derin_gradyansiz.py`. Rastgele alt katman + kapali cikis ~%87;
    hedef yayilimi 1 turda +0.5..1 puan, sonra COKER (L=2: %88->%43, L=3: %88.6->%30, L=4: %87.8->%17); Adam 5 epokta %97.2-97.6. Derinlik arttikca daha hizli bozuluyor.
