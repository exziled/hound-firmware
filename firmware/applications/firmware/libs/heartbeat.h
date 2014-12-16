/*!
 * @file heartbeat.h
 * 
 * @brief HOUND Heartbeat Library
 * 
 * @author Benjamin Carlson
 * @author Blake Bourque
 * 
 * @date December 6, 2014
 * 
 * Functional definitions for HOUND heartbeat functions.  Alerts the user if the board
 * ceases operation.
 */

#ifndef __HOUND_HEARTBEAT_H
#define __HOUND_HEARTBEAT_H

#include "stm32f10x_gpio.h"

#define HEARTBEAT_PIN GPIO_Pin_3
#define HEARTBEAT_PORT GPIOB

/*!
* @brief Initialize Heartbeat
* 
* Configures pins and outputs
*
* @param [in] 	ptr		GPIOx 		Port heartbeat should appear on
* @param [in]	int 	GPIO_Pin 	Pin heartbeat should appear on
*
* @returns	 Nothing 
*/
void heartbeat_initialize(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);

/*!
* @brief Do Heartbeat
* 
* Pin output = ! pin output
*
* @param [in] 	ptr		GPIOx 		Port heartbeat should appear on
* @param [in]	int 	GPIO_Pin 	Pin heartbeat should appear on
*
* @returns	 Nothing 
*/
void heartbeat_beat(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);

#endif //__HOUND_HEARTBEAT_H