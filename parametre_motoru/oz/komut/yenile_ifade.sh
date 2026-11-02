#!/bin/bash
# Bolum 02: komutlar.pmk bolum 11 (IFADE OPERATOR TABLOSU) ekler/yeniler ve src/ifade_ops_gen.h'i TABLODAN uretir.
# Kullanim: yenile_ifade.sh [--sadece-baslik]
D=$(cd "$(dirname "$0")" && pwd); R=$D/../..; cd $R || exit 1; PM=${PM:-$R/pm}
KOMUT() { { echo "@@$1"; cat; } | $PM run $R/oz/arac/komut.pmb; }
KOMUT gor < oz/komut/komutlar.pmk > /tmp/ifade_gor.$$ || exit 1
if [ "$1" != "--sadece-baslik" ]; then
  python3 oz/komut/gecis/ifade_ops_ekle.py < /tmp/ifade_gor.$$ > /tmp/ifade_gor2.$$ || exit 1
  KOMUT yaz < /tmp/ifade_gor2.$$ > /tmp/ifade_yeni.$$ || exit 1
  # bolum 11 yoksa/degistiyse yaz
  cmp -s /tmp/ifade_yeni.$$ oz/komut/komutlar.pmk || cp /tmp/ifade_yeni.$$ oz/komut/komutlar.pmk
  KOMUT gor < oz/komut/komutlar.pmk > /tmp/ifade_gor.$$
fi
python3 oz/komut/gecis/ifade_ops_h.py < /tmp/ifade_gor.$$ > src/ifade_ops_gen.h.new && { cmp -s src/ifade_ops_gen.h.new src/ifade_ops_gen.h || mv src/ifade_ops_gen.h.new src/ifade_ops_gen.h; rm -f src/ifade_ops_gen.h.new; }
rm -f /tmp/ifade_gor.$$ /tmp/ifade_gor2.$$ /tmp/ifade_yeni.$$
echo "yenile_ifade: tamam"
