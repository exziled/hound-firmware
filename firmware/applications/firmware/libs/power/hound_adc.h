#ifndef __HOUND_ADC_H
#define __HOUND_ADC_H

#include <stdint.h>
#include "stm32f10x_gpio.h"
#include "hound_rms_fixed.h"
#include "hound_fixed.h"

#define BLOCKSIZE 400

typedef struct
{
	fixed_t * currentBuffer;
	fixed_t * voltageBuffer;
	AggregatedRMS * rmsResults;
	uint16_t bufferSize;
	uint16_t sampleCount;
	GPIO_TypeDef* csPort;
	uint16_t csPin;
} sampleSetup_t;

extern "C"
{
void initADCSPI();
void initTIM3(void);
void initSWInt(void);

int getSampleBlock(volatile sampleSetup_t * sampleSetup);
}

#endif //__HOUND_ADC_H
