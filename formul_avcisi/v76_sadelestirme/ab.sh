#!/bin/bash
# V75 (acik bayraklarla) <-> V76 (varsayilan yol); dönüşümlü; tum program, ayni veri/ayar/4 is parcacigi
cd /home/user/name/formul_avcisi/v76_sadelestirme
for r in 1 2 3 4 5; do ./kos.sh A75_$r tek_hamle_v75 V76_NOP=1 | cut -c1-110; ./kos.sh B76_$r tek_hamle_v76 | cut -c1-110; done
