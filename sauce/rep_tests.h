#pragma once
#include "shm_utils/shm_utils_common_defines.h"

void run_file_read_tests(uint64 time_counter_frequency, const char* filename);
void run_incremental_file_read_tests(uint64 time_counter_frequency, const char* filename);
void run_cache_size_tests_pow_2(uint64 time_counter_frequency);
void run_cache_size_tests_non_pow_2(uint64 time_counter_frequency);
