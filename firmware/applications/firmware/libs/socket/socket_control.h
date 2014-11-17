#ifndef __HOUND_SOCKET_CONTROL
#define __HOUND_SOCKET_CONTROL

#include "socket_map.h"

void initializeSocket(uint16_t socket);
void socketSetState(uint16_t socket, bool socketState);
void socketToggleState(uint16_t socket);
uint8_t socketGetState(uint16_t socket);

#endif //__HOUND_SOCKET_CONTROL
