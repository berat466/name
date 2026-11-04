#!/bin/bash
cd /home/user/name/formul_avcisi/cpp
M=/tmp/v6log/manifest.tsv; START=$(date +%s); BUDGET=1020   # 17 dk
run() { # isim mod H rep forcefb maxit
  now=$(date +%s); if [ $((now-START)) -gt $BUDGET ]; then echo "butce doldu: $1 atlandi" >> /tmp/v6log/atlanan.txt; return; fi
  ./calistir_olc.py /tmp/v6log/$1.txt $M -- ./tek_hamle_v6 $2 $3 1 0.125 1 5 $5 $6
}
MODES="v4f32 v4f64 v4f32c256 v6dw_ref v6dw_pcg v6chol"
for H in 256 512 1024; do for md in $MODES; do run ${md}_H${H}_rep1 $md $H 1 0 60; done; done
run v6chol_H256_zorlafb3 v6chol 256 1 3 60
run v6chol_H512_zorlafb3 v6chol 512 1 3 60
for H in 256 512 1024; do run v6dw_pcg_H${H}_maxit300 v6dw_pcg $H 1 0 300; done
for H in 256 512 1024; do for md in $MODES; do run ${md}_H${H}_rep2 $md $H 2 0 60; done; done
for H in 256 512; do for md in $MODES; do run ${md}_H${H}_rep3 $md $H 3 0 60; done; done
echo "bitti $(( $(date +%s)-START )) sn" > /tmp/v6log/DONE
