/*!
 * @file com_strings.cpp
 * 
 * @brief HOUND Communication String Generation Functions
 * 
 * @author Benjamin Carlson
 * @author Blake Bourque
 * 
 * @date October 15, 2014
 * 
 * Functional implementation of lightweight string generation libraries, commonly used with CC3000
 * communication functions defined in @com_command.h
 * 
 */

#include "com_strings.h"

// Standard Libraries
#include <stdio.h>
// HOUND_Libraries
// ST Libraries


int Communication::strcat(char * strDest, uint16_t buffSize, uint32_t concat, char delim)
{
	return snprintf(strDest, buffSize, "%ld%c", concat, delim);
}

int Communication::strcat(char * strDest, uint16_t buffSize, fixed_t concat, char delim)
{
	int temp, powers_10;

	temp = fixed_integer(concat);
	powers_10 = integerPlaces(temp);

    int i, j, factor;

    for (i = 0; i < powers_10; i++)
    {
        factor = integerPower(10, (powers_10 - i -1));
        strDest[i] = 48 + (temp / factor);
        temp -= factor * (temp / factor);
    }

    strDest[i++] = '.';

    temp = fixed_fractional(concat);
    //powers_10 = integerPlaces(temp);
    // Only care about 3 decimal places
    powers_10 = 3;

    for (j = 0; j < powers_10; j++)
    {
    	// We always expect 3 decimal places, deal 
    	if (j == 0 && powers_10 < 3)
    	{
			strDest[i+j] = 48;
			i++;
    	}

    	factor = integerPower(10, (powers_10 - j -1));
    	strDest[i + j] = 48 + (temp / factor);
    	temp -= factor * (temp / factor);
    }

    if (delim != '\0')
    {
   		strDest[i + j++] = delim;
    }

	return i + j;
}

int Communication::strcat(char * strDest, uint16_t buffSize, char * strSrc)
{
	int srcSize = strlen(strSrc);

	//if (srcSize < buffSize)
	//{
		//::strcat(strDest-1, strSrc);

		return snprintf(strDest, buffSize, strSrc);
	//}

	//return 0;
}

int Communication::integerPower(int base, int exp)
{
    int result = 1;
    while (exp)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        base *= base;
    }

    return result;
}

int Communication::integerPlaces (int n) 
{
    if (n < 0) return 0;
    if (n < 10) return 1;
    if (n < 100) return 2;
    if (n < 1000) return 3;
    if (n < 10000) return 4;
    if (n < 100000) return 5;
    if (n < 1000000) return 6;
    if (n < 10000000) return 7;
    if (n < 100000000) return 8;
    if (n < 1000000000) return 9;
    return 10;
}