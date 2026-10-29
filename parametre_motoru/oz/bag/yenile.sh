#!/bin/bash
# MOTORU KAYNAKTAN YENIDEN URET (pm, g++, awk, sed, od OLMADAN): eski ELF takimi (ESKI) ile oz/ kaynaklarindan YENI takimi uretir.
# Kaynaklar: oz/komut/komutlar.pmk (KANONIK sayisal komut tablosu), oz/arac/*.pma, oz/asm/asm.pma.in, oz/elf2/{gen2.pma.in,statik.sozluk} (x86 sablonlari tabloda: komutlar.pmk bolum 8/9), oz/dogr/valid.pma.in, oz/yap/{yap.pma.in,fmt.txt}, oz/coz.pmy, oz/yaz.pmy.
# Adimlar: 1) araclar (ESKI arac ELF'leri) ile kaynaklardan .pma uret (opstab/fmtc/doldur)  2) .pma -> PMB1 (ESKI asm.elf); PMY1 -> yap.elf -> asm.elf
#          3) PMB1 -> liste (ESKI coz.elf), dogrulama (ESKI valid.elf: "1 ..." sart)  4) liste -> ELF: ESKI gen2.elf ile ara nesil, ara nesil gen2.elf ile son nesil
#          5) sabit nokta: son gen2.elf kendi ELF'ini bayt-bayt uretir.
# Kullanim: yenile.sh ESKI_DIZIN YENI_DIZIN [KAYNAK_KOK]   (KAYNAK_KOK: oz/ altini iceren agac; varsayilan depo koku)
ESKI=$(realpath "${1:?eski arac dizini}"); YENI=${2:?yeni dizin}; mkdir -p "$YENI"; YENI=$(realpath "$YENI"); D=$(cd "$(dirname "$0")" && pwd); R=$(realpath "${3:-$D/../..}")
W=$YENI/work; rm -rf $W; mkdir -p $W/oz; cp -r $R/oz/komut $W/oz/; cp -r $R/oz/arac $R/oz/asm $R/oz/elf2 $R/oz/dogr $R/oz/yap $W/oz/; cp $R/oz/paketle.pmy $R/oz/coz.sayi.in $R/oz/yaz.sayi.in $R/oz/uret_ikili.sh $W/oz/
rm -f $W/oz/asm/asm.pmb $W/oz/elf2/gen2.pmb $W/oz/dogr/valid.pmb $W/oz/yap/yap.pmb $W/oz/arac/*.pmb
cat > $W/asmwrap.sh <<EOW
#!/bin/bash
(ulimit -v 900000; timeout 60 $ESKI/asm.elf < "\$1" > "\$2") || { echo "asm.elf basarisiz: \$1" >&2; exit 1; }
EOW
chmod +x $W/asmwrap.sh; export ASM2=$W/asmwrap.sh ARAC=$ESKI
for t in arac asm elf2 dogr yap; do (cd $W/oz/$t && ./uret.sh) > $W/uret_$t.log 2>&1 || { echo "yenile: $t derlenemedi: $(tail -2 $W/uret_$t.log)"; exit 1; }; done
(cd $W/oz && ./uret_ikili.sh "" $W) > $W/uret_ikili.log 2>&1 || { echo "yenile: coz/yaz tablolari komut tablosundan uretilemedi: $(tail -2 $W/uret_ikili.log)"; exit 1; }
cp $W/oz/paketle.pmy $W/paketle.pmy
for t in paketle coz yaz; do (ulimit -v 900000; timeout 60 $ESKI/yap.elf < $W/$t.pmy > $W/$t.pma && timeout 60 $ESKI/asm.elf < $W/$t.pma > $W/$t.pmb) || { echo "yenile: $t.pmy cevrilemedi"; exit 1; }; done
declare -A KAYNAK=([gen2]=$W/oz/elf2/gen2.pmb [asm]=$W/oz/asm/asm.pmb [valid]=$W/oz/dogr/valid.pmb [yap]=$W/oz/yap/yap.pmb [doldur]=$W/oz/arac/doldur.pmb [opstab]=$W/oz/arac/opstab.pmb [fmtc]=$W/oz/arac/fmtc.pmb [komut]=$W/oz/arac/komut.pmb [paketle]=$W/paketle.pmb [coz]=$W/coz.pmb [yaz]=$W/yaz.pmb)
ARALAR="gen2 asm valid yap doldur opstab fmtc komut paketle coz yaz"
for b in $ARALAR; do (ulimit -v 900000; timeout 60 $ESKI/coz.elf < ${KAYNAK[$b]} > $W/$b.l) || { echo "yenile: coz.elf reddetti: $b"; exit 1; }
  read -r V < <( (ulimit -v 900000; timeout 60 $ESKI/valid.elf < $W/$b.l) ); case "$V" in 1*) ;; *) echo "yenile: dogrulayici reddetti $b ('$V')"; exit 1;; esac; done
mkdir -p $YENI/ara $YENI/son
for b in $ARALAR; do (ulimit -v 900000; timeout 60 $ESKI/gen2.elf < $W/$b.l > $YENI/ara/$b.elf) || { echo "yenile: ara $b uretilemedi"; exit 1; }; done
chmod +x $YENI/ara/*.elf
for b in $ARALAR; do (ulimit -v 900000; timeout 60 $YENI/ara/gen2.elf < $W/$b.l > $YENI/son/$b.elf) || { echo "yenile: son $b uretilemedi"; exit 1; }; done
chmod +x $YENI/son/*.elf
$YENI/son/gen2.elf < $W/gen2.l | cmp -s - $YENI/son/gen2.elf || { echo "yenile: sabit nokta bozuk"; exit 1; }
cmp -s $YENI/ara/gen2.elf $YENI/son/gen2.elf && echo "yenile: ara nesil == son nesil (uretici degismedi)" || echo "yenile: ara nesil != son nesil (uretici DEGISTI; son nesil kendini yeniden uretti)"
cmp -s $ESKI/gen2.elf $YENI/son/gen2.elf && echo "yenile: yeni gen2.elf == eski gen2.elf" || echo "yenile: yeni gen2.elf != eski gen2.elf ($(stat -c %s $ESKI/gen2.elf) -> $(stat -c %s $YENI/son/gen2.elf) bayt)"
for b in $ARALAR; do cp $YENI/son/$b.elf $YENI/$b.elf; done
echo "yenile: tamam -> $YENI/{${ARALAR// /,}}.elf"
