#!/bin/bash
# valid.pmb = SAYISAL NESNE (oz/nesne/valid.pmo) + komut tablosundan sozluk -> baglayici -> liste -> PMB1.  METIN (valid.pma.in) KULLANILMAZ.  ARAC (ELF) ya da PM (onyukleme) gerekir.
cd "$(dirname "$0")"; PM=${1:-${PM:-../../pm}}; . ../arac/arac.sh
OPT=${OPT:-../komut/komutlar.pmk}   # KANONIK sayisal komut tablosu
{ { echo "@@tab"; cat $OPT; } | arac komut; echo "@@NESNE"; cat ../nesne/valid.pmo; } | arac baglayici | arac_pmb valid.pmb
