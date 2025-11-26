#pragma once
#include "defines.h"

typedef struct
{
    float64 x0, y0;
    float64 x1, y1;
}
HaversinePair;

float64 haversine_reference(HaversinePair coords, float64 radius);

