#include "platform/shm_platform.h"
#include "utility/shm_profiler.h"
#include "haversine_generator.h"
#include "haversine.h"
#include "rep_tests.h"
#include <stdio.h>

int main(int argc, char** argv)
{
	shm_platform_context_init();
	shm_platform_context_init_additional_metrics();
	shm_platform_console_window_open();

	uint32 rdtsc_freq = shm_platform_get_cpu_timer_frequency(500);
	shm_profiler_init(rdtsc_freq);

	const char* json_path = "haversine_pairs.json";
	const char* results_path = "haversine_results.f64";
	uint32 pair_count = 10000000;
	#if 0
	generate_haversine_test_json(json_path, results_path, pair_count, 0xFFFFFAAA);
	#elif 0
	haversine_calculate_average_from_json(json_path, results_path, pair_count);
	#elif 1
	run_all_tests(json_path, rdtsc_freq);
	#endif

	shm_profiler_dump();
	printf_s("\nPress any key to exit...");
	shm_platform_sleep_until_key_pressed();
	shm_platform_console_window_close();
	return 0;
}
