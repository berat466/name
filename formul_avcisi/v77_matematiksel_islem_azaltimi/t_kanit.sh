#!/bin/bash
# T ust ucgen kaniti: (1) alt ucgen NaN -> W_F karmasi degismez (alt ucgen okunmuyor), (2) ust ucgen tam dgemm ile blok dgemm karsilastirmasi
cd /home/user/name/formul_avcisi/v77_matematiksel_islem_azaltimi
./kos.sh tkanit_tam tek_hamle_v77 V53H_HASH=1 V77_TSYM=0 | cut -c1-80
./kos.sh tkanit_blok tek_hamle_v77 V53H_HASH=1 V77_TSYM=1 | cut -c1-80
./kos.sh tkanit_zehir tek_hamle_v77 V53H_HASH=1 V77_TSYM=2 | cut -c1-80
grep -H "T_ust_ucgen\|HASH|WF\|HASH|T|" ham/tkanit_*.txt | cut -c1-120
echo BITTI > ham/tkanit_bitti.txt
