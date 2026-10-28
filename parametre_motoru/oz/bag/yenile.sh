#!/bin/bash
# MOTORU KAYNAKTAN YENIDEN URET (pm ve g++ OLMADAN): eski arac takimi (asm.elf, gen2.elf, coz.elf, valid.elf) ile
# oz/ kaynaklarindan (asm.pma.in, gen2.pma.in, sablon.txt, valid.pma.in, coz.pmb, yaz.pmb) YENI arac takimini uretir.
# Adimlar: 1) kaynak -> .pma (awk/bash bicim donusumu)  2) .pma -> PMB1 (eski asm.elf)  3) PMB1 -> liste (eski coz.elf)
#          4) dogrulama (eski valid.elf: "1 ..." sart)  5) liste -> ELF (eski gen2.elf)  6) yeni gen2.elf ile tum araclari yeniden uret
#          7) sabit nokta: yeni gen2.elf kendi ELF'ini bayt-bayt uretir.
# Kullanim: yenile.sh ESKI_DIZIN YENI_DIZIN [KAYNAK_KOK]   (KAYNAK_KOK: src/ops.h ve oz/ altini iceren agac; varsayilan depo koku)
ESKI=$(realpath "${1:?eski arac dizini}"); YENI=${2:?yeni dizin}; mkdir -p "$YENI"; YENI=$(realpath "$YENI"); D=$(cd "$(dirname "$0")" && pwd); R=$(realpath "${3:-$D/../..}")
W=$YENI/work; rm -rf $W; mkdir -p $W/src $W/oz/elf; cp $R/src/ops.h $W/src/; cp $R/oz/elf/T.txt $W/oz/elf/; cp -r $R/oz/asm $R/oz/elf2 $R/oz/dogr $W/oz/; cp $R/oz/coz.pmb $R/oz/yaz.pmb $W/oz/
rm -f $W/oz/asm/asm.pmb $W/oz/elf2/gen2.pmb $W/oz/dogr/valid.pmb
cat > $W/asmwrap.sh <<EOW
#!/bin/bash
(ulimit -v 900000; timeout 60 $ESKI/asm.elf < "\$1" > "\$2") || { echo "asm.elf basarisiz: \$1" >&2; exit 1; }
EOW
chmod +x $W/asmwrap.sh; export ASM2=$W/asmwrap.sh
for t in asm elf2 dogr; do (cd $W/oz/$t && ./uret.sh) > /dev/null || { echo "yenile: $t derlenemedi"; exit 1; }; done
liste() { (ulimit -v 900000; timeout 60 $ESKI/coz.elf < "$1" > "$2") || { echo "yenile: coz.elf reddetti: $1"; exit 1; }; }
for p in $W/oz/asm/asm.pmb $W/oz/elf2/gen2.pmb $W/oz/dogr/valid.pmb $W/oz/coz.pmb $W/oz/yaz.pmb; do b=$(basename $p .pmb); liste $p $W/$b.l
  read -r V < <( (ulimit -v 900000; timeout 60 $ESKI/valid.elf < $W/$b.l) ); case "$V" in 1*) ;; *) echo "yenile: dogrulayici reddetti $b ('$V')"; exit 1;; esac; done
# 5) eski gen2.elf ile ara nesil, 6) yeni gen2.elf ile son nesil
mkdir -p $YENI/ara $YENI/son
for b in gen2 asm coz yaz valid; do (ulimit -v 900000; timeout 60 $ESKI/gen2.elf < $W/$b.l > $YENI/ara/$b.elf) || { echo "yenile: ara $b uretilemedi"; exit 1; }; done
chmod +x $YENI/ara/*.elf
for b in gen2 asm coz yaz valid; do (ulimit -v 900000; timeout 60 $YENI/ara/gen2.elf < $W/$b.l > $YENI/son/$b.elf) || { echo "yenile: son $b uretilemedi"; exit 1; }; done
chmod +x $YENI/son/*.elf
# 7) sabit nokta: son gen2.elf, kendi listesinden kendini uretir
$YENI/son/gen2.elf < $W/gen2.l | cmp -s - $YENI/son/gen2.elf || { echo "yenile: sabit nokta bozuk"; exit 1; }
cmp -s $YENI/ara/gen2.elf $YENI/son/gen2.elf && echo "yenile: ara nesil == son nesil (uretici degismedi ya da ayni cikti)" || echo "yenile: ara nesil != son nesil (uretici DEGISTI; son nesil kendini yeniden uretti)"
cmp -s $ESKI/gen2.elf $YENI/son/gen2.elf && echo "yenile: yeni gen2.elf == eski gen2.elf" || echo "yenile: yeni gen2.elf != eski gen2.elf ($(stat -c %s $ESKI/gen2.elf) -> $(stat -c %s $YENI/son/gen2.elf) bayt)"
for b in gen2 asm coz yaz valid; do cp $YENI/son/$b.elf $YENI/$b.elf; done
echo "yenile: tamam -> $YENI/{gen2,asm,coz,yaz,valid}.elf"
