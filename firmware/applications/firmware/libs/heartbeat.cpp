#include "heartbeat.h"

static bool heartbeat_state = 0;

// Assumes RCC clock already enabled on choosen pin
void heartbeat_initialize(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
	// Configure GPIO Pins for SPI Output
	GPIO_InitTypeDef pinInit;

    pinInit.GPIO_Pin = GPIO_Pin;
    pinInit.GPIO_Speed = GPIO_Speed_50MHz;
    pinInit.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOx, &pinInit);
}

void heartbeat_beat(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
	if (heartbeat_state)
	{
		GPIO_ResetBits(GPIOx, GPIO_Pin);
	} else 
	{
		GPIO_SetBits(GPIOx, GPIO_Pin);
	}

	heartbeat_state = !heartbeat_state;
}