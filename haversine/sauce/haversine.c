#include "haversine.h"
#include "platform/shm_platform.h"
#include "utility/shm_profiler.h"
#include "utility/shm_json.h"
#include <math.h>
#include <stdio.h>
#include <malloc.h>

static inline float64 square(float64 x)
{
    return x * x;
}

static inline float64 deg_to_rad(float64 deg)
{
    return 0.01745329251994329577 * deg;
}

// NOTE: Slow reference version of haversine distance calculation
float64 haversine_reference(HaversinePair coords, float64 radius)
{
    float64 lat1 = coords.y0;
    float64 lat2 = coords.y1;
    float64 lon1 = coords.x0;
    float64 lon2 = coords.x1;

    float64 dlat = deg_to_rad(lat2 - lat1);
    float64 dlon = deg_to_rad(lon2 - lon1);
    lat1 = deg_to_rad(lat1);
    lat2 = deg_to_rad(lat2);

    float64 a = square(sin(dlat/2.0)) + cos(lat1) * cos(lat2) * square(sin(dlon/2.0));
    float64 c = 2.0 * asin(sqrt(a));

    return radius * c;
}

bool8 haversine_calculate_average_from_json(const char* json_path, const char* results_cmp_path, uint32 expected_pair_count)
{
	uint64 filesize = shm_platform_get_filesize(json_path);
	SHM_TIMER_START_DATA(read_file, "Read File", filesize);
	char* json_s = malloc(filesize+1);
	SHM_FileHandle file = shm_platform_file_open(json_path);
	shm_platform_file_read(file, json_s, (uint32)filesize);
	json_s[filesize] = 0;
	shm_platform_file_close(&file);
	SHM_TIMER_STOP(read_file);

	SHM_TIMER_START_DATA(parse_json, "Parse Json", filesize);
	SHM_JsonData json_data = {0};
	if (!shm_json_parse_text(json_s, expected_pair_count * 5 + 1, &json_data))
	{
		printf_s("Error: Failed to parse json file.");
		return false;
	}
	free(json_s);

	SHM_JsonNodeId pairs_arr_id = shm_json_get_child_id_by_key(&json_data, 0, "pairs");
	uint32 pair_count = shm_json_get_child_count(&json_data, pairs_arr_id);

	SHM_TIMER_START_DATA(read_pairs, "Read json pairs", sizeof(HaversinePair) * pair_count);
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
	SHM_TIMER_STOP(read_pairs);

	shm_json_data_destroy(&json_data);
	SHM_TIMER_STOP(parse_json);
	SHM_TIMER_START_DATA(haversine_calc, "Haversine calculation", sizeof(HaversinePair) * pair_count);
	float64 haversine_avg = 0.0;
    float64 earth_radius = 6372.8;
	for (uint32 i = 0; i < pair_count; i++)
		haversine_avg += haversine_reference(pairs[i], earth_radius);

	haversine_avg /= pair_count;
	SHM_TIMER_STOP(haversine_calc);

	SHM_TIMER_START(end, "Misc. end");
	printf_s("Pair Count: %u\n", pair_count);
	printf_s("Haversine average: %.5f\n\n", haversine_avg);
	SHM_TIMER_STOP(end);

    return true;
}