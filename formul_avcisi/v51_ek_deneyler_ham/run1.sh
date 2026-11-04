source env.sh
B=../cpp/tek_hamle_v51b; A="2 512 1 1 1 0.5 Z 1 Y - 85 85 3000"
for oof in 0 1; do for rk in 0 1; do V51_OOFPOOL=$oof V51_RIDGEK=$rk V51_GF=0 V51_GP=0 timeout 200 $B $A > L_oof${oof}_rk${rk}.txt 2>&1; done; done
touch DONE1
