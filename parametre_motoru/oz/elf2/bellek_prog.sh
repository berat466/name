#!/bin/bash
# Bellek siniri ve alloc programlari (dogrula_mikro.sh ve oz/sayisal/dogrula_svm.sh ortak): DIZIN altina bellek_*.pma ve alloc_*.pma yazar. Kullanim: bellek_prog.sh dizin pm
T=${1:?dizin}; PM=${2:?pm}; mkdir -p $T
n=0
for kind in ld st; do for ad in 0 4095 4096 4097 -1 9223372036854775807 -9223372036854775808 2147483648 4294967296; do for off in 0 1 -1 100; do
  n=$((n+1))
  { echo "memory 4096"; echo "data tab = $ad 77"
    echo "func main() {"; echo " mov z, 0"; echo " add p, z, tab"; echo " ld x, p, 0"; echo " ld w, p, 1"
    if [ $kind = ld ]; then echo " ld v, x, $off"; echo " host puti, v"; else echo " st w, x, $off"; echo " ld v, x, $off"; echo " host puti, v"; fi
    echo " halt"; echo "}"; echo "entry main"; } > $T/bellek_${kind}_$n.pma
done; done; done
printf 'memory 4096\nfunc main() {\n alloc r, 0\n host puti, r\n halt\n}\nentry main\n' > $T/al0.pma
$PM asm $T/al0.pma -o $T/al0.pmb --strip > /dev/null 2>&1; HEAP0=$($PM run $T/al0.pmb 2>/dev/null | tr -d '\n ' ); ROOM=$((4096 - ${HEAP0:-0}))
for k in 0 1 2 8 $((ROOM-1)) $ROOM $((ROOM+1)) $((ROOM-2)) -1 -9223372036854775808 9223372036854775807; do
  printf 'memory 4096\ndata tab = %s\nfunc main() {\n mov z, 0\n add p, z, tab\n ld n, p, 0\n alloc r, n\n host puti, r\n host putc, 32\n alloc q, 1\n host puti, q\n st n, r, 0\n halt\n}\nentry main\n' $k > $T/alloc_$k.pma
done
echo $ROOM
