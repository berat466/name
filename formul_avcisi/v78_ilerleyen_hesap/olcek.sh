#!/bin/bash
# olcek.sh <etiket> <veri_dizini> [HASH=0|1]: V77 ve V78 ayni veride; 15 haneli debug karsilastirmasi + sure
cd /home/user/name/formul_avcisi/v78_ilerleyen_hesap
et=$1; export DATA=$2; H=${3:-0}
./kos.sh ${et}_77 tek_hamle_v77 V76_DEBUG=1 V53H_HASH=$H | cut -c1-130
./kos.sh ${et}_78 tek_hamle_v78 V76_DEBUG=1 V53H_HASH=$H | cut -c1-130
for v in 77 78; do grep -E "V76_DBG|V53H_HASH|V53H_SONUC" ham/${et}_$v.txt | grep -v "T_ust_ucgen" > ham/${et}_$v.cmp; done
echo "--- $et: CE hari? her sey (hash/top-k/secim) ---"; diff <(grep -v "lin e=" ham/${et}_77.cmp | sed 's/|CE=[^|]*//; s/CE=[0-9.]* //') <(grep -v "lin e=" ham/${et}_78.cmp | sed 's/|CE=[^|]*//; s/CE=[0-9.]* //') && echo "AYNI"
python3 fark.py ham/${et}_77.txt ham/${et}_78.txt
