#!/bin/bash
# hash'siz (RSS sisirmez) olcek sureleri: 2.001 ve 5.001 icin 3 donusumlu V76<->V77 cifti
cd /home/user/name/formul_avcisi/v77_matematiksel_islem_azaltimi
for d in 2001 5001; do export DATA=/tmp/v59_$d
  for r in 1 2 3; do ./kos.sh sure${d}_76_$r tek_hamle_v76 | cut -c1-110; ./kos.sh sure${d}_77_$r tek_hamle_v77 | cut -c1-110; done; done
echo BITTI > ham/sure_bitti.txt
