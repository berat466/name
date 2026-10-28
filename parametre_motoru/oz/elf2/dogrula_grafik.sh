#!/bin/bash
# present / poll / frame (sanal cerceve tamponu) ELF denetimi: bagimsiz ELF = `pm run` (referans) -- stdout, cikis kodu, stderr (kare raporu), son kare (PPM), ASCII.
# Kullanim: dogrula_grafik.sh [pm] ; uretici: GEN2 (gen2.pmb), pm run ile ya da GENELF (ornegin bagimsiz gen2.elf)
# Secenekler stdin on eki ile ELF'e verilir: -2 n (kare kod)*n olaylar, -3 azami_kare, -4 ascii_K, -5 bayrak (1 kare raporu, 2 son kare PPM -> fd 3).
PM=${1:-../../pm}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d)
G2=${GEN2:-$D/gen2.pmb}; GEN="${GENELF:-$PM run $G2}"
ok=0; bad=0; n=0
pass() { ok=$((ok+1)); }; fail() { bad=$((bad+1)); echo "HATA: $1"; }
lim() { (ulimit -v 3000000; timeout 60 "$@"); }
# olaylari (kare kod) ELF on ekine cevir (pm ile ayni: yorum/gecersiz satirlar atlanir, kareye gore kararli sirala)
onek() {   # maxkare asciiK girdi_dosyasi
  local ev="" cnt=0
  if [ -n "$3" ]; then ev=$(grep -E '^[0-9]+[[:space:]]+[0-9]+' "$3" | sort -s -n -k1,1 | while read -r a b rest; do echo "$a $b"; done); cnt=$(echo "$ev" | grep -c .); fi
  printf -- '-2 %s %s -3 %s -4 %s -5 3\n' "$cnt" "$(echo $ev)" "${1:-0}" "${2:-0}"
}
# kos etiket pmb [pm secenekleri: --max-kare K --ascii K --girdi F] ; ELF ayni seceneklerle, stdin bos
kos() {
  local et=$1 pmb=$2; shift 2
  local mk=0 ak=0 gi=""
  while [ $# -gt 0 ]; do case "$1" in --max-kare) mk=$2; shift 2;; --ascii) ak=$2; shift 2;; --girdi) gi=$2; shift 2;; *) shift;; esac; done
  $PM liste $pmb > $T/l.txt 2>/dev/null || { fail "$et: liste"; return; }
  { onek $mk $ak "$gi"; cat $T/l.txt; } > $T/in.txt
  if ! lim $GEN < $T/in.txt > $T/p.elf 2>/dev/null; then fail "$et: ELF uretilemedi"; return; fi
  chmod +x $T/p.elf
  rm -f $T/pm.ppm $T/elf.ppm
  lim $PM run $pmb ${mk:+--max-kare $mk} ${gi:+--girdi $gi} ${ak:+--ascii $ak} --kare-rapor --ekran $T/pm.ppm < /dev/null > $T/o1 2> $T/e1; r1=$?
  lim $T/p.elf < /dev/null > $T/o2 2> $T/e2 3> $T/elf.ppm; r2=$?
  [ -s $T/elf.ppm ] || rm -f $T/elf.ppm
  n=$((n+1))
  grep -E '^ekran:' $T/e1 > $T/r1; grep -E '^ekran:' $T/e2 > $T/r2
  local t1 t2; t1=$(grep -a '^TRAP:' $T/e1 | cut -d' ' -f1-2); t2=$(grep -a '^TRAP:' $T/e2 | cut -d' ' -f1-2)
  if [ $r1 = $r2 ] && cmp -s $T/o1 $T/o2 && cmp -s $T/r1 $T/r2 && [ "$t1" = "$t2" ] && { [ ! -e $T/pm.ppm ] && [ ! -e $T/elf.ppm ] || cmp -s $T/pm.ppm $T/elf.ppm; }; then pass
  else fail "$et: pm rc=$r1 elf rc=$r2 | $(cmp $T/o1 $T/o2 2>&1 | head -c 50) | '$(cat $T/r1 | head -c 90)' <> '$(cat $T/r2 | head -c 90)' | $t1 <> $t2"; fi
}
asm() { $PM asm "$1" -o "$2" --strip > /dev/null 2>&1; }
# ---- 1) uygulamalar (ascii / ekran / rapor, farkli azami_kare)
for sec in "" "--max-kare 1" "--max-kare 7" "--max-kare 50 --ascii 4" "--ascii 8"; do
  for a in hayat sahne pong; do asm $R/uygulamalar/$a.pma $T/a.pmb; g=""; [ $a = pong ] && g="--girdi $R/uygulamalar/pong.girdi"; kos "$a $sec" $T/a.pmb $sec $g; done
  for a in parcacik_elle; do asm $R/uygulamalar/sablon/$a.pma $T/a.pmb; kos "$a $sec" $T/a.pmb $sec; done
  for a in tugla kovala parcacik veri pong; do $PM sablon $R/uygulamalar/sablon/$a.pmt -o $T/a.pmb --strip > /dev/null 2>&1 || { fail "$a.pmt derlenmedi"; continue; }; g=""; [ -f $R/uygulamalar/$a.girdi ] && g="--girdi $R/uygulamalar/$a.girdi"; kos "$a.pmt $sec" $T/a.pmb $sec $g; done
done
# ---- 2) elle yazilmis uc durumlar
for f in $D/grafik/*.pma; do
  b=$(basename $f .pma); asm $f $T/g.pmb || { fail "$b derlenmedi"; continue; }
  g=""; [ -f ${f%.pma}.girdi ] && g="--girdi ${f%.pma}.girdi"
  for sec in "" "--max-kare 1" "--max-kare 3" "--ascii 2"; do kos "$b $sec" $T/g.pmb $sec $g; done
done
echo "grafik: $ok gecti, $bad basarisiz ($n karsilastirma)"
rm -rf $T
[ $bad = 0 ]
