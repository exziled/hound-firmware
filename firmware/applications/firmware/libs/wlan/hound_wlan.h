#ifndef __HOUND_WLAN_H
#define __HOUND_WLAN_H

#include <stdint.h>

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

// Volatile due to interrupts
static volatile houndWLAN_t houndWLAN = {0, 0, 0, 0, 0, 0, 0, 0};

void WLAN_Initialize(void);
void WLAN_On(void);
void WLAN_Off(void);
void WLAN_Connect(void);
void WLAN_Disconnect(void);
uint8_t WLAN_GetStatus(void);

void WLAN_Async_Call(long lEventType, char *data, unsigned char length);
void WLAN_KeepAlive_Loop(void);

void WLAN_SmartConfigHandler(void);

char *WLAN_Firmware_Patch(unsigned long *length);
char *WLAN_Driver_Patch(unsigned long *length);
char *WLAN_BootLoader_Patch(unsigned long *length);
void Set_NetApp_Timeout(void);
uint32_t SPARK_WLAN_SetNetWatchDog(uint32_t timeOutInuS);

// Get SSID
// Get MAC

#endif	//__HOUND_WLAN_H