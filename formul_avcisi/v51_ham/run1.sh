source env.sh
B=../cpp/tek_hamle_v51; A="2 512 1 1 1 0.5 Z 1 Y - 85 85 3000"
for g in 0.05 0.15; do V51_GF=$g V51_GP=0 timeout 200 $B $A > B_gF$g.txt 2>&1; done
for g in 0.05 0.15; do V51_GF=0 V51_GP=$g timeout 200 $B $A > C_gP$g.txt 2>&1; done
for g in 0.05 0.15; do V51_GF=$g V51_GP=$g timeout 200 $B $A > D_gF${g}_gP$g.txt 2>&1; done
for g in 0.05 0.15; do V51_GF=$g V51_GP=$g V51_RIDGEK=1 timeout 200 $B $A > E_all_g$g.txt 2>&1; done
touch DONE1
