source ../v51_ek_deneyler_ham/env.sh
export V51_OOFPOOL=1 V51_GP=0 V51_GF=0.15
B=../cpp/tek_hamle_v53; A="2 512 1 1 1 0.5 Z 1 Y"
for lc in 25 50 85 170; do for lf in 1000 3000 9000; do
  $B $A /tmp/v53A_yeni $lc $lc $lf > ham/gridA_${lc}_${lf}.txt 2>&1; done; done
touch ham/gridA_DONE
