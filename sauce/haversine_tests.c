#include "haversine_tests.h"
#include "haversine_calc.h"
#include "shm_utils/platform/shm_platform.h"
#include "shm_utils/shm_profiler.h"
#include "shm_utils/shm_repetition_tester.h"
#include "shm_utils/shm_json.h"
#include <math.h>
#include <stdio.h>

static inline float64 _square(float64 x)
{
    return x * x;
}

static inline float64 _deg_to_rad(float64 deg)
{
    return 0.01745329251994329577 * deg;
}

static inline bool8 _float64_eq(float64 a, float64 b)
{
	float64 epsilon = 0.00000001;
	float64 diff = a - b;
	return (diff > -epsilon) && (diff < epsilon);
}

typedef struct HaversineTestContext
{
    uint64 haversine_pair_count;
    HaversinePair* haversine_pairs;
    float64 ref_results_avg;
    float64* ref_results;
    float64* ref_results_file_buffer;
	float64 earth_radius_est;
}
HaversineTestContext;

static bool8 _haversine_test_context_init(const char* json_path, const char* ref_results_file_path, uint32 expected_pair_count, HaversineTestContext* out_context)
{
    HaversinePair* pairs = 0;
	uint64 json_file_size = shm_platform_get_filesize(json_path);
	SHM_FileHandle file_handle = {0};
    float64* ref_results_file_buffer = 0;

	SHM_TIMER_START_DATA(read_json_file, "Read Json File", json_file_size);
	char* json_file_buffer = shm_platform_memory_allocate(json_file_size+1);
    if (!json_file_buffer)
        goto error;

    if (!shm_platform_file_open(json_path, false, &file_handle))
        goto error;

	shm_platform_file_read(file_handle, json_file_buffer, (uint32)json_file_size, 0);
	json_file_buffer[json_file_size] = 0;
	shm_platform_file_close(&file_handle);
	SHM_TIMER_STOP(read_json_file);

	SHM_TIMER_START_DATA(parse_json, "Parse Json", json_file_size);
	SHM_JsonData json_data = {0};
	if (!shm_json_parse_text(json_file_buffer, expected_pair_count * 5 + 1, &json_data))
	{
		printf_s("Error: Failed to parse json file.");
		goto error;
	}
	shm_platform_memory_free(json_file_buffer);

	SHM_JsonNodeId pairs_arr_id = shm_json_get_child_id_by_key(&json_data, 0, "pairs");
	uint32 pair_count = shm_json_get_child_count(&json_data, pairs_arr_id);
    out_context->haversine_pair_count = pair_count;

	SHM_TIMER_START_DATA(read_pairs, "Read Json pairs", sizeof(HaversinePair) * pair_count);
	pairs = shm_platform_memory_allocate(sizeof(HaversinePair) * pair_count);
    if (!pairs)
        goto error;

    out_context->haversine_pairs = pairs;
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

	uint64 ref_results_file_size = shm_platform_get_filesize(ref_results_file_path);
	SHM_TIMER_START_DATA(read_ref_file, "Read Reference Results File", ref_results_file_size);
	ref_results_file_buffer = shm_platform_memory_allocate(ref_results_file_size);
    if (!ref_results_file_buffer)
        goto error;
    out_context->ref_results_file_buffer = ref_results_file_buffer;

    if (!shm_platform_file_open(ref_results_file_path, false, &file_handle))
        goto error;

	shm_platform_file_read(file_handle, ref_results_file_buffer, (uint32)ref_results_file_size, 0);
	shm_platform_file_close(&file_handle);

    uint64 ref_results_count = *((uint64*)ref_results_file_buffer);
    if (ref_results_count != out_context->haversine_pair_count)
    {
		printf_s("Error: Reference haversine sum count does not match count read from json. (Ref: %llu, Read: %llu)\n", ref_results_count, out_context->haversine_pair_count);
		goto error;
    }

    out_context->ref_results = out_context->ref_results_file_buffer + 1;
    out_context->ref_results_avg = out_context->ref_results[ref_results_count];
	out_context->earth_radius_est = 6372.8;

	SHM_TIMER_STOP(read_ref_file);

    return true;

    error:
	shm_platform_file_close(&file_handle);
	shm_platform_memory_free(json_file_buffer);
	shm_platform_memory_free(ref_results_file_buffer);
	shm_platform_memory_free(pairs);
	shm_json_data_destroy(&json_data);

    return false;
}

static void _haversine_test_context_destroy(HaversineTestContext* context)
{
	shm_platform_memory_free(context->haversine_pairs);
	shm_platform_memory_free(context->ref_results_file_buffer);
	*context = (HaversineTestContext){0};
}

typedef float64 (*FP_haversine_dist_func)(HaversinePair coords, float64 radius);
static uint64 _verify_haversine_dist_function(HaversineTestContext context, FP_haversine_dist_func func)
{
	uint64 error_count = 0;
	for (uint64 pair_i = 0; pair_i < context.haversine_pair_count; pair_i++)
	{
		float64 dist = func(context.haversine_pairs[pair_i], context.earth_radius_est);
		if (!_float64_eq(dist, context.ref_results[pair_i]))
			error_count++;
	}

	return error_count;
}

// NOTE: Copy for reliable inlining of slow reference version of haversine distance calculation
static float64 _haversine_dist_reference(HaversinePair coords, float64 radius)
{
    float64 lat1 = coords.y0;
    float64 lat2 = coords.y1;
    float64 lon1 = coords.x0;
    float64 lon2 = coords.x1;

    float64 dlat = _deg_to_rad(lat2 - lat1);
    float64 dlon = _deg_to_rad(lon2 - lon1);
    lat1 = _deg_to_rad(lat1);
    lat2 = _deg_to_rad(lat2);

    float64 a = _square(sin(dlat/2.0)) + cos(lat1) * cos(lat2) * _square(sin(dlon/2.0));
    float64 c = 2.0 * asin(sqrt(a));

    return radius * c;
}

static float64 _test_haversine_dist_reference_avg(uint64 pair_count, HaversinePair* pairs, float64 earth_radius_estimate)
{
	float64 sum = 0.0;
	for (uint64 pair_i = 0; pair_i < pair_count; pair_i++)
		sum += _haversine_dist_reference(pairs[pair_i], earth_radius_estimate);

	return sum / pair_count;
}

bool8 rep_test_haversine_reference(uint64 time_counter_frequency, const char* json_path, const char* ref_results_file_path, uint32 expected_pair_count)
{
    HaversineTestContext context = {0};
    if (!_haversine_test_context_init(json_path, ref_results_file_path, expected_pair_count, &context))
	{
		printf_s("Error: Failed to intialize haversine test_context.");
        return false;
	}

	uint64 haversine_calc_error_count = 0;
	SHM_TIMER_START_DATA(verify_funcs, "Verify haversine dist functions", sizeof(HaversinePair) * context.haversine_pair_count);
	haversine_calc_error_count += _verify_haversine_dist_function(context, _haversine_dist_reference);
	SHM_TIMER_STOP(verify_funcs);
	if (haversine_calc_error_count > 0)
	{
		printf_s("Error: Haversine distance calculation functions result in %llu mismatches.", haversine_calc_error_count);
        return false;
	}

	SHM_RepetitionTester tester = {0};
	shm_repetition_tester_init(time_counter_frequency, 10.0, false, &tester);

	shm_repetition_tester_begin_test(&tester, "Naive Haversine Reference");
	while(shm_repetition_tester_next_run(&tester))
	{
		shm_repetition_test_begin_timer(&tester);
		volatile float64 haversine_avg = _test_haversine_dist_reference_avg(context.haversine_pair_count, context.haversine_pairs, context.earth_radius_est);
		shm_repetition_test_end_timer(&tester);
		shm_repetition_test_add_bytes_processed(&tester, context.haversine_pair_count * sizeof(context.haversine_pairs[0]));
	}

	shm_repetition_tester_print_last_test_results(&tester);

    return true;
}