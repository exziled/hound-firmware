#ifndef __HOUND_NTP_H
#define __HOUND_NTP_H

#include <stdint.h>

int generateNTPRequest(uint8_t * sendBuffer, int sendBufferSize);
int sendNTPRequest(uint8_t * sendBuffer, int sendBufferSize);
void parseNTPResponse(uint8_t * recieveBuffer, int recieveBufferSize);

#endif //__HOUND_NTP_H