source ../v51_ek_deneyler_ham/env.sh
export V51_OOFPOOL=1 V51_GP=0
B=../cpp/tek_hamle_v53; A="2 512 1 1 1 0.5 Z 1 Y"
V51_GF=0.15 $B $A /tmp/v53A_yeni 85 85 27000 > ham/gridA2_gF0.15_lf27000.txt 2>&1
for g in 0.10 0.20 0.25; do V51_GF=$g $B $A /tmp/v53A_yeni 85 85 9000 > ham/gridA2_gF${g}_lf9000.txt 2>&1; done
touch ham/gridA2_DONE
