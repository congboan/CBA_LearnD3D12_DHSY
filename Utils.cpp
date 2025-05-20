#include "Utils.h"
#include <math.h>
#include <algorithm>

float srandom()
{
    float number = float(rand()) / float(RAND_MAX);
    number *= 0.2;
    number -= 1;
    return number;
}
