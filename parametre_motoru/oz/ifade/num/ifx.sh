#!/bin/bash
# PMX1 sayi listesi -> PMA metni, tamamen SAYISAL zincir: ifx.pmb + komutlar.pmk (bolum 12 kural tablosu). C++ ifade derleyicisi cagrilmaz.
# Kullanim: ifx.sh PM PMX1_DOSYASI [TABLO]   (stdout: PMA)
PM=$1; X=$2; D=$(cd "$(dirname "$0")" && pwd); TAB=${3:-$D/../../komut/komutlar.pmk}
cat $TAB $X | (ulimit -v 3000000; $PM run ${IFX:-$D/ifx.pmb})
