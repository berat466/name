#!/bin/bash
# SEM'DEN (sayisal anlam tanimindan) YEREL URETIM denetimi: gen2'ye `-6 1` verilince x86 sablonu olan komutlar da komutun SEM mikro-programindan,
# mikro-islem sablonlariyla (sablon.txt: p_<islem>) uretilir. Bagimsiz referans = C++ `pm run`. Karsilastirma: stdout, cikis kodu, tuzak adi.
# 1) ondalik: ozel degerler (NaN, +-sonsuz, +-0, alt normal, 2^63 siniri ...) x tum ondalik komutlar (dogrula_float.sh, zorlanmis uretici ile)
# 2) tamsayi: ozel degerler (INT64_MIN/MAX, -1, 0, kaydirma 63/64/65 ...) x tum ikili komutlar + bolme/mod (INT64_MIN/-1) + dolayli degerli (addi, shli ...) komutlar
# 3) rastgele cok fonksiyonlu programlar (ld/st/alloc/tuzaklar dahil)
# 4) zorlanmis uretici gercekten mikro yolunu kullaniyor (bayt dizisi, sablonlu yoldan farkli)
# Kullanim: dogrula_mikro.sh [pm] [rastgele_sayisi]   (GEN2: uretici PMB1'i)
PM=${1:-../../pm}; NR=${2:-40}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d)
G2=${GEN2:-$D/gen2.pmb}
ok=0; bad=0
pass() { ok=$((ok+1)); }; fail() { bad=$((bad+1)); echo "HATA: $1"; }
lim() { (ulimit -v 700000; timeout 20 "$@"); }
printf '#!/bin/bash\n{ echo "-6 1"; cat; } | %s run %s\n' $PM $G2 > $T/zorla.sh; chmod +x $T/zorla.sh
# ---- 1) ondalik
if GEN2=$G2 GENELF=$T/zorla.sh ./dogrula_float.sh $PM > $T/f.log 2>&1; then pass; else fail "ondalik (zorlanmis mikro yolu): $(tail -3 $T/f.log | tr '\n' ' ')"; fi
# ---- 2) tamsayi ozel degerler
VALS="0 1 -1 2 3 7 63 64 65 -64 255 256 65535 2147483647 -2147483648 4294967295 4294967296 9223372036854775807 -9223372036854775808 -9223372036854775807 3074457345618258602 -6148914691236517206 1000003"
n=$(echo $VALS | wc -w)
{
  echo "memory 4096"; echo "data tab = $VALS"
  cat <<EOT
func pr(v) {
    host puti, v
    host putc, 32
    ret 0
}
func main() {
    mov i, 0
li: gei z, i, $n
    jnz z, ld
    add a, i, tab
    ld x, a, 0
    mov j, 0
lj: gei z, j, $n
    jnz z, nj
    add a, j, tab
    ld y, a, 0
EOT
  for op in add sub mul and or xor shl shr sar eq ne lt le; do printf '    %s r, x, y\n    call q, pr, r\n' $op; done
  cat <<EOT
    jz y, skipd
    div r, x, y
    call q, pr, r
    mod r, x, y
    call q, pr, r
skipd:
    addi j, j, 1
    jmp lj
nj:
EOT
  for op in neg not; do printf '    %s r, x\n    call q, pr, r\n' $op; done
  for imm in 0 1 -1 5 -77 100000 2147483647 -2147483648; do
    for op in addi muli andi eqi nei lti lei gti gei; do printf '    %s r, x, %s\n    call q, pr, r\n' $op $imm; done
  done
  for imm in 0 1 7 31 62 63; do for op in shli shri sari; do printf '    %s r, x, %s\n    call q, pr, r\n' $op $imm; done; done
  for imm in 1 2 3 7 -5 1000003; do for op in divi modi; do printf '    %s r, x, %s\n    call q, pr, r\n' $op $imm; done; done
  cat <<EOT
    mov r, 0
    mov r, x
    ldi r, 123456
    call q, pr, r
    host putc, 10
    addi i, i, 1
    jmp li
ld: ret 0
}
entry main
EOT
} > $T/int.pma
$PM asm $T/int.pma -o $T/int.pmb --strip > $T/asm.log 2>&1 || fail "tamsayi programi derlenemedi: $(head -c 100 $T/asm.log)"
cmpone() {   # etiket pmb
  $PM liste $2 > $T/p.l 2>/dev/null || { fail "$1: liste"; return; }
  lim $T/zorla.sh < $T/p.l > $T/p.elf 2>/dev/null || { fail "$1: zorlanmis ELF uretilemedi"; return; }; chmod +x $T/p.elf
  lim $PM run $2 < $T/in > $T/o1 2> $T/e1; r1=$?
  [ $r1 = 124 ] && return
  lim $T/p.elf < $T/in > $T/o2 2> $T/e2; r2=$?
  if [ $r1 = $r2 ] && cmp -s $T/o1 $T/o2; then pass; else fail "$1: pm rc=$r1 elf rc=$r2 $(cmp $T/o1 $T/o2 2>&1 | head -c 60)"; return; fi
  if [ $r1 = 2 ]; then [ "$(head -c 11 $T/e1 | cut -d' ' -f1-2)" = "$(head -c 11 $T/e2 | cut -d' ' -f1-2)" ] && pass || fail "$1: tuzak adi farkli"; fi
}
printf 'abc\n\200\377\000xyz 123\n' > $T/in
[ -s $T/int.pmb ] && cmpone "tamsayi ozel degerler" $T/int.pmb
[ $(stat -c %s $T/o1) -gt 50000 ] && pass || fail "tamsayi programi beklenen kadar cikti uretmedi ($(stat -c %s $T/o1) bayt)"
# bolme/mod tuzaklari: sifira bolme (yazmac) ayri program
for k in 1 2; do
  { echo "func main() {"; echo " ldi a, 10"; echo " ldi b, 0"; [ $k = 1 ] && echo " div c, a, b" || echo " mod c, a, b"; echo " host puti, c"; echo " halt"; echo "}"; } > $T/dz.pma
  $PM asm $T/dz.pma -o $T/dz.pmb --strip > /dev/null 2>&1 && cmpone "sifira bolme $k" $T/dz.pmb
done
# ---- 3) rastgele programlar
for s in $(seq 1 $NR); do
  awk -v seed=$s -f rastgele2.awk > $T/r.pma; $PM asm $T/r.pma -o $T/r.pmb --strip > /dev/null 2>&1 && cmpone "rastgele2-$s" $T/r.pmb
done
# ---- 4) mikro yolu gercekten kullanildi
$PM liste $T/int.pmb > $T/p.l; lim $PM run $G2 < $T/p.l > $T/n.elf 2>/dev/null; lim $T/zorla.sh < $T/p.l > $T/z.elf 2>/dev/null
if cmp -s $T/n.elf $T/z.elf; then fail "zorlanmis ELF sablonlu ELF ile ayni bayt (mikro yolu kullanilmadi)"; else pass; fi
[ $(stat -c %s $T/z.elf) -gt $(stat -c %s $T/n.elf) ] && pass || fail "mikro yolu ELF'i beklenenden kucuk"
echo "mikro-islem yerel uretimi: $ok gecti, $bad basarisiz"
rm -rf $T
[ $bad = 0 ]
