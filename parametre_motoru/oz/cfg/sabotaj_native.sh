#!/bin/bash
# PLAN Bolum 03 (03.009): yerel makine kodu (src/native_x64.cpp) BRX/jtab uretiminin sabotaj testi.
# Her mutant native_x64.cpp'nin kopyasina uygulanir, ayri pm uretilir; tests/jtab_*.pma programlari --native-strict altinda
# (uc promote kipi) yorumlayiciyla (cikti, cikis kodu, tuzak turu, kalan yakit) KARSILASTIRILIR. Mutant olmeli (test basarisiz olmali).
# Kullanim: sabotaj_native.sh [pm]
PM=${1:-../../pm}; D=$(cd "$(dirname "$0")" && pwd); R=$D/../..; PM=$(realpath "$PM"); cd $R || exit 1
T=$(mktemp -d); trap 'rm -rf $T' EXIT
SRC="src/main.cpp src/asm.cpp src/ikili.cpp src/vm.cpp src/dump.cpp src/sablon.cpp src/yapisal.cpp src/liste.cpp src/ifade.cpp"
mkdir -p $T/o; i=0
for f in $SRC; do g++ -std=c++17 -O1 -fno-fast-math -ffp-contract=off -w -c $f -o $T/o/$(basename $f .cpp).o & done; wait
build() { g++ -std=c++17 -O1 -fno-fast-math -ffp-contract=off -w -I src -c $1 -o $T/native.o && g++ -o $2 $T/o/*.o $T/native.o -lquadmath; }
build src/native_x64.cpp $T/pm_ok || { echo "temel derleme basarisiz"; exit 1; }
kos() {   # kos PMYOLU -> 0: tum jtab testleri gecer
  local P=$1 b f args rc_i rc_n bad=0
  for f in tests/jtab_*.pma; do
    b=${f%.pma}; args=""; [ -f $b.args ] && args=$(cat $b.args)
    $PM run $f $args --stats > $T/oi 2> $T/ei; rc_i=$?
    for prom in "" "--native-promote 0" "--native-promote 100000"; do
      { timeout 20 $P run $f $args --stats --native-strict $prom > $T/on 2> $T/en; } 2>/dev/null; rc_n=$?
      [ $rc_n = $rc_i ] && cmp -s $T/oi $T/on && [ "$(sed -n 's/^TRAP: \([A-Z0-9]*\).*/\1/p' $T/ei)" = "$(sed -n 's/^TRAP: \([A-Z0-9]*\).*/\1/p' $T/en)" ] || bad=1
    done
  done
  return $bad
}
kos $T/pm_ok && echo "temel: gecer" || { echo "HATA: mutantsiz native jtab testleri basarisiz"; exit 1; }
n=0; oldu=0; yasayan=""
mut() {   # mut ad eski yeni
  n=$((n+1)); cp src/native_x64.cpp $T/m.cpp
  python3 - "$T/m.cpp" "$3" "$4" <<'PY' || { echo "HATA: mutant deseni bulunamadi: $2"; exit 1; }
import sys
p,a,b=sys.argv[1:4]; s=open(p).read()
if s.count(a)!=1: sys.exit(1)
open(p,'w').write(s.replace(a,b))
PY
  cp $T/m.cpp $T/m_src.cpp; cp -r src/*.h $T/ 2>/dev/null
  g++ -std=c++17 -O1 -fno-fast-math -ffp-contract=off -w -I src -c $T/m_src.cpp -o $T/native.o 2>/dev/null && g++ -o $T/pm_m $T/o/*.o $T/native.o -lquadmath
  if kos $T/pm_m; then yasayan="$yasayan\n  $2"; else oldu=$((oldu+1)); fi
}
mut isaretli_karsilastirma "a.jcc(CC_B, ok)" "a.jcc(CC_B, ok)" "a.jcc(CC_L, ok)"
mut varsayilan_yanlis "mov_imm32(RAX, int32_t(n))" "a.mov_imm32(RAX, int32_t(n)); a.bind(ok);" "a.mov_imm32(RAX, 0); a.bind(ok);"
mut kapsam_dar "i <= n" "for (long i = 0; i <= n; i++) a.rel32" "for (long i = 0; i < n; i++) a.rel32"
mut yakit_ileri "checkpoint" "long n = long(tv[m.b].imm);" "long n = long(tv[m.b].imm); checkpoint();"
# (brx hedeflerinin tgt[] isaretlenmesi savunma amaclidir: dogrulayici merdiven girdilerini ileri jmp zorunlu kildigindan kopru/birlestirme (fuse) yan etkisi yoktur -> esdeger mutant, listelenmez)
mut tablo_kaydirma "tablo +4" "a.u8(0x4C); a.u8(0x81); a.u8(0x04);" "a.u8(0x4C); a.u8(0x81); a.u8(0x00);"
mut sabit_32bit "64 bit cmp" "a.load_to(RAX, tv[m.a]); a.alu_ri(7, RAX, int32_t(n));" "a.load_to(RAX, tv[m.a]); a.u8(0x89); a.u8(0xC0); a.alu_ri(7, RAX, int32_t(n));"
echo "native jtab sabotaj: $oldu/$n mutant yakalandi"
[ $oldu = $n ] || { printf "YASAYAN MUTANTLAR:$yasayan\n"; exit 1; }
