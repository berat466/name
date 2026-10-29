#!/bin/bash
# V76 <-> V77 donusumlu (5 cift); tum program, ayni veri/ayar/4 is parcacigi. N=cift sayisi (varsayilan 5)
cd /home/user/name/formul_avcisi/v77_matematiksel_islem_azaltimi
N=${N:-5}
for r in $(seq 1 $N); do ./kos.sh A76_$r tek_hamle_v76 | cut -c1-120; ./kos.sh B77_$r tek_hamle_v77 | cut -c1-120; done
