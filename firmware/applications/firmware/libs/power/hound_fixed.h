#ifndef __HOUND_FIXED_H
#define __HOUND_FIXED_H

#include <stdint.h>

typedef int32_t fixed_t;

#define FIXED_FRAC 16
#define FIXED_FRAC_MASK (2^FIXED_FRAC) -1

extern "C"
{
     /**
      * Fixed Point Addtion
      * Performs simple fixed point addition requring no additional bit
      * manipulations
      * @param a addition variable
      * @param b addition variable
      */
     fixed_t fixed_add(fixed_t a, fixed_t b);

     /**
      * Fixed Point Subtraction
      * Performs simple fixed point subtraction requiring no additional bit
      * manipulations
      * @param a subtraction variable
      * @param b subtraction variable
      */
     fixed_t fixed_sub(fixed_t a, fixed_t b);

     /**
      * Fixed Point Multiplication
      * Performs fixed point multiplication of two Q32 format numbers resulting
      * in a Q64 product.  Result is shifted by FIXED_FRAC and upper bits truncated
      * resulting in Q32 result
      * @param a multiplication variable
      * @param b multiplication variable
      */
     fixed_t fixed_mul(fixed_t a, fixed_t b);

     /**
      * Fixed Point Division
      * Performs fixed point division of two Q32 format numbers.
      * @param a dividend
      * @param b divisor
      */
     fixed_t fixed_div(fixed_t a, fixed_t b);

     /**
      * Fixed Point Square Root Approximation
      * Approximates square root of fixed point input using Newton-Raphson method.
      * Default of 10 iterations are performed with any integer value specifiable.
      * @param a number to take square root of
      * @param iterations numer of approximation iterations to perform
      */
     fixed_t fixed_sqrt(fixed_t a, int iterations);

     /**
      * Generate fixed_t format number from integer
      * @param a integer input
      */
     fixed_t fixed(int32_t a);

     uint16_t fixed_integer(fixed_t a);

     uint16_t fixed_fractional(fixed_t a);

     /**
      * Convert fixed_t format number to float representation
      * @param a fixed_t to convert to float
      */
     float float_from_fixed(fixed_t a);

     /**
      * Convert fixed_t format number to int representation
      * Equivalent to floor of float representation
      * @param a fixed_t to convert to int
      */
     int16_t int_from_fixed(fixed_t a);
}

#endif // __HOUND_FIXED
