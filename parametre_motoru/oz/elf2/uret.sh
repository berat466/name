#!/bin/bash
# gen2.pma.in + kanonik komut tablosu (komut @@tab) + x86 sablonlari (tabloda: bolum 8/9; komut @@tab TPLS/TPL uretir) + sabit veri (statik.sozluk) + calisma zamani kutuphanesi (rt.pma -> liste: RTL/RTLN) -> gen2.pma (doldur) -> gen2.pmb.
# awk/sed/od/tr yok: yalniz oz/arac araclari. ARAC (ELF) ya da PM (onyukleme) gerekir.
cd "$(dirname "$0")"; PM=${1:-${PM:-../../pm}}; . ../arac/arac.sh
OPT=${OPT:-../komut/komutlar.pmk}   # KANONIK sayisal komut tablosu (src/ops.h degil)
arac_asm rt.pma rt.pmb || exit 1
if [ -n "$ARAC" ] && [ -x "$ARAC/coz.elf" ]; then RTLL=$(ulimit -v 900000; "$ARAC/coz.elf" < rt.pmb); else RTLL=$("$PM" liste rt.pmb); fi
[ -n "$RTLL" ] || { echo "uret: rt.pmb listesi bos"; exit 1; }
set -- $RTLL; RTLN=$#
{ { echo "@@tab"; cat $OPT; } | arac komut; cat statik.sozluk; echo RTL $RTLL; echo "RTLN $RTLN"; echo "@@SABLON"; cat gen2.pma.in; } | arac doldur > gen2.pma || exit 1
arac_asm gen2.pma gen2.pmb
