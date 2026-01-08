#include "shm_utils/platform/shm_platform.h"
#include "shm_utils/shm_profiler.h"
#include "haversine_calc.h"
#include "haversine_generator.h"
#include "mem_tests.h"
#include "haversine_tests.h"
#include <stdio.h>

int main(int argc, char** argv)
{
	shm_platform_context_init();
	shm_platform_context_init_additional_metrics();
	if (!shm_platform_is_console_window_attached())
		shm_platform_console_window_open();

	uint64 rdtsc_freq = shm_platform_get_cpu_timer_frequency(500);
	shm_profiler_init(rdtsc_freq);

	const char* json_path = "haversine_pairs.json";
	const char* results_path = "haversine_results.f64";
	uint32 pair_count = 10000000;
	#if defined(HAVERSINE_GEN)
	generate_haversine_test_json(json_path, results_path, pair_count, 0xFFFFFAAA);
	#elif defined(HAVERSINE_CALC)
	haversine_calculate_average_from_json(json_path, results_path, pair_count);
	#elif defined(HAVERSINE_TESTS)
	//rep_test_mem_cache_size_pow_2(rdtsc_freq);
	//rep_test_mem_file_read(rdtsc_freq, json_path);
	//rep_test_mem_incremental_file_read(rdtsc_freq, json_path);
	rep_test_haversine_reference(rdtsc_freq, json_path, results_path, pair_count);
	#endif

	shm_profiler_dump();
	printf_s("\nPress any key to exit...");
	shm_platform_sleep_until_key_pressed();
	//shm_platform_console_window_close();
	return 0;
}
