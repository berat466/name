#!/bin/bash
# PLAN Bolum 04 (04.010/04.030): yerel makine kodu (src/native_x64.cpp) CALLI (dolayli cagri) ve TCALL (kuyruk cagrisi) uretiminin sabotaj testi.
# Her mutant native_x64.cpp'nin kopyasina uygulanir, ayri pm uretilir; tests/calli_*.pma ve tests/tcall_*.pma programlari --native-strict altinda
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
  for f in tests/calli_*.pma tests/tcall_*.pma tests/ek/*.pma; do
    b=${f%.pma}; args=""; [ -f $b.args ] && args=$(cat $b.args)
    $PM run $f $args --stats > $T/oi 2> $T/ei; rc_i=$?
    for prom in "" "--native-promote 0" "--native-promote 100000"; do
      { timeout 20 $P run $f $args --stats --native-strict $prom > $T/on 2> $T/en; } 2>/dev/null; rc_n=$?
      [ $rc_n = $rc_i ] && cmp -s $T/oi $T/on && [ "$(sed -n 's/^TRAP: \([A-Z0-9]*\).*/\1/p' $T/ei)" = "$(sed -n 's/^TRAP: \([A-Z0-9]*\).*/\1/p' $T/en)" ] || bad=1
    done
  done
  return $bad
}
kos $T/pm_ok && echo "temel: gecer" || { echo "HATA: mutantsiz native calli/tcall testleri basarisiz"; exit 1; }
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
mut calli_aralik_denetimi_yok "calli indeks siniri" "a.alu_ri(7, RAX, int32_t(P.funcs.size())); a.jcc(CC_AE, lOob);" "a.alu_ri(7, RAX, int32_t(P.funcs.size()));"
mut calli_arity_yanlis "calli prototip parametre sayisi" "a.mov_imm32(RDX, int32_t(pr.nparams));" "a.mov_imm32(RDX, int32_t(pr.nparams) + 1);"
mut calli_stub_boyu "calli stub kaydirmasi" "a.shift_imm(4, RAX, 7);" "a.shift_imm(4, RAX, 6);"
mut calli_pencere_ileri "calli pencere ileri" $'a.alu_ri(0, RBX, 8 * base);\n            a.call_rax();' $'a.alu_ri(0, RBX, 8 * base + 8);\n            a.call_rax();'
mut calli_pencere_geri "calli pencere geri" $'a.call_rax();\n            a.alu_ri(5, RBX, 8 * base);' $'a.call_rax();\n            a.alu_ri(5, RBX, 8 * base + 8);'
mut calli_derinlik_sayaci "calli derinlik sayaci" $'a.alu_ri(5, RBX, 8 * base);\n            a.incdec(1, R12);\n            break;\n          }\n          case MK_TCALLM:' $'a.alu_ri(5, RBX, 8 * base);\n            break;\n          }\n          case MK_TCALLM:'
mut tcall_yigin_siniri "tcall yigin siniri" $'a.alu_ri(7, RAX, int32_t(g.nregs)); a.jcc(CC_B, lStack);\n            if (base > 0 && g.nparams > 0)' $'a.alu_ri(7, RAX, int32_t(g.nregs));\n            if (base > 0 && g.nparams > 0)'
mut tcall_arguman_tasima "tcall arguman tasima" "a.mov_imm32(RCX, int32_t(g.nparams)); a.rep_movsq(); }" "a.mov_imm32(RCX, int32_t(g.nparams)); }"
mut tcall_yerel_sifirlama "tcall yerel sifirlama" $'if (!P.zero_free && g.nregs > g.nparams) { a.lea(RDI, RBX, 8 * int(g.nparams)); a.mov_imm32(RCX, int32_t(g.nregs - g.nparams)); a.xor_eax(); a.rep_stosq(); }\n            emit_unwind();' $'if (0 && !P.zero_free && g.nregs > g.nparams) { a.lea(RDI, RBX, 8 * int(g.nparams)); a.mov_imm32(RCX, int32_t(g.nregs - g.nparams)); a.xor_eax(); a.rep_stosq(); }\n            emit_unwind();'
mut tcall_yazmac_geri_yukleme "tcall unwind" $'emit_unwind();\n            a.jmp(fl[size_t(fld(in, m.a))]);' $'a.jmp(fl[size_t(fld(in, m.a))]);'
echo "native calli/tcall sabotaj: $oldu/$n mutant yakalandi"
[ $oldu = $n ] || { printf "YASAYAN MUTANTLAR:$yasayan\n"; exit 1; }
