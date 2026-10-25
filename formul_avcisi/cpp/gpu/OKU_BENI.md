# GPU kaydirmasi (shim)
Program aynen duruyor. Tek fark: tum FP32 matris carpimlari (`scipy_cblas_sgemm`) `gpu_sgemm`'e yonlendirilir (`-DUSE_GPU`). Buyuk carpimlar cuBLAS ile GPU'da, kucukler ve tum FP64/Cholesky/grup cozumleri CPU'da kalir.
- `gpu_shim.cu`: cuBLAS cagrisi. `sgemm_map.h`: satir-ana -> sutun-ana cevirimi (CPU'da `test_map.cpp` ile 8 kombinasyonda dogrulandi).
- `../tek_hamle_v51g.cpp`: tek_hamle_v51b.cpp + 4 satir (makro).
- DURUM: GPU'suz sunucuda yazildi. Cevirim mantigi test edildi, CUDA kismi DERLENMEDI/CALISTIRILMADI.
