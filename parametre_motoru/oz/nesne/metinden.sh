#!/bin/bash
# METINDEN YENIDEN URETIM (istege bagli on yuz; derleme zinciri ve testler bunu KULLANMAZ): sablonlu metin kaynaklari (asm.pma.in, valid.pma.in, gen2.pma.in) ->
#   sayisal nesneler (oz/nesne/*.pmo, ice_aktar.sh) -> PMB1 (her dizinin uret.sh'i) ; ayrica GORUNUM olarak ornekleme metni (X.pma = doldur(tablo, X.pma.in)).
# Bir metin kaynagi duzenlendikten sonra calistirilir; sayisal nesne ve .pmb kanonik olarak depoda tutulur, .pma gorunumu ve .pma.in metni isteğe baglidir.
# Kullanim: metinden.sh [pm]   (OPT: baska komut tablosu; TOOLS="asm valid gen2" alt kumesi; GORUNUM=0 .pma gorunumunu atlar)
D=$(cd "$(dirname "$0")" && pwd); PM=${1:-${PM:-$D/../../pm}}; PM=$(realpath "$PM"); export PM; . $D/../arac/arac.sh
OPT=${OPT:-$D/../komut/komutlar.pmk}
for t in asm:asm:asm.pma.in dogr:valid:valid.pma.in elf2:gen2:gen2.pma.in; do
  dir=${t%%:*}; r=${t#*:}; n=${r%%:*}; src=${r#*:}
  case " ${TOOLS:-asm valid gen2} " in *" $n "*) ;; *) continue;; esac
  "$D/ice_aktar.sh" "$D/../$dir/$src" "$D/$n.pmo" > /dev/null || exit 1
  (cd "$D/../$dir" && OPT=$OPT ./uret.sh "$PM") > /dev/null || exit 1
done
[ "$GORUNUM" = 0 ] && exit 0
# gorunum: X.pma
gor() {   # dizin ad [ek sozluk]
  { { echo "@@tab"; cat $OPT; } | arac komut; cat; echo "@@SABLON"; cat "$D/../$1/$2.pma.in"; } | arac doldur > "$D/../$1/$2.pma"
}
gor asm asm < /dev/null; gor dogr valid < /dev/null
RTLL=$("$PM" liste "$D/../elf2/rt.pmb"); set -- $RTLL; RTLN=$#
{ cat "$D/../elf2/statik.sozluk"; echo RTL $RTLL; echo "RTLN $RTLN"; } | gor elf2 gen2
echo "metinden: asm/valid/gen2 nesneleri, .pmb ve .pma gorunumleri yenilendi"
