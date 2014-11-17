#ifndef __HOUND_COMCMD_H
#define __HOUND_COMCMD_H

#include <stdint.h>

#include "stm32f10x_gpio.h"
#include "hound_rms_fixed.h"
#include "hound_identity.h"
#include "socket_control.h"

namespace Communication
{
	typedef struct
	{
		uint8_t rNode;
		uint8_t rParam;	
	} hRequest_t;

	enum reqParameters {REQ_V = 0x8, REQ_I = 0x4, REQ_A = 0x2};

	// Functions
	int parseCommand(uint8_t * arrCommands, int count, char * strResponse, int responseBuffSize, uint8_t reference);
	int parseRequest(hRequest_t * arrRequest, int count, char * strResponse, int responseBuffSize, AggregatedRMS * rmsAggregation, HoundIdentity * identity);

	int strcat(char * strDest, uint16_t buffSize, char * strSrc);
	int strcat(char * strDest, uint16_t buffSize, fixed_t concat, char delim = '\0');
	int strcat(char * strDest, uint16_t buffSize, uint32_t concat, char delim = '\0');
	int integerPower(int base, int exp);
	int integerPlaces(int n);
}

#endif // __HOUND_COMCMD_H