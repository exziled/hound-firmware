/*!
 * @file com_command.pp
 * 
 * @brief HOUND Communication Parsing Libraries
 * 
 * @author Benjamin Carlson
 * @author Blake Bourque
 * 
 * @date October 1, 2014
 * 
 *
 * Functional implementation for communication request/command parsing and response generation.
 * 
 */

#include "com_command.h"

// Standard Libraries
#include <cstddef>
#include <string.h>
#include <stdio.h>
#include <math.h>
// HOUND Libraries
#include "hound_debug.h"
#include "com_strings.h"
#include "socket_control.h"
#include "hound_rms_fixed.h"
// ST Libraries


// G++ doesn't like constant strings
#pragma GCC diagnostic ignored "-Wwrite-strings"

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

int Communication::parseRequest(hRequest_t * arrRequest, int count, char * strResponse, int responseBuffSize, HoundIdentity * identity)
{
	int i=0, j=0, replySize = 0;
	Communication::hRequest_t * request = NULL;

	for (int j = 0; j < count; j++)
	{
		request = &(arrRequest[j]);

		int socket = (request->rNode >> 4) & 0x0F;
		AggregatedRMS * rmsAggregation = socketGetStruct(socket)->rmsResults;

		// Socket not initialized correctly
		if (rmsAggregation == NULL)
		{
			return -1;
		}

		// Determine if user requested all data (aggregation) or the single latest data point
		int dataCount = request->rNode & 0x0F ? rmsAggregation->getSize() : 1;

		if (dataCount == 1)
		{
			// Packet header, begins JSON formatted string and provides node id
			replySize += snprintf(strResponse, responseBuffSize, "{\"e\":\"ws\",\"core_id\":\"%s\",\"s_id\":\"%d\",", identity->get(), socket);
		} else 
		{
			replySize += snprintf(strResponse, responseBuffSize, "{\"e\":\"samp\",\"core_id\":\"%s\",\"s_id\":\"%d\",", identity->get(), socket);
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

		if (request->rParam & Communication::REQ_A)
		{
			replySize += Communication::strcat(strResponse + replySize, responseBuffSize - replySize, (char *)"\"pf\":[\0");
			for (i = 0; i < dataCount; i++)
			{
				replySize += Communication::strcat(strResponse + replySize, responseBuffSize - replySize, rmsAggregation->getAt(i)->pf, i != (dataCount - 1) ? ',' : '\0');
			}
			replySize += Communication::strcat(strResponse + replySize, responseBuffSize - replySize, (char *)"],\0");
		}


		// Close JSON array
		replySize += snprintf(strResponse + replySize, responseBuffSize - replySize, "}");
	}

	return replySize;
}