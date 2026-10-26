#!/bin/bash
# tum_adimlar.sh [cift_sayisi=5] : derle -> veri uret -> dogrula -> hiz olc (V80 <-> V81). MSYS2 UCRT64 veya Linux bash. Ek talimat gerekmez.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"; ROOT="$HERE/.."; N=${1:-5}; B="$ROOT/build"; THREADS=${THREADS:-4}; export THREADS
cmake -S "$ROOT" -B "$B" -G Ninja -DCMAKE_BUILD_TYPE=Release -DTH_ISA=${TH_ISA:-auto}
cmake --build "$B"
BIN="$B/bin"; EXT=""; [ -f "$BIN/th_gen_data.exe" ] && EXT=".exe"
ISA=$("$HERE/pick_isa.sh" "$BIN"); echo "SECILEN ISA: $ISA"
DATA="$ROOT/data/v59_10001"; "$HERE/make_data.sh" "$BIN" "$DATA"
mkdir -p "$ROOT/results"
"$HERE/verify.sh" "$BIN" "$DATA" "$ISA" | tee "$ROOT/results/verify_ozet.txt" || true
"$HERE/bench.sh" "$BIN" "$DATA" "$ISA" "$N"
echo "BITTI. Sonuclar: $ROOT/results (verify_${ISA}.log, bench_${ISA}_ozet.md/json)"
