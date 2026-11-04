### hafif (mc=15165; 5 kosu medyan)

| Faz | Is | TEORIK FLOP (GF) | Sure (s) | CPU-s | Cekirdek kullanimi | Ulasilan GF/s | Tepe ile orani |
|---|---|---|---|---|---|---|---|
| onhazir+okuma+F1 | F1 gemm (n+va+te) x192x512 + tanh ~59M | 24.18 | 0.452 | 1.53 | 3.38 | 53.5 | %10 (f32 tepe 546) |
| OOF/Q: K-tablolari+kapi | 6 x sgemm 2001^3 + proj gemm + Sf gemm + kapi/top-k/qtab (FLOP yalniz gemm) | 107.28 | 0.981 | 3.88 | 3.95 | 109.4 | %20 (f32 tepe 546) |
| V39 havuz | ortalama+karisim (bellek bagimli) ~4 flop/eleman | 0.25 | 0.083 | 0.22 | 2.60 | 3.0 | (bellek/ozel fonksiyon) |
| V49 donusumu | istatistik+turetilmis sutunlar ~12 flop/eleman | 0.76 | 0.213 | 0.56 | 2.62 | 3.5 | (bellek/ozel fonksiyon) |
| Gram: F^T F | n x 513^2 sgemm | 59.50 | 0.184 | 0.72 | 3.93 | 323.4 | %59 (f32 tepe 546) |
| Gram: F^T Y | n x 513 skaler ekleme | 0.06 | 0.089 | 0.33 | 3.75 | 0.7 | (bellek/ozel fonksiyon) |
| Gram: slot sayimi (ACC,ACF,RC) | n x 2 slot x 513 ekleme | 0.12 | 0.068 | 0.26 | 3.87 | 1.7 | (bellek/ozel fonksiyon) |
| grup Cholesky + Y0 + X | Cholesky sum sg^3/3 + Y0 (yoluk) + X (513 sutun) | 3.07 | 0.115 | 0.42 | 3.67 | 26.7 | %12 (f64 tepe 232) |
| T=ACF'X + U=ACF'Y0 | T dgemm 513x513xmc + U YOGUN dgemm 513 x 2001 x mc (varsayilan, bit-ozdes); matematiksel asgari U = nz x 513 | 39.12 | 0.287 | 1.13 | 3.93 | 136.3 | %59 (f64 tepe 232) |
| Schur: W_F | Cholesky 513 + 513^2 x 2001 cozum | 1.14 | 0.020 | 0.08 | 4.15 | 57.2 | %25 (f64 tepe 232) |
| W_C = Y0 - X W_F | sgemm mc x 513 x 2001 | 31.13 | 0.076 | 0.30 | 3.97 | 409.7 | %75 (f32 tepe 546) |
| Q_havuz tablosu | ~10 komsu x V x V | 0.08 | 0.037 | 0.14 | 3.89 | 2.2 | (bellek/ozel fonksiyon) |
| **Toplam** | | **266.7** | **2.61** | | | **102.4** | sgemm tepe 546 GF/s |

V53-oncesi yogun Y0 cozumu ek is yuku: +4.7 GF (Y0 yogun/seyrek: 4.8x). U icin yurutulen yogun dgemm 31.1 GF; matematiksel asgari (yalniz sifir-olmayan) 2.4 GF (13.0x). Yurutulen toplam 266.7 GF, asgari toplam 238.0 GF.
