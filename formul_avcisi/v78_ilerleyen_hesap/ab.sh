#!/bin/bash
# V77 <-> V78 donusumlu (N cift); tum program, ayni veri/ayar/4 is parcacigi
cd /home/user/name/formul_avcisi/v78_ilerleyen_hesap
N=${N:-5}
for r in $(seq 1 $N); do ./kos.sh A77_$r tek_hamle_v77 | cut -c1-120; ./kos.sh B78_$r tek_hamle_v78 | cut -c1-120; done
