#!/bin/bash
# DEGISMEMIS orijinal v53 ikilisi: tam program (arastirma yollari dahil) x3, model-hazir kalemleri log'dan turetilir
cd "$(dirname "$0")"; mkdir -p ham/orig
for cfg in full hafif; do source env_$cfg.sh; for r in 1 2 3; do ../cpp/tek_hamle_v53 $ARGS > ham/orig/${cfg}_$r.txt 2>&1; done; done
touch ham/orig/DONE
