#!/bin/bash
# Tamsayi sinir degerleri programi (dogrula_mikro.sh ve oz/sayisal/dogrula_svm.sh ortak): DIZIN altina int.pma, dz1.pma, dz2.pma yazar.
T=${1:?dizin}; mkdir -p $T
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
for k in 1 2; do
  { echo "func main() {"; echo " ldi a, 10"; echo " ldi b, 0"; [ $k = 1 ] && echo " div c, a, b" || echo " mod c, a, b"; echo " host puti, c"; echo " halt"; echo "}"; } > $T/dz$k.pma
done
