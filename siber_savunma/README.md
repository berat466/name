# Siber Savunma Motoru

Savunma bilgisini (Cisco icerigi dahil) **calistirilabilir kurallara** cevirip log ve cihaz
yapilandirmalari uzerinde kendi kendine uygulayan sistem. Yalnizca savunma amaclidir
(tespit, denetim, onerilen mudahale).

```
Cisco metni --(extractor, yerel model)--> kural taslagi --(insan onayi)--> rules/approved
rules/approved + log/config --(engine)--> uyari + onerilen mudahale + HTML rapor
```

## Hizli baslangic (yalnizca Python 3, ek paket yok)

```bash
python run.py selftest                              # her kuralin gomulu testi
python run.py logs samples/ornek.log --year 2026    # log analizi
python run.py audit samples/ornek_router.cfg        # Cisco IOS sertlestirme denetimi
python run.py report samples/ornek.log samples/ornek_router.cfg   # report.html
python -m unittest discover -s tests -v
```

## Cisco iceriginden kural uretmek

1. Kendi Cisco iceriginin metnini bir `.txt` dosyasina cikar (PDF ise once metne cevir).
2. Yerel bir acik agirlikli model calistir (ornek: `ollama run qwen2.5:7b`).
3. `python -m extractor.draft_rules cisco_bolum.txt`
4. Taslaklar `rules/pending/` altina duser. **Her birini oku**, dogruysa `rules/approved/`
   altina tasi, sonra `python run.py selftest` calistir.

Taslak ancak sema dogrulamasini ve kendi gomulu testlerini (hem alarm veren hem vermeyen
ornek) gecerse yazilir. Model hata yapabilir; onay her zaman insanda.

## Kural turleri
`threshold` (esik), `distinct` (farkli deger sayisi, ornek: port tarama), `sequence`
(cok basarisiz deneme sonra basari), `config` (cihaz ayari var/yok). Ornekler:
`rules/approved/*.json`.

## Sinirlar (bilerek)
- Taninan log turleri: Linux sshd, Cisco ASA (106023/106100), Cisco IOS login hatasi.
  Yeni tur icin `engine/logparse.py` icine ayristirici eklenir.
- Mudahale **dry-run**: cihaza dokunmaz, yalnizca onerilen komutu yazar.
- Yanlis pozitif olabilir; engellemeden once kaynagi dogrula.
- `extractor` modelle gercek bir cagri bu ortamda denenmedi (testlerde sahte model kullanildi).
- Cisco Networking Academy icerigi telifli: kisisel/yerel kullan, herkese acik repoya koyma.
