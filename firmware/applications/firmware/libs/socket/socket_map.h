/*!
 * @file socket_map.h
 * 
 * @brief HOUND Socket Mapping
 * 
 * @author Benjamin Carlson
 * @author Blake Bourque
 * 
 * @date November 20, 2014
 * 
 * Defines structures and variables for storage of abstracted socket 
 * configuration.
 * 
 */

#ifndef __HOUND_SOCKET_MAP
#define __HOUND_SOCKET_MAP

// Standard Libraries
#include <cstddef>
// HOUND Libraries
#include "hound_def.h"
#include "hound_rms_fixed.h"
// ST Libraries
#include "stm32f10x_gpio.h"

/*!
 * @brief Storage of related socket confurations
 * 
 * Stores individual socket's CS pins for voltage and current,
 * any data that must be sent during SPI communcation, and the output
 * buffer where collected samples should be stored.
 */
typedef struct {
	GPIO_TypeDef * currentCSPort;
	uint8_t currentCSPin;			// PinSource, not Pin BitMask (i.e Pin 2 = 0x02 not 1 << 0x02)
	uint16_t currentSPIAlt;
	GPIO_TypeDef * voltageCSPort;
	uint8_t voltageCSPin;			// PinSource, not Pin BitMask (i.e Pin 2 = 0x02 not 1 << 0x02)
	GPIO_TypeDef * controlPort;
	uint8_t controlPin;
	AggregatedRMS * rmsResults;		// Storage of calculated values
} socketMap_t;


#ifdef NEW_SAMPLE_BOARD
	#define SOCKET_COUNT 2
	// Actual Socket Map.  Can not be const as initilization function sets storage buffer.
	static volatile socketMap_t socketMap[] = {
	//	|----Current CS -------|--------|------Voltage CS-------|------Control Pin -----| --- Storage --- |
		{GPIOB, GPIO_PinSource4, 0x1000, GPIOA, GPIO_PinSource3, GPIOA, GPIO_PinSource14, NULL},
		{GPIOA, GPIO_PinSource4, 0x1000, GPIOA, GPIO_PinSource3, GPIOA, GPIO_PinSource13, NULL}
	};
#else 
	#define SOCKET_COUNT 2

	static volatile socketMap_t socketMap[] = {
	//	|----Current CS -------|--------|------Voltage CS-------|------Control Pin -----| --- Storage --- |
		{GPIOA, GPIO_PinSource4, 0, GPIOA, GPIO_PinSource4, GPIOB, GPIO_PinSource0},
		{GPIOA, GPIO_PinSource4, 0, GPIOA, GPIO_PinSource4, GPIOB, GPIO_PinSource1}
	};
#endif

#endif //__HOUND_SOCKET_MAP