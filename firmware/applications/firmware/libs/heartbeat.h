#ifndef __HOUND_HEARTBEAT_H
#define __HOUND_HEARTBEAT_H

#include "stm32f10x_gpio.h"

#define HEARTBEAT_PIN GPIO_Pin_3
#define HEARTBEAT_PORT GPIOB

void heartbeat_initialize(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void heartbeat_beat(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);

#endif //__HOUND_HEARTBEAT_H