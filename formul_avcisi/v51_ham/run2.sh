source env.sh
B=../cpp/tek_hamle_v51
V51_GF=0.15 V51_GP=0.15 V51_RIDGEK=1 V51_SHUF=1 timeout 200 $B 2 512 1 1 1 0.5 Z 1 Y - 85 85 3000 > Es_shuf_g0.15.txt 2>&1
for sd in 2 3; do
 A="2 512 $sd 1 1 0.5 Z 1 Y - 85 85 3000"
 V51_GF=0 V51_GP=0 timeout 200 $B $A > seed${sd}_A.txt 2>&1
 V51_GF=0.15 V51_GP=0.15 V51_RIDGEK=1 timeout 200 $B $A > seed${sd}_E0.15.txt 2>&1
 V51_GF=0.15 V51_GP=0 timeout 200 $B $A > seed${sd}_B0.15.txt 2>&1
done
V51_GF=0.05 V51_GP=0.05 V51_RIDGEK=1 V51_SHUF=1 timeout 200 ../cpp/tek_hamle_v51 2 512 1 1 1 0.5 Z 1 Y - 85 85 3000 > Es_shuf_g0.05.txt 2>&1
touch DONE2
