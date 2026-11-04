#!/bin/bash
# asm.pmb = SAYISAL NESNE (oz/nesne/asm.pmo) + komut tablosundan sozluk (komut @@tab) -> baglayici -> liste -> PMB1.  METIN (asm.pma.in) KULLANILMAZ; awk/sed/doldur/assembler yok.
# ARAC (ELF) ya da PM (onyukleme) gerekir.  Metinden yeniden uretim (istege bagli on yuz): oz/nesne/metinden.sh
cd "$(dirname "$0")"; PM=${1:-${PM:-../../pm}}; . ../arac/arac.sh
OPT=${OPT:-../komut/komutlar.pmk}   # KANONIK sayisal komut tablosu
{ { echo "@@tab"; cat $OPT; } | arac komut; echo "@@NESNE"; cat ../nesne/asm.pmo; } | arac baglayici | arac_pmb asm.pmb
