// Windows: mallopt() yoktur (yalniz V78_MALLOPT=1 deneyinde); varsayilan yolda cagrilmaz. posix_memalign taklidi (yalniz kapali THP deneyi).
#pragma once
#include_next <malloc.h>
#include <stdlib.h>
#define M_MMAP_THRESHOLD -3
#define M_TRIM_THRESHOLD -1
#define M_TOP_PAD -2
static inline int mallopt(int, int) { return 1; }
static inline int posix_memalign(void** p, size_t al, size_t n) { *p = _aligned_malloc(n, al); return *p ? 0 : 12; }
