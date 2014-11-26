#ifndef __HOUND_ADC_H
#define __HOUND_ADC_H

#include <stdint.h>
#include "stm32f10x_gpio.h"
#include "hound_rms_fixed.h"
#include "hound_fixed.h"

#define BLOCKSIZE 200

typedef struct
{
	// Generic Storage
	fixed_t * currentBuffer;
	fixed_t * voltageBuffer;
	AggregatedRMS * rmsResults;
	uint16_t bufferSize;
	uint16_t sampleCount;
	// Voltage Selection Details
	GPIO_TypeDef* voltageCSPort;
	uint8_t voltageCSPin;
	// Current Selection Details
	GPIO_TypeDef* currentCSPort;
	uint8_t currentCSPin;
	uint16_t currentSPIAlt;
} sampleSetup_t;

extern "C"
{
void initADCSPI();
void initTIM3(void);
void initSWInt(void);

int getSampleBlock(volatile sampleSetup_t * sampleSetup);
}

#endif //__HOUND_ADC_H
