#pragma once
#include "../shm_utils_common_defines.h"

#ifdef _WIN32
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

static inline uint64 shm_intrin_rdtsc()
{
    return __rdtsc();
}