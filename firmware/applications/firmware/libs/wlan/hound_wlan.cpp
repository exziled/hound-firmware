/*!
 * @file hound_wlan.cpp
 * 
 * @brief HOUND WLAN Control Implementation
 * 
 * @author Benjamin Carlson
 * @author Blake Bourque
 * 
 * @date November 20, 2014
 * 
 * Manages WiFi connection, status, and any async callbacks.
 * 
 */


#include "hound_wlan.h"
#include <string.h>

extern "C" {
	#include "hw_config.h"
	#include "evnt_handler.h"
	#include "hci.h"
	#include "wlan.h"
	#include "nvmem.h"
	#include "socket.h"
	#include "netapp.h"
	#include "security.h"
}

#include "rgbled.h"
#include "spark_macros.h"
#include "heartbeat.h"

static char device[] = "CC3000";
static tNetappIpconfigRetArgs hound_ip_config;
/* Smart Config Prefix */
static char aucCC3000_prefix[] = {'T', 'T', 'T'};
/* AES key "sparkdevices2013" */
static const unsigned char smartconfigkey[] = "sparkdevices2013";	//16 bytes

extern volatile uint8_t SPARK_LED_FADE;

uint32_t SPARK_WLAN_SetNetWatchDog(uint32_t timeOutInMS)
{
  uint32_t rv = cc3000__event_timeout_ms;
  cc3000__event_timeout_ms = timeOutInMS;
  return rv;
}

void Clear_NetApp_Dhcp(void)
{
	// Clear out the DHCP settings
	unsigned long pucSubnetMask = 0;
	unsigned long pucIP_Addr = 0;
	unsigned long pucIP_DefaultGWAddr = 0;
	unsigned long pucDNS = 0;

	netapp_dhcp(&pucIP_Addr, &pucSubnetMask, &pucIP_DefaultGWAddr, &pucDNS);
}



void WLAN_Initialize(void)
{
	CC3000_WIFI_Init();

	// Initialize SPI/DMA interface
	CC3000_SPI_DMA_Init();

	// Initilize SmartConfig Interrupt Trigger Pin
	WLAN_SmartConfigInitializer(SMART_CONFIG_PORT, SMART_CONFIG_PIN, SMART_CONFIG_EXTI, SMART_CONFIG_NVIC);

	// Initilize CC3000 per wireless driver
	wlan_init(WLAN_Async_Call, WLAN_Firmware_Patch, WLAN_Driver_Patch, WLAN_BootLoader_Patch, CC3000_Read_Interrupt_Pin, CC3000_Interrupt_Enable, CC3000_Interrupt_Disable, CC3000_Write_Enable_Pin);

	Delay(100);
}

void WLAN_SmartConfigInitializer(GPIO_TypeDef * GPIOx, uint16_t GPIO_Pin, uint32_t EXTI_Line, uint8_t NVIC_IRQChannel)
{

	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	GPIO_InitTypeDef pinInit;

    pinInit.GPIO_Pin = GPIO_Pin;
    pinInit.GPIO_Speed = GPIO_Speed_50MHz;
    pinInit.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOx, &pinInit);

    // TODO: I hate ST.
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource7);

    EXTI_InitStructure.EXTI_Line = EXTI_Line;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;

	EXTI_Init(&EXTI_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = NVIC_IRQChannel;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02; 
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x02;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void WLAN_KeepAlive_Loop(void)
{
	//if (houndWLAN.DHCP && !SPARK_WLAN_SLEEP)
	if (houndWLAN.DHCP)
	{
		if (hound_ip_config.aucIP[3] == 0)
		{
			Delay(100);
			netapp_ipconfig(&hound_ip_config);
		} else if (hound_ip_config.aucIP[3] != 0) {
			memset(&hound_ip_config, 0, sizeof(tNetappIpconfigRetArgs));
		}

	}

	// SmartConfig Process should begin
	if (houndWLAN.SMART_CONFIG_START)
	{
		WLAN_SmartConfigHandler();
	}

	// SmartConfig Sucessful
	// mDNS packet stops SmartConfig application
	if (houndWLAN.SMART_CONFIG_STOP && houndWLAN.DHCP && houndWLAN.CONNECTED)
	{
		WLAN_Ping_Broadcasts();

		for (int i = 0; i < 3; i++)
		{
			mdnsAdvertiser(1, device, strlen(device));
		}

		houndWLAN.SMART_CONFIG_STOP = 0;
	}
}

char *WLAN_Firmware_Patch(unsigned long *length)
{
	*length = 0;
	return NULL;
}

char *WLAN_Driver_Patch(unsigned long *length)
{
	*length = 0;
	return NULL;
}

char *WLAN_BootLoader_Patch(unsigned long *length)
{
	*length = 0;
	return NULL;
}


void WLAN_On(void)
{
	if (!houndWLAN.STARTED)
	{
		wlan_start(0);
		houndWLAN.STARTED = 1;

		SPARK_LED_FADE = 1;
		LED_SetRGBColor(RGB_COLOR_BLUE);
    	LED_On(LED_RGB);

    	wlan_ioctl_set_connection_policy(DISABLE, DISABLE, DISABLE);//Disable auto connect
	}
}

void WLAN_Off(void)
{
	if (houndWLAN.STARTED)
	{
		wlan_stop();

		SPARK_LED_FADE = 1;
	    LED_SetRGBColor(RGB_COLOR_WHITE);
	    LED_On(LED_RGB);
	}
}


uint8_t WLAN_GetStatus(void)
{
	if (houndWLAN.STARTED && houndWLAN.DHCP)
	{
		return true;
	}

	return false;
}


void WLAN_Ping(ipAddr_t * ipAddress, unsigned long timeout)
{
	if (!WLAN_GetStatus())
	{
		return;
	}

	unsigned long ip[4] = {ipAddress->oct[0], ipAddress->oct[1], ipAddress->oct[2], ipAddress->oct[3]};

	// Send ping to ipAddress, 2 attempts, 64 bytes, 10 msec timeout
	netapp_ping_send(ip, 2, 64, timeout);
}

void WLAN_Ping_Broadcasts(void)
{
	tNetappIpconfigRetArgs ipConfig;
	ipAddr_t pingAddress;

	WLAN_IPConfig(&ipConfig);

	pingAddress.oct[0] = ipConfig.aucDefaultGateway[0];
	pingAddress.oct[1] = ipConfig.aucDefaultGateway[1];
	pingAddress.oct[2] = ipConfig.aucDefaultGateway[2];
	pingAddress.oct[3] = ipConfig.aucDefaultGateway[3];

	// 10 msec timeout
	WLAN_Ping(&pingAddress, 10);
}

void WLAN_IPConfig(tNetappIpconfigRetArgs * ipConfig)
{
	if (!WLAN_GetStatus())
	{
		return;
	}

	netapp_ipconfig(ipConfig);
}

void WLAN_Connect(void)
{
	if (!WLAN_GetStatus())
	{
		houndWLAN.DISCONNECT = 0;
		houndWLAN.STARTED = 1;
		wlan_start(0);

		wlan_set_event_mask(HCI_EVNT_WLAN_KEEPALIVE | HCI_EVNT_WLAN_UNSOL_INIT);

		SPARK_LED_FADE = 0;
		LED_SetRGBColor(RGB_COLOR_GREEN);
		LED_On(LED_RGB);
		// User Profile Connection Mode
		// Attempt to make connection to WiFi using stored user profiles
		wlan_ioctl_set_connection_policy(DISABLE, DISABLE, ENABLE);

		Set_NetApp_Timeout();
	}
}

void WLAN_Disconnect(void)
{
	if (WLAN_GetStatus())
	{
		houndWLAN.DISCONNECT = 1;
		wlan_ioctl_set_connection_policy(DISABLE, DISABLE, DISABLE);//Disable auto connect
		wlan_disconnect();
	}
}


void WLAN_Async_Call(long lEventType, char *data, unsigned char length)
{
	switch (lEventType)
	{
		// Smart config occured and is complete
		case HCI_EVNT_WLAN_ASYNC_SIMPLE_CONFIG_DONE:
			houndWLAN.SMART_CONFIG_DONE = 1;
			houndWLAN.SMART_CONFIG_STOP = 1;
			break;

		// A Connection was established
		case HCI_EVNT_WLAN_UNSOL_CONNECT:
			houndWLAN.CONNECTED = 1;
			break;

		// Disconnected occured
		case HCI_EVNT_WLAN_UNSOL_DISCONNECT:
			if (houndWLAN.CONNECTED)
			{

				SPARK_LED_FADE = 1;
				LED_SetRGBColor(RGB_COLOR_BLUE);
				LED_On(LED_RGB);
			}

			houndWLAN.CONNECTED = 0;
			houndWLAN.DHCP = 0;
			break;

		// DHCP Report
		case HCI_EVNT_WLAN_UNSOL_DHCP:
			if (*(data + 20) == 0)
			{
				houndWLAN.DHCP = 1;

				SPARK_LED_FADE = 1;
				LED_SetRGBColor(RGB_COLOR_ORANGE);
				LED_On(LED_RGB);
			} else {
				houndWLAN.DHCP = 0;
			}
			break;

		case HCI_EVENT_CC3000_CAN_SHUT_DOWN:
			houndWLAN.SHUTDOWN = 1;
			// Something
			break;

		case HCI_EVNT_WLAN_ASYNC_PING_REPORT:
			//LED_SetRGBColor(RGB_COLOR_WHITE);
			break;
	}
}

void WLAN_SmartConfigHandler(void)
{
	houndWLAN.SMART_CONFIG_STOP = 0;
	houndWLAN.CONNECTED = 0;
	houndWLAN.DHCP = 0;
	houndWLAN.SHUTDOWN = 0;

	WLAN_Disconnect();
	WLAN_On();

	/* Create new entry for AES encryption key */
	nvmem_create_entry(NVMEM_AES128_KEY_FILEID,16);

	/* Write AES key to NVMEM */
	aes_write_key((unsigned char *)(&smartconfigkey[0]));

	wlan_smart_config_set_prefix((char*)aucCC3000_prefix);

	/* Start the SmartConfig start process */
	wlan_smart_config_start(1);

	// 	//WiFiCredentialsReader wifi_creds_reader(wifi_add_profile_callback);

	// /* Wait for SmartConfig/SerialConfig to finish */
	// while (WiFi.listening())
	// {
	// 	if(WLAN_DELETE_PROFILES)
	// 	{
	// 		int toggle = 25;
	// 		while(toggle--)
	// 		{
	// 			LED_Toggle(LED_RGB);
	// 			Delay(50);
	// 		}
	// 		WiFi.clearCredentials();
	// 		WLAN_DELETE_PROFILES = 0;
	// 	}
	// 	else
	// 	{
	// 		LED_Toggle(LED_RGB);
	// 		Delay(250);
	// 		//wifi_creds_reader.read();
	// 	}
	// }

	LED_SetRGBColor(RGB_COLOR_BLUE);
	LED_On(LED_RGB);
	while (WLAN_IsConfiguring())
	{
    	static bool toggle = 1;

		if (toggle)
			GPIO_SetBits(HEARTBEAT_PORT, HEARTBEAT_PIN);
		else
			GPIO_ResetBits(HEARTBEAT_PORT, HEARTBEAT_PIN);

		toggle = !toggle;
    	Delay(100);
	}

	if(houndWLAN.SMART_CONFIG_DONE)
	{
		/* Decrypt configuration information and add profile */
		//SPARK_WLAN_SmartConfigProcess();
		wlan_smart_config_process();
	}

	WLAN_Connect();

	houndWLAN.SMART_CONFIG_START = 0;

	LED_SetRGBColor(RGB_COLOR_WHITE);

}

void WLAN_Configure(void)
{
	houndWLAN.SMART_CONFIG_START = 1;
	return;
}

bool WLAN_IsConfiguring(void)
{
	if (houndWLAN.SMART_CONFIG_START && !houndWLAN.SMART_CONFIG_DONE)
	{
		return true;
	}

	return false;
}

bool WLAN_Clear(void)
{
	if(wlan_ioctl_del_profile(255) == 0)
	{
		Clear_NetApp_Dhcp();
		return true;
	}

	return false;
}

void Set_NetApp_Timeout(void)
{
	unsigned long aucDHCP = 14400;
	unsigned long aucARP = 3600;
	unsigned long aucKeepalive = 10;
	unsigned long aucInactivity = DEFAULT_SEC_INACTIVITY;
	SPARK_WLAN_SetNetWatchDog(S2M(DEFAULT_SEC_NETOPS)+ (DEFAULT_SEC_INACTIVITY ? 250 : 0) );
	netapp_timeout_values(&aucDHCP, &aucARP, &aucKeepalive, &aucInactivity);
}