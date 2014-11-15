/**
 ******************************************************************************
 * @file    spark_utilities.cpp
 * @author  Satish Nair, Zachary Crockett and Mohit Bhoite
 * @version V1.0.0
 * @date    13-March-2013
 *
 * Updated: 14-Feb-2014 David Sidrane <david_s5@usa.net>
 * @brief   
 ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */
#include "spark_utilities.h"
#include "spark_wiring.h"
#include "socket.h"
#include "netapp.h"
#include "string.h"
#include <stdarg.h>

SparkProtocol spark_protocol;

#define INVALID_SOCKET (-1)

long sparkSocket = INVALID_SOCKET;
sockaddr tSocketAddr;

//char digits[] = "0123456789";

extern uint8_t LED_RGB_BRIGHTNESS;

// LED_Signaling_Override
__IO uint8_t LED_Spark_Signal;
__IO uint32_t LED_Signaling_Timing;
const uint32_t VIBGYOR_Colors[] = {
  0xEE82EE, 0x4B0082, 0x0000FF, 0x00FF00, 0xFFFF00, 0xFFA500, 0xFF0000};
int VIBGYOR_Size = sizeof(VIBGYOR_Colors) / sizeof(uint32_t);
int VIBGYOR_Index;

int User_Var_Count;
int User_Func_Count;

struct User_Var_Lookup_Table_t
{
	void *userVar;
	char userVarKey[USER_VAR_KEY_LENGTH];
	Spark_Data_TypeDef userVarType;
} User_Var_Lookup_Table[USER_VAR_MAX_COUNT];


SystemClass System;
RGBClass RGB;

System_Mode_TypeDef SystemClass::_mode = AUTOMATIC;

SystemClass::SystemClass()
{
}

SystemClass::SystemClass(System_Mode_TypeDef mode)
{
  switch(mode)
  {
    case AUTOMATIC:
      _mode = AUTOMATIC;
      SPARK_CLOUD_CONNECT = 1;
      break;

    case SEMI_AUTOMATIC:
      _mode = SEMI_AUTOMATIC;
      SPARK_CLOUD_CONNECT = 0;
      SPARK_WLAN_SLEEP = 1;
      break;

    case MANUAL:
      _mode = MANUAL;
      SPARK_CLOUD_CONNECT = 0;
      SPARK_WLAN_SLEEP = 1;
      break;
  }
}

System_Mode_TypeDef SystemClass::mode(void)
{
  return _mode;
}

void SystemClass::factoryReset(void)
{
  //Work in Progress
  //This method will work only if the Core is supplied
  //with the latest version of Bootloader
  Factory_Reset_SysFlag = 0xAAAA;
  Save_SystemFlags();

  reset();
}

void SystemClass::bootloader(void)
{
  //Work in Progress
  //The drawback here being it will enter bootloader mode until firmware
  //is loaded again. Require bootloader changes for proper working.
  BKP_WriteBackupRegister(BKP_DR10, 0xFFFF);
  FLASH_OTA_Update_SysFlag = 0xFFFF;
  Save_SystemFlags();

  reset();
}

void SystemClass::reset(void)
{
  NVIC_SystemReset();
}

bool RGBClass::_control = false;

bool RGBClass::controlled(void)
{
	return _control;
}

void RGBClass::control(bool override)
{
#if !defined (RGB_NOTIFICATIONS_ON)
	if(override == _control)
		return;
	else if (override)
		LED_Signaling_Start();
	else
		LED_Signaling_Stop();

	_control = override;
#endif
}

void RGBClass::color(uint32_t rgb) {
    color((rgb>>16)&0xFF, (rgb>>8)&0xFF, (rgb)&0xFF);
}

void RGBClass::color(int red, int green, int blue)
{
#if !defined (RGB_NOTIFICATIONS_ON)
	if (true != _control)
		return;

	LED_SetSignalingColor(red << 16 | green << 8 | blue);
	LED_On(LED_RGB);
#endif
}

void RGBClass::brightness(uint8_t brightness, bool update)
{
#if !defined (RGB_NOTIFICATIONS_ON)	
	LED_SetBrightness(brightness);
    if (_control && update)
        LED_On(LED_RGB);
#endif
}

void Enter_STOP_Mode(void)
{
  if((BKP_ReadBackupRegister(BKP_DR9) >> 12) == 0xA)
  {
    uint16_t wakeUpPin = BKP_ReadBackupRegister(BKP_DR9) & 0xFF;
    // InterruptMode edgeTriggerMode = (InterruptMode)((BKP_ReadBackupRegister(BKP_DR9) >> 8) & 0x0F);

    /* Clear Stop mode system flag */
    BKP_WriteBackupRegister(BKP_DR9, 0xFFFF);

    // if ((wakeUpPin < TOTAL_PINS) && (edgeTriggerMode <= FALLING))
    // {
    //   PinMode wakeUpPinMode = INPUT;

    //   /* Set required pinMode based on edgeTriggerMode */
    //   switch(edgeTriggerMode)
    //   {
    //     case CHANGE:
    //       wakeUpPinMode = INPUT;
    //       break;

    //     case RISING:
    //       wakeUpPinMode = INPUT_PULLDOWN;
    //       break;

    //     case FALLING:
    //       wakeUpPinMode = INPUT_PULLUP;
    //       break;
    //   }
    //   pinMode(wakeUpPin, wakeUpPinMode);

    //   /* Configure EXTI Interrupt : wake-up from stop mode using pin interrupt */
    //   attachInterrupt(wakeUpPin, NULL, edgeTriggerMode);

    //   /* Request to enter STOP mode with regulator in low power mode */
    //   PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);

    //   /* At this stage the system has resumed from STOP mode */
    //   /* Enable HSE, PLL and select PLL as system clock source after wake-up from STOP */

    //   /* Enable HSE */
    //   RCC_HSEConfig(RCC_HSE_ON);

    //   /* Wait till HSE is ready */
    //   if(RCC_WaitForHSEStartUp() != SUCCESS)
    //   {
    //     /* If HSE startup fails try to recover by system reset */
    //     NVIC_SystemReset();
    //   }

    //   /* Enable PLL */
    //   RCC_PLLCmd(ENABLE);

    //   /* Wait till PLL is ready */
    //   while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);

    //   /* Select PLL as system clock source */
    //   RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

    //   /* Wait till PLL is used as system clock source */
    //   while(RCC_GetSYSCLKSource() != 0x08);
    // }
  }
}

inline uint8_t isSocketClosed()
{
  uint8_t closed  = get_socket_active_status(sparkSocket)==SOCKET_STATUS_INACTIVE;

  if(closed)
  {
      DEBUG("get_socket_active_status(sparkSocket=%d)==SOCKET_STATUS_INACTIVE", sparkSocket);
  }
  if(closed && sparkSocket != INVALID_SOCKET)
  {
      DEBUG("!!!!!!closed && sparkSocket(%d) != INVALID_SOCKET", sparkSocket);
  }
  if(sparkSocket == INVALID_SOCKET)
    {
      DEBUG("sparkSocket == INVALID_SOCKET");
      closed = true;
    }
  return closed;
}

// Returns number of bytes sent or -1 if an error occurred
int Spark_Send(const unsigned char *buf, int buflen)
{
  if(SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || isSocketClosed())
  {
    DEBUG("SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || isSocketClosed()");
    //break from any blocking loop
    return -1;
  }

  // send returns negative numbers on error
  int bytes_sent = send(sparkSocket, buf, buflen, 0);

  return bytes_sent;
}

// Returns number of bytes received or -1 if an error occurred
int Spark_Receive(unsigned char *buf, int buflen)
{
  if(SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || isSocketClosed())
  {
    //break from any blocking loop
    DEBUG("SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || isSocketClosed()");
    return -1;
  }

  timeval timeout;
  _types_fd_set_cc3000 readSet;
  int bytes_received = 0;
  int num_fds_ready = 0;

  // reset the fd_set structure
  FD_ZERO(&readSet);
  FD_SET(sparkSocket, &readSet);

  // tell select to timeout after the minimum 5000 microseconds
  // defined in the SimpleLink API as SELECT_TIMEOUT_MIN_MICRO_SECONDS
  timeout.tv_sec = 0;
  timeout.tv_usec = 5000;

  num_fds_ready = select(sparkSocket + 1, &readSet, NULL, NULL, &timeout);

  if (0 < num_fds_ready)
  {
    if (FD_ISSET(sparkSocket, &readSet))
    {
      // recv returns negative numbers on error
      bytes_received = recv(sparkSocket, buf, buflen, 0);
      DEBUG("bytes_received %d",bytes_received);
    }
  }
  else if (0 > num_fds_ready)
  {
    // error from select
   DEBUG("select Error %d",num_fds_ready);
    return num_fds_ready;
  }
  return bytes_received;
}

void Spark_Prepare_For_Firmware_Update(void)
{
  SPARK_FLASH_UPDATE = 1;
  TimingFlashUpdateTimeout = 0;
  FLASH_Begin(EXTERNAL_FLASH_OTA_ADDRESS);
}

void Spark_Finish_Firmware_Update(void)
{
  SPARK_FLASH_UPDATE = 0;
  TimingFlashUpdateTimeout = 0;
  FLASH_End();
}

uint16_t Spark_Save_Firmware_Chunk(unsigned char *buf, long unsigned int buflen)
{
  TimingFlashUpdateTimeout = 0;
  return FLASH_Update(buf, buflen);
}

int numUserFunctions(void)
{
  return User_Func_Count;
}

int numUserVariables(void)
{
  return User_Var_Count;
}

void copyUserVariableKey(char *destination, int variable_index)
{
  memcpy(destination,
         User_Var_Lookup_Table[variable_index].userVarKey,
         USER_VAR_KEY_LENGTH);
}

SparkReturnType::Enum wrapVarTypeInEnum(const char *varKey)
{
  switch (userVarType(varKey))
  {
    case 1:
      return SparkReturnType::BOOLEAN;
      break;

    case 4:
      return SparkReturnType::STRING;
      break;

    case 9:
      return SparkReturnType::DOUBLE;
      break;

    case 2:
    default:
      return SparkReturnType::INT;
  }
}

void Spark_Protocol_Init(void)
{
  if (!spark_protocol.is_initialized())
  {
    SparkCallbacks callbacks;
    callbacks.send = Spark_Send;
    callbacks.receive = Spark_Receive;
    callbacks.prepare_for_firmware_update = Spark_Prepare_For_Firmware_Update;
    callbacks.finish_firmware_update = Spark_Finish_Firmware_Update;
    callbacks.calculate_crc = Compute_CRC32;
    callbacks.save_firmware_chunk = Spark_Save_Firmware_Chunk;
    callbacks.signal = Spark_Signal;
    callbacks.millis = millis;
    callbacks.set_time = Time.setTime;

    SparkDescriptor descriptor;
    descriptor.num_functions = numUserFunctions;
    descriptor.num_variables = numUserVariables;
    descriptor.copy_variable_key = copyUserVariableKey;
    descriptor.variable_type = wrapVarTypeInEnum;
    descriptor.get_variable = getUserVar;
    descriptor.was_ota_upgrade_successful = OTA_Flashed_GetStatus;
    descriptor.ota_upgrade_status_sent = OTA_Flashed_ResetStatus;

    unsigned char pubkey[EXTERNAL_FLASH_SERVER_PUBLIC_KEY_LENGTH];
    unsigned char private_key[EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH];

    SparkKeys keys;
    keys.server_public = pubkey;
    keys.core_private = private_key;

    FLASH_Read_ServerPublicKey(pubkey);
    FLASH_Read_CorePrivateKey(private_key);

    spark_protocol.init((const char *)ID1, keys, callbacks, descriptor);
  }
}

int Spark_Handshake(void)
{
  Spark_Protocol_Init();
  spark_protocol.reset_updating();
  int err = spark_protocol.handshake();

  Multicast_Presence_Announcement();
  spark_protocol.send_time_request();

  unsigned char patchver[2];
  nvmem_read_sp_version(patchver);
  char patchstr[8];
  snprintf(patchstr, 8, "%d.%d", patchver[0], patchver[1]);
  //Spark.publish("spark/cc3000-patch-version", patchstr, 60, PRIVATE);

  return err;
}

// Returns true if all's well or
//         false on error, meaning we're probably disconnected
bool Spark_Communication_Loop(void)
{
  return spark_protocol.event_loop();
}

void Multicast_Presence_Announcement(void)
{
  long multicast_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (0 > multicast_socket)
    return;

  unsigned char announcement[19];
  spark_protocol.presence_announcement(announcement, (const char *)ID1);

  // create multicast address 224.0.1.187 port 5683
  sockaddr addr;
  addr.sa_family = AF_INET;
  addr.sa_data[0] = 0x16; // port MSB
  addr.sa_data[1] = 0x33; // port LSB
  addr.sa_data[2] = 0xe0; // IP MSB
  addr.sa_data[3] = 0x00;
  addr.sa_data[4] = 0x01;
  addr.sa_data[5] = 0xbb; // IP LSB

  //why loop here? Uncommenting this leads to SOS(HardFault Exception) on local cloud
  //for (int i = 3; i > 0; i--)
  {
    sendto(multicast_socket, announcement, 19, 0, &addr, sizeof(sockaddr));
  }

  closesocket(multicast_socket);
}

/* This function MUST NOT BlOCK!
 * It will be executed every 1ms if LED_Signaling_Start() is called
 * and stopped as soon as LED_Signaling_Stop() is called */
void LED_Signaling_Override(void)
{
  if (0 < LED_Signaling_Timing)
  {
    --LED_Signaling_Timing;
  }
  else
  {
    LED_SetSignalingColor(VIBGYOR_Colors[VIBGYOR_Index]);
    LED_On(LED_RGB);

    LED_Signaling_Timing = 100;	// 100 ms

    ++VIBGYOR_Index;
    if(VIBGYOR_Index >= VIBGYOR_Size)
    {
      VIBGYOR_Index = 0;
    }
  }
}

void Spark_Signal(bool on)
{
  if (on)
  {
    LED_Signaling_Start();
    LED_Spark_Signal = 1;
  }
  else
  {
    LED_Signaling_Stop();
    LED_Spark_Signal = 0;
  }
}

int Internet_Test(void)
{
    long testSocket;
    sockaddr testSocketAddr;
    int testResult = 0;
    DEBUG("socket");
    testSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    DEBUG("socketed testSocket=%d",testSocket);


    if (testSocket < 0)
    {
        return -1;
    }

	// the family is always AF_INET
    testSocketAddr.sa_family = AF_INET;

	// the destination port: 53
    testSocketAddr.sa_data[0] = 0;
    testSocketAddr.sa_data[1] = 53;

    // the destination IP address: 8.8.8.8
    testSocketAddr.sa_data[2] = 8;
    testSocketAddr.sa_data[3] = 8;
    testSocketAddr.sa_data[4] = 8;
    testSocketAddr.sa_data[5] = 8;

    uint32_t ot = SPARK_WLAN_SetNetWatchDog(S2M(MAX_SEC_WAIT_CONNECT));
    DEBUG("connect");
    testResult = connect(testSocket, &testSocketAddr, sizeof(testSocketAddr));
    DEBUG("connected testResult=%d",testResult);
    SPARK_WLAN_SetNetWatchDog(ot);

#if defined(SEND_ON_CLOSE)
    DEBUG("send");
    char c = 0;
    int rc = send(testSocket, &c,1, 0);
    DEBUG("send %d",rc);
#endif

    DEBUG("Close");
    int rv = closesocket(testSocket);
    DEBUG("Closed rv=%d",rv);

    //if connection fails, testResult returns -1
    return testResult;
}

// Same return value as connect(), -1 on error
int Spark_Connect(void)
{
  DEBUG("sparkSocket Now =%d",sparkSocket);

  // Close Original

  Spark_Disconnect();

  sparkSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  DEBUG("socketed sparkSocket=%d",sparkSocket);

  if (sparkSocket < 0)
  {
    return -1;
  }

  // the family is always AF_INET
  tSocketAddr.sa_family = AF_INET;

  // the destination port
  tSocketAddr.sa_data[0] = (SPARK_SERVER_PORT & 0xFF00) >> 8;
  tSocketAddr.sa_data[1] = (SPARK_SERVER_PORT & 0x00FF);

  ServerAddress server_addr;
  FLASH_Read_ServerAddress(&server_addr);

  uint32_t ip_addr = 0;

  switch (server_addr.addr_type)
  {
    case IP_ADDRESS:
      ip_addr = server_addr.ip;
      break;

    default:
    case INVALID_INTERNET_ADDRESS:
    {
      const char default_domain[] = "device.spark.io";
      // Make sure we copy the NULL terminator, so subsequent strlen() calls on server_addr.domain return the correct length
      memcpy(server_addr.domain, default_domain, strlen(default_domain) + 1);
      // and fall through to domain name case
    }

    case DOMAIN_NAME:
      // CC3000 unreliability workaround, usually takes 2 or 3 attempts
      int attempts = 10;
      while (!ip_addr && 0 < --attempts)
      {
        gethostbyname(server_addr.domain, strnlen(server_addr.domain, 126), &ip_addr);
      }
  }

  if (!ip_addr)
  {
    // final fallback
    ip_addr = (54 << 24) | (208 << 16) | (229 << 8) | 4;
  }

  tSocketAddr.sa_data[2] = BYTE_N(ip_addr, 3);
  tSocketAddr.sa_data[3] = BYTE_N(ip_addr, 2);
  tSocketAddr.sa_data[4] = BYTE_N(ip_addr, 1);
  tSocketAddr.sa_data[5] = BYTE_N(ip_addr, 0);

  uint32_t ot = SPARK_WLAN_SetNetWatchDog(S2M(MAX_SEC_WAIT_CONNECT));
  DEBUG("connect");
  int rv = connect(sparkSocket, &tSocketAddr, sizeof(tSocketAddr));
  DEBUG("connected connect=%d",rv);
  SPARK_WLAN_SetNetWatchDog(ot);
  return rv;
}

int Spark_Disconnect(void)
{
  int retVal= 0;
  DEBUG("");
  if (sparkSocket >= 0)
  {
#if defined(SEND_ON_CLOSE)
      DEBUG("send");
      char c = 0;
      int rc = send(sparkSocket, &c,1, 0);
      DEBUG("send %d",rc);
#endif
      DEBUG("Close");
      retVal = closesocket(sparkSocket);
      DEBUG("Closed retVal=%d", retVal);
      sparkSocket = INVALID_SOCKET;
  }
  return retVal;
}

void Spark_ConnectAbort_WLANReset(void)
{
	//Work around to exit the blocking nature of socket calls
	tSLInformation.usEventOrDataReceived = 1;
	tSLInformation.usRxEventOpcode = 0;
	tSLInformation.usRxDataPending = 0;

	SPARK_WLAN_RESET = 1;
}

int userVarType(const char *varKey)
{
	for (int i = 0; i < User_Var_Count; ++i)
	{
		if (0 == strncmp(User_Var_Lookup_Table[i].userVarKey, varKey, USER_VAR_KEY_LENGTH))
		{
			return User_Var_Lookup_Table[i].userVarType;
		}
	}
	return -1;
}

void *getUserVar(const char *varKey)
{
	for (int i = 0; i < User_Var_Count; ++i)
	{
		if (0 == strncmp(User_Var_Lookup_Table[i].userVarKey, varKey, USER_VAR_KEY_LENGTH))
		{
			return User_Var_Lookup_Table[i].userVar;
		}
	}
	return NULL;
}

long socket_connect(long sd, const sockaddr *addr, long addrlen)
{
	return connect(sd, addr, addrlen);
}
