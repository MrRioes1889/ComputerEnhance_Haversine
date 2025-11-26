#include "haversine.h"
#include <math.h>

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