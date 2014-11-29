/*!
 * @file hound_time.cpp
 * 
 * @brief HOUND NTP Sync Function Implementations
 * 
 * @author Benjamin Carlson
 * @author Blake Bourque
 * 
 * @date November 13, 2014
 * 
 * NTP Time sync library to manage initial request, return values, and updating
 * real time clock with updated time data.
 * 
 */

#include "hound_time.h"

// Standard Libraries
#include <string.h>
// HOUND Libraries
// ST Libraries
#include "stm32f10x_rtc.h"
// CC3000 Libraries
#include "data_types.h"
#include "socket.h"

 // These Need to be Phased Out
#include "spark_wlan.h"
#include "spark_macros.h"

#define NTP_PACKET_SIZE 48

int generateNTPRequest(uint8_t * sendBuffer, int sendBufferSize)
{
	if (sendBufferSize < NTP_PACKET_SIZE)
	{
		// If send buffer is less than 48 bytes, NTP can't happen
		return -1;
	}

	memset(sendBuffer, 0, NTP_PACKET_SIZE);
	sendBuffer[0] = 0xe3;
	sendBuffer[2] = 0x06;
	sendBuffer[3] = 0xea;

	return NTP_PACKET_SIZE;
}

int sendNTPRequest(uint8_t * sendBuffer, int sendBufferSize)
{
	int ret, ntpSockHandle, sent;
	sockaddr ntpSockAddress;
	socklen_t sockAddressSize = sizeof(ntpSockAddress);

	// Create new socket and setup recive timeout
	ntpSockHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (ntpSockHandle == -1)
	{
		// We couldn't open up a new socket
		return -2;
	}

	UINT32 timeout_ms = 30000;
	setsockopt(ntpSockHandle, SOL_SOCKET, SOCKOPT_RECV_TIMEOUT, (void*)&timeout_ms, sizeof(UINT32));

	// We need to listen on port 123 (ntp) for response from server
	ntpSockAddress.sa_family = AF_INET;
	ntpSockAddress.sa_data[0] = (123 >> 8 ) & 0xFF;
	ntpSockAddress.sa_data[1] = (123 & 0xFF);

	ntpSockAddress.sa_data[2] = 0;
	ntpSockAddress.sa_data[3] = 0;
	ntpSockAddress.sa_data[4] = 0;
	ntpSockAddress.sa_data[5] = 0;

	// Bind socket to a local port
	ret = bind(ntpSockHandle, &ntpSockAddress, sockAddressSize);

	// Bind Failure
	if (ret != 0)
	{
		// Close socket and get out of there
		closesocket(ntpSockHandle);
		return -3;
	}

	ntpSockAddress.sa_data[2] = 198;
	ntpSockAddress.sa_data[3] = 55;
	ntpSockAddress.sa_data[4] = 111;
	ntpSockAddress.sa_data[5] = 5;

    // Send data to NTP server
    sent = 0;
    while(sent < NTP_PACKET_SIZE)
    {
    	ret = sendto(ntpSockHandle, sendBuffer + sent, NTP_PACKET_SIZE - sent, 0, &ntpSockAddress, sockAddressSize);

    	sent += ret;
    }

    // Get response from NTP server
    ret = recvfrom(ntpSockHandle, sendBuffer, sendBufferSize, 0, &ntpSockAddress, &sockAddressSize);

	closesocket(ntpSockHandle);

	// Return number of bytes recieved
	return ret;

}

void parseNTPResponse(uint8_t * recieveBuffer, int recieveBufferSize)
{
	uint32_t baseTimestamp;
	// Enable RTC modifications
	RTC_EnterConfigMode();

	if (recieveBufferSize > 44)
	{
		baseTimestamp = recieveBuffer[40] << 8 | recieveBuffer[41];
		baseTimestamp <<= 16;
		baseTimestamp |= (recieveBuffer[42] << 8) | recieveBuffer[43];

		// NTP Server returns seconds since 1900, make that unix time
		baseTimestamp = baseTimestamp - 2208988800UL;

		RTC_SetCounter(baseTimestamp);
	}

	RTC_ExitConfigMode();
}