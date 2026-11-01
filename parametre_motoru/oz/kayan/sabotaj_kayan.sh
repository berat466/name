#!/bin/bash
# dogrula_kayan.sh'in yakaladigini kanitlar: genis kayan nokta / karmasik komutlarinin SEM kayitlarinda (tablo) rastgele ama tohumlu MUTANTLAR uretir (islem degistirme, karsilastirma degistirme, sabit +-1,
# sel kollarini takas); kendi kopya agacinda gen2'yi yeniden uretir ve dogrula_kayan.sh'in BASARISIZ olmasini bekler. KACAN mutantlar listelenir (denk mutant olabilir: elle incelenir).
# Kullanim: sabotaj_kayan.sh [pm] [paralel] [mutant_sayisi_komut_basina]   (SADECE: komut adi deseni)
PM=${1:-../../pm}; JOBS=${2:-4}; PERK=${3:-4}; D=$(cd "$(dirname "$0")" && pwd); PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d); . $D/../slot.sh; [ -n "$HIZLI_SLOT" ] \&\& JOBS=1000
taban() { rm -rf $1; mkdir -p $1; cp -r $R/oz $1/oz; }
KM() { { echo "@@$1"; cat; } | $PM run $R/oz/arac/komut.pmb; }
KM gor < $R/oz/komut/komutlar.pmk > $T/gor.txt
ilgili() {   # komut adi -> ilk asamada kosulacak test gruplari (dogrula_kayan.sh ONLY duzenli ifadesi); kacan mutant yine TAM kosuyla sinanir
  case "$1" in
    ADD_C*|SUB_C*|MUL_C*|DIV_C*|EQ_C*|NE_C*|TOBOOL_C*) echo "cbin|cun";;
    CV_C*|CV_F80_C*|CV_F64_C*) echo "karmasik don";;
    SQRT_*) echo "sqrt|un";;
    ADD_F*|SUB_F*|MUL_F*|DIV_F*|EQ_F*|NE_F*|LT_F*|LE_F*) echo "bin";;
    CV_F64_F*) echo " cd";;
    CV_I64_F*|CV_U64_F*|CV_I128_F*|CV_U128_F*) echo " ci|dar kayan";;
    *) echo " un|bin";;
  esac
}
sonuc() { O=$(ilgili "$3"); if $D/../ilkhata.sh $T/log_$2 bash -c "cd $1/oz/kayan && ONLY='$O' LIMS=200 timeout 600 ./dogrula_kayan.sh $PM 40 3" && $D/../ilkhata.sh $T/log_$2 bash -c "cd $1/oz/kayan && LIMS=200 timeout 600 ./dogrula_kayan.sh $PM 70 4"; then echo "KACTI: $2"; else echo "yakalandi: $2"; fi; }
mut() {   # etiket komut satir_no_degisimi_python
  ( w=$T/w_$1; taban $w
    python3 - "$2" "$3" "$T/gor.txt" <<'PY' | KM yaz > $w/oz/komut/komutlar.pmk || { echo "SABOTAJ HATASI: $1"; exit 3; }
import sys, random
op, seed, path = sys.argv[1], int(sys.argv[2]), sys.argv[3]
rr = random.Random(seed); out = []
SW = {'alu_add': 'alu_sub', 'alu_sub': 'alu_add', 'cmp_lt': 'cmp_le', 'cmp_le': 'cmp_lt', 'cmp_gt': 'cmp_ge', 'cmp_ge': 'cmp_gt', 'cmp_ltu': 'cmp_leu', 'cmp_leu': 'cmp_ltu', 'alu_and': 'alu_or', 'alu_or': 'alu_and',
      'alu_xor': 'alu_or', 'alu_shl': 'alu_shr', 'alu_shr': 'alu_shl', 'cmp_eq': 'cmp_ne', 'cmp_ne': 'cmp_eq', 'fadd': 'fsub', 'fsub': 'fadd', 'fmul': 'fdiv', 'fdiv': 'fmul', 'feq': 'fle'}
done = False
for l in open(path).read().split('\n'):
    if l.startswith('sem ' + op + ' '):
        ms = l[len('sem ' + op + ' '):].split(' ; ')
        cand = []
        for i, m in enumerate(ms):
            t = m.split()
            if t[0] in SW: cand.append((i, 'sw'))
            elif t[0] == 'lit' and t[2].lstrip('-').isdigit() and int(t[2]) not in (0, 1): cand.append((i, 'lit'))
            elif t[0] == 'sel': cand.append((i, 'sel'))
        i, k = rr.choice(cand); t = ms[i].split()
        if k == 'sw': t[0] = SW[t[0]]
        elif k == 'lit': t[2] = str(int(t[2]) + rr.choice([-1, 1]))
        else: t[3], t[4] = t[4], t[3]
        ms[i] = ' '.join(t); l = 'sem ' + op + ' ' + ' ; '.join(ms); done = True
    out.append(l)
if not done: sys.exit(5)
print('\n'.join(out))
PY
    (cd $w/oz/elf2 && ./uret.sh $PM > /dev/null 2>&1) || { echo "mutant $1: gen2 uretilemedi"; exit 4; }
    sonuc $w $1 $2; rm -rf $w ) > $T/sonuc_$1 2>&1; cat $T/sonuc_$1
}
bg() { while [ "$(jobs -rp | wc -l)" -ge "$JOBS" ]; do wait -n 2>/dev/null || sleep 0.2; done; ( slot_al; "$@"; slot_birak ) & }
for op in ADD_F80 SUB_F128 MUL_F80 MUL_F128 DIV_F80 DIV_F128 SQRT_F80 SQRT_F128 LT_F80 LE_F128 EQ_F80 NE_F128 TOBOOL_F80 CV_F64_F80 CV_F128_F64 CV_F80_F128 CV_F128_F80 CV_F80_F32 CV_F128_BF16 CV_I64_F80 CV_U64_F128 CV_I128_F80 CV_U128_F128 CV_F80_I64 CV_F128_U64 CV_F80_I128 CV_F128_U128 CV_F80_I16 \
          ADD_C64 MUL_C64 DIV_C64 ADD_C128 MUL_C128 DIV_C128 ADD_C160 MUL_C160 DIV_C160 EQ_C160 TOBOOL_C128 CV_C160_C64 CV_C128_C160 CV_F80_C128; do
  [ -z "$SADECE" ] || [[ "$op" =~ $SADECE ]] || continue
  for k in $(seq 1 $PERK); do bg mut ${op}_$k $op $((k*7919+${#op}*31)); done
done
wait
# ozet: kacanlar sabotaj_kayan_denk.txt'deki gerekceli denk mutantlarla eslesmeli; baska kacan / uygulanamayan varsa basarisiz
cat $T/sonuc_* > $T/hepsi.txt
Y=$(grep -c '^yakalandi' $T/hepsi.txt); K=$(grep '^KACTI' $T/hepsi.txt | sed 's/KACTI: //' | sort); H=$(grep -c 'SABOTAJ HATASI\|uretilemedi' $T/hepsi.txt)
BEK=0; DENK=0; for k in $K; do if grep -q "^$k	" $D/sabotaj_kayan_denk.txt; then DENK=$((DENK+1)); else BEK=$((BEK+1)); echo "BEKLENMEYEN KACAN: $k"; fi; done
echo "sabotaj(kayan): $Y yakalandi, $((DENK+BEK)) kacti ($DENK gerekceli denk, $BEK beklenmeyen), $H uygulanamadi/uretilemedi"
rm -rf "$T"; [ $BEK = 0 ] && [ $H = 0 ]
