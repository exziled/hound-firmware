/*!
 * @file hound_adc.h
 * 
 * @brief HOUND ADC Setup and Sampling Header
 * 
 * @author Benjamin Carlson
 * @author Blake Bourque
 * 
 * @date August 27, 2014
 * 
 *
 * Defines structures and functions to manage low level sampling interfaces
 * and interrupts associated with Spark Core development platform and 
 * STM32F10xx series of ARM microcontrollers.
 * 
 */

#ifndef __HOUND_ADC_H
#define __HOUND_ADC_H

// Standard Libraries
#include <stdint.h>

// HOUND Libraries
#include "hound_rms_fixed.h"
#include "hound_fixed.h"

// ST Libraries
#include "stm32f10x_gpio.h"


// Defines Sampling Blocksize
#define BLOCKSIZE 200


/*!
 * @brief Structure for storing sampling confiuration settings
 * 
 * Includes fields for output buffers and CS/data settings for
 * SPI communcation.
 */
typedef struct
{
	// Generic Storage
	fixed_t * currentBuffer;
	fixed_t * voltageBuffer;
	AggregatedRMS * rmsResults;
	uint16_t bufferSize;
	uint16_t sampleCount;
	uint8_t socket_id;
	// Voltage Selection Details
	GPIO_TypeDef* voltageCSPort;
	uint8_t voltageCSPin;			// PinSource, not Pin BitMask (i.e Pin 2 = 0x02 not 1 << 0x02)
	// Current Selection Details
	GPIO_TypeDef* currentCSPort;
	uint8_t currentCSPin;			// PinSource, not Pin BitMask (i.e Pin 2 = 0x02 not 1 << 0x02)
	uint16_t currentSPIAlt;			// Data to send with current request
} sampleSetup_t;

extern "C"
{

/*!
 * @brief Initialize ADC SPI Interface
 * 
 * Uses SPI1 Interface to manage sampling.
 *		- PA7 - MISO
 *		- PA6 - MOSI
 * 		- PA5 - SCLK
 *		- XXX - CS
 			* Chip select chosen as part of socket_map.h
 * 
 * Other relevant settings:
 * 	- 16 Bit Data Size
 *  - Clock Starts High
 *  - Data Read of Rising Edge
 *  - /32 Prescaler on @TODO
 * 
 * Also initializes sample timer (initTIM3())
 *
 * @returns	 Nothing 
 */
void initADCSPI();

/*!
 * @brief Initialize TIM3 for Sample Capture Management
 * 
 * Configures TIM3 to generate interrupts, allowing BLOCKSIZE
 * samples to be captured in 1/60 seconds or one period of a 60Hz
 * AC wave.
 *
 * Note, currently function is not dynamic and is configured for 200 samples
 * in 1/60 seconds.  @TODO: Make timer interrupts dynamic.
 *
 * @returns	 Nothing 
 */
void initTIM3(void);


/*!
 * @brief Configure sampling for a new block of data.
 * 
 * Given a sampleSetup_t struct, configure TIM3 interrupt vector with appropriate
 * storage buffers and SPI chip select lines.
 *
 * 
 * @param[in] ptr   Pointer to volatile sampleSetup struct with configuration
 * 
 * @returns	 		0 if sample job accecpted or -1 if another sample is currently active
 */
int getSampleBlock(volatile sampleSetup_t * sampleSetup);
}

#endif //__HOUND_ADC_H
