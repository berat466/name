// Windows (MinGW-w64/clang) icin getrusage(RUSAGE_SELF) taklidi: yalnizca ru_maxrss (KB) ve ru_utime/ru_stime kullanilir.
#pragma once
#include <windows.h>
#include <psapi.h>
#include <sys/time.h>
#define RUSAGE_SELF 0
#define RUSAGE_CHILDREN -1
struct rusage { struct timeval ru_utime, ru_stime; long ru_maxrss, ru_minflt, ru_majflt, ru_nvcsw, ru_nivcsw; };
static inline int getrusage(int, struct rusage* u) {
    memset(u, 0, sizeof *u); PROCESS_MEMORY_COUNTERS pmc; if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof pmc)) u->ru_maxrss = (long)(pmc.PeakWorkingSetSize / 1024);
    FILETIME c, e, k, us; if (GetProcessTimes(GetCurrentProcess(), &c, &e, &k, &us)) { ULONGLONG kk = ((ULONGLONG)k.dwHighDateTime << 32) | k.dwLowDateTime, uu = ((ULONGLONG)us.dwHighDateTime << 32) | us.dwLowDateTime; u->ru_stime.tv_sec = (long)(kk / 10000000ULL); u->ru_stime.tv_usec = (long)((kk % 10000000ULL) / 10); u->ru_utime.tv_sec = (long)(uu / 10000000ULL); u->ru_utime.tv_usec = (long)((uu % 10000000ULL) / 10); }
    return 0; }
