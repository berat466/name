#!/bin/bash
# V76 <-> V77: model ara-buyuklukleri (BLAKE/ozet hash) + 15 haneli debug (8 alfa val CE/top-k, test CE/top-k) + secilen parametreler
cd /home/user/name/formul_avcisi/v77_matematiksel_islem_azaltimi
./kos.sh dv76 tek_hamle_v76 V76_DEBUG=1 V53H_HASH=${HASH:-1} | cut -c1-120
./kos.sh dv77 tek_hamle_v77 V76_DEBUG=1 V53H_HASH=${HASH:-1} | cut -c1-120
for f in dv76 dv77; do grep -E "V76_DBG|V53H_HASH|V53H_SONUC" ham/$f.txt | grep -v T_ust_ucgen > ham/$f.cmp; done
echo "--- lin tablosu HARIC her sey (hash, 8 alfa CE/top-k, test, secimler) ---"; diff <(grep -v "lin e=" ham/dv76.cmp) <(grep -v "lin e=" ham/dv77.cmp) && echo "BIT-OZDES"
echo "--- lin tablosu farklari (secim tablosu; toplama sirasi) ---"; diff <(grep "lin e=" ham/dv76.cmp) <(grep "lin e=" ham/dv77.cmp) | grep -c "^<" 
