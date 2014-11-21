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

#define SMART_CONFIG_PROFILE_SIZE       67

/* CC3000 EEPROM - Spark File Data Storage */
#define NVMEM_SPARK_FILE_ID		14	//Do not change this ID
#define NVMEM_SPARK_FILE_SIZE		16	//Change according to requirement
#define WLAN_PROFILE_FILE_OFFSET	0
#define WLAN_POLICY_FILE_OFFSET		1       //Not used henceforth
#define WLAN_TIMEOUT_FILE_OFFSET	2
#define ERROR_COUNT_FILE_OFFSET		3

static char device[] = "CC3000";
static tNetappIpconfigRetArgs hound_ip_config;
// netapp_pingreport_args_t hound_ping_report;
// int ping_report_num;

/* Smart Config Prefix */
static char aucCC3000_prefix[] = {'T', 'T', 'T'};
/* AES key "sparkdevices2013" */
static const unsigned char smartconfigkey[] = "sparkdevices2013";	//16 bytes
/* device name used by smart config response */
static char device_name[] = "CC3000";

/* Manual connect credentials; only used if WLAN_MANUAL_CONNECT == 1 */
static char _ssid[] = "ssid";
static char _password[] = "password";
// Auth options are WLAN_SEC_UNSEC, WLAN_SEC_WPA, WLAN_SEC_WEP, and WLAN_SEC_WPA2
static unsigned char _auth = WLAN_SEC_WPA2;

unsigned char WLANProfileIndex;
unsigned char NVMEM_File_Data[NVMEM_SPARK_FILE_SIZE];




extern volatile uint8_t SPARK_LED_FADE;

uint32_t SPARK_WLAN_SetNetWatchDog(uint32_t timeOutInMS)
{
  uint32_t rv = cc3000__event_timeout_ms;
  cc3000__event_timeout_ms = timeOutInMS;
  return rv;
}

static void recreate_spark_nvmem_file(void)
{
  // Spark file IO on old TI Driver was corrupting nvmem
  // so remove the entry for Spark file in CC3000 EEPROM
  nvmem_create_entry(NVMEM_SPARK_FILE_ID, 0);

  // Create new entry for Spark File in CC3000 EEPROM
  nvmem_create_entry(NVMEM_SPARK_FILE_ID, NVMEM_SPARK_FILE_SIZE);

  // Zero out our array copy of the EEPROM
  memset(NVMEM_File_Data, 0, NVMEM_SPARK_FILE_SIZE);

  // Write zeroed-out array into the EEPROM
  nvmem_write(NVMEM_SPARK_FILE_ID, NVMEM_SPARK_FILE_SIZE, 0, NVMEM_File_Data);
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
			LED_SetRGBColor(RGB_COLOR_WHITE);
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

	/* read count of wlan profiles stored */
	nvmem_read(NVMEM_SPARK_FILE_ID, 1, WLAN_PROFILE_FILE_OFFSET, &NVMEM_File_Data[WLAN_PROFILE_FILE_OFFSET]);

	if(houndWLAN.SMART_CONFIG_DONE)
	{
		/* Decrypt configuration information and add profile */
		SPARK_WLAN_SmartConfigProcess();
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
		recreate_spark_nvmem_file();
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

void SPARK_WLAN_SmartConfigProcess()
{
        unsigned int ssidLen, keyLen;
        unsigned char *decKeyPtr;
        unsigned char *ssidPtr;
        extern unsigned char profileArray[];

        // read the received data from fileID #13 and parse it according to the followings:
        // 1) SSID LEN - not encrypted
        // 2) SSID - not encrypted
        // 3) KEY LEN - not encrypted. always 32 bytes long
        // 4) Security type - not encrypted
        // 5) KEY - encrypted together with true key length as the first byte in KEY
        //       to elaborate, there are two corner cases:
        //              1) the KEY is 32 bytes long. In this case, the first byte does not represent KEY length
        //              2) the KEY is 31 bytes long. In this case, the first byte represent KEY length and equals 31
        if(SMART_CONFIG_PROFILE_SIZE != nvmem_read(NVMEM_SHARED_MEM_FILEID, SMART_CONFIG_PROFILE_SIZE, 0, profileArray))
        {
          return;
        }

        ssidPtr = &profileArray[1];

        ssidLen = profileArray[0];

        decKeyPtr = &profileArray[profileArray[0] + 3];

        UINT8 expandedKey[176];
        aes_decrypt(decKeyPtr, (unsigned char *)smartconfigkey, expandedKey);
        if (profileArray[profileArray[0] + 1] > 16)
        {
          aes_decrypt((UINT8 *)(decKeyPtr + 16), (unsigned char *)smartconfigkey, expandedKey);
        }

        if (*(UINT8 *)(decKeyPtr +31) != 0)
        {
                if (*decKeyPtr == 31)
                {
                        keyLen = 31;
                        decKeyPtr++;
                }
                else
                {
                        keyLen = 32;
                }
        }
        else
        {
                keyLen = *decKeyPtr;
                decKeyPtr++;
        }

        WLAN_SetCredentials((char *)ssidPtr, ssidLen, (char *)decKeyPtr, keyLen, profileArray[profileArray[0] + 2]);
}

void WLAN_SetCredentials(char *ssid, unsigned int ssidLen, char *password, unsigned int passwordLen, unsigned long security)
{

  if (0 == password[0]) {
    security = WLAN_SEC_UNSEC;
  }

  // add a profile
  switch (security)
  {
  case WLAN_SEC_UNSEC://None
    {
      wlan_add_profile(WLAN_SEC_UNSEC,   // Security type
        (unsigned char *)ssid,                                // SSID
        ssidLen,                                              // SSID length
        NULL,                                                 // BSSID
        1,                                                    // Priority
        0, 0, 0, 0, 0);

      break;
    }

  case WLAN_SEC_WEP://WEP
    {
      //if(!WLAN_SMART_CONFIG_FINISHED)
      //{
        // Get WEP key from string, needs converting
        passwordLen = (strlen(password)/2); // WEP key length in bytes
        char byteStr[3]; byteStr[2] = '\0';

        for (UINT32 i = 0 ; i < passwordLen ; i++) { // Basic loop to convert text-based WEP key to byte array, can definitely be improved
          byteStr[0] = password[2*i]; byteStr[1] = password[(2*i)+1];
          password[i] = strtoul(byteStr, NULL, 16);
        }
      //}

      wlan_add_profile(WLAN_SEC_WEP,    // Security type
        (unsigned char *)ssid,                                // SSID
        ssidLen,                                              // SSID length
        NULL,                                                 // BSSID
        1,                                                    // Priority
        passwordLen,                                          // KEY length
        0,                                                    // KEY index
        0,
        (unsigned char *)password,                            // KEY
        0);

      break;
    }

  case WLAN_SEC_WPA://WPA
  case WLAN_SEC_WPA2://WPA2
    {
      wlan_add_profile(WLAN_SEC_WPA2,    // Security type
        (unsigned char *)ssid,                                // SSID
        ssidLen,                                              // SSID length
        NULL,                                                 // BSSID
        1,                                                    // Priority
        0x18,                                                 // PairwiseCipher
        0x1e,                                                 // GroupCipher
        2,                                                    // KEY management
        (unsigned char *)password,                            // KEY
        passwordLen);                                         // KEY length

      break;
    }
  }

  // if(wlan_profile_index != -1)
  // {
  //   NVMEM_Spark_File_Data[WLAN_PROFILE_FILE_OFFSET] = wlan_profile_index + 1;
  // }

  //  write count of wlan profiles stored 
  // nvmem_write(NVMEM_SPARK_FILE_ID, 1, WLAN_PROFILE_FILE_OFFSET, &NVMEM_Spark_File_Data[WLAN_PROFILE_FILE_OFFSET]);
}
