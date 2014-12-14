#include "websocket.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "hound_debug.h"
#include "cc3000_common.h"

/**
 * Contructor
 * char*    host    Must be an ipaddress in the form "255.255.255.255"
 */
WebSocket::WebSocket(char * host, int port)
{
    int ret;
    sockaddr connectAddress;

    // Open a socket and get the handle
    m_sockHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    UINT32 timeout_ms = 1000;
    setsockopt(m_sockHandle, SOL_SOCKET, SOCKOPT_RECV_TIMEOUT, (void*)&timeout_ms, sizeof(UINT32));

    // On -1 return, socket creation had an error
    if (m_sockHandle == -1)
    {
        HoundDebug::logError(LOG_LOCATION_LCD, -1, "Socket Creation");
        // TODO: Handle error appropriately
        return;
    }

    // TODO: Evaluate need for member variables
    // Where we'll be connecting to
    strcpy(m_hostAddr, host);
    m_hostPort = port;

    // the family is always AF_INET
    connectAddress.sa_family = AF_INET;

    // the destination port: 8080
    // TODO: this should be hooked up to argument
    connectAddress.sa_data[0] = (port >> 8) & 0xff;
    connectAddress.sa_data[1] = (0xff & port);

    // Prepare the destination ip address
    char ipaddy[20];    // Do we need this?
    strncpy(ipaddy, host, 20);
    char * oct4 = strtok (ipaddy,".");
    char * oct3 = strtok (NULL,".");
    char * oct2 = strtok (NULL,".");
    char * oct1 = strtok (NULL,".");

    connectAddress.sa_data[2] = atoi(oct4);
    connectAddress.sa_data[3] = atoi(oct3);
    connectAddress.sa_data[4] = atoi(oct2);
    connectAddress.sa_data[5] = atoi(oct1);

    // TODO: Look into this
    // uint32_t ot = SPARK_WLAN_SetNetWatchDog(S2M(MAX_SEC_WAIT_CONNECT));
    ret = connect(m_sockHandle, (sockaddr *)&connectAddress, sizeof(connectAddress));
    // SPARK_WLAN_SetNetWatchDog(ot);

    if (ret < 0)
    {
        // Alert User
        HoundDebug::logError(LOG_LOCATION_LCD, -2, "Connection Error");
        
        // Cleanup and return
        closesocket(m_sockHandle);
        m_sockHandle = -1;
        return;
    }


    // Our send/recieve buffer
    //m_dataBuffer = (char *)malloc(BUFF_SIZE);

    upgradeConnection();

    return;
}

WebSocket::WebSocket(Communication::ipAddr_t * host, int port)
{
    int ret;
    sockaddr connectAddress;

    // Open a socket and get the handle
    m_sockHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    UINT32 timeout_ms = 1000;
    setsockopt(m_sockHandle, SOL_SOCKET, SOCKOPT_RECV_TIMEOUT, (void*)&timeout_ms, sizeof(UINT32));

    // On -1 return, socket creation had an error
    if (m_sockHandle == -1)
    {
        HoundDebug::logError(LOG_LOCATION_LCD, -1, "Socket Creation");
        // TODO: Handle error appropriately
        return;
    }

    // TODO: Evaluate need for member variables
    // Where we'll be connecting to
    m_hostPort = port;

    // the family is always AF_INET
    connectAddress.sa_family = AF_INET;

    // the destination port: 8080
    // TODO: this should be hooked up to argument
    connectAddress.sa_data[0] = (port >> 8) & 0xff;
    connectAddress.sa_data[1] = (0xff & port);

    connectAddress.sa_data[2] = host->oct[0];
    connectAddress.sa_data[3] = host->oct[1];
    connectAddress.sa_data[4] = host->oct[2];
    connectAddress.sa_data[5] = host->oct[3];

    // uint32_t ot = SPARK_WLAN_SetNetWatchDog(S2M(MAX_SEC_WAIT_CONNECT));
    ret = connect(m_sockHandle, (sockaddr *)&connectAddress, sizeof(connectAddress));
    // SPARK_WLAN_SetNetWatchDog(ot);

    if (ret < 0)
    {
        // Alert User
        HoundDebug::logError(LOG_LOCATION_LCD, -2, "Connection Error");
        
        // Cleanup and return
        closesocket(m_sockHandle);
        m_sockHandle = -1;
        return;
    }


    // Our send/recieve buffer
    //m_dataBuffer = (char *)malloc(BUFF_SIZE);

    upgradeConnection();

    return;
}

WebSocket::~WebSocket()
{
    closesocket(m_sockHandle);
}

int WebSocket::checkSocketStatus(int operation_check)
{
    int ret = 0;
    _types_fd_set_cc3000 set;

    struct timeval delay;
    delay.tv_sec = 0;
    delay.tv_usec = 5000;

    FD_ZERO(&set);
    FD_SET(m_sockHandle, &set); 

    if (operation_check & CHECK_SOCK_READ)
    {
        ret = select(m_sockHandle + 1, &set, NULL, NULL, &delay);
    } else if (operation_check & CHECK_SOCK_EXCEPT) {
        ret = select(m_sockHandle + 1, NULL, NULL, &set, &delay);
    } else if (operation_check & CHECK_SOCK_WRITE) {
        ret = select(m_sockHandle + 1, NULL, &set, NULL, &delay);
    }

    if (ret > 0)
    {
        return 1;
    }

    return 0;
}

int WebSocket::sendText(char * dataPacket)
{
    if (m_sockHandle == -1)
    {
        HoundDebug::logError(LOG_LOCATION_LCD, E_SOCKET_ERROR, "sendText");
        return E_SOCKET_ERROR;
    }

    // TODO: This is supposed to be secure
    uint8_t mask_key[4] = {0x12, 0x34, 0x56, 0x78};

    memset(m_dataBuffer, 0, BUFF_SIZE);

    // Assume we're sending one packet and it will be text formatted
    m_dataBuffer[0] = 0x80 | 0x01;

    unsigned int packetSize = strlen(dataPacket);

    // 8 is the number of frame bits needed to handle messages > 126 bytes
    if (packetSize < BUFF_SIZE - 8)
    {
        // Single packet, text formatted
        m_dataBuffer[0] = 0x80 | 0x01;

        if (packetSize < 126)
        {
            // Use mask (client-> server) and size
            m_dataBuffer[1] = 0x80 | packetSize;

            // Pass in our mask
            m_dataBuffer[2] = mask_key[0];
            m_dataBuffer[3] = mask_key[1];
            m_dataBuffer[4] = mask_key[2];
            m_dataBuffer[5] = mask_key[3];

            // Stream masked message in
            for (unsigned int i = 0; i < packetSize; i++)
            {
                m_dataBuffer[i+6] = dataPacket[i] ^ mask_key[i%4];
            }

            // Packet generation complete, send data
            send(6 + strlen(dataPacket));
        }
        else if (packetSize < 65536) {
            m_dataBuffer[1] = 0x80 | 126;
            m_dataBuffer[2] = (packetSize >> 8) & 0xFF;
            m_dataBuffer[3] = (packetSize >> 0) & 0xFF;

            // Pass in our mask
            m_dataBuffer[4] = mask_key[0];
            m_dataBuffer[5] = mask_key[1];
            m_dataBuffer[6] = mask_key[2];
            m_dataBuffer[7] = mask_key[3];

            // Stream masked message in, we know packetSize will fit in buffer
            for (unsigned int i = 0; i < packetSize; i++)
            {
                m_dataBuffer[i+8] = dataPacket[i] ^ mask_key[i%4];
            }

            // Packet generation complete, send data
            send(8 + strlen(dataPacket));
        }
    } else {
        unsigned int packetPosition = 0, transmitSize;
        
        // Single packet, text formatted
        m_dataBuffer[0] = 0x80 | 0x01;

        while(packetPosition < packetSize)
        {
            if (packetSize - packetPosition > BUFF_SIZE - 8)
            {
                // Packet is still to big, only transmit a buffer
                transmitSize = BUFF_SIZE - 8;

                // Indicate in framing that there are more packets and this is text
                m_dataBuffer[0] = 0x00 | 0x01;
            } else {
                // Last packet, less data
                transmitSize = packetSize - packetPosition;

                // Indcate that this is indeed the last packet
                m_dataBuffer[0] = 0x80 | 0x01;
            }

            m_dataBuffer[1] = 0x80 | 126;
            m_dataBuffer[2] = (transmitSize >> 8) & 0xFF;
            m_dataBuffer[3] = (transmitSize >> 0) & 0xFF;

            // Pass in our mask
            m_dataBuffer[4] = mask_key[0];
            m_dataBuffer[5] = mask_key[1];
            m_dataBuffer[6] = mask_key[2];
            m_dataBuffer[7] = mask_key[3];

            // Stream masked message in, we know packetSize will fit in buffer
            for (unsigned int i = 0; i < transmitSize; i++)
            {
                m_dataBuffer[i+8] = dataPacket[i] ^ mask_key[i%4];
            }

            // Move to the next bit of data;
            packetPosition += transmitSize;

            // Packet generation complete, send data
            return send(transmitSize);
        }
    }

}

// Send data currently in data buffer
// returns 0 on success
// returns E_SOCKET_ERROR or E_SEND_ERROR on error
int WebSocket::send(int sendBufferSize)
{
    unsigned int sent = 0;
    int ret;

    while (sent < sendBufferSize)
    {
        // should also check for sockets exceptions
        if (m_sockHandle == -1 || checkSocketStatus(CHECK_SOCK_EXCEPT))
        {
            HoundDebug::logError(LOG_LOCATION_LCD, E_SOCKET_ERROR, "send");
            return E_SOCKET_ERROR;
        }
        
        ret = ::send(m_sockHandle, m_dataBuffer + sent, sendBufferSize - sent, 0);

        // For example, if the socket is incorrectly closed
        if (ret <= 0)
        {
            HoundDebug::logError(LOG_LOCATION_LCD, E_SEND_ERROR, "send");
            return E_SEND_ERROR;
            // TODO: Error here (data didn't send)
        }

        sent += ret;
    }

    return 0;
}

// check for (and read) any buffered text for this socket
// returns 0 if no data was read, otherwise returns size of read
// returns E_SOCKET_ERROR on socket error, E_READ_ERROR
int WebSocket::readText(char * outputBuffer, int bufferSize)
{
    if (m_sockHandle == -1 || checkSocketStatus(CHECK_SOCK_EXCEPT))
    {
        HoundDebug::logError(LOG_LOCATION_LCD, E_SOCKET_ERROR, "read");
        return E_SOCKET_ERROR;
    }

    int ret;
    _types_fd_set_cc3000 set;

    struct timeval delay;
    delay.tv_sec = 0;
    delay.tv_usec = 5000;

    FD_ZERO(&set);
    FD_SET(m_sockHandle, &set); 

    ret = select(m_sockHandle + 1, &set, NULL, NULL, &delay);

    if (ret > 0)
    {
        ret = recv(m_sockHandle, m_dataBuffer, BUFF_SIZE, 0);

        if (ret > 0)
        {
            HoundDebug::logError(LOG_LOCATION_LCD, ret, "ReadData");

            // Let's not write to random places in memory
            memcpy(outputBuffer, m_dataBuffer, bufferSize > ret ? ret : bufferSize);

            return ret;
        } else {
            HoundDebug::logError(LOG_LOCATION_LCD, E_SOCKET_READ, "No Actual Data");

            return E_SOCKET_READ;
        }
    } else {
        HoundDebug::logError(LOG_LOCATION_LCD, 0, "No Updates");

        return 0;
    }

    return 0;
}

// Upgrades the current initialized websocket connection
void WebSocket::upgradeConnection()
{
    int count, ret;

    // Let's start fresh
    memset(m_dataBuffer, 0, BUFF_SIZE);

    const char temp[] = "GET / HTTP/1.1\r\nHost: %s\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\nSec-WebSocket-Version: 13\r\nOrigin: http://127.0.0.1\r\n\r\n";
    count = sprintf(m_dataBuffer, temp, "10.0.2.23");

    send(count);
    // while(sent < count)
    // {
    //     ret = send(m_sockHandle, m_dataBuffer + sent, count - sent, 0);
    //
    //     if (ret == -1)
    //     {
    //         // Data didn't send, why's that?
    //     }
    //
    //     sent += ret;
    // }

    // Reset the buffer to recieve data
    memset(m_dataBuffer, 0, BUFF_SIZE);

    ret = recv(m_sockHandle, m_dataBuffer, BUFF_SIZE, 0);

    if (ret > 0)
    {
        // We recieved data back
        // hopefully it's the server responding with an upgrade
        // let's check that later TODO:
    } else if (ret == -1) {
        // TODO: error here
    }

}
