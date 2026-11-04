#!/bin/bash
# bench.sh <bin_dizini> <veri_dizini> <isa> [cift_sayisi=5] : V80 <-> V81 donusumlu A/B, ayni veri/bayrak/is parcacigi
EXE=""; [ -f "$1/th_gen_data.exe" ] && EXE=".exe"
HERE="$(cd "$(dirname "$0")" && pwd)"; source "$HERE/env_v81.sh"; BIN="$1"; DATA="$2"; ISA="$3"; N=${4:-5}; OUT="$HERE/../results"; mkdir -p "$OUT"
for r in $(seq 1 $N); do for v in 80 81; do
  THREADS=${THREADS:-4} "$HERE/run_v81.sh" "$BIN/tek_hamle_v${v}_${ISA}${EXE}" "$DATA" > "$OUT/bench_v${v}_${ISA}_$r.log" 2>&1
  echo "v$v pair $r: wall=$(grep -oE 'TH_WALL_SECONDS=[0-9.]+' "$OUT/bench_v${v}_${ISA}_$r.log" | cut -d= -f2) model=$(grep -oE 'MODEL_HAZIR\|[0-9.]+' "$OUT/bench_v${v}_${ISA}_$r.log" | cut -d'|' -f2) $(grep -oE 'TEST CE=[0-9.]+ PPL=[0-9.]+' "$OUT/bench_v${v}_${ISA}_$r.log" | head -1)"
done; done | tee "$OUT/bench_${ISA}_ozet.txt"
$PYTHON "$HERE/bench_ozet.py" "$OUT" "$ISA" "$N"
