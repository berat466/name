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
lim() { (ulimit -v 700000; timeout ${LIMS:-4} "$@"); }
printf '#!/bin/bash\n{ echo "-6 1"; cat; } | %s run %s\n' $PM $G2 > $T/zorla.sh; chmod +x $T/zorla.sh
# ---- 1) ondalik
if LIMS=${LIMS:-4} GEN2=$G2 GENELF=$T/zorla.sh ./dogrula_float.sh $PM > $T/f.log 2>&1; then pass; else fail "ondalik (zorlanmis mikro yolu): $(tail -3 $T/f.log | tr '\n' ' ')"; fi
# ---- 2) tamsayi ozel degerler
./tamsayi_prog.sh $T > /dev/null
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
# ---- 2b) bellek siniri ve alloc (bellek_prog.sh): ld/st dizin sinirlari, alloc sinirlari (kalan-1, kalan, kalan+1, negatif ...)
./bellek_prog.sh $T/bl $PM > /dev/null
for f in $T/bl/bellek_*.pma $T/bl/alloc_*.pma; do $PM asm $f -o $T/mem.pmb --strip > /dev/null 2>&1 && cmpone "$(basename $f .pma)" $T/mem.pmb; done
# bolme/mod tuzaklari: sifira bolme (yazmac) ayri program
for k in 1 2; do
  cp $T/dz$k.pma $T/dz.pma
  $PM asm $T/dz.pma -o $T/dz.pmb --strip > /dev/null 2>&1 && cmpone "sifira bolme $k" $T/dz.pmb
done
# ---- 3) rastgele programlar
for s in $(seq 1 $NR); do
  awk -v seed=$s -f rastgele2.awk > $T/r.pma; $PM asm $T/r.pma -o $T/r.pmb --strip > /dev/null 2>&1 && cmpone "rastgele2-$s" $T/r.pmb
done
# ---- 5) genisletilmis tablo (yalniz OP+SEM kayitlari): FMIN (sel), AIMM (im; gecici kimligi != operand yuvasi), LDK2 (lk; ayni).
#   Program listesinde standart komutlar yeni komut numarasina cevrilir (add->FMIN, addi->AIMM, ldk->LDK2); oracle: C++ (FMIN) ve pm'in standart komutla sonucu (AIMM, LDK2).
ELF2DIR=${ELF2DIR:-$D}; X=$T/x; mkdir -p $X/oz; cp -r $ELF2DIR $X/oz/elf2; cp -r $ELF2DIR/../arac $ELF2DIR/../komut $X/oz/; rm -f $X/oz/elf2/gen2.pmb
KM() { { echo "@@$1"; cat; } | (ulimit -v 900000; timeout 60 $PM run $X/oz/arac/komut.pmb); }
KM gor < $X/oz/komut/komutlar.pmk | awk '{print} /^op 54 GEI/{print "op 55 FMIN fmin d,f,f"; print "op 56 AIMM aimm d,r,i"; print "op 57 LDK2 ldk2 d,k"} /^sem GEI /{print "sem FMIN rd t1 b ; rd t2 c ; flt t3 t1 t2 ; sel t4 t3 t1 t2 ; wr a t4"; print "sem AIMM rd t3 b ; im t5 c ; alu_add t2 t3 t5 ; wr a t2"; print "sem LDK2 lk t7 b ; wr a t7"}' | KM yaz > $X/ext.pmk
if [ "$(KM dogrula < $X/ext.pmk)" = "1" ] && (cd $X/oz/elf2 && OPT=$X/ext.pmk ./uret.sh $PM > /dev/null 2>&1); then pass; else fail "genisletilmis tablodan gen2 uretilemedi"; fi
yama() {   # from to nf : program listesinde (on bolumden sonra) NF==nf ve ilk jeton from olan satirlari to yap
  awk -v from=$1 -v to=$2 -v nf=$3 'BEGIN{pre=1;lone=0} pre{print; if(NF==1){lone++; if(lone==2)pre=0} next} NF==nf && $1==from {$1=to} {print}'
}
xcmp() {   # etiket pmb(standart komutlu) from to nf beklenen_dosya(yoksa pm'in standart komutla sonucu)
  $PM liste $2 | yama $3 $4 $5 > $T/xp.l
  for mod in "" "-6 1"; do
    { [ -n "$mod" ] && echo "$mod"; cat $T/xp.l; } | lim $PM run $X/oz/elf2/gen2.pmb > $T/xp.elf 2>/dev/null || { fail "$1 [$mod]: ELF uretilemedi"; continue; }; chmod +x $T/xp.elf
    lim $T/xp.elf < $T/in > $T/xo 2> /dev/null
    if [ -n "$6" ]; then cmp -s $T/xo $6 && pass || fail "$1 [$mod]: ELF cikti beklenenden farkli: $(cmp $T/xo $6 2>&1 | head -c 70)"
    else lim $PM run $2 < $T/in > $T/xr 2>/dev/null; cmp -s $T/xo $T/xr && [ -s $T/xr ] && pass || fail "$1 [$mod]: ELF cikti pm ile farkli"; fi
  done
}
$R/oz/elf2/ondalik_prog.sh $T/fo > /dev/null 2>&1; VALS=$(sed -n 's/^VALS="\(.*\)"$/\1/p' tamsayi_prog.sh)
OZ=$(grep '^data tab' $T/fo/ozel.pma); NP=$(( $(echo "$OZ" | wc -w) - 3 ))
{ echo "memory 4096"; echo "$OZ"; cat <<EOT
func main() {
    mov i, 0
li: gei z, i, $NP
    jnz z, ld
    add a, i, tab
    ld x, a, 0
    mov j, 0
lj: gei z, j, $NP
    jnz z, nj
    add a, j, tab
    ld y, a, 0
    add r, x, y
    host puti, r
    host putc, 32
    addi j, j, 1
    jmp lj
nj: host putc, 10
    addi i, i, 1
    jmp li
ld: halt
}
entry main
EOT
} > $T/fmall.pma
cat > $T/fmin_oracle.cpp <<'EOT'
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
int main(int argc, char** argv) {
    int n = argc - 1; uint64_t v[64];
    for (int i = 0; i < n; i++) v[i] = strtoull(argv[i + 1], 0, 16);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            double x, y; memcpy(&x, &v[i], 8); memcpy(&y, &v[j], 8);
            double r = (x < y) ? x : y; int64_t b; memcpy(&b, &r, 8);
            printf("%lld ", (long long)b);
        }
        printf("\n");
    }
}
EOT
HEX=$(sed -n 's/^OZEL="\(.*\)"$/\1/p' ondalik_prog.sh | head -1 | tr ' ' '\n' | head -$NP | tr '\n' ' ')
$PM asm $T/fmall.pma -o $T/fmall.pmb --strip > /dev/null 2>&1 || fail "fmin tarama programi derlenemedi"
if g++ -O0 -o $T/fmin_oracle $T/fmin_oracle.cpp 2>/dev/null && $T/fmin_oracle $HEX > $T/fm.exp; then xcmp "FMIN tum cift desenleri" $T/fmall.pmb 3 55 4 $T/fm.exp; else fail "fmin oracle derlenemedi"; fi
# AIMM = ADDI (anlik toplama): standart komutlu programin pm sonucu ile esit
printf 'memory 4096\ndata tab = %s\nfunc main() {\n mov i, 0\nl: gei z, i, 23\n jnz z, d\n add a, i, tab\n ld x, a, 0\n addi r, x, 5\n host puti, r\n host putc, 32\n addi r, x, -77\n host puti, r\n host putc, 10\n addi i, i, 1\n jmp l\nd: halt\n}\nentry main\n' "$VALS" > $T/aimm.pma
$PM asm $T/aimm.pma -o $T/aimm.pmb --strip > /dev/null 2>&1 && xcmp "AIMM" $T/aimm.pmb 41 56 4 || fail "aimm programi derlenemedi"
# LDK2 = LDK (sabit havuzu): buyuk sabitli program
printf 'func main() {\n mov a, 1.5\n host puti, a\n host putc, 32\n mov b, 0x7ff8000000000001\n host puti, b\n host putc, 32\n mov c, -123456789012345\n host puti, c\n host putc, 10\n halt\n}\n' > $T/ldk2.pma
$PM asm $T/ldk2.pma -o $T/ldk2.pmb --strip > /dev/null 2>&1 && xcmp "LDK2" $T/ldk2.pmb 2 57 3 || fail "ldk2 programi derlenemedi"
# ---- 4) mikro yolu gercekten kullanildi
$PM liste $T/int.pmb > $T/p.l; lim $PM run $G2 < $T/p.l > $T/n.elf 2>/dev/null; lim $T/zorla.sh < $T/p.l > $T/z.elf 2>/dev/null
if cmp -s $T/n.elf $T/z.elf; then fail "zorlanmis ELF sablonlu ELF ile ayni bayt (mikro yolu kullanilmadi)"; else pass; fi
[ $(stat -c %s $T/z.elf) -gt $(stat -c %s $T/n.elf) ] && pass || fail "mikro yolu ELF'i beklenenden kucuk"
echo "mikro-islem yerel uretimi: $ok gecti, $bad basarisiz"
rm -rf $T
[ $bad = 0 ]
