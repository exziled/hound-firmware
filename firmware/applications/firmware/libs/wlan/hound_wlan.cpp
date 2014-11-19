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

char device[] = "CC3000";
tNetappIpconfigRetArgs hound_ip_config;
// netapp_pingreport_args_t hound_ping_report;
// int ping_report_num;

extern volatile uint8_t SPARK_LED_FADE;

uint32_t SPARK_WLAN_SetNetWatchDog(uint32_t timeOutInMS)
{
  uint32_t rv = cc3000__event_timeout_ms;
  cc3000__event_timeout_ms = timeOutInMS;
  return rv;
}


void WLAN_Initialize(void)
{
	CC3000_WIFI_Init();

	// Initialize SPI/DMA interface
	CC3000_SPI_DMA_Init();

	// Initilize CC3000 per wireless driver
	wlan_init(WLAN_Async_Call, WLAN_Firmware_Patch, WLAN_Driver_Patch, WLAN_BootLoader_Patch, CC3000_Read_Interrupt_Pin, CC3000_Interrupt_Enable, CC3000_Interrupt_Disable, CC3000_Write_Enable_Pin);

	Delay(100);
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
			LED_SetRGBColor(RGB_COLOR_WHITE);
			break;
	}
}

void WLAN_SmartConfigHandler(void)
{

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
