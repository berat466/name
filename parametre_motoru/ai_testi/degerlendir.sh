#!/bin/bash
# AI testi degerlendirici: bir .pmt dosyasini gorev kehaneti ile karsilastirir.
# Kullanim: ./degerlendir.sh GOREV DOSYA.pmt        (GOREV: meteor | dalga | sensor)
# Cikti tek satir: SONUC derleme=<ok|HATA> davranis=<TUTTU|FARKLI|-> sure_yorum_ms sure_native_ms pmb_bayt tanim_sozcuk ...
G=$1; F=$2; D=$(cd "$(dirname "$0")" && pwd); PM=$D/../pm; KIT=${KIT:-/tmp/claude-0/-home-user-name/2c981dbf-12d3-5bd2-95cd-af645b836dba/scratchpad/ai_kit/$G}
W=$(mktemp -d); cp "$F" $W/t.pmt; cp $D/referans/std.pmt $W/std.pmt
nc() { grep -v '^[[:space:]]*;' "$1" | sed '/^[[:space:]]*$/d'; }
ms() { local b=99; for i in 1 2 3; do local t=$("$@" --stats 2>&1 >/dev/null </dev/null | sed -n 's/.*sure: \([0-9.]*\) s.*/\1/p'); b=$(echo "if ($t < $b) $t else $b" | bc -l); done; printf "%.1f" $(echo "$b * 1000" | bc -l); }
cd $W
if ! "$PM" sablon t.pmt -o t.pmb --strip > $W/derle.txt 2> $W/hata.txt; then echo "SONUC derleme=HATA davranis=- mesaj=\"$(head -c 200 $W/hata.txt | tr '\n' ' ')\""; rm -rf $W; exit 0; fi
case $G in
  meteor)
    "$PM" run t.pmt --girdi $D/meteor.girdi --ekran s.ppm > cikti.txt 2> hata2.txt; rc=$?
    awk -v LIVES=12 -v ppm=exp.txt -f $D/kehanet/meteor.awk $D/meteor.girdi > exp_cikti.txt
    if [ $rc = 0 ] && cmp -s cikti.txt exp_cikti.txt; then s=ok; else s="FARKLI(cikti='$(head -c 60 cikti.txt | tr '\n' ' ')' beklenen='$(cat exp_cikti.txt)' rc=$rc)"; fi
    if [ -f s.ppm ]; then tail -c +16 s.ppm | od -An -v -tu1 | tr -s ' \n' '\n' | sed '/^$/d' > got.txt; cmp -s got.txt exp.txt && p=ok || p="FARKLI($(cmp got.txt exp.txt 2>&1 | head -c 80))"; else p="YOK"; fi
    dav="cikti:$s;son_kare:$p"; ok=$([ "$s" = ok ] && [ "$p" = ok ] && echo TUTTU || echo FARKLI)
    ym=$(ms "$PM" run t.pmt --girdi $D/meteor.girdi); nm=$(ms "$PM" run t.pmt --girdi $D/meteor.girdi --native) ;;
  dalga)
    "$PM" run t.pmt --ekran s.ppm > cikti.txt 2> hata2.txt; rc=$?
    awk -f $D/kehanet/dalga.awk > exp.txt
    if [ -f s.ppm ] && [ "$(head -c 15 s.ppm | tr '\n' ' ')" = "P6 160 120 255 " ]; then tail -c +16 s.ppm | od -An -v -tu1 | tr -s ' \n' '\n' | sed '/^$/d' > got.txt; cmp -s got.txt exp.txt && p=ok || p="FARKLI($(cmp got.txt exp.txt 2>&1 | head -c 80))"; else p="YOK_ya_da_boyut_yanlis"; fi
    n=$(grep -c '' cikti.txt); k=$("$PM" run t.pmt --kare-rapor 2>&1 >/dev/null </dev/null | sed -n 's/.*kare=\([0-9]*\).*/\1/p')
    dav="son_kare:$p;kare_sayisi:$k"; ok=$([ "$p" = ok ] && [ "$k" = 60 ] && echo TUTTU || echo FARKLI)
    ym=$(ms "$PM" run t.pmt); nm=$(ms "$PM" run t.pmt --native) ;;
  sensor)
    "$PM" run t.pmt > cikti.txt 2> hata2.txt; rc=$?; awk -f $D/kehanet/sensor.awk > exp.txt
    if [ $rc = 0 ] && cmp -s cikti.txt exp.txt; then ok=TUTTU; dav="cikti:ok"; else ok=FARKLI; dav="cikti='$(head -c 100 cikti.txt | tr '\n' ' ')' beklenen='$(cat exp.txt)' rc=$rc"; fi
    ym=$(ms "$PM" run t.pmt); nm=$(ms "$PM" run t.pmt --native) ;;
esac
echo "SONUC derleme=ok davranis=$ok pmb_bayt=$(stat -c %s t.pmb) tanim_sozcuk=$(nc t.pmt | wc -w) tanim_satir=$(nc t.pmt | wc -l) tanim_bayt=$(nc t.pmt | wc -c) yorum_ms=$ym native_ms=$nm detay=[$dav]"
rm -rf $W
