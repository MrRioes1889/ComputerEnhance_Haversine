#include "platform/shm_platform.h"
#include "platform/shm_intrin.h"
#include "utility/shm_json.h"
#include "utility/shm_profiler.h"
#include "haversine_generator.h"
#include "haversine.h"
#include <stdio.h>
#include <malloc.h>

int main(int argc, char** argv)
{
	SHM_PlatformContext plat_context = {0};
	shm_platform_context_init(&plat_context);
	shm_platform_console_window_open();

	uint32 rdtsc_freq = shm_platform_get_cpu_timer_frequency(500);
	shm_profiler_init(rdtsc_freq);

	const char* json_path = "haversine_pairs.json";
	const char* results_path = "haversine_results.f64";
	uint32 pair_count = 10000000;
	#if 0
	generate_haversine_test_json(json_path, results_path, pair_count, 0xFFFFFAAA);
	#elif 1

	SHM_TIMER_START("Start");
	uint64 filesize = shm_platform_get_filesize(json_path);
	char* json_s = malloc(filesize+1);
	SHM_FileHandle file = shm_platform_file_open(json_path);
	shm_platform_file_read(file, json_s, (uint32)filesize);
	json_s[filesize] = 0;
	shm_platform_file_close(&file);
	SHM_TIMER_STOP();

	SHM_TIMER_START("Parse Json");
	SHM_JsonData json_data = {0};
	if (!shm_json_parse_text(json_s, pair_count * 5 + 1, &json_data))
	{
		printf_s("Error: Failed to parse json file.");
		return 1;
	}
	free(json_s);

	SHM_TIMER_START("Read json pairs");
	SHM_JsonNodeId pairs_arr_id = shm_json_get_child_id_by_key(&json_data, 0, "pairs");
	pair_count = shm_json_get_child_count(&json_data, pairs_arr_id);
	HaversinePair* pairs = malloc(sizeof(HaversinePair) * pair_count);
	SHM_JsonNodeId pair_id = shm_json_get_first_child_id(&json_data, pairs_arr_id);
	for (uint32 i = 0; i < pair_count; i++)
	{
		HaversinePair* pair = &pairs[i];
		SHM_JsonNodeId x0_id = shm_json_get_first_child_id(&json_data, pair_id);
		SHM_JsonNodeId y0_id = shm_json_get_next_child_id(&json_data, x0_id);
		SHM_JsonNodeId x1_id = shm_json_get_next_child_id(&json_data, y0_id);
		SHM_JsonNodeId y1_id = shm_json_get_next_child_id(&json_data, x1_id);
		shm_json_get_float_value(&json_data, x0_id, &pair->x0);
		shm_json_get_float_value(&json_data, y0_id, &pair->y0);
		shm_json_get_float_value(&json_data, x1_id, &pair->x1);
		shm_json_get_float_value(&json_data, y1_id, &pair->y1);

		pair_id = shm_json_get_next_child_id(&json_data, pair_id);
	}
	SHM_TIMER_STOP();
	shm_json_data_destroy(&json_data);
	SHM_TIMER_STOP();

	SHM_TIMER_START("Haversine calculation");
	float64 haversine_avg = 0.0;
    float64 earth_radius = 6372.8;
	for (uint32 i = 0; i < pair_count; i++)
		haversine_avg += haversine_reference(pairs[i], earth_radius);

	haversine_avg /= pair_count;
	SHM_TIMER_STOP();

	SHM_TIMER_START("Misc. end");
	printf_s("Pair Count: %u\n", pair_count);
	printf_s("Haversine average: %.5f\n\n", haversine_avg);
	SHM_TIMER_STOP();

	shm_profiler_dump();
	#endif

	printf_s("\nPress any key to exit...");
	shm_platform_sleep_until_key_pressed();
	shm_platform_console_window_close();
	return 0;
}
