#include "hound_time.h"

#include "data_types.h"
#include "socket.h"
#include "spark_wlan.h"
#include "spark_macros.h"

#include <string.h>
#include "stm32f10x_rtc.h"

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
	sendBuffer[3] = 0xec;
	sendBuffer[12] = 49;
	sendBuffer[13] = 0x4E;
	sendBuffer[14] = 49;
	sendBuffer[15] = 52;

	return NTP_PACKET_SIZE;
}

int sendNTPRequest(uint8_t * sendBuffer, int sendBufferSize)
{
	int ret, sockHandle, sendHandle, sent;
	sockaddr sockAddress, sendAddress;
	socklen_t sockAddressSize = sizeof(sockAddress);

	sockHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	UINT32 timeout_ms = 1000;
	setsockopt(sockHandle, SOL_SOCKET, SOCKOPT_RECV_TIMEOUT, (void*)&timeout_ms, sizeof(UINT32));

	if (sockHandle == -1)
	{
		// We couldn't open up a new socket
		return -1;
	}

	// We need to listen on port 123 (ntp) for response froms erver
	sockAddress.sa_family = AF_INET;
	sockAddress.sa_data[0] = (123 >> 8 ) & 0xFF;
	sockAddress.sa_data[1] = (123 & 0xFF);

	sockAddress.sa_data[2] = 0;
	sockAddress.sa_data[3] = 0;
	sockAddress.sa_data[4] = 0;
	sockAddress.sa_data[5] = 0;

	ret = bind(sockHandle, &sockAddress, sockAddressSize);

	// Bind Failure
	if (ret != 0)
	{
		// Close socket and get out of there
		closesocket(sockHandle);
		return -1;
	}

	ret = listen(sockHandle, 1);

	if (ret != 0)
	{
		closesocket(sockHandle);
		return -1;
	}

	// Listening ready, make request
	sendHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (sendHandle == -1)
	{
		closesocket(sockHandle);
		return -1;
	}

	sendAddress.sa_family = AF_INET;
	sendAddress.sa_data[0] = (123 >> 8 ) & 0xFF;
	sendAddress.sa_data[1] = (123 & 0xFF);

	sendAddress.sa_data[2] = 217;
	sendAddress.sa_data[3] = 79;
	sendAddress.sa_data[4] = 179;
	sendAddress.sa_data[5] = 106;

	// Make connection to NTP server
	uint32_t ot = SPARK_WLAN_SetNetWatchDog(S2M(MAX_SEC_WAIT_CONNECT));
    ret = connect(sendHandle, &sendAddress, sizeof(sendAddress));
    SPARK_WLAN_SetNetWatchDog(ot);

    // Connection error handling
    if (ret < 0)
    {
		closesocket(sockHandle);
		closesocket(sendHandle);
		return -1;
    }

    // Send data to NTP server
    sent = 0;
    while(sent < NTP_PACKET_SIZE)
    {
    	ret = send(sendHandle, sendBuffer + sent, NTP_PACKET_SIZE - sent, 0);

    	sent += ret;
    }

    // Get NTP response
    ret = recv(sockHandle, sendBuffer, sendBufferSize, 0);

	closesocket(sockHandle);
	closesocket(sendHandle);

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