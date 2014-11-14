#include "hound_fixed.h"

fixed_t fixed_add(fixed_t a, fixed_t b)
{
    return a + b;
}

fixed_t fixed_sub(fixed_t a, fixed_t b)
{
    return a - b;
}

fixed_t fixed_mul(fixed_t a, fixed_t b)
{
    int64_t temp;
    temp = (int64_t)a * (int64_t)b;

    return temp >> FIXED_FRAC;
}


fixed_t fixed_div(fixed_t a, fixed_t b)
{
    return (fixed_t)((((int64_t)a) << FIXED_FRAC) / ((int64_t)b));
}


fixed_t fixed_sqrt(fixed_t a, int iterations)
{
    static fixed_t previous = fixed(1);
    fixed_t temp;

    for (int i = 0; i < iterations; i++)
    {
        temp = fixed_div(a, previous);
        temp = fixed_add(temp, previous);
        previous = fixed_div(temp, fixed(2));
    }

    temp = previous;
    previous = fixed(1);
    return temp;
}

uint16_t fixed_integer(fixed_t a)
{
    return (uint16_t)(a >> FIXED_FRAC);
}


uint16_t fixed_fractional(fixed_t a)
{
    a = fixed_mul((fixed_t)(a & 0xFFFF), fixed(1000));

    return fixed_integer(a);
}


fixed_t fixed(int32_t a)
{
    return (fixed_t) (a << FIXED_FRAC);
}

float float_from_fixed(fixed_t a)
{
    return (float)a / (1 << FIXED_FRAC);
}

int16_t int_from_fixed(fixed_t a)
{
    return (int16_t)(a >> FIXED_FRAC);
}
