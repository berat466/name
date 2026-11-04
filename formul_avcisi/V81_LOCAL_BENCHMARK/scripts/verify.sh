#!/bin/bash
# verify.sh <bin_dizini> <veri_dizini> [isa: avx512|avx2] -> referans degerlerle karsilastirir (hafif: tepe RAM ~1.6 GB; V78_AUTO=0 -> deterministik)
EXE=""; [ -f "$1/th_gen_data.exe" ] && EXE=".exe"
HERE="$(cd "$(dirname "$0")" && pwd)"; source "$HERE/env_v81.sh"; BIN="$1"; DATA="$2"; ISA="${3:-avx2}"
OUT="$HERE/../results"; mkdir -p "$OUT"; LOG="$OUT/verify_${ISA}.log"
THREADS=${THREADS:-4} "$HERE/run_v81.sh" "$BIN/tek_hamle_v81_${ISA}${EXE}" "$DATA" V78_AUTO=0 V76_DEBUG=1 V78_WFHASH=1 V80_SHASH=1 V80_GHASH=1 > "$LOG" 2>&1
$PYTHON "$HERE/check_ref.py" "$LOG" "$HERE/../reference/expected_v81.json"
