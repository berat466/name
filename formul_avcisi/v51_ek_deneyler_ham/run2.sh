source env.sh
export V51_OOFPOOL=1
B=../cpp/tek_hamle_v51b; A="2 512 1 1 1 0.5 Z 1 Y - 85 85 3000"
# 2) karisik cekirdek kontrolleri (temiz havuz tabani), sabit katsayilar
V51_GF=0.15 V51_GP=0 V51_SHUF=1 timeout 200 $B $A > S_B_gF0.15.txt 2>&1
V51_GF=0 V51_GP=0.05 V51_SHUF=1 timeout 200 $B $A > S_C_gP0.05.txt 2>&1
V51_GF=0.05 V51_GP=0.05 V51_SHUF=1 timeout 200 $B $A > S_D_g0.05.txt 2>&1
V51_GF=0.15 V51_GP=0.15 V51_RIDGEK=1 V51_SHUF=1 timeout 200 $B $A > S_E_g0.15.txt 2>&1
V51_GF=0.15 V51_GP=0 timeout 200 $B $A > R_B_gF0.15.txt 2>&1
V51_GF=0 V51_GP=0.05 timeout 200 $B $A > R_C_gP0.05.txt 2>&1
V51_GF=0.05 V51_GP=0.05 timeout 200 $B $A > R_D_g0.05.txt 2>&1
V51_GF=0.15 V51_GP=0.15 V51_RIDGEK=1 timeout 200 $B $A > R_E_g0.15.txt 2>&1
touch DONE2
# 3) bagimsiz kapi izgarasi (rho=0, alpha satiri)
for gf in 0 0.05 0.15 0.30; do for gp in 0 0.05 0.15 0.30; do
  [ "$gf" = "0" ] && [ "$gp" = "0" ] && continue
  V51_GF=$gf V51_GP=$gp timeout 200 $B $A > G_gF${gf}_gP${gp}.txt 2>&1
done; done
touch DONE3
# 4) eslem tohumlari (E, g=0.15)
for ms in 1 2 3; do V51_MSEED=$ms V51_GF=0.15 V51_GP=0.15 V51_RIDGEK=1 timeout 200 $B $A > M_E_mseed${ms}.txt 2>&1; done
touch DONE4
