/**
 ******************************************************************************
 * @file    application.cpp
 * @authors Blake Bourque, Benjamin Carlson
 * @version V1.0.0
 * @date    05-November-2013
 * @brief   Tinker application
 ******************************************************************************/
// G++ doesn't like constant strings
#pragma GCC diagnostic ignored "-Wwrite-strings"


/* Includes ------------------------------------------------------------------*/
#include "application.h"
#include "stm32_it.h"
#include "cloud_dfu.h"
#include "tc74.h"
#include "websocket.h"
#include "com_proto.h"
#include "com_command.h"
#include "hd44780.h"
#include "hound_adc.h"
#include "hound_fixed.h"
#include "hound_debug.h"
#include "hound_identity.h"
#include "watchdog.h"

/* Defines and Globals -------------------------------------------------------*/
bool g_subscriptionEnabled = false; //@todo write state to eeprom maybe of pins aswell.

#define delayms delay
#define g_heartbeatLED D7
#define g_subscriptionLED D3

#define g_socketOne A6
#define g_socketTwo A7

#define HEARTBEAT_MILLS (250)
unsigned long g_lastBeat = millis();

#define ONE_DAY_MILLIS (24 * 60 * 60 * 1000)
unsigned long g_lastSync = millis();

#define CONNECTION_CHECK_MILLIS (1 * 1000)
unsigned long g_lastConnectionCheck = millis();

#define UPDATE_INTERVAL_MILLS (1 * 60000)
unsigned long g_lastUpdate = millis();

#define SOCKET_UPDATE_INTERVAL_MILLS (1000 * 3)
unsigned long g_lastSocketUpdate = millis();

#define STARTUP_BROADCAST_DELAY (10 * 1000)
unsigned long g_startupMillis = millis();

#define WATCHDOG_UPDATE_DELAY (1000)
unsigned long g_watchdogMillis = millis();

// Sample interval in seconds
#define SAMPLE_INTERVAL 2

#define COM_BUFFSIZE 300
uint8_t * pComBuff;
uint8_t * sComBuff;
int buffSendSize;

#define BLOCKSIZE 400
volatile sampleSetup_t * primarySample = NULL;
//volatile rmsValues_t * rmsResult = NULL;
AggregatedRMS * rmsAggregation = NULL;
WebSocket * g_sampleSocket = NULL;
HoundIdentity * g_Identity = NULL;

Communication::ipAddr_t recvAddress;

Communication::hRequest_t g_subscriptionRequest;
Communication::hRequest_t g_fastSubRequest;
Communication::ipAddr_t g_subscriptionAddress;
Communication::ipAddr_t g_fastSubAddress;
Communication::ipAddr_t g_broadcastAddress;

int ret = 0;
/* Function prototypes -------------------------------------------------------*/
int subscription(String command);
int control(String command);

SYSTEM_MODE(AUTOMATIC);

Communication::HoundProto * com_demo;

/* This function is called once at start up ----------------------------------*/
void setup()
{
	// Disconnect from the spark cloud
	Spark_Disconnect();

	g_Identity = new HoundIdentity;
	g_Identity->retrieveIdentity();

	pinMode(g_heartbeatLED, OUTPUT);
	pinMode(g_subscriptionLED, OUTPUT);

	pinMode(g_socketOne, OUTPUT);
	pinMode(g_socketTwo, OUTPUT);
	// pinMode(g_activeLED, OUTPUT);

	// LCDPinConfig_t pinConfig;
	// pinConfig.portRS = GPIOB;
	// pinConfig.pinRS = GPIO_Pin_7;
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

	// lcd->clear();
	// lcd->home();
	// lcd->printf("Activated");

	com_demo = new Communication::HoundProto(9080);

	//delete &sock_demo;

	// Memory for sampling
	primarySample = (sampleSetup_t *)malloc(sizeof(sampleSetup_t));
	primarySample->bufferSize = BLOCKSIZE;
	primarySample->csPin = GPIO_Pin_4;
	primarySample->csPort = GPIOA;
	primarySample->currentBuffer = (fixed_t *)malloc(sizeof(fixed_t) * BLOCKSIZE);
	primarySample->voltageBuffer = (fixed_t *)malloc(sizeof(fixed_t) * BLOCKSIZE);

	rmsAggregation = new AggregatedRMS(6);

	primarySample->rmsResults = rmsAggregation;

    pComBuff = (uint8_t *)malloc(COM_BUFFSIZE);
    sComBuff = (uint8_t *)malloc(COM_BUFFSIZE);

    // Node 0 (unimplemented) and All sampels
    g_subscriptionRequest.rNode = 0x0F;
    g_subscriptionRequest.rParam = 0xFF;

    g_fastSubRequest.rNode = 0x00;
    g_fastSubRequest.rParam = 0xFF;

    initADCSPI();

    //Spark.publish("StartupComplete");
}

/* This function loops forever -----------------------------------------------*/
void loop() {
	static int count = 0;
	static bool bBroacast = FALSE;
	static bool bWatchdog = FALSE;
	static bool bSampleSocket = FALSE;

	HD44780 * lcd = HD44780::getInstance();

	if (millis() - g_lastBeat > HEARTBEAT_MILLS)
	{
		if (millis() - g_lastBeat > 2 * HEARTBEAT_MILLS)
		{
			lcd->clear();
			lcd->home();
			lcd->printf("I Failed %d", count++);
		}

		digitalWrite(g_heartbeatLED, !digitalRead(g_heartbeatLED));
		g_lastBeat = millis();
	}

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
				buffSendSize = Communication::parseRequest((Communication::hRequest_t *)(pComBuff + 2), (ret-2) / sizeof(Communication::hRequest_t), (char *)sComBuff, COM_BUFFSIZE, rmsAggregation, g_Identity);
				
				// Server Reply
				Communication::HoundProto::sendData(sComBuff, buffSendSize, &recvAddress);
			} 
			// Socket Control Operation
			else if (operation & 0x01)
			{
				// Process Socket Operation
				buffSendSize = Communication::parseCommand((Communication::hCommand_t *)(pComBuff + 2), (ret-2) / sizeof(Communication::hCommand_t), (char *)sComBuff, COM_BUFFSIZE, reference);

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

		buffSendSize = Communication::parseRequest(&g_subscriptionRequest, 1, (char *)sComBuff, COM_BUFFSIZE, rmsAggregation, g_Identity);
		Communication::HoundProto::sendData(sComBuff, buffSendSize, &g_subscriptionAddress);

		g_lastUpdate = millis();
	}

	if ((bSampleSocket) && (millis() - g_lastSocketUpdate) > SOCKET_UPDATE_INTERVAL_MILLS)
	{
		buffSendSize = Communication::parseRequest(&g_fastSubRequest, 1, (char *)sComBuff, COM_BUFFSIZE, rmsAggregation, g_Identity);
		Communication::HoundProto::sendData(sComBuff, buffSendSize, &g_fastSubAddress);
		
		g_lastSocketUpdate = millis();
	}

	if (bWatchdog && millis() - g_watchdogMillis > WATCHDOG_UPDATE_DELAY)
	{
		updateWatchdog();
	}

	if (!bBroacast && millis() - g_startupMillis > STARTUP_BROADCAST_DELAY)
	{
		bBroacast = TRUE;
		g_broadcastAddress.oct[0] = 224;
	    g_broadcastAddress.oct[1] = 111;
		g_broadcastAddress.oct[2] = 112;
		g_broadcastAddress.oct[3] = 113;
	    g_Identity->broadcast(&g_broadcastAddress, (char *)sComBuff, COM_BUFFSIZE);

	    bWatchdog = TRUE;
	    enableWatchdog();
	    g_watchdogMillis = millis();
	}

	if (millis() - g_lastSync > ONE_DAY_MILLIS) {
		// Request time synchronization from the Spark Cloud
		Spark.syncTime();
		g_lastSync = millis();
	}
}

