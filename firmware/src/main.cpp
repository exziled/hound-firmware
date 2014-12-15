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
#include <stdio.h>

#include "hound_wlan.h"

#include "stm32_it.h"
#include "websocket.h"
#include "com_proto.h"
#include "com_command.h"
#include "com_subscription.h"
#include "hd44780.h"
#include "hound_adc.h"
#include "hound_fixed.h"
#include "hound_debug.h"
#include "hound_identity.h"
#include "watchdog.h"
#include "hound_time.h"

#include "socket_control.h"

#include "heartbeat.h"
#include "hound_alignment.h"

#include "stm32f10x_gpio.h"


static volatile sampleSetup_t * primarySample = NULL;


/* Private variables ---------------------------------------------------------*/
volatile uint32_t TimingFlashUpdateTimeout;

uint8_t  USART_Rx_Buffer[USART_RX_DATA_SIZE];
uint32_t USART_Rx_ptr_in = 0;
uint32_t USART_Rx_ptr_out = 0;
uint32_t USART_Rx_length  = 0;



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
#define SAMPLE_INTERVAL 1

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

	/* Main Loop Action Counters */
	unsigned long g_lastBeat = millis();
	unsigned long g_lastSync = millis();
	unsigned long g_lastConnectionCheck = millis();
	unsigned long g_lastUpdate = millis();
	unsigned long g_lastSocketUpdate = millis();
	unsigned long g_startupMillis = millis();

	/* Variable Definitions */
	bool bBroacast = false;
	uint8_t reference = 0;
	uint8_t operation = 0;
	uint8_t * recvBuff = NULL;

	int recieveSize = 0;

	// Communication Buffers
	uint8_t * pComBuff;
	uint8_t * sComBuff;
	int buffSendSize;

	// Communication Support
	Subscription * g_Subscription = NULL;
	Subscription * g_FastSubscription = NULL;
	Communication::ipAddr_t recvAddress;
	Communication::ipAddr_t g_broadcastAddress;
	Communication::HoundProto * com_demo;

	// Identity Storage
	HoundIdentity * g_Identity = NULL;

	// Setup Heartbeat LED
	heartbeat_initialize(HEARTBEAT_PORT, HEARTBEAT_PIN);

	// LCD Setup
	// LCDPinConfig_t pinConfig;
	// pinConfig.portRS = GPIOA;
	// pinConfig.pinRS = GPIO_Pin_0;
	// pinConfig.portEnable = GPIOB;
	// pinConfig.pinEnable = GPIO_Pin_5;
	// pinConfig.portRW = GPIOB;
	// pinConfig.pinRW = GPIO_Pin_6;
	// pinConfig.portData0 = GPIOB;
	// pinConfig.pinData0 = GPIO_Pin_4;
	// pinConfig.portData1 = GPIOB;
	// pinConfig.pinData1 = GPIO_Pin_3;
	// pinConfig.portData2 = GPIOA;
	// pinConfig.pinData2 = GPIO_Pin_15;
	// pinConfig.portData3 = GPIOA;
	// pinConfig.pinData3 = GPIO_Pin_14;
	// HD44780 * lcd = HD44780::getInstance(&pinConfig);

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

			// lcd->clear();
			// lcd->home();
			// lcd->printf("Setup Complete");


			HoundDebug::logMessage(0, "Setup Complete");
		}

		if(WLAN_GetStatus() && setup_complete)
		{
			// Command Loop
			if (millis() - g_lastConnectionCheck > CONNECTION_CHECK_MILLIS)
			{

				// Recieve data from CC3000
				recieveSize = com_demo->getData(pComBuff, COM_BUFFSIZE, &recvAddress);

				if (recieveSize > 0)
				{
					// More than 2 values recieved, we hopefully have a correctly formulated packet
					if (recieveSize > 2)
					{
						reference = pComBuff[0];
						operation = pComBuff[1];
						recvBuff = pComBuff + 2;

						// We only care about parseable data now
						recieveSize -= 2;
					} else {
						continue;
					}

					// DEMO for now, after we get one ping from the server, a "subscription"
					// will be made, triggering the core to send updates every 10seconds
					#ifdef DEBUG_ON
					HoundDebug::logMessage(pComBuff[1], "Got Data");
					#endif

					// Socket Data Operation
					if (operation == 0x00)
					{
						// Process Socket Data Operation
						buffSendSize = Communication::parseRequest((Communication::hRequest_t *)(recvBuff), recieveSize/sizeof(Communication::hRequest_t), (char *)sComBuff, COM_BUFFSIZE, g_Identity);
						
						// Server Reply
						Communication::HoundProto::sendData(sComBuff, buffSendSize, &recvAddress);
					} 
					// Socket Control Operation
					else if (operation & 0x01)
					{
						// Process Socket Operation
						buffSendSize = Communication::parseCommand((uint8_t *)(recvBuff), recieveSize/sizeof(uint8_t), (char *)sComBuff, COM_BUFFSIZE, reference);

						// Server Reply
						Communication::HoundProto::sendData(sComBuff, buffSendSize, &recvAddress);
					} 

					// Subscription Request
					// Subscription request should be made in format similar to single data request, indicating
					// to the core which sockets/which values should be sent back to the server
					else if (operation & 0x02)
					{
						// Create subscription if none exists
						if (g_Subscription != NULL)
						{
							delete g_Subscription;
							g_Subscription = NULL;
						}

						if (g_Subscription == NULL)
						{
							g_Subscription = new Subscription(&recvAddress, sComBuff, COM_BUFFSIZE, g_Identity);
							
							Communication::hRequest_t * subscriptionSockets = (Communication::hRequest_t *)recvBuff;

							for (uint8_t i = 0; i < recieveSize/sizeof(Communication::hRequest_t); i++)
							{
								g_Subscription->addSocket(subscriptionSockets->rNode, subscriptionSockets->rParam);
								
								subscriptionSockets++;
							}
							// Send back response
							buffSendSize = snprintf((char *)sComBuff, COM_BUFFSIZE, "{\"e\":%d,\"core_id\":\"%s\",\"op\":\"sub\",\"result\":1}", reference, g_Identity->get());
							Communication::HoundProto::sendData(sComBuff, buffSendSize, &recvAddress);
						} else {
							buffSendSize = snprintf((char *)sComBuff, COM_BUFFSIZE, "{\"e\":%d,\"op\":\"sub\",\"result\":-1,\"msg\":\"Sub Already Mapped\"}", reference);
							Communication::HoundProto::sendData(sComBuff, buffSendSize, &recvAddress);
						}

					} 
					// Websocket Connection Request

					// TODO: Originally, websocket [2] was used to indicate cancelation
					else if (operation & 0x04)
					{
						if (g_FastSubscription != NULL)
						{
							delete g_FastSubscription;
							g_FastSubscription = NULL;
						}

						if (g_FastSubscription == NULL)
						{
							g_FastSubscription = new Subscription(&recvAddress, sComBuff, COM_BUFFSIZE, g_Identity);

							if (!g_FastSubscription->isValid())
							{
								delete g_FastSubscription;
								g_FastSubscription = NULL;

								buffSendSize = snprintf((char *)sComBuff, COM_BUFFSIZE, "{\"e\":%d,\"core_id\":\"%s\",\"op\":\"ws\",\"result\":-2,\"msg\":\"Sub Failed\"}", reference, g_Identity->get());
								Communication::HoundProto::sendData(sComBuff, buffSendSize, &recvAddress);

								break;
							}

							Communication::hRequest_t * subscriptionSockets = (Communication::hRequest_t *)recvBuff;

							for (uint8_t i = 0; i < recieveSize/sizeof(Communication::hRequest_t); i++)
							{
								g_FastSubscription->addSocket(subscriptionSockets->rNode, subscriptionSockets->rParam);

								subscriptionSockets++;
							}

							buffSendSize = snprintf((char *)sComBuff, COM_BUFFSIZE, "{\"e\":%d,\"op\":\"ws\",\"result\":1}", reference);
							Communication::HoundProto::sendData(sComBuff, buffSendSize, &recvAddress);
						} else {
							buffSendSize = snprintf((char *)sComBuff, COM_BUFFSIZE, "{\"e\":%d,\"core_id\":\"%s\",\"op\":\"ws\",\"result\":-1,\"msg\":\"Sub Already Mapped\"}", reference, g_Identity->get());
							Communication::HoundProto::sendData(sComBuff, buffSendSize, &recvAddress);
						}
					}

					else if (operation & 0x08)
					{
						if (recvBuff[0] == 0x2)
						{
							if (g_Subscription != NULL)
							{
								delete g_Subscription;

								g_Subscription = NULL;

								// Send back response
								buffSendSize = snprintf((char *)sComBuff, COM_BUFFSIZE, "{\"e\":%d,\"op\":\"sub\",\"result\":0}", reference);
								Communication::HoundProto::sendData(sComBuff, buffSendSize, &recvAddress);
							}

						} else if (recvBuff[0] == 0x4) {
							if (g_FastSubscription != NULL)
							{
								delete g_FastSubscription;

								g_FastSubscription = NULL;

								buffSendSize = snprintf((char *)sComBuff, COM_BUFFSIZE, "{\"e\":%d,\"op\":\"ws\",\"result\":0}", reference);
								Communication::HoundProto::sendData(sComBuff, buffSendSize, &recvAddress);
							}
						}
					}
				}

				g_lastConnectionCheck = millis();
			}

			if (g_Subscription != NULL && millis() - g_lastUpdate > UPDATE_INTERVAL_MILLS) {

				g_Subscription->sendSubscription();

				g_lastUpdate = millis();
			}


			if ((g_FastSubscription != NULL) && (millis() - g_lastSocketUpdate) > SOCKET_UPDATE_INTERVAL_MILLS)
			{

				g_FastSubscription->sendSubscription();

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
			}

			if (millis() - g_lastSync > ONE_DAY_MILLIS) {
				// Request time synchronization from the Spark Cloud
				//Spark.syncTime();
				g_lastSync = millis();
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

			if (socketSample != NULL)
			{
				primarySample->voltageCSPort = socketSample->voltageCSPort;
				primarySample->voltageCSPin = socketSample->voltageCSPin;
				primarySample->currentCSPort = socketSample->currentCSPort;
				primarySample->currentCSPin = socketSample->currentCSPin;
				primarySample->currentSPIAlt = socketSample->currentSPIAlt;
				primarySample->socket_id = socketSample->socket_id;
				primarySample->rmsResults = socketSample->rmsResults;

				int ret = getSampleBlock(primarySample);

				if (ret < 0)
				{
					LED_SetRGBColor(RGB_COLOR_RED);
				}

			} else {
				LED_SetRGBColor(RGB_COLOR_RED);
			}
			
			sampleIntervalCount = 0;
		}

		/* Wait until last write operation on RTC registers has finished */
		RTC_WaitForLastTask();
	}
}


system_tick_t millis(void)
{
	return GetSystem1MsTick();
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
}