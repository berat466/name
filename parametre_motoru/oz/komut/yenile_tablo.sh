#!/bin/bash
# komutlar.pmk (kanonik sayisal tablo) degistikten sonra TABLODAN URETILEN her seyi yeniden uretir (kanonik kaynak degismez):
#   src/ops_gen.h (komut @@opsh), pm (C++ referans), asm/valid/gen2 (.pmb + .pma gorunumu), coz/yaz (PMY1 cozucu/yazici), paketle.pmb, arac sozlukleri
# Kullanim: yenile_tablo.sh   (PM ortam degiskeni ya da ../../pm; yeni pm derlenir)
D=$(cd "$(dirname "$0")" && pwd); R=$D/../..; cd $R || exit 1
PM=${PM:-$R/pm}
KOMUT() { { echo "@@$1"; cat; } | $PM run $R/oz/arac/komut.pmb; }
(cd oz/arac && ./uret.sh $PM > /dev/null) || exit 1
KOMUT opsh < oz/komut/komutlar.pmk > src/ops_gen.h.new && mv src/ops_gen.h.new src/ops_gen.h || exit 1
make -s pm > /dev/null 2>&1 || { make pm 2>&1 | tail -5; exit 1; }
PM=$R/pm
(cd oz/nesne && ./metinden.sh $PM > /dev/null 2>&1) || { echo "yenile_tablo: asm/valid/gen2 (nesne + .pma gorunumu) uretilemedi"; exit 1; }
(cd oz && ./uret_ikili.sh $PM $R/oz > /dev/null 2>&1) || { echo "yenile_tablo: coz/yaz uretilemedi"; exit 1; }
(cd oz/arac && ./uret.sh $PM > /dev/null) || exit 1
echo "yenile_tablo: tamam"
