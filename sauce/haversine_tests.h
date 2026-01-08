#pragma once
#include "shm_utils/shm_utils_common_defines.h"

bool8 rep_test_haversine_reference(uint64 time_counter_frequency, const char* json_path, const char* ref_results_file_path, uint32 expected_pair_count);