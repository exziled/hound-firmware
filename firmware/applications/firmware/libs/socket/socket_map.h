#ifndef __HOUND_SOCKET_MAP
#define __HOUND_SOCKET_MAP

#include "stm32f10x_gpio.h"

typedef struct {
	GPIO_TypeDef * currentCSPort;
	uint8_t currentCSPin;			// GPIO_PinSource
	uint8_t currentSPIAlt;
	GPIO_TypeDef * voltageCSPort;
	uint8_t voltageCSPin;			// GPIO_PinSource
	GPIO_TypeDef * controlPort;
	uint8_t controlPin;
} socketMap_t;

static const socketMap_t socketMap[] = {
//	|----Current CS -------|--|------Voltage CS-------|------Control Pin -----|
	{GPIOA, GPIO_PinSource4, 0, GPIOA, GPIO_PinSource4, GPIOB, GPIO_PinSource0},
	{GPIOA, GPIO_PinSource4, 0, GPIOA, GPIO_PinSource4, GPIOB, GPIO_PinSource1}
};

#endif //__HOUND_SOCKET_MAP