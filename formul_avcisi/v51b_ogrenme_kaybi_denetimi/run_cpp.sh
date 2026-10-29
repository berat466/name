source ../v51_ek_deneyler_ham/env.sh
export V51_OOFPOOL=1 V51_GP=0
B=../cpp/tek_hamle_v52; A="2 512 1 1 1 0.5 Z 1 Y - 85 85 3000"
V49=0 V51_GF=0.15 V52_DUMP=/tmp/v52Q timeout 250 $B $A > cpp_ham/plainF_gF0.15.txt 2>&1
for k in 3 5 20 40; do V49=2 V51_GF=0.15 V52_KNN=$k timeout 250 $B $A > cpp_ham/K$k.txt 2>&1; done
V49=2 V51_GF=0.15 V52_KNN=10 timeout 250 $B $A > cpp_ham/K10.txt 2>&1
touch cpp_ham/DONE
