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

# yerel makine kodu: --native-strict (donusturulebilen programlarda) yorumlayici ile ayni cikti ve cikis kodu; donusturulemeyen (rc 3) sayilir
nativecmp() {   # ad pmb beklenen_cikti beklenen_rc
  lim $PM run $2 --native-strict > $T/nat.$$ 2>/dev/null; r=$?
  if [ $r = 3 ]; then echo 1 >> $T/nat_atla; else cmp -s $T/nat.$$ $3 && [ $r = $4 ] && { pass; echo 1 >> $T/nat_ok; } || fail "$1: native-strict cikti/cikis yorumlayiciyla farkli (rc $r/$4)"; fi
  rm -f $T/nat.$$
}
# yakit TEKDUZELIGI: iyilestirilmis program hicbir yakit degerinde orijinalden FAZLA yakit harcamamali. Orijinal N yakitla biterse (rc 0) iyilestirilmis de ayni ciktiyla bitmeli;
# orijinal FUEL tuzagiyla kesilirse iyilestirilmis ya tamamlanir (cikti tam cikti) ya FUEL ile kesilir (cikti tam ciktinin oneki). Diger tuzaklar (DIV0/OOB...) korunmali.
fuelmono_raw() {   # ad orijinal.pmb iyilestirilmis.pmb -> ihlal varsa mesaj yazar
  local n=$1 a=$2 b=$3 N full ra rb
  lim $PM run $a > $T/fm_full.$$ 2>/dev/null; ra=$?
  for N in 1 2 3 5 8 13 21 40 60 75 100 150 300 1000 5000; do
    lim $PM run $a --fuel $N > $T/fm_a.$$ 2> $T/fm_ae.$$; ra=$?
    lim $PM run $b --fuel $N > $T/fm_b.$$ 2> $T/fm_be.$$; rb=$?
    if [ $ra = 0 ]; then cmp -s $T/fm_a.$$ $T/fm_b.$$ && [ $rb = 0 ] || { echo "$n: yakit $N: orijinal bitiyor, iyilestirilmis farkli (rc $rb)"; break; }
    elif grep -q "TRAP: FUEL" $T/fm_ae.$$; then
      if [ $rb = 0 ]; then cmp -s $T/fm_b.$$ $T/fm_full.$$ || { echo "$n: yakit $N: iyilestirilmis tamamlandi ama cikti farkli"; break; }
      else grep -q "TRAP: FUEL" $T/fm_be.$$ && [ "$(head -c $(stat -c%s $T/fm_b.$$) $T/fm_full.$$ | cmp - $T/fm_b.$$ && echo ok)" = ok ] || { echo "$n: yakit $N: iyilestirilmis kesintisinde cikti tam ciktinin oneki degil / baska tuzak"; break; }; fi
    else   # baska tuzak: ayni tuzak turu ve ayni cikti (yakit yetse de yetmese de)
      [ "$(grep -o 'TRAP: [A-Z0-9]*' $T/fm_ae.$$)" = "$(grep -o 'TRAP: [A-Z0-9]*' $T/fm_be.$$)" ] && cmp -s $T/fm_a.$$ $T/fm_b.$$ || { echo "$n: yakit $N: tuzak/cikti iyilestirmede degisti"; break; }
    fi
  done
  rm -f $T/fm_*.$$
}
fuelmono() { m=$(fuelmono_raw "$@"); [ -z "$m" ] && pass || fail "$m"; }
export -f nativecmp fuelmono fuelmono_raw fuelmono
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
  nativecmp "$n" $w/p.pmb $w/o1 $rc1; nativecmp "$n/opt" $w/q.pmb $w/o1 $rc1
  fuelmono "$n" $w/p.pmb $w/q.pmb
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
  nativecmp "akis/$n" $w/p.pmb $exp $erc
  lim $BAG/yaz.elf < $w/q.l > $w/q.pmb 2>/dev/null; lim $PM run $w/q.pmb > $w/o4 2>/dev/null; rc=$?; [ $rc = $erc ] && cmp -s $w/o4 $exp && pass || fail "akis/$n: iyilestirilmis pm farkli (rc $rc)"
  nativecmp "akis/$n/opt" $w/q.pmb $exp $erc; fuelmono "akis/$n" $w/p.pmb $w/q.pmb
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
# 2c) yakit tekduzeligi denetiminin kendi duyarliligi: ayni donguyu 1 yerine 2 geri atlamayla yapan program "iyilestirilmis" diye verilirse ihlal YAKALANMALI (ters yon gecmeli)
cat > $T/m1.pma <<'EOP'
func main() {
  ldi i, 0
L: addi i, i, 1
  lti c, i, 50
  jnz c, L
  host puti, i
  halt
}
EOP
cat > $T/m2.pma <<'EOP'
func main() {
  ldi i, 0
L: addi i, i, 1
  lti c, i, 50
  jz c, E
  jmp B1
B2: jmp L
B1: jmp B2
E: host puti, i
  halt
}
EOP
lim $BAG/asm.elf < $T/m1.pma > $T/m1.pmb 2>/dev/null; lim $BAG/asm.elf < $T/m2.pma > $T/m2.pmb 2>/dev/null
[ -n "$(fuelmono_raw meta $T/m1.pmb $T/m2.pmb)" ] && pass || fail "yakit tekduzeligi: fazla yakit harcayan program yakalanmadi"
[ -z "$(fuelmono_raw meta $T/m2.pmb $T/m1.pmb)" ] && pass || fail "yakit tekduzeligi: az yakit harcayan program yanlis reddedildi"
# 3) olumsuz: bozuk liste (kisa/eksik/gecersiz komut) cokmez; cikis kodu 0/1/2 (tuzak) disinda sinyal yok
head -c 200 $T/p_ca_switch/p.l > $T/bad1.l; { echo 0; cat $TAB; cat $T/bad1.l; } | lim $T/cfgopt.elf > /dev/null 2>&1; r=$?; [ $r -lt 128 ] && pass || fail "kisa liste: sinyalle oldu ($r)"
sed '5s/.*/9999/' $T/p_ca_switch/p.l > $T/bad2.l; { echo 0; cat $TAB; cat $T/bad2.l; } | lim $T/cfgopt.elf > /dev/null 2>&1; r=$?; [ $r -lt 128 ] && pass || fail "bozuk liste: sinyalle oldu ($r)"
# 3b) gecersiz kontrol akisi AKILLICA reddedilir (DIV0 tuzagi, cikis 2): jtab tablosu tasmasi, merdiven girdisi jmp degil / geri, hedef disari, negatif n, bilinmeyen komut, kesik girdi
lim $BAG/asm.elf < $D/akis/tablo.pma > $T/tb.pmb 2>/dev/null && lim $BAG/coz.elf < $T/tb.pmb > $T/tb.l || fail "tablo.pma: asm/coz"
jt=$(awk 'NF==3 && $1==343 {print $0; exit}' $T/tb.l); jop=$(awk 'NF==2 && $1==31 {print $1; exit}' $T/tb.l)
if [ -n "$jt" ]; then
  tr_rej() {   # ad sed-ifadesi
    sed "$2" $T/tb.l > $T/tb2.l; cmp -s $T/tb.l $T/tb2.l && { fail "cfg-red/$1: mutasyon uygulanmadi"; return; }
    { echo 0; cat $TAB; cat $T/tb2.l; } | lim $T/cfgopt.elf > /dev/null 2> $T/tb.err; r=$?; [ $r = 2 ] && grep -q "TRAP: DIV0" $T/tb.err && pass || fail "cfg-red/$1: gecersiz liste reddedilmedi (rc $r)"
  }
  set -- $jt; jn=$3; fe=$(awk 'NF==2 && $1==31 {print NR; exit}' $T/tb.l)
  tr_rej jtab-n-buyuk "s/^343 $2 $jn\$/343 $2 9999/"
  tr_rej jtab-n-negatif "s/^343 $2 $jn\$/343 $2 -1/"
  tr_rej merdiven-jmp-degil "${fe}s/.*/1 0 5/"
  tr_rej merdiven-geri "${fe}s/.*/31 0/"
  tr_rej hedef-disi "${fe}s/.*/31 99999/"
  tr_rej merdiven-kosullu "${fe}s/.*/32 0 13/"
  tr_rej bilinmeyen-komut-son "\$s/.*/1399/"
  tr_rej bilinmeyen-komut-buyuk "${fe}s/.*/9999 0 0/"
fi
head -c 120 $T/tb.l > $T/tb3.l; { echo 0; cat $TAB; cat $T/tb3.l; } | lim $T/cfgopt.elf > /dev/null 2>&1; [ $? = 2 ] && pass || fail "cfg-red/kesik girdi reddedilmedi"
# gecerli jtab programlari (ifx + el yazimi) iyilestiricide merdiveni korur: iyilestirilmis listede de ayni sayida jtab + ardisik ileri jmp
for lst in $T/tb.l $T/p_ca_switch/p.l; do [ -s $lst ] || continue; opt $lst > $T/tbq.l 2>/dev/null; a=$(awk -v j=343 '$1==j && NF==3' $lst | wc -l); b=$(awk -v j=343 '$1==j && NF==3' $T/tbq.l | wc -l); [ "$a" = "$b" ] && [ "$a" -gt 0 ] && pass || fail "jtab sayisi iyilestirmede degisti ($a -> $b)"; done
# jtab merdiveni gecersiz programlari (oz/cfg/red/jtab_*.pma): valid.elf, C++ dogrulayici (listedogrula) ve yukleyici (pm run: cikis 1 + ileti) AYNI SEKILDE reddeder
for rp in $D/red/jtab_*.pma; do
  rn=jtab_red_$(basename $rp .pma); lim $BAG/asm.elf < $rp > $T/rd.pmb 2>/dev/null && [ -s $T/rd.pmb ] || { fail "$rn: asm.elf derlemedi"; continue; }
  lim $BAG/coz.elf < $T/rd.pmb > $T/rd.l; v=$(lim $BAG/valid.elf < $T/rd.l | head -1); c=$(lim $PM listedogrula $T/rd.l | head -1)
  [ "${v%% *}" = 0 ] && [ "${c%% *}" = 0 ] && pass || fail "$rn: valid.elf='$v' C++='$c' (ikisi de 0 olmali)"
  lim $PM run $rp > /dev/null 2> $T/rd.err; r=$?; [ $r = 1 ] && grep -qF "$(cat ${rp%.pma}.errmatch)" $T/rd.err && pass || fail "$rn: pm run cikis 1 + '$(cat ${rp%.pma}.errmatch)' beklenirdi (rc $r): $(head -c 100 $T/rd.err)"
done
if [ -s $T/sayi ]; then awk '/^arac/{next} {a+=$1; b+=$2} END{printf "cfg: toplam komut %d -> %d (%.1f%% azalma)\n", a, b, 100*(a-b)/a}' $T/sayi; grep "^arac" $T/sayi; fi
ok=$(cat $T/gecen 2>/dev/null | wc -l); bad=$(grep -c . $T/hatalar 2>/dev/null); bad=${bad:-0}
[ -s $T/hatalar ] && sort $T/hatalar | head -30
echo "cfg: native-strict karsilastirmasi: $(cat $T/nat_ok 2>/dev/null | wc -l) program (donusturulemeyen: $(cat $T/nat_atla 2>/dev/null | wc -l))"
echo "cfg: $ok gecti, $bad basarisiz"; rm -rf $T; [ "$bad" = 0 ]
