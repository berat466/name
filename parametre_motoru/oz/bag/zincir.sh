#!/bin/bash
# pm, g++, awk OLMADAN arac zinciri (yalniz ELF araclar). Girdi PMY1 (yapisal tablo) ya da PMB1 olabilir:
#   PMY1 --yap.elf--> .pma --asm.elf--> PMB1 --coz.elf--> sayi listesi --valid.elf--> "1 Z" ise --gen2.elf--> bagimsiz ELF
# Dogrulayici (ya da bicim cozucu) reddederse ELF URETILMEZ. Kullanim: zincir.sh girdi.(pmy|pmb) cikti.elf
# ONEK (ortam): ELF uretecine verilen secenek oneki (ornegin "-3 5 -4 4 -5 3": azami kare, ascii K, kare raporu + son kare PPM fd 3; "-2 n kare kod ..." olaylar).
D=${BAG:-$(cd "$(dirname "$0")" && pwd)}; IN=${1:?girdi}; OUT=${2:?cikti.elf}; L=$OUT.liste.$$; PT=$OUT.pmb.$$
trap 'rm -f $L $PT $OUT.pma.$$' EXIT
IFS= read -r -n 4 H < "$IN"
if [ "$H" = "PMY1" ]; then
  (ulimit -v 900000; timeout 60 $D/yap.elf < "$IN" > $OUT.pma.$$ 2>/dev/null) || { echo "zincir: PMY1 reddedildi (yap)" >&2; rm -f "$OUT"; exit 1; }
  (ulimit -v 900000; timeout 60 $D/asm.elf < $OUT.pma.$$ > $PT 2>/dev/null) || { echo "zincir: assembler reddetti" >&2; rm -f "$OUT"; exit 1; }
P=$PT
else P="$IN"; fi
(ulimit -v 700000; $D/coz.elf < "$P" > $L 2>/dev/null) || { echo "zincir: cozucu reddetti (bicim)" >&2; rm -f "$OUT"; exit 1; }
read -r V < <( (ulimit -v 700000; timeout 20 $D/valid.elf < $L 2>/dev/null) )
case "$V" in 1*) ;; *) echo "zincir: dogrulayici reddetti ('$V')" >&2; rm -f "$OUT"; exit 1;; esac
(ulimit -v 700000; { [ -z "$ONEK" ] || printf "%s\n" "$ONEK"; cat $L; } | timeout 20 $D/gen2.elf > "$OUT" 2>/dev/null) || { echo "zincir: uretici desteklemiyor" >&2; rm -f "$OUT"; exit 1; }
chmod +x "$OUT"; echo "zincir: tamam ($V) -> $OUT"
