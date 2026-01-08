#pragma once
#include "shm_utils/shm_utils_common_defines.h"

void rep_test_mem_file_read(uint64 time_counter_frequency, const char* filename);
void rep_test_mem_incremental_file_read(uint64 time_counter_frequency, const char* filename);
void rep_test_mem_cache_size_pow_2(uint64 time_counter_frequency);
void rep_test_mem_cache_size_non_pow_2(uint64 time_counter_frequency);
