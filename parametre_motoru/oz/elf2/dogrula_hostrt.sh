#!/bin/bash
# host putf (ve ayni calisma zamani cagrisi yolundaki present/poll/frame) YAZMAC EZME KUSURU -- KALICI GERILEME TESTI. Kullanim: dogrula_hostrt.sh [pm]
# Kusur (kok neden): host RT cagrisinin cercevesi pencere tabaninda basliyordu; RT islevi ve ic ice cagrilari cagiranin pencerenin USTUNDEKI canli yazmaclarini eziyordu
# (pm / svm / --native-strict ezmez). Duzeltme (komut tablosu `tpl sp 30 host_rt` / `tpl sp 31 host_rtr`, token CURNR8): RT cercevesi cagiranin cerceve SONUNA tasinir,
# bagimsiz degiskenler (<=3) kopyalanir, sonuc pencere[0]'a yazilir (referans anlami: her host sonucu pencere[0]'a yazar, sonucsuz host 0).
# Girdiler oz/elf2/hostrt/*.l (PMB1 sayi listeleri): putf_nan_feq (asil kusur: NaN yazdirma sonrasi yazmac eziliyor), mutant_k26_m00022 (mutasyonla bulunan program), ek_*.pma (canli yazmac taramasi).
# Karsilastirma: pm (BAGIMSIZ REFERANS) = --native-strict = svm = gen2 ELF (stdout + cikis kodu).
PM=${1:-../../pm}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d); ok=0; bad=0
GEN2=${GEN2:-$R/oz/elf2/gen2.pmb}; TAB=${TAB:-$R/oz/komut/komutlar.pmk}
pass() { ok=$((ok+1)); }; fail() { bad=$((bad+1)); echo "HATA: $1"; }
lim() { (ulimit -v 900000; timeout ${LIMS:-30} "$@"); }
for src in $D/hostrt/*.l $D/hostrt/*.pma; do
  n=$(basename $src); n=${n%.*}; l=$T/$n.l
  case $src in
    *.pma) $PM asm $src -o $T/$n.pmb --strip > /dev/null 2>&1 || { fail "$n: derlenemedi"; continue; }; $PM liste $T/$n.pmb > $l;;
    *) $PM yazliste $src -o $T/$n.pmb > /dev/null 2>&1 || { fail "$n: liste PMB1'e yazilamadi"; continue; }; cp $src $l;;
  esac
  lim $PM run $T/$n.pmb > $T/$n.pm 2>$T/$n.pme; rp=$?
  cp $T/$n.pm $T/$n.pmx; [ $rp = 2 ] && printf '@@TRAP %s\n' "$(grep -a '^TRAP:' $T/$n.pme | cut -d' ' -f2)" >> $T/$n.pmx   # svm tuzagi kendi stdout'una yazar
  lim $PM run $T/$n.pmb --native-strict > $T/$n.nat 2>/dev/null; rn=$?
  [ $rn = 3 ] && pass || { cmp -s $T/$n.pm $T/$n.nat && [ $rp = $rn ] && pass || fail "$n: pm [$(head -c 50 $T/$n.pm|tr '\n' ' ')] native-strict [$(head -c 50 $T/$n.nat|tr '\n' ' ')] rc $rp/$rn"; }
  # svm adsiz (soyulmus) liste ister: bayrak 1 -> 0 ve ad bolumu (islev sayisi kadar satir) atilir
  if [ "$(head -c2 $l)" = "1 " ]; then nf=$(sed -n 5p $l); sl=$(sed '1s/^1 /0 /' $l | grep -v "^\$" | head -n -$nf); else sl=$(cat $l); fi
  { cat $TAB; echo "@@PROGRAM"; echo "$sl"; echo "@@GIRDI"; } > $T/$n.sin
  lim $PM run $R/oz/sayisal/svm.pmb < $T/$n.sin > $T/$n.svm 2>/dev/null
  cmp -s $T/$n.pmx $T/$n.svm && pass || fail "$n: svm [$(head -c 50 $T/$n.svm|tr '\n' ' ')] pm [$(head -c 50 $T/$n.pm|tr '\n' ' ')]"
  lim $PM run $GEN2 < $l > $T/$n.elf 2>$T/$n.gerr; [ -s $T/$n.elf ] || { fail "$n: ELF uretilemedi: $(head -c 100 $T/$n.gerr)"; continue; }
  chmod +x $T/$n.elf; lim $T/$n.elf > $T/$n.eo 2>/dev/null; re=$?
  cmp -s $T/$n.pm $T/$n.eo && [ $rp = $re ] && pass || fail "$n: pm [$(head -c 60 $T/$n.pm|tr '\n' ' ')] ELF [$(head -c 60 $T/$n.eo|tr '\n' ' ')] rc $rp/$re"
done
# ---- yakit taramasi: putf yakit sinirinda pm ile ayni noktada tuzaga dusmeli (ELF'e "-1 N" oneki = pm --fuel N)
$PM asm $D/hostrt_ek/yakit.pma -o $T/y.pmb --strip > /dev/null 2>&1 && $PM liste $T/y.pmb > $T/y.l || fail "yakit: derlenemedi"
for N in 1 2 3 4 5 6; do
  lim $PM run $T/y.pmb --fuel $N > $T/y.pm 2>$T/y.pme; rp=$?
  { echo "-1 $N"; cat $T/y.l; } | lim $PM run $GEN2 > $T/y.elf 2>/dev/null; chmod +x $T/y.elf; lim $T/y.elf > $T/y.eo 2>$T/y.eoe; re=$?
  [ $rp = $re ] && cmp -s $T/y.pm $T/y.eo && [ "$(grep -a -o 'TRAP: [A-Z0-9]*' $T/y.pme)" = "$(grep -a -o 'TRAP: [A-Z0-9]*' $T/y.eoe)" ] && pass || fail "yakit N=$N: pm rc=$rp [$(head -c 30 $T/y.pm|tr '\n' ' ')] ELF rc=$re [$(head -c 30 $T/y.eo|tr '\n' ' ')]"
done
# ---- yigin ozelligi (yalniz ELF: yazmac yigini ELF'te 4 MiB, pm'de 8 MiB -- kapasite farki onceden vardir): RT cercevesi yigin sonunu ASAMAZ.
# Kucuk cerceveli yinelemede putc ile (RT yok) kullanici cagrisi, putf ile (RT cercevesi gerekir) host STACK tuzagina duser: putf, putc'den EN FAZLA ~RT cercevesi kadar once ve STACK tuzagi ile (bozulma/sessiz tasma yok) durur.
for op in "putc c" "putf x"; do set -- $op
  sed "s/HOSTOP/$1/;s/HOSTARG/$2/" $D/hostrt_ek/yigin.pma.in > $T/ys_$1.pma; for k in 0 1 2 3 4 5 6 7 8 9; do sed -i "s/^    mov a, 1\$/    mov a, 1\n    mov g$k, $k/" $T/ys_$1.pma; done
  $PM asm $T/ys_$1.pma -o $T/ys_$1.pmb --strip > /dev/null 2>&1 && $PM liste $T/ys_$1.pmb | lim $PM run $GEN2 > $T/ys_$1.elf 2>/dev/null; chmod +x $T/ys_$1.elf
  lim $T/ys_$1.elf > $T/ys_$1.o 2>$T/ys_$1.e; rc=$?; eval "L_$1=$(wc -l < $T/ys_$1.o)"; eval "R_$1=$rc"; grep -aq 'TRAP: STACK' $T/ys_$1.e && pass || fail "yigin: $1 STACK tuzagi yok (rc $rc: $(head -c 60 $T/ys_$1.e))"
done
if [ "${L_putc:-0}" -gt 0 ] && [ "${L_putf:-0}" -lt "${L_putc:-0}" ] && [ $((L_putc - L_putf)) -le 20 ]; then pass; else fail "yigin: putf duzey sayisi ($L_putf) putc'den ($L_putc) 1..20 duzey az olmali (RT cercevesi payi)"; fi
rm -rf $T; echo "dogrula_hostrt: gecen=$ok hata=$bad"; [ $bad = 0 ]
