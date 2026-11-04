export V33_BLAS=1 V36_C0=1 V37_BIAS=1 V39=1 V39_BETA=0.9 V40_STAGES=1 V40_W=512
B=../cpp/tek_hamle_v42
run() { # seed lc lf mode
  tag=s$1_c$2_f$3_$4
  if [ "$4" = dense ]; then export V41_GRP=0; else export V41_GRP=1; fi
  V33_DUMP=/tmp/d42_$tag timeout 100 $B 2 512 $1 1 1 0.5 Z 1 Y - $2 $2 $3 > log_$tag.txt 2>&1
}
for lc in 10 20 100; do for lf in 300 3000 30000; do for m in dense grp; do run 1 $lc $lf $m; done; done; done
for pair in "10 300" "20 3000" "100 30000"; do set -- $pair; for m in dense grp; do run 2 $1 $2 $m; done; done
touch EQDONE
for r in 1 2 3; do for m in dense grp; do run 1 100 3000 $m; cp log_s1_c100_f3000_$m.txt time_${m}_r$r.txt; done; done
touch DONE
