#include "haversine_generator.h"
#include "platform/platform.h"
#include "utility/shm_string.h"
#include <stdio.h>

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

void generate_haversine_test_json(const char* output_filepath, uint32 coord_pair_count, uint32 seed)
{
    SHM_FileHandle file = platform_file_create(output_filepath, true);

    _rand_seed = seed;
    char json_start[] = "{\n\t\"pairs\": [\n";
    platform_file_append(file, json_start, array_count(json_start) - 1);

    char line_buf[256] = {0};
    for (uint32 i = 0; i < coord_pair_count; i++)
    {
        bool8 last_pair = i == (coord_pair_count - 1);
        float64 x0 = _generate_random_x();
        float64 y0 = _generate_random_y();
        float64 x1 = _generate_random_x();
        float64 y1 = _generate_random_y();
        sprintf_s(line_buf, 256, "\t\t{\"x0\":%.16lf, \"y0\":%.16lf, \"x1\":%.16lf, \"y1\":%.16lf}%s\n", x0, y0, x1, y1, last_pair ? "" : ",");
        platform_file_append(file, line_buf, cstring_length(line_buf));
    }

    char json_end[] = "\t]\n}";
    platform_file_append(file, json_end, array_count(json_end) - 1);

    platform_file_close(&file);
}