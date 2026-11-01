#!/bin/bash
# PMB1 GERIYE UYUMLULUK (komut kodu >= 255 kacis kodlamasi oncesi/sonrasi): ESKI C++ pm (kacis kodlamasindan onceki commit, git worktree) ile uretilmis PMB1 dosyalari
# YENI araclarda bayt-bayt ayni uretilir (C++ asm, sayisal asm.pmb), ayni listeye cozulur (C++ liste, coz.pmy), yeniden kodlanir (yazliste, yaz.pmy) ve ayni cikti/cikis koduyla calisir.
# Kullanim: dogrula_uyum.sh [pm]   (TABAN: eski commit, varsayilan 3b9f001)
D=$(cd "$(dirname "$0")" && pwd); PM=${1:-$D/../../pm}; PM=$(realpath "$PM"); R=$D/../..; TABAN=${TABAN:-3b9f001}; T=$(mktemp -d); ok=0; bad=0
fail() { bad=$((bad+1)); echo "HATA: $1"; }
G=$(cd $R && git rev-parse --show-toplevel); git -C "$G" worktree add -f $T/eski $TABAN > /dev/null 2>&1 || { echo "taban commit $TABAN alinamadi"; exit 1; }
(cd $T/eski/parametre_motoru && make pm > /dev/null 2>&1) || { echo "eski pm derlenemedi"; exit 1; }
OLD=$T/eski/parametre_motoru/pm
for f in $R/tests/*.pma $R/uygulamalar/*.pma $R/uygulamalar/*/*.pma; do
  [ -f "$f" ] || continue; $OLD asm $f -o $T/o.pmb --strip > /dev/null 2>&1 || continue
  $PM asm $f -o $T/n.pmb --strip > /dev/null 2>&1; cmp -s $T/o.pmb $T/n.pmb && ok=$((ok+1)) || fail "$f: C++ asm ciktisi eski ile ayni degil"
  $PM run $R/oz/asm/asm.pmb < $f 2>/dev/null | cmp -s - $T/o.pmb && ok=$((ok+1)) || fail "$f: sayisal asm.pmb ciktisi eski ile ayni degil"
  $OLD liste $T/o.pmb > $T/o.l; $PM liste $T/o.pmb | cmp -s - $T/o.l && ok=$((ok+1)) || fail "$f: liste farkli"
  $PM run $R/oz/coz.pmy < $T/o.pmb 2>/dev/null | cmp -s - $T/o.l && ok=$((ok+1)) || fail "$f: coz.pmy farkli"
  $PM yazliste $T/o.l -o $T/y.pmb > /dev/null 2>&1; cmp -s $T/y.pmb $T/o.pmb && ok=$((ok+1)) || fail "$f: yazliste farkli"
  $PM run $R/oz/yaz.pmy < $T/o.l 2>/dev/null | cmp -s - $T/o.pmb && ok=$((ok+1)) || fail "$f: yaz.pmy farkli"
  timeout 30 $OLD run $T/o.pmb < /dev/null > $T/o.out 2>&1; r1=$?; timeout 30 $PM run $T/o.pmb < /dev/null > $T/n.out 2>&1; r2=$?
  [ $r1 = $r2 ] && cmp -s $T/o.out $T/n.out && ok=$((ok+1)) || fail "$f: eski PMB1 yeni pm'de farkli calisti"
done
# yeni araclar eski programi ELF'e cevirir ve ayni cikti
git -C "$G" worktree remove --force $T/eski > /dev/null 2>&1
echo "geriye uyumluluk: $ok gecti, $bad basarisiz"; rm -rf $T; [ $bad = 0 ] && [ $ok -gt 100 ]
