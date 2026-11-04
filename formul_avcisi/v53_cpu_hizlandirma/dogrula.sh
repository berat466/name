#!/bin/bash
# Dogruluk: orijinal aritmetik yolu (FAST=0) / hizli-tam (U=0) / hizli-seyrek-U (U=1) -> dump + hash; orijinal ikili agirliklariyla cmp
cd "$(dirname "$0")"
for cfg in full hafif; do source env_$cfg.sh
  for m in o f s; do rm -rf /tmp/hd_${cfg}_$m; mkdir -p /tmp/hd_${cfg}_$m; done
  V53H_LEAN=1 V53H_TEST=0 V53H_HASH=1 V53H_DUMP=/tmp/hd_${cfg}_o ../cpp/tek_hamle_v53h $ARGS > ham/dogr_${cfg}_orijinal_yol.txt 2>&1
  V53H_LEAN=1 V53H_FAST=1 V53H_U=0 V53H_TEST=0 V53H_HASH=1 V53H_DUMP=/tmp/hd_${cfg}_f ../cpp/tek_hamle_v53h $ARGS > ham/dogr_${cfg}_hizli_tam.txt 2>&1
  V53H_LEAN=1 V53H_FAST=1 V53H_U=1 V53H_TEST=0 V53H_HASH=1 V53H_DUMP=/tmp/hd_${cfg}_s ../cpp/tek_hamle_v53h $ARGS > ham/dogr_${cfg}_hizli_seyrekU.txt 2>&1
  { echo "=== $cfg : orijinal v53 ikilisi (V33_DUMP) ile cmp"; for m in o f s; do for w in WC WF; do if cmp -s /tmp/hd_${cfg}_$m/$w.bin /tmp/href_${cfg}_$w.bin; then echo "$m $w: BIT-OZDES"; else echo "$m $w: FARKLI"; fi; done; done
    echo "--- orijinal yol vs hizli-tam"; python3 fark.py /tmp/hd_${cfg}_o /tmp/hd_${cfg}_f
    echo "--- orijinal yol vs hizli-seyrekU"; python3 fark.py /tmp/hd_${cfg}_o /tmp/hd_${cfg}_s; } > ham/dogr_${cfg}_ozet.txt 2>&1
done
touch ham/dogr_DONE
