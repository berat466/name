#!/bin/bash
# PLAN Bolum 03 (03.019): CFG merkezli genel iyilestirici (cfgopt.pmb: sayisal program, ifx ile derlenmis) dogrulamasi.
#  * tazelik: cfgopt.cx -> ifx.elf -> asm.elf bayt-bayt = cfgopt.pmb ; pm'siz zincir (cfgopt.elf = gen2)
#  * her test programi:  ifx.elf -> asm.elf -> coz.elf -> [orijinal] ; cfgopt -> valid.elf kabul -> tipdogr kabul -> yaz.elf ; orijinal pm = iyilestirilmis pm = iyilestirilmis ELF = (alt kume) svm ; komut sayisi AZALIR ; iyilestirici idempotent
#  * arac zinciri uzerinde: asm.pmb / valid.pmb iyilestirilir ve ayni girdilerde ayni cikti uretir (iyilestiricinin asil sinavi)
#  * olumsuz: bozuk liste -> temiz ret/cokme yok ; mutasyon: sabotaj_cfg.sh
# Kullanim: dogrula_cfg.sh [pm]   (ORTAM: SVM=0 svm atla)
PM=${1:-../../pm}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d); export PM D R T
TAB=${TAB:-$R/oz/komut/komutlar.pmk}; CFGB=${CFGB:-$D/cfgopt.pmb}; export TAB CFGB
lim() { (ulimit -v 3000000; timeout ${LIMS:-180} "$@"); }; export -f lim
pass() { echo 1 >> $T/gecen; }; fail() { echo "HATA: $1" >> $T/hatalar; }; export -f pass fail
$R/oz/bag/kur.sh $PM $T/bag > /dev/null 2>&1 || { echo "kur.sh basarisiz"; exit 1; }; BAG=$T/bag; export BAG
{ echo "@@imza"; cat $TAB; } | $BAG/komut.elf | tr '\\n' ' ' > $T/imza.txt; echo >> $T/imza.txt; export IMZ=$T/imza.txt
$PM liste $R/oz/ifade/num/ifx.pmb > $T/ifx.l && $BAG/gen2.elf < $T/ifx.l > $T/ifx.elf && chmod +x $T/ifx.elf && pass || fail "ifx.elf uretilemedi"
$PM liste $R/oz/tip/tipdogr.pmb > $T/tip.l && $BAG/gen2.elf < $T/tip.l > $T/tipdogr.elf && chmod +x $T/tipdogr.elf; export TIPE=$T/tipdogr.elf
# 1) tazelik: cfgopt.cx -> pmb (pm'siz) = depodaki cfgopt.pmb
if [ -z "$SABOTAJ" ]; then
  $PM ifade $D/cfgopt.cx --pmx1 $T/c.pmx > /dev/null 2>&1; { cat $TAB; cat $IMZ; cat $T/c.pmx; } | lim $T/ifx.elf > $T/c.pma 2>/dev/null; $BAG/asm.elf < $T/c.pma > $T/c.pmb 2>/dev/null; cmp -s $T/c.pmb $D/cfgopt.pmb && pass || fail "cfgopt.pmb guncel degil (cfg/uret.sh)"
fi
$PM liste $CFGB > $T/cfg.l && $BAG/gen2.elf < $T/cfg.l > $T/cfgopt.elf && chmod +x $T/cfgopt.elf && pass || fail "cfgopt.elf uretilemedi"
opt() { { echo 0; cat $TAB; cat $1; } | lim $T/cfgopt.elf; }; st() { { echo 1; cat $TAB; cat $1; } | lim $T/cfgopt.elf; }; export -f opt st
cone() {
  f=$1; n=$(basename $f .cx); w=$T/p_$n; mkdir -p $w
  $PM ifade $f --pmx1 $w/x.pmx > /dev/null 2>&1; [ -s $w/x.pmx ] || return
  { cat $TAB; cat $IMZ; echo; cat $w/x.pmx; } | lim $T/ifx.elf > $w/p.pma 2>/dev/null || return
  lim $BAG/asm.elf < $w/p.pma > $w/p.pmb 2>/dev/null && lim $BAG/coz.elf < $w/p.pmb > $w/p.l || { fail "$n: asm/coz"; return; }
  lim $PM run $w/p.pmb > $w/o1 2>/dev/null; rc1=$?
  opt $w/p.l > $w/q.l 2> $w/opt.err && [ -s $w/q.l ] && pass || { fail "$n: iyilestirici basarisiz"; return; }
  v=$(lim $BAG/valid.elf < $w/q.l | head -1); [ "${v%% *}" = 1 ] && pass || { fail "$n: valid.elf iyilestirilmis programi reddetti: $v"; return; }
  grep -a '^;;PMT1' $w/p.pma | sed 's/^;;PMT1 //' > $w/p.t; tv=$( { echo '@@IMZA'; cat $IMZ; echo '@@PROGRAM'; cat $w/q.l; echo '@@TIPLER'; cat $w/p.t; } | lim $TIPE | head -1 ); [ "${tv%% *}" = 1 ] && pass || fail "$n: tipdogr iyilestirilmis programi reddetti: $tv"
  lim $BAG/yaz.elf < $w/q.l > $w/q.pmb 2>/dev/null && [ -s $w/q.pmb ] || { fail "$n: yaz.elf"; return; }
  lim $PM run $w/q.pmb > $w/o2 2>/dev/null; rc2=$?; [ $rc1 = $rc2 ] && cmp -s $w/o1 $w/o2 && pass || fail "$n: iyilestirilmis pm cikti/cikis farkli (rc $rc1/$rc2)"
  lim $BAG/gen2.elf < $w/q.l > $w/q.elf 2>/dev/null && chmod +x $w/q.elf && { lim $w/q.elf > $w/o3 2>/dev/null; cmp -s $w/o1 $w/o3 && pass || fail "$n: iyilestirilmis ELF cikti farkli"; } || fail "$n: iyilestirilmis ELF uretilemedi"
  s=$(st $w/p.l); set -- $s; [ "$2" -lt "$1" ] && pass || fail "$n: komut sayisi azalmadi ($s)"; echo "$1 $2" >> $T/sayi
  opt $w/q.l > $w/q2.l 2>/dev/null; cmp -s $w/q.l $w/q2.l && pass || fail "$n: iyilestirici idempotent degil"
  if [ "${SVM:-1}" = 1 ] && [ -n "$SV" ]; then { cat $TAB; echo "@@PROGRAM"; cat $w/q.l; echo "@@GIRDI"; } > $w/sv.in; LIMS=300 lim $PM run $R/oz/sayisal/svm.pmb < $w/sv.in > $w/o4 2>/dev/null; cmp -s $w/o1 $w/o4 && pass || fail "$n: iyilestirilmis program svm cikti farkli"; fi
}
export -f cone
rcok() { if [ $1 = 0 ]; then [ $2 = 0 ]; else [ $2 != 0 ]; fi; }
SV=1; export SV
ls $D/../ifade/num/testler/*.cx $D/../ifade/testler/*.cx | xargs -P $(nproc) -I{} bash -c 'cone {}'
# 2b) cekirdek akis komutlari (03.001-005, 03.009): elle yazilmis PMA programlari, bagimsiz hesaplanmis beklenen cikti; pm = ELF = svm = iyilestirilmis pm/ELF
for pma in $D/akis/*.pma; do
  n=$(basename $pma .pma); w=$T/a_$n; mkdir -p $w; erc=0; [ -f ${pma%.pma}.rc ] && erc=$(cat ${pma%.pma}.rc); exp=${pma%.pma}.out
  lim $BAG/asm.elf < $pma > $w/p.pmb 2>/dev/null && lim $BAG/coz.elf < $w/p.pmb > $w/p.l || { fail "akis/$n: asm/coz"; continue; }
  v=$(lim $BAG/valid.elf < $w/p.l | head -1); [ "${v%% *}" = 1 ] && pass || fail "akis/$n: valid reddetti $v"
  lim $PM run $w/p.pmb > $w/o1 2>/dev/null; rc=$?; [ $rc = $erc ] && cmp -s $w/o1 $exp && pass || fail "akis/$n: pm cikti/cikis beklenenden farkli (rc $rc)"
  lim $BAG/gen2.elf < $w/p.l > $w/p.elf 2>/dev/null && chmod +x $w/p.elf && { lim $w/p.elf > $w/o2 2>/dev/null; rc=$?; }; rcok $erc $rc && cmp -s $w/o2 $exp && pass || fail "akis/$n: ELF cikti/cikis farkli (rc $rc)"
  if [ $erc = 0 ]; then { cat $TAB; echo "@@PROGRAM"; cat $w/p.l; echo "@@GIRDI"; } > $w/sv.in; LIMS=300 lim $PM run $R/oz/sayisal/svm.pmb < $w/sv.in > $w/o3 2>/dev/null; cmp -s $w/o3 $exp && pass || fail "akis/$n: svm cikti farkli"; fi
  opt $w/p.l > $w/q.l 2>/dev/null; v=$(lim $BAG/valid.elf < $w/q.l | head -1); [ "${v%% *}" = 1 ] && pass || fail "akis/$n: iyilestirilmis valid reddetti"
  lim $BAG/yaz.elf < $w/q.l > $w/q.pmb 2>/dev/null; lim $PM run $w/q.pmb > $w/o4 2>/dev/null; rc=$?; [ $rc = $erc ] && cmp -s $w/o4 $exp && pass || fail "akis/$n: iyilestirilmis pm farkli (rc $rc)"
  lim $BAG/gen2.elf < $w/q.l > $w/q.elf 2>/dev/null && chmod +x $w/q.elf && { lim $w/q.elf > $w/o5 2>/dev/null; rc=$?; }; rcok $erc $rc && cmp -s $w/o5 $exp && pass || fail "akis/$n: iyilestirilmis ELF farkli (rc $rc)"
done
# 2) arac zinciri: asm.pmb ve valid.pmb iyilestirilir; ayni girdide ayni cikti
for tool in asm:$R/oz/asm/asm.pmb dogr:$R/oz/dogr/valid.pmb; do
  nm=${tool%%:*}; pmb=${tool#*:}; $PM liste $pmb > $T/$nm.l; opt $T/$nm.l > $T/$nm.q.l 2>/dev/null && [ -s $T/$nm.q.l ] && pass || { fail "$nm: iyilestirici basarisiz"; continue; }
  v=$(lim $BAG/valid.elf < $T/$nm.q.l | head -1); [ "${v%% *}" = 1 ] && pass || fail "$nm: valid.elf reddetti: $v"
  lim $BAG/yaz.elf < $T/$nm.q.l > $T/$nm.q.pmb 2>/dev/null; s=$(st $T/$nm.l); set -- $s; [ "$2" -lt "$1" ] && pass || fail "$nm: komut sayisi azalmadi"; echo "arac $nm: $1 -> $2" >> $T/sayi
done
for pma in $T/p_ca_switch/p.pma $T/p_ca_korutin/p.pma $T/p_uyg_agac/p.pma; do
  [ -s $pma ] || continue; lim $PM run $R/oz/asm/asm.pmb < $pma > $T/a1.pmb 2>/dev/null; lim $PM run $T/asm.q.pmb < $pma > $T/a2.pmb 2>/dev/null; cmp -s $T/a1.pmb $T/a2.pmb && [ -s $T/a1.pmb ] && pass || fail "asm.pmb iyilestirilmis farkli cikti ($(basename $(dirname $pma)))"
  lim $PM liste $T/a1.pmb > $T/a1.l; lim $PM run $R/oz/dogr/valid.pmb < $T/a1.l > $T/v1 2>/dev/null; lim $PM run $T/dogr.q.pmb < $T/a1.l > $T/v2 2>/dev/null; cmp -s $T/v1 $T/v2 && pass || fail "valid.pmb iyilestirilmis farkli karar"
  lim $BAG/gen2.elf < $T/a1.l > $T/g1.elf 2>/dev/null; lim $PM run $R/oz/elf2/gen2.pmb < $T/a1.l > $T/g2.elf 2>/dev/null; cmp -s $T/g1.elf $T/g2.elf && pass || fail "gen2 uretimi farkli"
done
# gen2.pmb de iyilestirilir (ELF uretici): ayni programdan ayni ELF
$PM liste $R/oz/elf2/gen2.pmb > $T/gen2.l; opt $T/gen2.l > $T/gen2.q.l 2>/dev/null && lim $BAG/yaz.elf < $T/gen2.q.l > $T/gen2.q.pmb 2>/dev/null && [ -s $T/gen2.q.pmb ] && pass || fail "gen2: iyilestirme/yazim basarisiz"
if [ -s $T/gen2.q.pmb ] && [ -s $T/p_ca_switch/q.l ]; then lim $PM run $T/gen2.q.pmb < $T/p_ca_switch/p.l > $T/g3.elf 2>/dev/null; lim $PM run $R/oz/elf2/gen2.pmb < $T/p_ca_switch/p.l > $T/g4.elf 2>/dev/null; cmp -s $T/g3.elf $T/g4.elf && [ -s $T/g3.elf ] && pass || fail "iyilestirilmis gen2.pmb farkli ELF uretti"; fi
# 3) olumsuz: bozuk liste (kisa/eksik/gecersiz komut) cokmez; cikis kodu 0/1/2 (tuzak) disinda sinyal yok
head -c 200 $T/p_ca_switch/p.l > $T/bad1.l; { echo 0; cat $TAB; cat $T/bad1.l; } | lim $T/cfgopt.elf > /dev/null 2>&1; r=$?; [ $r -lt 128 ] && pass || fail "kisa liste: sinyalle oldu ($r)"
sed '5s/.*/9999/' $T/p_ca_switch/p.l > $T/bad2.l; { echo 0; cat $TAB; cat $T/bad2.l; } | lim $T/cfgopt.elf > /dev/null 2>&1; r=$?; [ $r -lt 128 ] && pass || fail "bozuk liste: sinyalle oldu ($r)"
if [ -s $T/sayi ]; then awk '/^arac/{next} {a+=$1; b+=$2} END{printf "cfg: toplam komut %d -> %d (%.1f%% azalma)\n", a, b, 100*(a-b)/a}' $T/sayi; grep "^arac" $T/sayi; fi
ok=$(cat $T/gecen 2>/dev/null | wc -l); bad=$(grep -c . $T/hatalar 2>/dev/null); bad=${bad:-0}
[ -s $T/hatalar ] && sort $T/hatalar | head -30
echo "cfg: $ok gecti, $bad basarisiz"; rm -rf $T; [ "$bad" = 0 ]
