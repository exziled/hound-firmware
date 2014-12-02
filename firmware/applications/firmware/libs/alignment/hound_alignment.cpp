#include "hound_alignment.h"

#include "stm32f10x_gpio.h"
#include "stm32f10x_adc.h"

// Currently using PA0/ADC_12_IN0
int16_t alignHOUND_currentReference(bool reSample)
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
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0 ;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// Ensure ADC Clock is Enabled
	RCC_ADCCLKConfig (RCC_PCLK2_Div6);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;      // we work in continuous sampling mode
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;

	ADC_RegularChannelConfig(ADC1, ADC_Channel_12, 1, ADC_SampleTime_28Cycles5); // define regular conversion config
	ADC_Init (ADC1, &ADC_InitStructure);   //set config of ADC1
	
	ADC_Cmd (ADC1, ENABLE);  //enable ADC1

	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC1));

	// start conversion
	ADC_Cmd (ADC1, ENABLE);  

	ADC_SoftwareStartConvCmd(ADC1, ENABLE);

	while( ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET )
	{
	    // do nothing (or something useful perhaps)
	}

	reference = ADC_GetConversionValue(ADC1);

	return reference;
}