source env.sh
export V51_OOFPOOL=1 V51_SHUFGATE=1
while [ ! -f DONE4 ]; do sleep 2; done
B=../cpp/tek_hamle_v51c; A="2 512 1 1 1 0.5 Z 1 Y - 85 85 3000"
V51_GF=0.15 V51_GP=0 timeout 200 $B $A > SG_B_gF0.15.txt 2>&1
V51_GF=0 V51_GP=0.05 timeout 200 $B $A > SG_C_gP0.05.txt 2>&1
V51_GF=0.05 V51_GP=0.05 timeout 200 $B $A > SG_D_g0.05.txt 2>&1
V51_GF=0.15 V51_GP=0.15 V51_RIDGEK=1 timeout 200 $B $A > SG_E_g0.15.txt 2>&1
touch DONE5
