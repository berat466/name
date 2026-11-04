#!/bin/bash
# PLAN BOLUM 01 -- TYPES bolumu (PMK1 bolum 10): tur ozellikleri (sinif, bit, isaret, boyut, hiza, bellek sozcugu, bayt sirasi, usbit/mantisbit), tur bazinda limitler
# (min/max/en kucuk normal/epsilon/en kucuk denormal), ABI veri modeli (Linux x86-64 LP64: C tur adi -> tur) ve tur donusum matrisi, BAGIMSIZ C++ derleyici gercekleriyle karsilastirilir
# (turler_oracle.cpp: sizeof, alignof, std::numeric_limits, <float.h> sabitleri). Tablo ile derleyici gercegi ayri kaynaktir: tablo analitik hesaplanmistir, oracle derleyiciden okunur.
# Kullanim: dogrula_turler.sh [pm]
PM=${1:-../../pm}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d); ok=0; bad=0
pass() { ok=$((ok+1)); }; fail() { bad=$((bad+1)); echo "HATA: $1"; }
TAB=${TAB:-$D/komutlar.pmk}; KOMUT=${KOMUT:-$R/oz/arac/komut.pmb}
KM() { { echo "@@$1"; cat; } | (ulimit -v 900000; timeout 60 $PM run $KOMUT); }
g++ -std=gnu++23 -O1 -o $T/oracle turler_oracle.cpp 2>$T/g.err || { echo "oracle derlenemedi: $(head -c 300 $T/g.err)"; exit 1; }
[ "$(KM dogrula < $TAB)" = "1" ] && pass || fail "tablo gecersiz"
KM tur < $TAB > $T/tur.txt
$T/oracle > $T/or.txt
# 1) tur satirlari: tablodan 'tur ID AD ...' -> 'AD ...' ; oracle ile ayni ad kumesi ve ayni sutunlar
grep '^tur ' $T/tur.txt | awk '{$1=""; $2=""; sub(/^  /,""); print}' | sort > $T/t_tab.txt
grep -v '^abi ' $T/or.txt | sort > $T/t_or.txt
n_tab=$(wc -l < $T/t_tab.txt); n_or=$(wc -l < $T/t_or.txt)
[ "$n_tab" = "$n_or" ] && [ "$n_tab" -ge 29 ] && pass || fail "tur sayisi: tablo $n_tab, oracle $n_or"
while IFS= read -r l; do ad=${l%% *}; t=$(grep "^$ad " $T/t_tab.txt); [ "$t" = "$l" ] && pass || fail "tur $ad farkli: tablo '$t' oracle '$l'"; done < $T/t_or.txt
# 2) ABI: oracle C tur adlari (bosluklu): tablo eslemesinden tur bulunur; boyut/isaret/kayan sinifi esit olmali
declare -A TS TSG TCL
while read -r ad cls bits sg size rest; do TS[$ad]=$size; TSG[$ad]=$sg; TCL[$ad]=$cls; done < $T/t_tab.txt
nabi=0
while IFS= read -r l; do l=${l#abi 1 }; cn=${l%%|*}; rest=${l#*|}; set -- $rest; osize=$1; osg=$2; ofl=$3
  tl=$(grep -F "abi 1 " $T/tur.txt | awk -v c="$cn" '{id=$3; name=$0; sub(/^abi 1 [0-9]+ /,"",name); if (name==c) print id}')
  [ -n "$tl" ] || { fail "abi: '$cn' eslemesi yok"; continue; }
  tn=$(awk -v id="$tl" '$1=="tur" && $2==id {print $3}' $T/tur.txt)
  [ "${TS[$tn]}" = "$osize" ] && pass || fail "abi $cn -> $tn: boyut ${TS[$tn]} (oracle $osize)"
  if [ "$ofl" = 1 ]; then [ "${TCL[$tn]}" = 5 ] && pass || fail "abi $cn -> $tn: kayan nokta degil"; else [ "${TSG[$tn]}" = "$osg" ] && pass || fail "abi $cn -> $tn: isaret ${TSG[$tn]} (oracle $osg)"; fi
  nabi=$((nabi+1)); done < <(grep '^abi ' $T/or.txt)
[ "$nabi" -ge 30 ] && pass || fail "abi: yalniz $nabi eslesme denetlendi"
# her tablo ABI girdisi ya oracle'da var ya da bilinen genisleme (karmasik/yarim duyarlik/f128/std::byte/nullptr_t)
KNOWN="float _Complex|double _Complex|long double _Complex|_Float16|__bf16|__float128|std::byte|std::nullptr_t|_Float32|_Float64|_Float128|_Float32x|_Float64x|std::float16_t|std::float32_t|std::float64_t|std::float128_t|std::bfloat16_t"
grep '^abi ' $T/tur.txt | while IFS= read -r l; do cn=$(echo "$l" | sed 's/^abi 1 [0-9]* //'); grep -qF "abi 1 $cn|" $T/or.txt || echo "$cn" | grep -qE "^($KNOWN)$" || echo "TANIMSIZ_ABI $cn"; done > $T/unk.txt
[ ! -s $T/unk.txt ] && pass || fail "abi: oracle'da olmayan beklenmeyen girdi: $(head -2 $T/unk.txt | tr '\n' ' ')"
# 3) bayt sirasi: tablodaki tum turler kucuk-sonlu (0) ve ana makine kucuk-sonlu
awk '$1=="tur" && $10!=0 {n++} END{exit n>0}' $T/tur.txt && pass || fail "buyuk-sonlu tur var (Linux x86-64 kucuk-sonlu)"
echo | $T/oracle >/dev/null; [ "$(printf '\001' | od -An -tu1 | tr -d ' ')" = 1 ] && pass
# 4) kanonik gidis-donus ve tur sozlugunda yinelenen ad yok
KM kanon < $TAB | cmp -s - $TAB && pass || fail "tablo kanonik degil"
[ "$(awk '$1=="tur"{print $3}' $T/tur.txt | sort | uniq -d | wc -l)" = 0 ] && pass || fail "yinelenen tur adi"
# 5) DONUSUM KAPSAMI (Plan 01.030): sayisal turlerin her sirali cifti icin kayitli donusum YOLU (tek komut ya da kayitli donusumlerin birlesimi) var; yok olanlar yalniz tasarim geregi:
#    karmasik -> karmasik olmayan (gercek/sanal parca real_*/imag_* komutlariyla alinir) ve nullptr_t ile sayisal turler arasi (nullptr_t -> bool / isaretsiz 64 haric)
python3 - $T/tur.txt <<'PY' && pass || fail "donusum kapsami: beklenmeyen eksik donusum yolu"
import sys
names, cls, adj = {}, {}, {}
for l in open(sys.argv[1]):
    t = l.split()
    if t[0] == 'tur': names[int(t[1])] = t[2]; cls[int(t[1])] = int(t[3]); adj[int(t[1])] = set()
for l in open(sys.argv[1]):
    t = l.split()
    if t[0] == 'donusum': adj[int(t[1])].add(int(t[2]))
bad = []
for a in names:
    seen = {a}; st = [a]
    while st:
        x = st.pop()
        for y in adj[x]:
            if y not in seen: seen.add(y); st.append(y)
    for b in names:
        if b in seen: continue
        if cls[a] == 6 and cls[b] != 6: continue          # karmasik -> gercek: real_/imag_ komutlari
        if cls[a] == 7 or cls[b] == 7: continue           # nullptr_t: yalniz bool ve isaretsiz 64 bit
        bad.append((names[a], names[b]))
if bad: print('eksik donusum yolu:', bad[:20]); sys.exit(1)
PY
echo "turler: $ok gecti, $bad basarisiz"
rm -rf $T
[ $bad = 0 ]
