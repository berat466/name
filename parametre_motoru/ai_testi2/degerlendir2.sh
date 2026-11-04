#!/bin/bash
# Dogrudan sayisal parametre testi degerlendiricisi.
# Kullanim: ./degerlendir2.sh GOREV(A|B|C) KIP(sayi|pmt) DOSYA
# Test sirasinda dogrular sifreliydi; test bitince gizli_dogrular/ altina acildi.
G=$1; M=$2; F=$3; D=$(cd "$(dirname "$0")" && pwd); PM=$D/../pm
W=$(mktemp -d /dev/shm/ai2.XXXXXX); trap 'rm -rf $W' EXIT
cp -r $D/gizli_dogrular $W/gizli
GZ=$W/gizli; cp "$F" $W/t.$M; cp $D/../uygulamalar/sablon/std.pmt $W/std.pmt
nc() { sed 's/#.*//; s/;.*//' "$1" | sed '/^[[:space:]]*$/d'; }
cd $W
if [ $M = sayi ]; then
  if ! LC_ALL=C awk -f $D/paketle.awk t.sayi > t.pmb 2> hata.txt; then echo "SONUC katman=PAKETLEME davranis=- mesaj=\"$(head -c 150 hata.txt | tr '\n' ' ')\""; exit 0; fi
  RUN=t.pmb; EXTRA="--max-kare 100"
  "$PM" dump t.pmb > dump.txt 2>&1
else
  if ! "$PM" sablon t.pmt -o t.pmb --strip > /dev/null 2> hata.txt; then echo "SONUC katman=DERLEME davranis=- mesaj=\"$(head -c 150 hata.txt | tr '\n' ' ')\""; exit 0; fi
  RUN=t.pmt; EXTRA=""
fi
"$PM" run t.pmb --fuel 1 > /dev/null 2> yuk.txt; if grep -qE "ikili:|dogrulama:" yuk.txt; then echo "SONUC katman=DOSYA_GECERLILIGI davranis=- mesaj=\"$(head -c 150 yuk.txt | tr '\n' ' ')\""; exit 0; fi
norm() { tr -s ' \n' ' ' < "$1" | sed 's/^ //; s/ $//'; }
ms() { local b=99; for i in 1 2 3; do local t=$("$@" --stats 2>&1 >/dev/null </dev/null | sed -n 's/.*sure: \([0-9.]*\) s.*/\1/p'); [ -n "$t" ] && b=$(echo "if ($t < $b) $t else $b" | bc -l); done; printf "%.1f" $(echo "$b * 1000" | bc -l); }
dav=""; ok=1; katman=-; fuel=-
case $G in
  A|C)
    awk -f $GZ/$(echo $G | tr A-Z a-z).awk > exp.txt
    "$PM" run $RUN > cikti.txt 2> hata2.txt; rc=$?
    fuel=$("$PM" run $RUN --stats 2>&1 >/dev/null | sed -n 's/^yakit: //p')
    min=$([ $G = A ] && echo 3000 || echo 50000)
    if [ $rc = 2 ]; then katman=CALISMA_TUZAGI; ok=0; dav="tuzak:$(head -c 80 hata2.txt | tr '\n' ' ')"
    elif [ "$(norm cikti.txt)" != "$(norm exp.txt)" ]; then katman=ALGORITMA; ok=0; dav="cikti='$(head -c 100 cikti.txt | tr '\n' ' ')' beklenen='$(cat exp.txt)'"
    elif [ "${fuel:-0}" -lt $min ]; then katman=SABIT_GOMME; ok=0; dav="yakit=$fuel < $min"
    else dav="cikti:ok;yakit=$fuel"; fi ;;
  B)
    tum=0; gecen=0
    for e in 99 1 2 3; do
      tum=$((tum+1))
      "$PM" run $RUN $EXTRA --girdi $GZ/ev$e.girdi --ekran s.ppm --kare-rapor > c$e.txt 2> r$e.txt; rc=$?
      awk -v ppm=px$e.txt -f $GZ/b.awk $GZ/ev$e.girdi > exp$e.txt
      if [ $rc = 2 ]; then dav="$dav ev$e:tuzak;"; katman=CALISMA_TUZAGI; continue; fi
      okc=0; okp=0
      [ "$(norm c$e.txt)" = "$(norm exp$e.txt)" ] && okc=1
      if [ -f s.ppm ] && [ "$(head -2 s.ppm | tr '\n' ' ')" = "P6 160 96 " ]; then
        h=$(head -3 s.ppm | wc -c); tail -c +$((h+1)) s.ppm | od -An -v -tu1 | tr -s ' \n' '\n' | sed '/^$/d' > got$e.txt; cmp -s got$e.txt px$e.txt && okp=1; fi
      kr=$(sed -n 's/.*kare=\([0-9]*\).*/\1/p' r$e.txt | tail -1)
      rm -f s.ppm
      if [ $okc = 1 ] && [ $okp = 1 ] && [ "$kr" = 100 ]; then gecen=$((gecen+1)); dav="$dav ev$e:ok;"; else dav="$dav ev$e:cikti=$okc,piksel=$okp,kare=$kr;"; [ $katman = - ] && katman=ALGORITMA; fi
    done
    [ $gecen = $tum ] || ok=0
    dav="olay_dosyasi=$gecen/$tum;$dav"; fuel=$("$PM" run $RUN $EXTRA --girdi $GZ/ev1.girdi --stats 2>&1 >/dev/null | sed -n 's/^yakit: //p') ;;
esac
extra=""; [ $G = B ] && extra="--girdi $GZ/ev99.girdi"
ym=$(ms "$PM" run $RUN $EXTRA $extra); nm=$(ms "$PM" run $RUN $EXTRA $extra --native)
if [ $M = sayi ]; then tanim="sayi_adedi=$(nc t.sayi | wc -w) tanim_bayt=$(nc t.sayi | wc -c) komut_adedi=$(grep -cE '^ +[0-9]+  ' dump.txt)"
else tanim="tanim_sozcuk=$(nc t.pmt | wc -w) tanim_satir=$(nc t.pmt | wc -l) tanim_bayt=$(nc t.pmt | wc -c)"; fi
echo "SONUC kip=$M gorev=$G davranis=$([ $ok = 1 ] && echo TUTTU || echo FARKLI) katman=$katman pmb_bayt=$(stat -c %s t.pmb) $tanim yakit=${fuel:--} yorum_ms=$ym native_ms=$nm detay=[$dav]"
