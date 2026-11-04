#!/bin/bash
# her V77 degisikligini tek tek kapatarak (ayni ikili, ortam degiskeni) 3 tur donusumlu olcum
cd /home/user/name/formul_avcisi/v77_matematiksel_islem_azaltimi
for r in 1 2 3; do
  ./kos.sh ab_taban_$r tek_hamle_v77 | cut -c1-70
  for v in SPEK DIV S2 RHS2 COLS; do ./kos.sh ab_${v}0_$r tek_hamle_v77 V77_${v}=0 | cut -c1-70; done
  ./kos.sh ab_TSYM1_$r tek_hamle_v77 V77_TSYM=1 | cut -c1-70
done
echo BITTI > ham/abl_bitti.txt
