/*!
 * @file com_proto.cpp
 * 
 * @brief HOUND Communication Libraries
 * 
 * @author Benjamin Carlson
 * @author Blake Bourque
 * 
 * @date October 1, 2014
 * 
 *
 * Definitions for socket based network communication.
 * 
 */

#include "com_proto.h"

// Standard Libraries
#include <string.h>
#include <stdlib.h>
// HOUND Libraries
#include "hound_debug.h"
// ST Libraries
// TODO: REMOVE
#include "spark_wlan.h"
#include "spark_macros.h"
#include "rgbled.h"


// G++ doesn't like constant strings
#pragma GCC diagnostic ignored "-Wwrite-strings"

using namespace Communication;

// Ctor
HoundProto::HoundProto(int listenPort)
{
	int ret;
	m_sockAddress = (sockaddr *)malloc(sizeof(sockaddr));
	m_sockAddressSize = sizeof(sockaddr);

	// Open a new TCP socket to listen on
	m_sockHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	// // Enable "non-blocking" mode for accept
	// UINT32 var = SOCK_OFF;
    //   setsockopt(m_sockHandle, SOL_SOCKET, SOCKOPT_ACCEPT_NONBLOCK, (void*)&var, sizeof(UINT32));

    // Make sure we don't ever block forever on our socket
    // Even though data SHOULD be there if it's opened in this case
    UINT32 timeout_ms = 1000;
    setsockopt(m_sockHandle, SOL_SOCKET, SOCKOPT_RECV_TIMEOUT, (void*)&timeout_ms, sizeof(UINT32));


    // Need to also destroy object gracefully if we couldn't open the socket
    if (m_sockHandle == -1)
    {
        LED_SetRGBColor(RGB_COLOR_RED);
        #ifdef DEBUG_ON
    	HoundDebug::logError(LOG_LOCATION_LCD, -1, "Socket Creation");
        #endif

    	return;
    }

    // Only supported family of CC3000
    m_sockAddress->sa_family = AF_INET;

    // Port to open
    m_sockAddress->sa_data[0] = (listenPort >> 8) & 0xFF;
    m_sockAddress->sa_data[1] = (listenPort & 0xFF);

    // POF: Not sure if loopback is implemented on CC3000
    m_sockAddress->sa_data[2] = 0;
    m_sockAddress->sa_data[3] = 0;
    m_sockAddress->sa_data[4] = 0;
    m_sockAddress->sa_data[5] = 0;

    ret = bind(m_sockHandle, m_sockAddress, m_sockAddressSize);

    // Binding failed
    if (ret != 0)
    {
        #ifdef DEBUG_ON
    	HoundDebug::logError(LOG_LOCATION_LCD, ret, "Bind Failed");
        #endif

    	closesocket(m_sockHandle);
    	m_sockHandle = -1;
    	return;
    }

    // Enable listening on specified port with 1 connection buffer;
    ret = listen(m_sockHandle, 1);

    if (ret < 0)
    {
        #ifdef DEBUG_ON
    	HoundDebug::logError(LOG_LOCATION_LCD, ret, "Listen failed");
        #endif

    	closesocket(m_sockHandle);
    	m_sockHandle = -1;
    	return;
    }

   	// Reset socket address struct
	memset(m_sockAddress, 0, m_sockAddressSize);
}

// Dtor
HoundProto::~HoundProto()
{
	closesocket(m_sockHandle);
}

// TODO: monitor socket state, if for some reason it closes, reopen (can't loose this one)
int HoundProto::getData(uint8_t * recvBuffer, int buffSize, ipAddr_t * ipAddr)
{
	int ret;
    _types_fd_set_cc3000 set;
    sockaddr recvAddr;
    socklen_t recvAddr_size = sizeof(recvAddr);

    if (m_sockHandle == -1)
    {
        #ifdef DEBUG_ON
        HoundDebug::logError(LOG_LOCATION_LCD, -1, "SOCKET");
        #endif

        return -1;
    }

    struct timeval del;
    del.tv_sec = 0;
    del.tv_usec = 5000;

    FD_ZERO(&set);
    FD_SET(m_sockHandle, &set); 

	ret = select(m_sockHandle +1, &set, NULL, NULL, &del);

	if (ret > 0)
	{
        // Get data and figure out where it's from
	    ret = recvfrom(m_sockHandle, recvBuffer, buffSize, 0, &recvAddr, &recvAddr_size);
        ipAddr->oct[0] = recvAddr.sa_data[2];
        ipAddr->oct[1] = recvAddr.sa_data[3];
        ipAddr->oct[2] = recvAddr.sa_data[4];
        ipAddr->oct[3] = recvAddr.sa_data[5];

	    if (ret > 0)
	    {
            #ifdef DEBUG_ON
	    	HoundDebug::logMessage(ret, "Recived Data");
            #endif

	    } else {
            #ifdef DEBUG_ON
	    	HoundDebug::logError(LOG_LOCATION_LCD, -1, "Recive Failure");
            #endif
	    }

	    memset(m_sockAddress, 0, m_sockAddressSize);

	} else {
		#ifdef DEBUG_ON
		HoundDebug::logError(LOG_LOCATION_LCD, -1, "No Data");
        #endif
	}

	return ret;

}

int HoundProto::sendData(uint8_t * sendBuffer, int bufferSize, ipAddr_t * ipAddr)
{
    int sendSocket, ret;
    unsigned int sent = 0;
    sockaddr connectAddress;

    sendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // Need to also destroy object gracefully if we couldn't open the socket
    if (sendSocket == -1)
    {

        #ifdef DEBUG_ON
        HoundDebug::logError(LOG_LOCATION_LCD, -1, "Socket Creation");
        #endif

        return -1;
    }

    // Only supported family of CC3000
    connectAddress.sa_family = AF_INET;

    // Port to open
    connectAddress.sa_data[0] = (8081 >> 8) & 0xFF;
    connectAddress.sa_data[1] = (8081 & 0xFF);

    // Needs to be dynamic based on incoming request
    connectAddress.sa_data[2] = ipAddr->oct[0];
    connectAddress.sa_data[3] = ipAddr->oct[1];
    connectAddress.sa_data[4] = ipAddr->oct[2];
    connectAddress.sa_data[5] = ipAddr->oct[3];

    while (sent < bufferSize)
    {
        ret = ::sendto(sendSocket, sendBuffer + sent, bufferSize - sent, 0, &connectAddress, sizeof(sockaddr));

        if (ret <= 0)
        {
            if (ret == 0)
                LED_SetRGBColor(RGB_COLOR_WHITE);
            else 
                LED_SetRGBColor(RGB_COLOR_RED);
            #ifdef DEBUG_ON
            HoundDebug::logError(LOG_LOCATION_LCD, ret, "Send Error");
            #endif
            closesocket(sendSocket);
            return ret;
        }

        sent += ret;
    }

    closesocket(sendSocket);

    return 0;
}
