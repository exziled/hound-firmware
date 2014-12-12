/*!
 * @file hound_wlan.h
 * 
 * @brief HOUND WLAN Control Header
 * 
 * @author Benjamin Carlson
 * @author Blake Bourque
 * 
 * @date November 20, 2014
 * 
 * Manages WiFi connection, status, and any async callbacks.
 * 
 */

#ifndef __HOUND_WLAN_H
#define __HOUND_WLAN_H

// Standard Libraries
#include <stdint.h>
// HOUND Libraries
// ST Libraries
#include "stm32f10x_gpio.h"
#include "stm32f10x_exti.h"
// CC3000 Libraries
#include "misc.h"
#include "netapp.h"


typedef struct {
	uint8_t STARTED;
	uint8_t CONNECTED;
	uint8_t DISCONNECT;
	uint8_t DHCP;
	uint8_t SHUTDOWN;
	uint8_t SMART_CONFIG_START;
	uint8_t SMART_CONFIG_STOP;
	uint8_t SMART_CONFIG_DONE;
} houndWLAN_t;

typedef struct
{
	uint8_t oct[4];
} ipAddr_t;


#define SMART_CONFIG_PORT GPIOB
#define SMART_CONFIG_PIN GPIO_Pin_0
#define SMART_CONFIG_EXTI EXTI_Line0
#define SMART_CONFIG_NVIC EXTI0_IRQn

// WLAN State Machine variable
// Volatile due to interrupts
static volatile houndWLAN_t houndWLAN = {0, 0, 0, 0, 0, 0, 0, 0};


/*!
* @brief Inititilze CC3000 interface
* 
* Initilizes CC3000 SPI/DMA interfaces configures SmartConfig interrupts
* and begins CC3000 operation per TI's standard driver
*
* @returns	 Nothing 
*/
void WLAN_Initialize(void);


void WLAN_On(void);
void WLAN_Off(void);
void WLAN_Connect(void);
void WLAN_Disconnect(void);
void WLAN_Ping(ipAddr_t ipAddress);
void WLAN_Ping_Broadcasts(void);
void WLAN_IPConfig(tNetappIpconfigRetArgs * ipConfig);
uint8_t WLAN_GetStatus(void);

void WLAN_Async_Call(long lEventType, char *data, unsigned char length);
void WLAN_KeepAlive_Loop(void);

void WLAN_Configure(void);
bool WLAN_IsConfiguring(void);
bool WLAN_Clear(void);
void WLAN_SmartConfigHandler(void);
void WLAN_SmartConfigInitializer(GPIO_TypeDef * GPIOx, uint16_t GPIO_Pin, uint32_t EXTI_Line, uint8_t NVIC_IRQChannel);

char *WLAN_Firmware_Patch(unsigned long *length);
char *WLAN_Driver_Patch(unsigned long *length);
char *WLAN_BootLoader_Patch(unsigned long *length);
void Set_NetApp_Timeout(void);
uint32_t SPARK_WLAN_SetNetWatchDog(uint32_t timeOutInuS);

// Get SSID
// Get MAC

#endif	//__HOUND_WLAN_H