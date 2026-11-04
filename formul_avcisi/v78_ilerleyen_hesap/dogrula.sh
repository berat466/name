#!/bin/bash
# V77 <-> V78: model ara-buyuklukleri (hash) + 15 haneli debug (8 alfa val CE/top-k, test CE/top-k) + secilen parametreler
cd /home/user/name/formul_avcisi/v78_ilerleyen_hesap
./kos.sh dv77 tek_hamle_v77 V76_DEBUG=1 V53H_HASH=${HASH:-1} | cut -c1-120
./kos.sh dv78 tek_hamle_v78 V76_DEBUG=1 V53H_HASH=${HASH:-1} V78_WFHASH=1 | cut -c1-120
for f in dv77 dv78; do grep -E "V76_DBG|V53H_HASH|V53H_SONUC" ham/$f.txt | grep -v "T_ust_ucgen" > ham/$f.cmp; done
echo "--- hash + top-k + secimler (CE haric) ---"; diff <(grep -v "lin e=\|CE=" ham/dv77.cmp | sed 's/|CE=[^|]*//') <(grep -v "lin e=\|CE=" ham/dv78.cmp | sed 's/|CE=[^|]*//') && echo "AYNI"
python3 fark.py ham/dv77.txt ham/dv78.txt
