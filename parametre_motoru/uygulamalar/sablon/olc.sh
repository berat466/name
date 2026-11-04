#!/bin/bash
# Sablon katmani olcumu: tanim boyutu, uretilen/derlenmis boyut, bellek, sure; elle yazilmis surumle karsilastirma.
PM=${1:-../../pm}; cd "$(dirname "$0")"; PM=$(realpath "$PM")
W=/tmp/pm_olc; mkdir -p $W
nc() { grep -v '^[[:space:]]*;' "$1" | sed '/^[[:space:]]*$/d'; }          # yorum ve bos satirsiz
words() { nc "$1" | wc -w; }; lines() { nc "$1" | wc -l; }
best() { local b=99; for i in 1 2 3 4 5; do local t=$("$@" --stats 2>&1 >/dev/null </dev/null | sed -n 's/.*sure: \([0-9.]*\) s.*/\1/p'); b=$(echo "if ($t < $b) $t else $b" | bc -l); done; printf "%.4f" $b; }
rss() { "$@" --stats 2>&1 >/dev/null </dev/null | sed -n 's/.*tepe-bellek: \([0-9]*\) KiB.*/\1/p'; }
echo "### A) Tanim miktari (yorum ve bos satirlar sayilmaz)"
printf "%-26s %7s %7s %9s %10s\n" tanim satir sozcuk bayt "(paylasilan kutuphane std.pmt: $(lines std.pmt) satir, $(words std.pmt) sozcuk)"
for f in pong.pmt parcacik.pmt tugla.pmt veri.pmt parcacik_elle.pma ../pong.pma; do printf "%-26s %7s %7s %9s\n" $(basename $f) $(lines $f) $(words $f) $(nc $f | wc -c); done
echo; echo "### B) Uretilen kod, parametre dosyasi (--strip) ve bellek"
printf "%-16s %12s %12s %14s %s\n" uygulama "uretilen-satir" "pmb-bayt" "bellek-sozcuk" "sanal/kayitli alan sozcugu"
for a in pong parcacik tugla veri; do
  line=$("$PM" sablon $a.pmt --pma $W/$a.pma -o $W/$a.pmb --strip)
  ozet=$(sed -n 's/^; ozet //p' $W/$a.pma)
  printf "%-16s %12s %12s %14s %s\n" $a $(wc -l < $W/$a.pma) $(stat -c %s $W/$a.pmb) $(echo "$line" | sed 's/.*bellek \([0-9]*\) sozcuk.*/\1/') "$ozet"
done
for f in parcacik_elle.pma ../pong.pma; do "$PM" asm $f -o $W/e.pmb --strip > /dev/null; printf "%-16s %12s %12s %14s %s\n" "$(basename $f .pma) (elle)" "$(wc -l < $f)" "$(stat -c %s $W/e.pmb)" "$(sed -n 's/^memory \([0-9]*\).*/\1/p' $f)" "(elle yazilmis)"; done
echo; echo "### C) Sure (yurutme, en iyi 5 kosu, s) ve tepe bellek (KiB) -- yorumlayici / makine kodu"
printf "%-22s %9s %9s %9s %9s %12s\n" uygulama yorumlayici native "kare/s(nat)" "RSS(KiB)" yakit
row() { n=$1; shift; i=$(best "$PM" run "$@"); m=$(best "$PM" run "$@" --native); k=$("$PM" run "$@" --kare-rapor 2>&1 >/dev/null </dev/null | sed -n 's/.*kare=\([0-9]*\).*/\1/p'); r=$(rss "$PM" run "$@" --native); y=$("$PM" run "$@" --stats 2>&1 >/dev/null </dev/null | sed -n 's/^yakit: //p'); fps=$(echo "scale=0; ${k:-0} / $m" | bc -l 2>/dev/null); printf "%-22s %9s %9s %9s %9s %12s\n" "$n" "$i" "$m" "${fps:-0}" "$r" "$y"; }
row "pong.pmt" pong.pmt --girdi ../pong.girdi
row "pong.pma (elle)" ../pong.pma --girdi ../pong.girdi
row "parcacik.pmt" parcacik.pmt
row "parcacik_elle.pma" parcacik_elle.pma
row "tugla.pmt" tugla.pmt --girdi ../tugla.girdi
row "veri.pmt (kare yok)" veri.pmt
