#!/bin/bash
# pm ve g++ OLMADAN arac zinciri: x.pmb --coz.elf--> sayi listesi --valid.elf--> "1 Z" ise --gen2.elf--> bagimsiz ELF.
# Dogrulayici reddederse (ya da coz.elf bicimi reddederse) ELF URETILMEZ. Kullanim: zincir.sh girdi.pmb cikti.elf
D=${BAG:-$(cd "$(dirname "$0")" && pwd)}; IN=${1:?girdi.pmb}; OUT=${2:?cikti.elf}; L=$OUT.liste.$$
trap 'rm -f $L' EXIT
(ulimit -v 700000; $D/coz.elf < "$IN" > $L 2>/dev/null) || { echo "zincir: cozucu reddetti (bicim)" >&2; rm -f "$OUT"; exit 1; }
read -r V < <( (ulimit -v 700000; timeout 20 $D/valid.elf < $L 2>/dev/null) )
case "$V" in 1*) ;; *) echo "zincir: dogrulayici reddetti ('$V')" >&2; rm -f "$OUT"; exit 1;; esac
(ulimit -v 700000; timeout 20 $D/gen2.elf < $L > "$OUT" 2>/dev/null) || { echo "zincir: uretici desteklemiyor" >&2; rm -f "$OUT"; exit 1; }
chmod +x "$OUT"; echo "zincir: tamam ($V) -> $OUT"
