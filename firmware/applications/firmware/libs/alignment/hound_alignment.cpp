#include "hound_alignment.h"
#include "socket_map.h"

#include "stm32f10x_gpio.h"
#include "stm32f10x_adc.h"


static int16_t socketOffset[SOCKET_COUNT];

// Currently using PA0/ADC_12_IN0
int16_t alignHOUND_currentReference(uint32_t bits, bool reSample)
{
	static int16_t reference = -1;

	if (reference != -1 && !reSample)
	{
		return reference;
	}

	// Initialize PA0 for analog input
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;

	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_1 ;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	// Ensure ADC Clock is Enabled
	RCC_ADCCLKConfig (RCC_PCLK2_Div6);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);

	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;      // we work in continuous sampling mode
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;

	ADC_RegularChannelConfig(ADC2, ADC_Channel_9, 1, ADC_SampleTime_28Cycles5); // define regular conversion config
	ADC_Init (ADC2, &ADC_InitStructure);   //set config of ADC2
	
	ADC_Cmd (ADC2, ENABLE);  //enable ADC2

	ADC_ResetCalibration(ADC2);
	while(ADC_GetResetCalibrationStatus(ADC2));
	ADC_StartCalibration(ADC2);
	while(ADC_GetCalibrationStatus(ADC2));

	// start conversion
	ADC_Cmd (ADC2, ENABLE);  

	ADC_SoftwareStartConvCmd(ADC2, ENABLE);

	while( ADC_GetFlagStatus(ADC2, ADC_FLAG_EOC) == RESET )
	{
	    // do nothing (or something useful perhaps)
	}

	reference = ADC_GetConversionValue(ADC2);

	// ADC is 12 Bits, normalize if requested
	if (bits != 4095)
	{
		reference = (reference * bits)/4095;
	}

	return reference;
}

int16_t alignHOUND_HallEffectGetOffset(uint8_t socket_id)
{
	// Return currently measured offset if no data buffer is given
	if (socket_id < SOCKET_COUNT)
	{
		return socketOffset[socket_id];
	}
}

// fixed_t alignHOUND_HallEffectSetOffset(uint8_t socket_id, fixed_t * currentBuffer, int16_t bufferSize)
// {

// 	uint64_t sum = 0;

// 	for(int i = 0; i < bufferSize; i++)
// 	{
// 		sum += currentBuffer[i];
// 	}

// 	fixed_t avg = (fixed_t)(sum >> FIXED_FRAC);

// 	avg = fixed_div(avg, fixed(bufferSize));

// 	return avg;
// }

fixed_t alignHOUND_HallEffectSetOffset(uint8_t socket_id, fixed_t * currentBuffer, int16_t bufferSize)
{
     fixed_t x, y, a, b;
     fixed_t sy = 0,
            sxy = 0,
            sxx = 0;
     int i;

     x = fixed_div(bufferSize * -1, fixed(2));
     fixed_t temp = fixed(1);
     temp = fixed_div(temp, 2);
     x = x + temp;

     for (i=0; i<bufferSize; i++, x+=1.0)
     {
     	y = currentBuffer[i];
     	sy += y;
     	sxy += fixed_mul(x, y);
     	sxx += fixed_mul(x, x);
     }

     b = fixed_div(sxy, sxx);
     a = fixed_div(sy, fixed(bufferSize));

     return (a + fixed_mul(b, x));
}