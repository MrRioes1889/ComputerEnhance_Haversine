#include "platform/shm_platform.h"
#include "platform/shm_intrin.h"
#include <stdio.h>

#define SHM_UTILS_GENERAL_LOG(msg, ...) fprintf_s(stdout, msg, ##__VA_ARGS__)
#define SHM_UTILS_GENERAL_LOG_ERR(msg, ...) fprintf_s(stderr, msg, ##__VA_ARGS__)
#define SHM_UTILS_GENERAL_GET_PAGE_FAULT_COUNT() shm_platform_metrics_get_page_fault_count()
#define SHM_UTILS_GENERAL_GET_TSC() shm_intrin_rdtsc()

#define SHM_REPETITION_TESTER_IMPL
#include <utility/shm_repetition_tester.h>