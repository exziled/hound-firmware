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

/*!
* @brief Activate CC3000
* 
* Call activate functions from TI CC3000 library and update user facing
* identification.
*
* @returns	 Nothing 
*/
void WLAN_On(void);

/*!
* @brief Deactivate CC3000
* 
* Call deactivate functions from TI CC3000 library and update user facing
* identification.
*
* @returns	 Nothing 
*/
void WLAN_Off(void);

/*!
* @brief Enable CC3000 WiFi Connection
* 
* Allow CC3000 chip to assoicate with any access point within range it is
* aware of.
*
* @returns	 Nothing 
*/
void WLAN_Connect(void);

/*!
* @brief Disable CC3000 WiFi Connection
* 
* Disconnect CC3000 chip from any assoicated access points.
*
* @returns	 Nothing 
*/
void WLAN_Disconnect(void);

/*!
* @brief Ping a Remote IP Address
* 
* 2 attempts, 64 bytes, 10 msec timeout
*
* @param [in] 	ipAddr_t 	IP Address to ping
*
* @returns	 Nothing 
*/
void WLAN_Ping(ipAddr_t ipAddress);

/*!
* @brief Automated Ping Utility
* 
* Due to a bug in the CC3000 chip, prior to sending UDP Multicast packets,
* some sort of non-multicast connection must be made.  This utility will automatically
* ping the default gateway as configured by the DHCP Server, allowing UDP Multicast to
* function after.
*
* @returns	 Nothing 
*/
void WLAN_Ping_Broadcasts(void);

/*!
* @brief Retrieve IP Configuration Information
* 
* @param 	[in|out] 	ptr		Pointer to CC3000 spec ip config struct
*
* @returns	 Nothing 
*/
void WLAN_IPConfig(tNetappIpconfigRetArgs * ipConfig);

/*!
* @brief Get Network Status
* 
* Gets status from CC3000 chip, as defined by TI
*
* 0x0 -
* 0x1 - 
* 0x2 - 
* 0x4 - 
* 
* @returns	 uint8_t status code as defined above 
*/
uint8_t WLAN_GetStatus(void);

/*!
* @brief CC3000 Async Callback Interrupt
* 
* Respond to CC3000 events generated via interrupt
*
* @param	[in] 	long	Event type
* @param 	[in]	ptr		Data generated from interrupt
* @param	[in]	char 	Length of generated data
*
* @returns	 Nothing 
*/
void WLAN_Async_Call(long lEventType, char *data, unsigned char length);

/*!
* @brief CC3000 Book Keeping
* 
* Called at specified interval to handle CC3000 events such as smart config.
*
* @returns	 Nothing 
*/
void WLAN_KeepAlive_Loop(void);

/*!
* @brief Trigger CC3000 SmartConfig Process
* 
* Updates SmartConfig state machine with start bit.
*
* @returns	 Nothing 
*/
void WLAN_Configure(void);

/*!
* @brief Check CC3000 Smart Config State
* 
* Updates Determine if SmartConfig has completed
*
* @returns	 bool 		true if still configuring, false otherwise 
*/
bool WLAN_IsConfiguring(void);

/*!
* @brief Clear CC3000 Stored Networks
* 
* Removes all stored networks for CC3000 NVRAM, does not disconnect
* from any currently connected AP
*
* @returns	 Nothing 
*/
bool WLAN_Clear(void);

/*!
* @brief Handle SmartConfig Credentials
* 
* Communicates with CC3000 to store credientials after a sucessfully completed
* smart config process.
*
* @returns	 Nothing 
*/
void WLAN_SmartConfigHandler(void);

/*!
* @brief SmartConfig Start Interrupt
* 
* Configures Port/Pin with which a button press should begin the smart config process.
*
* @returns	 Nothing 
*/
void WLAN_SmartConfigInitializer(GPIO_TypeDef * GPIOx, uint16_t GPIO_Pin, uint32_t EXTI_Line, uint8_t NVIC_IRQChannel);


/* Empty Functions for CC3000 */
char *WLAN_Firmware_Patch(unsigned long *length);
char *WLAN_Driver_Patch(unsigned long *length);
char *WLAN_BootLoader_Patch(unsigned long *length);


void Set_NetApp_Timeout(void);
void Clear_NetApp_Dhcp(void);
uint32_t SPARK_WLAN_SetNetWatchDog(uint32_t timeOutInuS);

// Get SSID
// Get MAC

#endif	//__HOUND_WLAN_H