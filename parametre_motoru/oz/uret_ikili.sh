#!/bin/bash
# PMB1 cozucu/kodlayici tablolari (coz.pmy, yaz.pmy) = sayi kaynaklari (coz.sayi.in, yaz.sayi.in) + KANONIK KOMUT TABLOSU:
#   {komut @@tab ; @@SABLON ; *.sayi.in} --doldur--> *.sayi --paketle--> *.pmy      (awk yok: oz/arac araclari; yalniz bash+cat gerekir)
# Kullanim: uret_ikili.sh [pm] [cikti_dizini]   (varsayilan: bu dizin; ARAC=ELF dizini ile pm'siz)
cd "$(dirname "$0")"; PM=${1:-${PM:-../pm}}; OUT=${2:-.}; . arac/arac.sh
OPT=${OPT:-komut/komutlar.pmk}
for t in coz yaz; do
  { { echo "@@tab"; cat $OPT; } | arac komut; echo "@@SABLON"; cat $t.sayi.in; } | arac doldur | arac paketle > $OUT/$t.pmy || { echo "uret_ikili: $t uretilemedi"; exit 1; }
done
