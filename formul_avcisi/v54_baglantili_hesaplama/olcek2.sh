#!/bin/bash
# D asamasi: is parcacigi sayisi + OpenMP/OpenBLAS yerlesim varyantlari (3 tur, ara ara)
cd "$(dirname "$0")"; BIN=${1:-../cpp/tek_hamle_v54}; CFG=${2:-full}; OUT=${3:-ham/olcek2}; mkdir -p $OUT; source env_$CFG.sh
run() { name=$1; shift; for r in 1 2 3; do env "$@" V53H_LEAN=1 V53H_FAST=1 V53H_TEST=0 python3 calistir.py $OUT/${CFG}_${name}_$r $BIN $ARGS; done; }
env V53H_LEAN=1 V53H_FAST=1 V53H_TEST=0 $BIN $ARGS >/dev/null 2>&1
for T in 1 2 3 4 8 16; do run T$T OMP_NUM_THREADS=$T OPENBLAS_NUM_THREADS=$T; done
run omp4_ob1 OMP_NUM_THREADS=4 OPENBLAS_NUM_THREADS=1
run omp4_ob2 OMP_NUM_THREADS=4 OPENBLAS_NUM_THREADS=2
run omp2_ob4 OMP_NUM_THREADS=2 OPENBLAS_NUM_THREADS=4
run bind_close OMP_NUM_THREADS=4 OPENBLAS_NUM_THREADS=4 OMP_PROC_BIND=close OMP_PLACES=cores
run bind_spread OMP_NUM_THREADS=4 OPENBLAS_NUM_THREADS=4 OMP_PROC_BIND=spread OMP_PLACES=cores
run passive OMP_NUM_THREADS=4 OPENBLAS_NUM_THREADS=4 OMP_WAIT_POLICY=passive
run active OMP_NUM_THREADS=4 OPENBLAS_NUM_THREADS=4 OMP_WAIT_POLICY=active
touch $OUT/DONE
