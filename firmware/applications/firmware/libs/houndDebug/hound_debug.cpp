/*!
 * @file hound_debug.cpp
 * 
 * @brief HOUND Debug Library
 * 
 * @author Benjamin Carlson
 * @author Blake Bourque
 * 
 * @date November 20, 2014
 * 
 * Functional implementation for HOUND Debug class.  Allows for remote and local debugging
 * using HD44780 library, remote socket library, and LED.
 */

// Standard Libraries
#include <stdio.h>
// HOUND Libraries
#include "hd44780.h"
#include "hound_debug.h"
// ST Libraries
// TI Libraries
#include "data_types.h"	// type defines for socket.h/c
#include "socket.h"	// CC3000 std driver library
#include "rgbled.h"

// G++ doesn't like constant strings
#pragma GCC diagnostic ignored "-Wwrite-strings"

void HoundDebug::logError(int location, int error_number, char * log_message)
{
	HoundDebug::log(location, error_number, log_message);
}

void HoundDebug::logError(int error_number, char * log_message)
{
	HoundDebug::log(DEFAULT_LOG_LOCATION, error_number, log_message);
}

void HoundDebug::logMessage(int message_number, char * log_message)
{
	HoundDebug::log(DEFAULT_LOG_LOCATION, message_number, log_message);
}

void HoundDebug::log(int location, int error_number, char * log_message)
{
	#ifndef DISABLE_LCD_LOG
		if (location & LOG_LOCATION_LCD)
		{
			HD44780 * lcd = HD44780::getInstance();
			lcd->clear();
			lcd->printf("Error: %d", error_number);
			lcd->setPosition(1, 0);
			lcd->printf("%s", log_message);
		}
	#endif

	#ifndef DISABLE_NET_LOG
		if (location & LOG_LOCATION_NET)
		{
			char debugMessage[DEBUG_BUFF_SIZE] = {0};
			int sendSocket, ret;
		    unsigned int sent = 0;
		    sockaddr connectAddress;

		    sendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		    if (sendSocket == -1)
		    {
		        return;
		    }

		    // Only supported family of CC3000
		    connectAddress.sa_family = AF_INET;

		    // Port to open (8088 is debug)
		    connectAddress.sa_data[0] = (LOG_NET_PORT >> 8) & 0xFF;
		    connectAddress.sa_data[1] = (LOG_NET_PORT & 0xFF);

		    // 224.111.112.114 is defined as net debug port
		    connectAddress.sa_data[2] = 224;
		    connectAddress.sa_data[3] = 111;
		    connectAddress.sa_data[4] = 112;
		    connectAddress.sa_data[5] = 113;

		    snprintf(debugMessage, DEBUG_BUFF_SIZE, "E: %d - %s", error_number, log_message);

		    while (sent < DEBUG_BUFF_SIZE)
		    {
		        ret = ::sendto(sendSocket, debugMessage + sent, DEBUG_BUFF_SIZE - sent, 0, &connectAddress, sizeof(sockaddr));

		        if (ret <= 0)
		        {
		            #ifdef DEBUG_ON
		            HoundDebug::logError(LOG_LOCATION_LCD, ret, "Send Error");
		            #endif

		            closesocket(sendSocket);
		            return;
		        }

		        sent += ret;
		    }

		    closesocket(sendSocket);

		    return;
		}
	#endif

	if (location & LOG_LOCATION_LED)
	{

	}
}