/**
 * @file
 *
 * @brief Fixed Point RMS Calculation
 *
 * @author Benjamin Carlson
 * @author Blake Bourque
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

// Standard Libraries
#include <stdint.h>
// HOUND Libraries
#include "hound_fixed.h"
// ST Libraries

/*!
 * @brief Structure for storing related calculated values
 * 
 * Stores associated RMS Voltage and Current, Apparant, Real
 * and Power Factor values along with a sampled timestamp.
 *
 */
typedef struct {
	fixed_t voltage;
	fixed_t current;
	fixed_t apparent;
	fixed_t real;
	fixed_t pf;
	uint32_t timestamp;
} rmsValues_t;


/**
 *  Class managaing storage of rmsValues_t structures
 *  while maintaining relative sample time among each other
 */
class AggregatedRMS {
	public:

		AggregatedRMS(uint8_t buffSize);
		~AggregatedRMS();

		 /* @brief Get next buffer and store values there
		 *
		 * @param[in] ptr    Pointer to new rmsValues_t struct to store in buffer
		 * 
		 * @returns	 Nothing
		 */
		void pushBack(rmsValues_t * result);

		 /* @brief Get pointer to next buffer
		 * 
		 * @returns	 Pointer to next rmsValues_t structure buffer
		 */
		rmsValues_t * getBack();
		
		/* @brief Get pointer to a buffer
		 * 
		 * @param[in] int 	Index of rmsValues_t buffer to return
		 *
		 * @returns	 Pointer to selected rmsValues_t structure buffer
		 */
		rmsValues_t * getAt(int i);

		/* @brief Get total number of storage buffers
		 * 
		 * @returns Total number of storage buffers
		 */
		int getSize(void);

	private:
		// Circular buffer base pointer
		rmsValues_t * results;

		// Circular buffer data values, maximum size 255
		uint8_t head;
		uint8_t size;
};

extern "C"
{
     /**
      * Calcuate RMS of given fixed_t format array
      * 
      * @param ptr[in] 			Array containing ADC samples corrected for actual voltage/current values
      * @param int[in]			Size of sample array
      * @param ptr[out] 		Pointer to store results
      *
      * @returns Nothing
      */
     void calc_rms(fixed_t * samples, int blocksize, fixed_t * result);
 }


#endif // __HOUND_FIXED
