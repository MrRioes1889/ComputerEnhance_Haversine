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
	uint32 pair_count = 10000000;
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
	shm_json_data_destroy(&json_data);
	uint64 tsc_json_parsed = shm_intrin_rdtsc();

	float64 haversine_avg = 0.0;
    float64 earth_radius = 6372.8;
	for (uint32 i = 0; i < pair_count; i++)
		haversine_avg += haversine_reference(pairs[i], earth_radius);

	haversine_avg /= pair_count;
	uint64 tsc_haversine_calc = shm_intrin_rdtsc();

	printf_s("Pair Count: %u\n", pair_count);
	printf_s("Haversine average: %.5f\n\n", haversine_avg);

	uint64 tsc_end = shm_intrin_rdtsc();

	uint64 tsc_total = (tsc_end - tsc_start);
	printf_s("RDTSC frequency: %llu cycles/second\n", rdtsc_freq);
	printf_s("RDTSC total count: %llu\n", tsc_total);
	printf_s("Estimated total compute time: %.5f ms\n", ((float64)(tsc_total)/(float64)rdtsc_freq) * 1000);
	printf_s("RDTSC counts:\n");
	uint64 file_read_count = tsc_file_read - tsc_start;
	uint64 json_parse_count = tsc_json_parsed - tsc_file_read;
	uint64 haversine_calc_count = tsc_haversine_calc - tsc_json_parsed;
	uint64 misc_end_count = tsc_end - tsc_haversine_calc;
	printf_s("File Read: %llu (%.2f%%)\n", file_read_count, ((float64)file_read_count/(float64)tsc_total) * 100.0);
	printf_s("JSON Parse: %llu (%.2f%%)\n", json_parse_count, ((float64)json_parse_count/(float64)tsc_total)* 100.0);
	printf_s("Haversine calc: %llu (%.2f%%)\n", haversine_calc_count, ((float64)haversine_calc_count/(float64)tsc_total) * 100.0);
	printf_s("Misc end / cleanup: %llu (%.2f%%)\n", misc_end_count, ((float64)misc_end_count/(float64)tsc_total) * 100.0);
	#endif

	printf_s("\nPress any key to exit...");
	shm_platform_sleep_until_key_pressed();
	shm_platform_console_window_close();
	return 0;
}
