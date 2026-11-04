# ORIJINAL v53 ikilisi (degismemis): referans agirliklar (V33_DUMP) + tam program ciktisi
cd "$(dirname "$0")"
for cfg in full hafif; do source env_$cfg.sh; V33_DUMP=/tmp/href_$cfg ../cpp/tek_hamle_v53 $ARGS > ham/ref_orijinal_$cfg.txt 2>&1; done
touch ham/ref_orijinal_DONE
