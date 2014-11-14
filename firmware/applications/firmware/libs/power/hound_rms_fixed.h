/**
 * @file
 *
 * @brief Fixed Point RMS Calculation
 *
 * @author Benjamin Carlson
 *
 * @date Sept 7, 2014
 *
 * Function headers supporting Fixed Point (Q31)
 * calcuation of a sampled signal's root-mean-square (RMS) value.  Code is
 * targeted for Cortex-M3 based ARM microcontrollers without hardware float
 * support but with hardware division (unlike Cortex-M0).
 */
#ifndef __HOUND_RMSFIXED_H
#define __HOUND_RMSFIXED_H

#include <stdint.h>
#include "hound_fixed.h"

typedef struct {
	fixed_t voltage;
	fixed_t current;
	fixed_t apparent;
	fixed_t real;
	uint32_t timestamp;
} rmsValues_t;

class AggregatedRMS {
	public:
		AggregatedRMS(uint8_t buffSize);
		~AggregatedRMS();

		void pushBack(fixed_t * result);
		rmsValues_t * getBack();
		
		//rmsValues_t * operator[](int i);
		rmsValues_t * getAt(int i);
		int getSize(void);

	private:
		rmsValues_t * results;
		uint8_t head;
		uint8_t size;
};

extern "C"
{
     /**
      * Calcuate RMS of given fixed_t format array
      * @param fixed_rms rms struct containing input array, element count, output variable
      */
     void calc_rms(fixed_t * samples, int blocksize, fixed_t * result);
 }


#endif // __HOUND_FIXED
