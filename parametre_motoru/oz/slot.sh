#!/bin/bash
# GLOBAL CALISMA YUVASI (slot) havuzu: hizli.sh tum paketler ve sabotaj mutantlari icin ayni anda en cok HIZLI_SLOTSAYI islem calistirir (CPU asiri yuklenmesin, bos kalmasin).
# Bekleme yoklama (polling) DEGIL, engelleyici okumadir: hizli.sh bir FIFO'yu 9 numarali dosya tanimlayicisinda acar ve HIZLI_SLOTSAYI jeton yazar; slot_al bir jeton okur (jeton yoksa bloke olur, CPU harcamaz),
# slot_birak jetonu geri yazar. 9 numarali tanimlayici alt islemlere miras kalir. HIZLI_SLOT ayarli degilse (betik tek basina kosuyor) hicbir sey yapmaz.
slot_al() { [ -n "$HIZLI_SLOT" ] && { read -r -n1 -u 9 _jeton && SLOT_VAR=1; }; return 0; }
slot_birak() { [ -n "$HIZLI_SLOT" ] && [ -n "$SLOT_VAR" ] && { printf x >&9; SLOT_VAR=; }; return 0; }
