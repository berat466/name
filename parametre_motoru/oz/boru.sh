#!/bin/bash
# Parametre tabanli arac zinciri (yalniz PMY1/PMB1 tablolari; C++ `pm` bunlari calistiran onyukleme yorumlayicisi):
#   x.pmb --coz.pmy--> liste --dogr/valid.pmb--> "1 Z" ise --elf/elf.pmy--> bagimsiz ELF
# Dogrulayici reddederse ELF URETILMEZ (guvenilmeyen girdiden makine kodu uretilmez). Kullanim: boru.sh pm girdi.pmb cikti.elf
PM=${1:?pm}; IN=${2:?girdi.pmb}; OUT=${3:?cikti.elf}; D=$(cd "$(dirname "$0")" && pwd); T=$(mktemp -d)
(ulimit -v 600000; $PM run $D/coz.pmy < $IN > $T/l.txt 2>/dev/null) || { echo "boru: cozucu reddetti (bicim)" >&2; rm -rf $T; exit 1; }
V=$( (ulimit -v 600000; timeout 20 $PM run $D/dogr/valid.pmb < $T/l.txt 2>/dev/null) | head -1)
case "$V" in 1*) ;; *) echo "boru: dogrulayici reddetti ('$V')" >&2; rm -rf $T; exit 1;; esac
(ulimit -v 600000; timeout 20 $PM run $D/elf/elf.pmy < $T/l.txt > $OUT 2>/dev/null) || { echo "boru: uretici desteklemiyor" >&2; rm -f $OUT; rm -rf $T; exit 1; }
chmod +x $OUT; echo "boru: tamam ($V) -> $OUT ($(stat -c %s $OUT) bayt)"; rm -rf $T
