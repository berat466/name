#!/bin/bash
# olcek.sh <etiket> <veri_dizini> [HASH=0|1]: V76 ve V77 ayni veride; 15 haneli debug karsilastirmasi + sure
cd /home/user/name/formul_avcisi/v77_matematiksel_islem_azaltimi
et=$1; export DATA=$2; H=${3:-0}
./kos.sh ${et}_76 tek_hamle_v76 V76_DEBUG=1 V53H_HASH=$H | cut -c1-130
./kos.sh ${et}_77 tek_hamle_v77 V76_DEBUG=1 V53H_HASH=$H | cut -c1-130
for v in 76 77; do grep -E "V76_DBG|V53H_HASH|V53H_SONUC" ham/${et}_$v.txt | grep -v T_ust_ucgen > ham/${et}_$v.cmp; done
echo "--- $et: lin hari? her sey ---"; diff <(grep -v "lin e=" ham/${et}_76.cmp) <(grep -v "lin e=" ham/${et}_77.cmp) && echo "BIT-OZDES"
grep -E "V77_SPEK|V77_SECIM|V77_MADD" ham/${et}_77.txt | cut -c1-200
