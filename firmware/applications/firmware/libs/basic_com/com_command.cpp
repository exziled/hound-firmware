#include "com_command.h"

#include <cstddef>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "hound_debug.h"

#include "socket_control.h"

// G++ doesn't like constant strings
#pragma GCC diagnostic ignored "-Wwrite-strings"


// We should make this return something someday
// Assumes ports commands are executed on have already been initialized (which isn't unfair)
// Should we define an array of "valid" ports so you can't arbitarily command things here
// All commands transmitted in format:
// 16 bits
// 16     12     8          1
// | ---- | ---- | -------- |
//   port   pin    operation 

// 8 bits
// 8      4      1
// | ---- | ---- | 
//  socket   op
// EX:
// Data 0xA401
// Performs Operation (0x01) on Port A, Pin 4
int Communication::parseCommand(uint8_t * arrCommands, int count, char * strResponse, int responseBuffSize, uint8_t reference)
{
	int replySize = 0;
	uint8_t sock, op, command; 	// Conform to stm32f std lib

	replySize += snprintf(strResponse, responseBuffSize, "{\"e\":%d ,\"result\": [", reference);

	for (int i = 0; i < count; i++)
	{
		command = arrCommands[i];

		// Parse values out of binary stream
		sock = (command >> 4) & 0x0F;
		op = command & 0x0F;
		
		#ifdef DEBUG_ON
		HoundDebug::logMessage(op, "Command");
		#endif

		if (op == 0)
		{
			socketSetState(sock, 0);

			replySize += snprintf(strResponse + replySize, responseBuffSize - replySize, "{\"socket\": \"%X\", \"state\": 0 }," , sock);
		} else if (op == 1) {
			socketSetState(sock, 1);

			replySize += snprintf(strResponse + replySize, responseBuffSize - replySize, "{\"socket\": \"%X\", \"state\": 1 }," , sock);
		} else if (op == 2) {

			if (socketGetState(sock))
			{
				replySize += snprintf(strResponse + replySize, responseBuffSize - replySize, "{\"socket\": \"%X\", \"state\": 1 }," , sock);
			} else  {
				replySize += snprintf(strResponse + replySize, responseBuffSize - replySize, "{\"socket\": \"%X\", \"state\": 0 }," , sock);
			}
		}
	}

	replySize += snprintf(strResponse + replySize, responseBuffSize - replySize, "]}");

	return replySize;
}

// Maybe in the future, we can support data across multiple sockets
// thus, we have an array of requests as opposed to a single request

// 16        12        8          1
// | ------- | ------- | -------- |
//   sock_id   count*    paramters
// Socket ID - Future Use - When multiple sample sockets are tied to a single core
// sample count - Supports All 0xF or Single 0x00
// Parameters -  
// all these snprintfs probably aren't that efficient
int Communication::parseRequest(hRequest_t * arrRequest, int count, char * strResponse, int responseBuffSize, AggregatedRMS * rmsAggregation, HoundIdentity * identity)
{
	int i=0, j=0, replySize = 0;
	Communication::hRequest_t * request = NULL;

	for (int j = 0; j < count; j++)
	{
		request = &(arrRequest[j]);

		// Determine if user requested all data (aggregation) or the single latest data point
		int dataCount = request->rNode & 0x0F ? rmsAggregation->getSize() : 1;

		if (dataCount == 1)
		{
			// Packet header, begins JSON formatted string and provides node id
			replySize += snprintf(strResponse, responseBuffSize, "{\"e\":\"ws\",\"core_id\":\"%s\",\"s_id\":\"%d\",", identity->get(), (request->rNode & 0xF0) >> 4);
		} else 
		{
			replySize += snprintf(strResponse, responseBuffSize, "{\"e\":\"samp\",\"core_id\":\"%s\",\"s_id\":\"%d\",", identity->get(), (request->rNode & 0xF0) >> 4);
		}

		// TIMESTAMP - Required
		replySize += Communication::strcat(strResponse + replySize, responseBuffSize - replySize, (char *)"\"t\":[\0");

		for (i = 0; i < dataCount; i++)
		{
			replySize += Communication::strcat(strResponse + replySize, responseBuffSize - replySize, rmsAggregation->getAt(i)->timestamp, i != (dataCount - 1) ? ',' : '\0');
		}

		replySize += Communication::strcat(strResponse + replySize, responseBuffSize - replySize, (char *)"],\0");


		// VOLTAGE - Optional
		if (request->rParam & Communication::REQ_V)
		{
			replySize += Communication::strcat(strResponse + replySize, responseBuffSize - replySize, (char *)"\"vrms\":[\0");
			for (i = 0; i < dataCount; i++)
			{
				replySize += Communication::strcat(strResponse + replySize, responseBuffSize - replySize, rmsAggregation->getAt(i)->voltage, i != (dataCount - 1) ? ',' : '\0');
			}
			replySize += Communication::strcat(strResponse + replySize, responseBuffSize - replySize, (char *)"],\0");
		}

		// CURRENT - Optional
		if (request->rParam & Communication::REQ_I)
		{
			replySize += Communication::strcat(strResponse + replySize, responseBuffSize - replySize, (char *)"\"irms\":[\0");
			for (i = 0; i < dataCount; i++)
			{
				replySize += Communication::strcat(strResponse + replySize, responseBuffSize - replySize, rmsAggregation->getAt(i)->current, i != (dataCount - 1) ? ',' : '\0');
			}
			replySize += Communication::strcat(strResponse + replySize, responseBuffSize - replySize, (char *)"],\0");
		}

		// APPARENT POWER - Optional
		if (request->rParam & Communication::REQ_A)
		{
			replySize += Communication::strcat(strResponse + replySize, responseBuffSize - replySize, (char *)"\"app\":[\0");
			for (i = 0; i < dataCount; i++)
			{
				replySize += Communication::strcat(strResponse + replySize, responseBuffSize - replySize, rmsAggregation->getAt(i)->apparent, i != (dataCount - 1) ? ',' : '\0');
			}
			replySize += Communication::strcat(strResponse + replySize, responseBuffSize - replySize, (char *)"],\0");
		}

		if (request->rParam & Communication::REQ_A)
		{
			replySize += Communication::strcat(strResponse + replySize, responseBuffSize - replySize, (char *)"\"real\":[\0");
			for (i = 0; i < dataCount; i++)
			{
				replySize += Communication::strcat(strResponse + replySize, responseBuffSize - replySize, rmsAggregation->getAt(i)->real, i != (dataCount - 1) ? ',' : '\0');
			}
			replySize += Communication::strcat(strResponse + replySize, responseBuffSize - replySize, (char *)"],\0");
		}

		// Close JSON array
		replySize += snprintf(strResponse + replySize, responseBuffSize - replySize, "}");
	}

	return replySize;
}

int Communication::strcat(char * strDest, uint16_t buffSize, uint32_t concat, char delim)
{
	return snprintf(strDest, buffSize, "%ld%c", concat, delim);
}

int Communication::strcat(char * strDest, uint16_t buffSize, fixed_t concat, char delim)
{
	int temp, powers_10;

	temp = fixed_integer(concat);
	powers_10 = integerPlaces(temp);

    int i, j, factor;

    for (i = 0; i < powers_10; i++)
    {
        factor = integerPower(10, (powers_10 - i -1));
        strDest[i] = 48 + (temp / factor);
        temp -= factor * (temp / factor);
    }

    strDest[i++] = '.';

    temp = fixed_fractional(concat);
    //powers_10 = integerPlaces(temp);
    // Only care about 3 decimal places
    powers_10 = 3;

    for (j = 0; j < powers_10; j++)
    {
    	// We always expect 3 decimal places, deal 
    	if (j == 0 && powers_10 < 3)
    	{
			strDest[i+j] = 48;
			i++;
    	}

    	factor = integerPower(10, (powers_10 - j -1));
    	strDest[i + j] = 48 + (temp / factor);
    	temp -= factor * (temp / factor);
    }

    if (delim != '\0')
    {
   		strDest[i + j++] = delim;
    }

	return i + j;
}

int Communication::strcat(char * strDest, uint16_t buffSize, char * strSrc)
{
	int srcSize = strlen(strSrc);

	//if (srcSize < buffSize)
	//{
		//::strcat(strDest-1, strSrc);

		return snprintf(strDest, buffSize, strSrc);
	//}

	//return 0;
}

int Communication::integerPower(int base, int exp)
{
    int result = 1;
    while (exp)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        base *= base;
    }

    return result;
}

int Communication::integerPlaces (int n) 
{
    if (n < 0) return 0;
    if (n < 10) return 1;
    if (n < 100) return 2;
    if (n < 1000) return 3;
    if (n < 10000) return 4;
    if (n < 100000) return 5;
    if (n < 1000000) return 6;
    if (n < 10000000) return 7;
    if (n < 100000000) return 8;
    if (n < 1000000000) return 9;
    return 10;
}