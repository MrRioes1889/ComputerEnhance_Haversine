#pragma once
#include "defines.h"

typedef struct
{
    float64 x0, y0;
    float64 x1, y1;
}
HaversinePair;

float64 haversine_reference(HaversinePair coords, float64 radius);
bool8 haversine_calculate_average_from_json(const char* json_path, const char* results_cmp_path, uint32 expected_pair_count);

