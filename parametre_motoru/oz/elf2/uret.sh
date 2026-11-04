#!/bin/bash
# gen2.pmb = SAYISAL NESNE (oz/nesne/gen2.pmo) + sozluk {komut tablosu (komut @@tab: komutlar, anlamlar, kurallar, x86 sablonlari), statik.sozluk, calisma zamani kutuphanesi rt.pmb listesi} -> baglayici -> liste -> PMB1.
# METIN (gen2.pma.in, rt.pma) KULLANILMAZ: rt.pmb de sayisal kaynaktir. ARAC (ELF) ya da PM (onyukleme) gerekir.
cd "$(dirname "$0")"; PM=${1:-${PM:-../../pm}}; . ../arac/arac.sh
OPT=${OPT:-../komut/komutlar.pmk}   # KANONIK sayisal komut tablosu
if [ -n "$ARAC" ] && [ -x "$ARAC/coz.elf" ]; then RTLL=$(ulimit -v 900000; "$ARAC/coz.elf" < rt.pmb); else RTLL=$("$PM" liste rt.pmb); fi
[ -n "$RTLL" ] || { echo "uret: rt.pmb listesi bos"; exit 1; }
set -- $RTLL; RTLN=$#
{ { echo "@@tab"; cat $OPT; } | arac komut; cat statik.sozluk; echo RTL $RTLL; echo "RTLN $RTLN"; echo "@@NESNE"; cat ../nesne/gen2.pmo; } | arac baglayici | arac_pmb gen2.pmb
