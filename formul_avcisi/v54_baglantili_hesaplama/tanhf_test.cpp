// tum 2^32 float bit-deseninde (NaN/inf haric) tv_tanhf == libm tanhf ?  (bit karsilastirma)
#include <bits/stdc++.h>
#include <omp.h>
#include "../cpp/tanhf_vec.h"
int main() {
    const uint64_t N = 1ull << 32; const size_t B = 1 << 20; std::atomic<uint64_t> mis(0), tot(0); uint64_t first = ~0ull; double t0 = omp_get_wtime();
#pragma omp parallel
    { std::vector<float> in(B), out(B); uint64_t lm = 0;
#pragma omp for schedule(dynamic, 1)
        for (long blk = 0; blk < (long)(N / B); blk++) { for (size_t i = 0; i < B; i++) { uint32_t u = (uint32_t)(blk * B + i); in[i] = tv_u2f(u); }
            tv_tanhf_array(in.data(), out.data(), B);
            for (size_t i = 0; i < B; i++) { uint32_t u = tv_f2u(in[i]); if ((u & 0x7fffffffu) > 0x7f800000u) continue; float ref = tanhf(in[i]); if (tv_f2u(ref) != tv_f2u(out[i])) { lm++; if (lm <= 3) printf("FARK x=%08x ref=%08x vec=%08x\n", u, tv_f2u(ref), tv_f2u(out[i])); } } }
        mis += lm; }
    printf("tum 2^32 girdi: bit-farkli = %llu ; sure %.1f s\n", (unsigned long long)mis.load(), omp_get_wtime() - t0); }
