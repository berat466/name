#!/bin/bash
cd /home/user/name/formul_avcisi/v78_ilerleyen_hesap
for r in 1 2 3; do
  ./kos.sh ab_taban_$r tek_hamle_v78 | cut -c1-70
  ./kos.sh ab_LIN0_$r tek_hamle_v78 V78_LIN=0 | cut -c1-70
  ./kos.sh ab_S1V0_$r tek_hamle_v78 V78_S1V=0 | cut -c1-70
  ./kos.sh ab_FZ1_$r tek_hamle_v78 V78_FZ=1 | cut -c1-70
  ./kos.sh ab_TZ1_$r tek_hamle_v78 V78_TZ=1 | cut -c1-70
  ./kos.sh ab_V77_$r tek_hamle_v77 | cut -c1-70
done
echo BITTI > ham/abl_bitti.txt
