/**
 ******************************************************************************
 * @file    main.cpp
 * @author  Satish Nair, Zachary Crockett, Zach Supalla and Mohit Bhoite
 * @version V1.0.0
 * @date    13-March-2013
 * 
 * Updated: 14-Feb-2014 David Sidrane <david_s5@usa.net>
 * 
 * @brief   Main program body.
 ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "debug.h"
#include "spark_utilities.h"

#include "hound_wlan.h"

#include "stm32_it.h"
#include "websocket.h"
#include "com_proto.h"
#include "com_command.h"
#include "hd44780.h"
#include "hound_adc.h"
#include "hound_fixed.h"
#include "hound_debug.h"
#include "hound_identity.h"
#include "watchdog.h"
#include "hound_time.h"

#include "socket_control.h"

#include "heartbeat.h"

#include "stm32f10x_gpio.h"

extern "C" {
#include "usb_conf.h"
#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_pwr.h"
#include "usb_prop.h"
#include "sst25vf_spi.h"
}


static volatile sampleSetup_t * primarySample = NULL;

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
volatile uint32_t TimingFlashUpdateTimeout;

uint8_t  USART_Rx_Buffer[USART_RX_DATA_SIZE];
uint32_t USART_Rx_ptr_in = 0;
uint32_t USART_Rx_ptr_out = 0;
uint32_t USART_Rx_length  = 0;

uint8_t USB_Rx_Buffer[VIRTUAL_COM_PORT_DATA_SIZE];
uint16_t USB_Rx_length = 0;
uint16_t USB_Rx_ptr = 0;

uint8_t  USB_Tx_State = 0;
uint8_t  USB_Rx_State = 0;

uint32_t USB_USART_BaudRate = 9600;

static void IntToUnicode (uint32_t value , uint8_t *pbuf , uint8_t len);

/* Extern variables ----------------------------------------------------------*/
extern LINE_CODING linecoding;

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
 * Function Name  : SparkCoreConfig.
 * Description    : Called in startup routine, before calling C++ constructors.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
extern "C" void SparkCoreConfig(void)
{
        DECLARE_SYS_HEALTH(ENTERED_SparkCoreConfig);
#ifdef DFU_BUILD_ENABLE
	/* Set the Vector Table(VT) base location at 0x5000 */
	NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x5000);

	USE_SYSTEM_FLAGS = 1;
#endif

#ifdef SWD_JTAG_DISABLE
	/* Disable the Serial Wire JTAG Debug Port SWJ-DP */
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);
#endif

	Set_System();

	SysTick_Configuration();

	/* Enable CRC clock */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);
#if !defined (RGB_NOTIFICATIONS_ON)	&& defined (RGB_NOTIFICATIONS_OFF)
	LED_RGB_OVERRIDE = 1;
#endif

#if defined (SPARK_RTC_ENABLE)
	RTC_Configuration();
#endif

        // /* Execute Stop mode if STOP mode flag is set via Spark.sleep(pin, mode) */
        // Enter_STOP_Mode();

        // LED_SetRGBColor(RGB_COLOR_WHITE);
        // LED_On(LED_RGB);
        // SPARK_LED_FADE = 1;

#ifdef DFU_BUILD_ENABLE
	Load_SystemFlags();
#endif

#ifdef SPARK_SFLASH_ENABLE
	sFLASH_Init();
#endif
}

/* Main Loop Counter Defines */
#define HEARTBEAT_MILLS (250)
#define ONE_DAY_MILLIS (24 * 60 * 60 * 1000)
#define CONNECTION_CHECK_MILLIS (1 * 1000)
#define UPDATE_INTERVAL_MILLS (1 * 60000)
#define SOCKET_UPDATE_INTERVAL_MILLS (1000 * 3)
#define STARTUP_BROADCAST_DELAY (10 * 1000)
#define WATCHDOG_UPDATE_DELAY (1000)

// Sample interval in seconds
#define SAMPLE_INTERVAL 2

#define delayms delay

#define COM_BUFFSIZE 400


/*******************************************************************************
 * Function Name  : main.
 * Description    : main routine.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
int main(void)
{
	int ret = 0;
	bool g_subscriptionEnabled = false; //@todo write state to eeprom maybe of pins aswell.

	// We have running firmware, otherwise we wouldn't have gotten here
	uint8_t SPARK_WIRING_APPLICATION = 0;

	/* Main Loop Action Counters */
	unsigned long g_lastBeat = millis();
	unsigned long g_lastSync = millis();
	unsigned long g_lastConnectionCheck = millis();
	unsigned long g_lastUpdate = millis();
	unsigned long g_lastSocketUpdate = millis();
	unsigned long g_startupMillis = millis();
	unsigned long g_watchdogMillis = millis();

	/* Variable Definitions */

	// Communication Buffers
	uint8_t * pComBuff;
	uint8_t * sComBuff;
	int buffSendSize;

	// Communication Support
	WebSocket * g_sampleSocket = NULL;
	Communication::ipAddr_t recvAddress;
	Communication::hRequest_t g_subscriptionRequest;
	Communication::hRequest_t g_fastSubRequest;
	Communication::ipAddr_t g_subscriptionAddress;
	Communication::ipAddr_t g_fastSubAddress;
	Communication::ipAddr_t g_broadcastAddress;
	Communication::HoundProto * com_demo;

	// Identity Storage
	HoundIdentity * g_Identity = NULL;

	// Setup Heartbeat LED
	heartbeat_initialize(HEARTBEAT_PORT, HEARTBEAT_PIN);

	// LCD Setup
	LCDPinConfig_t pinConfig;
	pinConfig.portRS = GPIOA;
	pinConfig.pinRS = GPIO_Pin_0;
	pinConfig.portEnable = GPIOB;
	pinConfig.pinEnable = GPIO_Pin_5;
	pinConfig.portRW = GPIOB;
	pinConfig.pinRW = GPIO_Pin_6;
	pinConfig.portData0 = GPIOB;
	pinConfig.pinData0 = GPIO_Pin_4;
	pinConfig.portData1 = GPIOB;
	pinConfig.pinData1 = GPIO_Pin_3;
	pinConfig.portData2 = GPIOA;
	pinConfig.pinData2 = GPIO_Pin_15;
	pinConfig.portData3 = GPIOA;
	pinConfig.pinData3 = GPIO_Pin_14;
	HD44780 * lcd = HD44780::getInstance(&pinConfig);

	// Enable CC3000 SPI Connection
	uint8_t setup_complete = 0;

	WLAN_Off();
	WLAN_Initialize();
	// WLAN_On();

	WLAN_Connect();
	//WLAN_Clear();

  	/* Main loop */
	while (1)
	{

		if (millis() - g_lastBeat > HEARTBEAT_MILLS)
		{
			heartbeat_beat(HEARTBEAT_PORT, HEARTBEAT_PIN);
			g_lastBeat = millis();
			WLAN_KeepAlive_Loop();
		}

		if (WLAN_GetStatus() && !setup_complete)
		{
			WLAN_Ping_Broadcasts();

			g_Identity = new HoundIdentity;
			g_Identity->retrieveIdentity();

			initializeSocket(0);
			initializeSocket(1);

			com_demo = new Communication::HoundProto(9080);

			// Sampling storage buffers and associated setup, everything else handled in RTC interrupt
			primarySample = (sampleSetup_t *)malloc(sizeof(sampleSetup_t));
			primarySample->bufferSize = BLOCKSIZE;
			primarySample->currentBuffer = (fixed_t *)malloc(sizeof(fixed_t) * BLOCKSIZE);
			primarySample->voltageBuffer = (fixed_t *)malloc(sizeof(fixed_t) * BLOCKSIZE);

		    pComBuff = (uint8_t *)malloc(COM_BUFFSIZE);
		    sComBuff = (uint8_t *)malloc(COM_BUFFSIZE);

		    // Node 0 (unimplemented) and All sampels
		    g_subscriptionRequest.rNode = 0x0F;
		    g_subscriptionRequest.rParam = 0xFF;

		    g_fastSubRequest.rNode = 0x00;
		    g_fastSubRequest.rParam = 0xFF;

		    initADCSPI();

		    // Update RealTime Clock
			ret = generateNTPRequest(pComBuff, COM_BUFFSIZE);

			if (ret > 0)
			{
				ret = sendNTPRequest(pComBuff, ret);

				if (ret > 0)
				{
					parseNTPResponse(pComBuff, ret);
				} else if (ret == -2){
					LED_SetRGBColor(RGB_COLOR_RED);
				} else if (ret == -3) {
					LED_SetRGBColor(RGB_COLOR_WHITE);
				} else {
					LED_SetRGBColor(RGB_COLOR_GREEN);
				}
			} else {
				LED_SetRGBColor(RGB_COLOR_GREEN);
			}
			
			setup_complete = 1;

			lcd->clear();
			lcd->home();
			lcd->printf("Setup Complete");
		}

		if(WLAN_GetStatus() && setup_complete)
		{
			if(1)
			{
				static int count = 0;
				static bool bBroacast = FALSE;
				static bool bWatchdog = FALSE;
				static bool bSampleSocket = FALSE;

				HD44780 * lcd = HD44780::getInstance();

				// Command Loop
				if (millis() - g_lastConnectionCheck > CONNECTION_CHECK_MILLIS)
				{
					ret = com_demo->getData(pComBuff, COM_BUFFSIZE, &recvAddress);

					// Task handling loop
					uint8_t reference = pComBuff[0];
					uint8_t operation = pComBuff[1];

					if (ret > 0)
					{
						// DEMO for now, after we get one ping from the server, a "subscription"
						// will be made, triggering the core to send updates every 10seconds
						#ifdef DEBUG_ON
						HoundDebug::logMessage(pComBuff[1], "Got Data");
						#endif

						// Socket Data Operation
						if (operation == 0x00)
						{
							// Process Socket Data Operation
							buffSendSize = Communication::parseRequest((Communication::hRequest_t *)(pComBuff + 2), (ret-2) / sizeof(Communication::hRequest_t), (char *)sComBuff, COM_BUFFSIZE, g_Identity);
							
							// Server Reply
							Communication::HoundProto::sendData(sComBuff, buffSendSize, &recvAddress);
						} 
						// Socket Control Operation
						else if (operation & 0x01)
						{
							// Process Socket Operation
							buffSendSize = Communication::parseCommand((uint8_t *)(pComBuff + 2), (ret-2) / sizeof(uint8_t), (char *)sComBuff, COM_BUFFSIZE, reference);

							// Server Reply
							Communication::HoundProto::sendData(sComBuff, buffSendSize, &recvAddress);
						} 

						// Subscription Request
						else if (operation & 0x02)
						{
							g_subscriptionAddress.oct[0] = recvAddress.oct[0];
							g_subscriptionAddress.oct[1] = recvAddress.oct[1];
							g_subscriptionAddress.oct[2] = recvAddress.oct[2];
							g_subscriptionAddress.oct[3] = recvAddress.oct[3];

							g_subscriptionEnabled = true;

							buffSendSize = snprintf((char *)sComBuff, COM_BUFFSIZE, "{\"e\":%d,\"op\":\"sub\",\"result\":1}", reference);

							Communication::HoundProto::sendData(sComBuff, buffSendSize, &recvAddress);

						} 
						// Websocket Connection Request
						else if (operation & 0x04)
						{
							if (pComBuff[2] == 1)
							{
								bSampleSocket = FALSE;

							} else {

								g_fastSubAddress.oct[0] = recvAddress.oct[0];
								g_fastSubAddress.oct[1] = recvAddress.oct[1];
								g_fastSubAddress.oct[2] = recvAddress.oct[2];
								g_fastSubAddress.oct[3] = recvAddress.oct[3];

								bSampleSocket = TRUE;
							}

							buffSendSize = snprintf((char *)sComBuff, COM_BUFFSIZE, "{\"e\":%d,\"op\":\"ws\",\"result\":%d}", reference, pComBuff[2]);
							Communication::HoundProto::sendData(sComBuff, buffSendSize, &recvAddress);

						}
					}

					g_lastConnectionCheck = millis();
				}

				if (g_subscriptionEnabled && millis() - g_lastUpdate > UPDATE_INTERVAL_MILLS) {

					buffSendSize = Communication::parseRequest(&g_subscriptionRequest, 1, (char *)sComBuff, COM_BUFFSIZE, g_Identity);
					Communication::HoundProto::sendData(sComBuff, buffSendSize, &g_subscriptionAddress);

					g_lastUpdate = millis();
				}

				if ((bSampleSocket) && (millis() - g_lastSocketUpdate) > SOCKET_UPDATE_INTERVAL_MILLS)
				{
					buffSendSize = Communication::parseRequest(&g_fastSubRequest, 1, (char *)sComBuff, COM_BUFFSIZE, g_Identity);
					Communication::HoundProto::sendData(sComBuff, buffSendSize, &g_fastSubAddress);

					// int size = 0, i = 0;;
					// while (i < BLOCKSIZE && size < COM_BUFFSIZE)
					// {
					// 	size += Communication::strcat((char *)sComBuff + size, COM_BUFFSIZE - size, primarySample->voltageBuffer[i++], ',');
					// }

					// Communication::HoundProto::sendData(sComBuff, size, &g_fastSubAddress);
					
					g_lastSocketUpdate = millis();
				}

				if (!bBroacast && millis() - g_startupMillis > STARTUP_BROADCAST_DELAY)
				{

					bBroacast = TRUE;
					g_broadcastAddress.oct[0] = 224;
				    g_broadcastAddress.oct[1] = 111;
					g_broadcastAddress.oct[2] = 112;
					g_broadcastAddress.oct[3] = 113;
				    g_Identity->broadcast(&g_broadcastAddress, (char *)sComBuff, COM_BUFFSIZE);
				
				    Multicast_Presence_Announcement();
				}

				if (millis() - g_lastSync > ONE_DAY_MILLIS) {
					// Request time synchronization from the Spark Cloud
					//Spark.syncTime();
					g_lastSync = millis();
				}
			}
		}
	}
}

void RTC_IRQHandler(void)
{
	static int sampleIntervalCount = 0;
	static const uint8_t socket_count = socketGetCount();
	static uint8_t socket = 0;

	if(RTC_GetITStatus(RTC_IT_SEC) != RESET)
	{
		/* Clear the RTC Second Interrupt pending bit */
		RTC_ClearITPendingBit(RTC_IT_SEC);

		if (socket >= socket_count)
		{
			socket = 0;
		}

		// Trigger voltage/current sampling once every SAMPLE_INTERVAL seconds
		if (primarySample != NULL && sampleIntervalCount++ >= SAMPLE_INTERVAL)
		{
			volatile socketMap_t * socketSample = socketGetStruct(socket++);

			primarySample->voltageCSPort = socketSample->voltageCSPort;
			primarySample->voltageCSPin = socketSample->voltageCSPin;
			primarySample->currentCSPort = socketSample->currentCSPort;
			primarySample->currentCSPin = socketSample->currentCSPin;
			primarySample->currentSPIAlt = socketSample->currentSPIAlt;
			primarySample->rmsResults = socketSample->rmsResults;

			int ret = getSampleBlock(primarySample);
			
			if (ret < 0)
			{
				LED_SetRGBColor(RGB_COLOR_RED);
			}

			sampleIntervalCount = 0;
		}

		/* Wait until last write operation on RTC registers has finished */
		RTC_WaitForLastTask();
	}
}


/*******************************************************************************
 * Function Name  : Timing_Decrement
 * Description    : Decrements the various Timing variables related to SysTick.
 * Input          : None
 * Output         : Timing
 * Return         : None
 *******************************************************************************/
void Timing_Decrement(void)
{
	if (TimingDelay != 0x00)
	{
		TimingDelay--;
	}

	updateWatchdog();

#if !defined (RGB_NOTIFICATIONS_ON)	&& defined (RGB_NOTIFICATIONS_OFF)
	//Just needed in case LED_RGB_OVERRIDE is set to 0 by accident
	if (LED_RGB_OVERRIDE == 0)
	{
		LED_RGB_OVERRIDE = 1;
		LED_Off(LED_RGB);
	}
#endif

	if (LED_RGB_OVERRIDE != 0)
	{
		if ((LED_Spark_Signal != 0) && (NULL != LED_Signaling_Override))
		{
			LED_Signaling_Override();
		}
	}
	else if (TimingLED != 0x00)
	{
		TimingLED--;
	}
	else if(WLAN_SMART_CONFIG_START || SPARK_FLASH_UPDATE || Spark_Error_Count)
	{
		//Do nothing
	}
	else if(SPARK_LED_FADE)
	{
		LED_Fade(LED_RGB);
		TimingLED = 20;//Breathing frequency kept constant
	}
	else if(SPARK_WLAN_SETUP && SPARK_CLOUD_CONNECTED)
	{
#if defined (RGB_NOTIFICATIONS_CONNECTING_ONLY)
		LED_Off(LED_RGB);
#else
		LED_SetRGBColor(RGB_COLOR_CYAN);
		LED_On(LED_RGB);
		SPARK_LED_FADE = 1;
#endif
	}
	else
	{
		LED_Toggle(LED_RGB);
		if(SPARK_CLOUD_SOCKETED)
			TimingLED = 50;		//50ms
		else
			TimingLED = 100;	//100ms
	}
}

/*******************************************************************************
 * Function Name  : USB_USART_Init
 * Description    : Start USB-USART protocol.
 * Input          : baudRate.
 * Return         : None.
 *******************************************************************************/
void USB_USART_Init(uint32_t baudRate)
{
	linecoding.bitrate = baudRate;
	USB_Disconnect_Config();
	USB_Cable_Config(DISABLE);
	Delay_Microsecond(100000);
	Set_USBClock();
	USB_Interrupts_Config();
	USB_Init();
}

/*******************************************************************************
 * Function Name  : USB_USART_Available_Data.
 * Description    : Return the length of available data received from USB.
 * Input          : None.
 * Return         : Length.
 *******************************************************************************/
uint8_t USB_USART_Available_Data(void)
{
	if(bDeviceState == CONFIGURED)
	{
		if(USB_Rx_State == 1)
		{
			return (USB_Rx_length - USB_Rx_ptr);
		}
	}

	return 0;
}

/*******************************************************************************
 * Function Name  : USB_USART_Receive_Data.
 * Description    : Return data sent by USB Host.
 * Input          : None
 * Return         : Data.
 *******************************************************************************/
int32_t USB_USART_Receive_Data(void)
{
	if(bDeviceState == CONFIGURED)
	{
		if(USB_Rx_State == 1)
		{
			if((USB_Rx_length - USB_Rx_ptr) == 1)
			{
				USB_Rx_State = 0;

				/* Enable the receive of data on EP3 */
				SetEPRxValid(ENDP3);
			}

			return USB_Rx_Buffer[USB_Rx_ptr++];
		}
	}

	return -1;
}

/*******************************************************************************
 * Function Name  : USB_USART_Send_Data.
 * Description    : Send Data from USB_USART to USB Host.
 * Input          : Data.
 * Return         : None.
 *******************************************************************************/
void USB_USART_Send_Data(uint8_t Data)
{
	if(bDeviceState == CONFIGURED)
	{
		USART_Rx_Buffer[USART_Rx_ptr_in] = Data;

		USART_Rx_ptr_in++;

		/* To avoid buffer overflow */
		if(USART_Rx_ptr_in == USART_RX_DATA_SIZE)
		{
			USART_Rx_ptr_in = 0;
		}

		if(CC3000_Read_Interrupt_Pin())
		{
			//Delay 100us to avoid losing the data
			Delay_Microsecond(100);
		}
	}
}

/*******************************************************************************
 * Function Name  : Handle_USBAsynchXfer.
 * Description    : send data to USB.
 * Input          : None.
 * Return         : None.
 *******************************************************************************/
void Handle_USBAsynchXfer (void)
{

	uint16_t USB_Tx_ptr;
	uint16_t USB_Tx_length;

	if(USB_Tx_State != 1)
	{
		if (USART_Rx_ptr_out == USART_RX_DATA_SIZE)
		{
			USART_Rx_ptr_out = 0;
		}

		if(USART_Rx_ptr_out == USART_Rx_ptr_in)
		{
			USB_Tx_State = 0;
			return;
		}

		if(USART_Rx_ptr_out > USART_Rx_ptr_in) /* rollback */
		{
			USART_Rx_length = USART_RX_DATA_SIZE - USART_Rx_ptr_out;
		}
		else
		{
			USART_Rx_length = USART_Rx_ptr_in - USART_Rx_ptr_out;
		}

		if (USART_Rx_length > VIRTUAL_COM_PORT_DATA_SIZE)
		{
			USB_Tx_ptr = USART_Rx_ptr_out;
			USB_Tx_length = VIRTUAL_COM_PORT_DATA_SIZE;

			USART_Rx_ptr_out += VIRTUAL_COM_PORT_DATA_SIZE;
			USART_Rx_length -= VIRTUAL_COM_PORT_DATA_SIZE;
		}
		else
		{
			USB_Tx_ptr = USART_Rx_ptr_out;
			USB_Tx_length = USART_Rx_length;

			USART_Rx_ptr_out += USART_Rx_length;
			USART_Rx_length = 0;
		}
		USB_Tx_State = 1;
		UserToPMABufferCopy(&USART_Rx_Buffer[USB_Tx_ptr], ENDP1_TXADDR, USB_Tx_length);
		SetEPTxCount(ENDP1, USB_Tx_length);
		SetEPTxValid(ENDP1);
	}

}

/*******************************************************************************
 * Function Name  : Get_SerialNum.
 * Description    : Create the serial number string descriptor.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void Get_SerialNum(void)
{
	uint32_t Device_Serial0, Device_Serial1, Device_Serial2;

	Device_Serial0 = *(uint32_t*)ID1;
	Device_Serial1 = *(uint32_t*)ID2;
	Device_Serial2 = *(uint32_t*)ID3;

	Device_Serial0 += Device_Serial2;

	if (Device_Serial0 != 0)
	{
		IntToUnicode (Device_Serial0, &Virtual_Com_Port_StringSerial[2] , 8);
		IntToUnicode (Device_Serial1, &Virtual_Com_Port_StringSerial[18], 4);
	}
}

/*******************************************************************************
 * Function Name  : HexToChar.
 * Description    : Convert Hex 32Bits value into char.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
static void IntToUnicode (uint32_t value , uint8_t *pbuf , uint8_t len)
{
	uint8_t idx = 0;

	for( idx = 0 ; idx < len ; idx ++)
	{
		if( ((value >> 28)) < 0xA )
		{
			pbuf[ 2* idx] = (value >> 28) + '0';
		}
		else
		{
			pbuf[2* idx] = (value >> 28) + 'A' - 10;
		}

		value = value << 4;

		pbuf[ 2* idx + 1] = 0;
	}
}

#ifdef USE_FULL_ASSERT
/*******************************************************************************
 * Function Name  : assert_failed
 * Description    : Reports the name of the source file and the source line number
 *                  where the assert_param error has occurred.
 * Input          : - file: pointer to the source file name
 *                  - line: assert_param error line source number
 * Output         : None
 * Return         : None
 *******************************************************************************/
void assert_failed(uint8_t* file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while (1)
	{
	}
}
#endif
