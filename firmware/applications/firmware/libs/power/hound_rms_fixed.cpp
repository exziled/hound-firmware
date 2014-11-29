/**
 * @file
 *
 * @brief Fixed Point RMS Calculation
 *
 * @author Benjamin Carlson
 * @author Blake Bourque
 *
 * @date Aug 20, 2014
 *
 * Implementation of functions supporting Fixed Point (Q31 (15.16))
 * calcuation of a sampled signal's root-mean-square (RMS) value.  Code is
 * targeted for Cortex-M3 based ARM microcontrollers without hardware float
 * support but with hardware division (unlike Cortex-M0).
 */

#include "hound_rms_fixed.h"

#include <cstddef>
#include <cstdlib>
#include <string.h>
#include "stm32f10x_gpio.h"


AggregatedRMS::AggregatedRMS(uint8_t buffSize)
{
    results = NULL;
    results = (rmsValues_t *)calloc(buffSize, sizeof(rmsValues_t));

    size = buffSize;

    if (results == NULL)
    {
        // Error, what do we even do?
    }

    head = 0;
}

AggregatedRMS::~AggregatedRMS()
{
    free(results);
    results = NULL;
}

// This is borken, memset doesn't work like that
void AggregatedRMS::pushBack(rmsValues_t * result)
{
    // if (++head - results > size)
    // {
    //     head = results;
    // }

    // memset(head, *result, sizeof(fixed_t));
}

// Same operation as pushback
rmsValues_t * AggregatedRMS::getBack()
{
    //return results;

    if (++head >= size)
    {
        head = 0;
    }

   return results + head;
}

int AggregatedRMS::getSize(void)
{
    return size;
}

//rmsValues_t * operator[](int i)
rmsValues_t * AggregatedRMS::getAt(int i)
{
    // Shortcut to return last calculated value
    if (i < 0)
    {
        return results;
    }

    if (i > size)
    {
        return results;
    }

    if (i <= head)
    {
        return results + head - i;
    } else 
    {
        //return results;
        return results + size - (i - head);
    }

    //return results + i;
}

void calc_rms(fixed_t * samples, int blocksize, fixed_t * result)
{
    uint32_t fixed_unroll = blocksize >> 2;
    fixed_t * sample = samples;
    fixed_t temp = 0;
    int64_t sum = 0;

    for(int i = 0; i < fixed_unroll; i++)
    {
        int mult = i * 4;
        sum += (int64_t)sample[0 + mult] * (int64_t)sample[0 + mult];
        sum += (int64_t)sample[1 + mult] * (int64_t)sample[1 + mult];
        sum += (int64_t)sample[2 + mult] * (int64_t)sample[2 + mult];
        sum += (int64_t)sample[3 + mult] * (int64_t)sample[3 + mult];
    }

    for(int i = fixed_unroll; i < fixed_unroll + fixed_unroll % 4; i++)
    {
        sum += (int64_t)sample[i] * (int64_t)sample[i];
    }

    // Scale back down to QX.FIXED_FRAC
    temp = (fixed_t)(sum >> FIXED_FRAC);

    // Average The Sum
    temp = fixed_div(temp, fixed(blocksize));

    // Square Root
    *result = fixed_sqrt(temp, 10);

    return;
}
