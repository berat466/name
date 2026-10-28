#!/bin/bash
# Buyuk olcekli test degerlendiricisi. Gizli dogrular sifreli arsivdedir (ANAHTAR, ENC4); yalniz degerlendirme aninda /dev/shm'de acilir.
# Kullanim: ANAHTAR=... ENC4=arsiv.enc ./degerlendir4.sh GOREV(D1..D3,U1..U3,S1..S3) KIP(yap|sayi|pmt) DOSYA
G=$1; M=$2; F=$3; D=$(cd "$(dirname "$0")" && pwd); PM=${PM:-$D/../pm}
W=$(mktemp -d /dev/shm/ai4.XXXXXX); trap 'rm -rf $W' EXIT
openssl enc -d -aes-256-cbc -pbkdf2 -in "$ENC4" -pass pass:"$ANAHTAR" 2>/dev/null | tar xz -C $W || { echo "SONUC HATA anahtar/arsiv"; exit 1; }
GZ=$W/src; cp "$F" $W/t.$M; cp $D/../uygulamalar/sablon/std.pmt $W/std.pmt; cd $W
nc() { sed 's/#.*//; s/;.*//' "$1" | sed '/^[[:space:]]*$/d'; }
TYPE=out; EXTRA=""
case $G in
  D1) ORACLE="awk -f $GZ/d1.awk"; FMIN=2000; BASE="d1";;
  D2) ORACLE="awk -f $GZ/d2.awk"; FMIN=20000; BASE="d2";;
  D3) ORACLE="awk -v spec=$GZ/d3_spec.txt -f $GZ/d3.awk"; FMIN=100000; BASE="d3 $GZ/d3_spec.txt";;
  S1) ORACLE="awk -v N=500 -v C=2 -v D=100 -v tab=$GZ/s1_tab.txt -f $GZ/s.awk"; FMIN=12000; BASE="s 500 2 100 $GZ/s1_tab.txt";;
  S2) ORACLE="awk -v N=5000 -v C=8 -v D=200 -v tab=$GZ/s2_tab.txt -f $GZ/s.awk"; FMIN=250000; BASE="s 5000 8 200 $GZ/s2_tab.txt";;
  S3) ORACLE="awk -v N=20000 -v C=32 -v D=300 -v tab=$GZ/s3_tab.txt -f $GZ/s.awk"; FMIN=1500000; BASE="s 20000 32 300 $GZ/s3_tab.txt";;
  U1) TYPE=ui; NW=1; CPW=6; WC=2; NK=3; UW=64; UH=48; FR=60; EV=u1;;
  U2) TYPE=ui; NW=1; CPW=24; WC=4; NK=4; UW=112; UH=76; FR=150; EV=u2;;
  U3) TYPE=ui; NW=8; CPW=20; WC=5; NK=8; UW=560; UH=128; FR=200; EV=u3;;
  *) echo "SONUC HATA bilinmeyen gorev"; exit 1;;
esac
fail() { echo "SONUC kip=$M gorev=$G davranis=- katman=$1 mesaj=\"$(head -c 220 $2 | tr '\n' ' ')\""; exit 0; }
# ---- derleme / yukleme
if [ $M = yap ]; then
  LC_ALL=C awk -f $D/paketle.awk t.yap > t.pmy 2> h.txt || fail PAKETLEME h.txt
  "$PM" yapisal t.pmy -o t.pmb --strip > /dev/null 2> h.txt || fail TABLO_DOGRULAMA h.txt
  RUN=t.pmy; PARAM=t.pmy
elif [ $M = sayi ]; then
  LC_ALL=C awk -f $D/paketle.awk t.sayi > t.pmb 2> h.txt || fail PAKETLEME h.txt
  RUN=t.pmb; PARAM=t.pmb
else
  "$PM" sablon t.pmt -o t.pmb --strip > /dev/null 2> h.txt || fail DERLEME h.txt
  RUN=t.pmt; nc t.pmt > t.nc; PARAM=t.nc
fi
"$PM" run t.pmb --fuel 1 > /dev/null 2> h.txt; grep -qE "ikili:|dogrulama:" h.txt && fail DOSYA_GECERLILIGI h.txt
if [ $M = sayi ] && [ $TYPE = ui ]; then EXTRA="--max-kare $FR"; fi
norm() { tr -s ' \n' ' ' < "$1" | sed 's/^ //; s/ $//'; }
ok=1; katman=-; dav=""
if [ $TYPE = out ]; then
  eval "$ORACLE" > exp.txt 2>/dev/null
  timeout 300 "$PM" run $RUN --native > out.txt 2> r.txt; rc=$?
  fuel=$("$PM" run $RUN --native --stats 2>&1 >/dev/null | sed -n 's/^yakit: //p')
  if [ $rc = 124 ]; then ok=0; katman=ZAMAN_ASIMI; dav="300 sn"
  elif [ $rc = 2 ]; then ok=0; katman=CALISMA_TUZAGI; dav="$(head -c 100 r.txt | tr '\n' ' ')"
  elif [ "$(norm out.txt)" != "$(norm exp.txt)" ]; then ok=0; katman=ALGORITMA; n1=$(norm out.txt | wc -w); n2=$(norm exp.txt | wc -w); fark=$(paste <(norm out.txt | tr ' ' '\n') <(norm exp.txt | tr ' ' '\n') | awk '$1 != $2' | wc -l); dav="sayi=$n1/$n2 farkli=$fark ilk='$(norm out.txt | head -c 60)' beklenen='$(norm exp.txt | head -c 60)'"
  elif [ "${fuel:-0}" -lt $FMIN ]; then ok=0; katman=SABIT_GOMME; dav="yakit=$fuel < $FMIN"
  else dav="cikti:ok"; fi
  TIMEARGS="--native"
else
  tum=0; gec=0
  for e in 5 6 7 8; do tum=$((tum+1))
    awk -v NW=$NW -v CPW=$CPW -v WC=$WC -v NK=$NK -v W=$UW -v H=$UH -v FR=$FR -v kt=$GZ/${EV}_tur.txt -v ppm=px$e.txt -f $GZ/ui.awk $GZ/${EV}_ev$e.txt > exp$e.txt
    timeout 120 "$PM" run $RUN --native $EXTRA --girdi $GZ/${EV}_ev$e.txt --ekran s.ppm --kare-rapor > c$e.txt 2> r$e.txt; rc=$?
    if [ $rc = 2 ]; then katman=CALISMA_TUZAGI; dav="$dav ev$e:tuzak;"; continue; fi
    okc=0; okp=0; [ "$(norm c$e.txt)" = "$(norm exp$e.txt)" ] && okc=1
    if [ -f s.ppm ] && [ "$(head -2 s.ppm | tr '\n' ' ')" = "P6 $UW $UH " ]; then h=$(head -3 s.ppm | wc -c); tail -c +$((h+1)) s.ppm | od -An -v -tu1 | tr -s ' \n' '\n' | sed '/^$/d' > got$e.txt; cmp -s got$e.txt px$e.txt && okp=1; fi
    kr=$(sed -n 's/.*kare=\([0-9]*\).*/\1/p' r$e.txt | tail -1); rm -f s.ppm
    if [ $okc = 1 ] && [ $okp = 1 ] && [ "$kr" = $FR ]; then gec=$((gec+1)); dav="$dav ev$e:ok;"; else dav="$dav ev$e:cikti=$okc,piksel=$okp,kare=$kr;"; [ $katman = - ] && katman=ALGORITMA; fi
  done
  [ $gec = $tum ] || ok=0; dav="olay_dosyasi=$gec/$tum;$dav"
  fuel=$("$PM" run $RUN --native $EXTRA --girdi $GZ/${EV}_ev5.txt --stats 2>&1 >/dev/null | sed -n 's/^yakit: //p')
  TIMEARGS="--native $EXTRA --girdi $GZ/${EV}_ev5.txt"
fi
# ---- olcumler
us() { local b=999999999; for i in $1; do local t0=$(date +%s%N); "${@:2}" > /dev/null 2>&1 </dev/null; local t1=$(date +%s%N); local d=$(( (t1 - t0) / 1000 )); [ $d -lt $b ] && b=$d; done; echo $b; }
printf "func main() {\n  halt\n}\n" > bos.pma; "$PM" asm bos.pma -o bos.pmb > /dev/null; bos=$(us "1 2 3 4 5" "$PM" run bos.pmb --native)
yuk=$(us "1 2 3 4 5" "$PM" run $RUN --native --fuel 1)
tot=$(us "1 2 3" "$PM" run $RUN $TIMEARGS)
run_us=$(( tot - yuk )); [ $run_us -lt 0 ] && run_us=0
mem=$("$PM" run $RUN $TIMEARGS --stats 2>&1 >/dev/null </dev/null | sed -n 's/.*tepe-bellek: \([0-9]*\) KiB.*/\1/p')
t0=$(date +%s%N); timeout 60 "$PM" run $RUN ${TIMEARGS#--native} > /dev/null 2>&1 </dev/null; rc=$?; t1=$(date +%s%N); yor=$(( (t1 - t0) / 1000 )); [ $rc = 124 ] && yor=">60000000"
pbayt=$(stat -c %s $PARAM); gz=$(gzip -9 < $PARAM | wc -c)
if [ $M = pmt ]; then psay=$(wc -w < t.nc); else psay=$(nc t.$M | wc -w); fi
case $M in
  yap) "$PM" tablo t.pmy > tab.txt 2>/dev/null; yapi="sablon=$(grep -c '^sablon' tab.txt) davranis=$(grep -c '^davranis' tab.txt) islev=$(grep -c '^islev' tab.txt) dizi=$(grep -c '^dizi' tab.txt) etkilesim=$(grep -c '^etkilesim' tab.txt) girdi=$(grep -c '^girdi' tab.txt) toplama=$(grep -c '^toplama' tab.txt)"; tk=$(awk 'length($0)>20 { n++; if (seen[$0]++) d++ } END { printf "%.3f", n ? d / n : 0 }' tab.txt);;
  sayi) "$PM" dump t.pmb > tab.txt 2>/dev/null; yapi="komut=$(grep -cE '^ +[0-9]+  ' tab.txt)"; tk=$(grep -E '^ +[0-9]+  ' tab.txt | sed 's/^ *[0-9]*  //' | awk '{ n++; if (seen[$0]++) d++ } END { printf "%.3f", n ? d / n : 0 }');;
  pmt) yapi="sablon=$(grep -c '^sablon' t.nc) davranis=$(grep -c '^davranis' t.nc) etkilesim=$(grep -c '^etkilesim' t.nc) girdi=$(grep -c '^girdi' t.nc)"; tk=$(awk '{ n++; if (seen[$0]++) d++ } END { printf "%.3f", n ? d / n : 0 }' t.nc);;
esac
if [ -n "$BASE" ]; then bt=$(us "1 2 3" $GZ/base $BASE); bm=$($GZ/base $BASE 2>&1 >/dev/null | sed -n 's/bellek_kib=//p'); tb="taban_cpp_us=$bt taban_cpp_kib=$bm"; else tb=""; fi
echo "SONUC kip=$M gorev=$G davranis=$([ $ok = 1 ] && echo TUTTU || echo FARKLI) katman=$katman param_sayisi=$psay param_bayt=$pbayt param_gzip=$gz ara_pmb_bayt=$(stat -c %s t.pmb) yukleme_kod_us=$yuk bos_surec_us=$bos calisma_us=$run_us toplam_us=$tot yorumlayici_us=$yor bellek_kib=${mem:--} yakit=${fuel:--} tekrar=$tk $yapi $tb detay=[$dav]"
