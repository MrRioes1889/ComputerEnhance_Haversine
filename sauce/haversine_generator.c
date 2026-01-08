#include "haversine_generator.h"
#include "shm_utils/platform/shm_platform.h"
#include "shm_utils/shm_string.h"
#include "haversine_calc.h"
#include <stdio.h>
#include <malloc.h>

static uint32 _rand_seed = 0;
static uint32 _pcg_hash()
{
    uint32 state = _rand_seed * 747796405u + 2891336453u;
    uint32 word = ((state >> ((state >> 28u) + 4u)) ^ state);
    return (word >> 22u) ^ word;
}

static uint32 rand_uint32()
{
    _rand_seed = _pcg_hash();
    return _rand_seed;
}

static float64 _generate_random_x()
{
    uint64 r = (uint32)rand_uint32();
    r <<= 32;
    r |= (uint32)rand_uint32();
    r %= 3600000000000000000;
    float64 ret = r / 10000000000000000.0; 
    ret -= 180.0;
    return ret;
}

static float64 _generate_random_y()
{
    uint64 r = (uint32)rand_uint32();
    r <<= 32;
    r |= (uint32)rand_uint32();
    r %= 1800000000000000000;
    float64 ret = r / 10000000000000000.0; 
    ret -= 90.0;
    return ret;
}

void generate_haversine_test_json(const char* json_filepath, const char* results_filepath, uint32 coord_pair_count, uint32 seed)
{
    SHM_FileHandle file = {0};
    shm_platform_file_create(json_filepath, true, false, &file);

    _rand_seed = seed;
    char json_start[] = "{\n\t\"pairs\": [\n";
    shm_platform_file_append(file, json_start, array_count(json_start) - 1);
    float64 haversine_sum = 0.0;
    uint64 haversine_results_size = sizeof(haversine_sum) + (sizeof(float64) * (coord_pair_count + 1));
    float64* haversine_results = malloc(haversine_results_size);
    *((uint64*)haversine_results) = coord_pair_count;

    float64 earth_radius = 6372.8;
    SHM_String lines_s = {0};
    shm_string_init(128 * coord_pair_count, &lines_s);
    for (uint32 i = 0; i < coord_pair_count; i++)
    {
        bool8 last_pair = i == (coord_pair_count - 1);
        HaversinePair coords = {0};
        coords.x0 = _generate_random_x();
        coords.y0 = _generate_random_y();
        coords.x1 = _generate_random_x();
        coords.y1 = _generate_random_y();
        int line_length = sprintf_s(&lines_s.buffer[lines_s.length], 256, "\t\t{\"x0\":%.16lf, \"y0\":%.16lf, \"x1\":%.16lf, \"y1\":%.16lf}%s\n",
             coords.x0, coords.y0, coords.x1, coords.y1, last_pair ? "" : ",");
        lines_s.length += line_length;
        
        float64 haversine_res = haversine_reference(coords, earth_radius);
        haversine_results[i+1] = haversine_res;
        haversine_sum += haversine_res;
    }

    shm_platform_file_append(file, lines_s.buffer, (uint32)lines_s.length);
    char json_end[] = "\t]\n}";
    shm_platform_file_append(file, json_end, array_count(json_end) - 1);

    float64 haversine_average = haversine_sum / coord_pair_count;
    haversine_results[coord_pair_count+1] = haversine_average;
    printf_s("Expected haversine average: %.5lf\n", haversine_average);

    shm_platform_file_close(&file);

    SHM_FileHandle results_file = {0};
    shm_platform_file_create(results_filepath, true, false, &results_file);
    shm_platform_file_append(results_file, haversine_results, (uint32)haversine_results_size);
    shm_platform_file_close(&results_file);
}