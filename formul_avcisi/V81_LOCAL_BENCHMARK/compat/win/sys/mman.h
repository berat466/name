// Windows: madvise() yoktur; V81 varsayilan yolunda KULLANILMAZ (yalniz kapali V81_REUSE / THP deneylerinde). Bos taklit.
#pragma once
#define MADV_HUGEPAGE 14
#define MADV_DONTNEED 4
static inline int madvise(void*, size_t, int) { return 0; }
