#include "platform/shm_platform.h"
#include "platform/shm_intrin.h"
#include "utility/shm_json.h"
#include "haversine_generator.h"
#include "haversine.h"
#include <stdio.h>
#include <malloc.h>

int main(int argc, char** argv)
{
	SHM_PlatformContext plat_context = {0};
	shm_platform_context_init(&plat_context);
	shm_platform_console_window_open();

	uint32 rdtsc_freq = shm_platform_get_rdtsc_frequency(500);

	const char* json_path = "haversine_pairs.json";
	const char* results_path = "haversine_results.f64";
	uint32 pair_count = 1000000;
	#if 0
	generate_haversine_test_json(json_path, results_path, pair_count, 0xFFFFFAAA);
	#elif 1
	uint64 tsc_start = shm_intrin_rdtsc();
	uint64 filesize = shm_platform_get_filesize(json_path);
	char* json_s = malloc(filesize+1);
	SHM_FileHandle file = shm_platform_file_open(json_path);
	shm_platform_file_read(file, json_s, (uint32)filesize);
	json_s[filesize] = 0;
	shm_platform_file_close(&file);
	uint64 tsc_file_read = shm_intrin_rdtsc();

	SHM_JsonData json_data = {0};
	if (!shm_json_parse_text(json_s, pair_count * 5 + 1, &json_data))
	{
		printf_s("Error: Failed to parse json file.");
		return 1;
	}
	free(json_s);
	uint64 tsc_json_parsed = shm_intrin_rdtsc();

	SHM_JsonNodeId pairs_arr_id = shm_json_get_child_id_by_key(&json_data, 0, "pairs");
	SHM_JsonNodeId pair_id = shm_json_get_first_child_id(&json_data, pairs_arr_id);
	pair_count = 0;
	float64 haversine_avg = 0.0;
    float64 earth_radius = 6372.8;
	while (pair_id != SHM_JSON_INVALID_ID)
	{
		HaversinePair pair = {0};
		SHM_JsonNodeId x0_id = shm_json_get_first_child_id(&json_data, pair_id);
		shm_json_get_float_value(&json_data, x0_id, &pair.x0);
		SHM_JsonNodeId y0_id = shm_json_get_next_child_id(&json_data, x0_id);
		shm_json_get_float_value(&json_data, y0_id, &pair.y0);
		SHM_JsonNodeId x1_id = shm_json_get_next_child_id(&json_data, y0_id);
		shm_json_get_float_value(&json_data, x1_id, &pair.x1);
		SHM_JsonNodeId y1_id = shm_json_get_next_child_id(&json_data, x1_id);
		shm_json_get_float_value(&json_data, y1_id, &pair.y1);
		
		haversine_avg += haversine_reference(pair, earth_radius);
		pair_id = shm_json_get_next_child_id(&json_data, pair_id);
		pair_count++;
	}
	haversine_avg /= pair_count;
	uint64 tsc_values_summed = shm_intrin_rdtsc();

	printf_s("Pair Count: %u\n", pair_count);
	printf_s("Haversine average: %.5lf\n\n", haversine_avg);

	shm_json_data_destroy(&json_data);
	uint64 tsc_end = shm_intrin_rdtsc();

	printf_s("Estimated total compute time: %.5lf ms\n", ((float64)(tsc_end - tsc_start)/(float64)rdtsc_freq) * 1000);
	printf_s("RDTSC counts:\n");
	printf_s("File Read: %lu\n", (tsc_file_read - tsc_start));
	printf_s("JSON Parse: %lu\n", (tsc_json_parsed - tsc_file_read));
	printf_s("Haversine calc: %lu\n", (tsc_values_summed - tsc_json_parsed));
	printf_s("Misc end / cleanup: %lu\n", (tsc_end - tsc_values_summed));
	#endif

	printf_s("\nPress any button to exit...");
	shm_platform_sleep_until_key_pressed();
	shm_platform_console_window_close();
	return 0;
}
