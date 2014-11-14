#ifndef __HOUND_WEBSOCKET_H
#define __HOUND_WEBSOCKET_H

#include "com_proto.h"

extern "C"
{
    #include "data_types.h"
    #include "socket.h"
}

#define BUFF_SIZE 255

#define CHECK_SOCK_WRITE (1 << 0)
#define CHECK_SOCK_READ (1 << 1)
#define CHECK_SOCK_EXCEPT (1 << 2)

#define E_SOCKET_ERROR -10
#define E_SOCKET_READ -11
#define E_SEND_ERROR -12

class WebSocket
{
public:
    WebSocket(char * host, int port);
    WebSocket(Communication::ipAddr_t * host, int port);
    ~WebSocket();

    int sendText(char * dataPacket);
    int readText(char * outputBuffer, int bufferSize);

    int checkSocketStatus(int operation_check);

private:
    void upgradeConnection();
    int send(int sendBufferSize);

    INT16 m_sockHandle;
    char m_dataBuffer[BUFF_SIZE];
    char m_hostAddr[20];
    int m_hostPort;
};

#endif // __HOUND_WEBSOCKET_H
