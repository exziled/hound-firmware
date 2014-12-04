/*!
 * @file com_command.h
 * 
 * @brief HOUND Communication Parsing Libraries
 * 
 * @author Benjamin Carlson
 * @author Blake Bourque
 * 
 * @date October 1, 2014
 * 
 *
 * Definitions for communication request/command parsing and response generation.
 * 
 */

#ifndef __HOUND_COMCMD_H
#define __HOUND_COMCMD_H

// Standard Libraries
#include <stdint.h>
// HOUND Libraries
#include "hound_identity.h"
#include "socket_control.h"
// ST Libraries
#include "stm32f10x_gpio.h"

namespace Communication
{
	typedef struct
	{
		uint8_t rNode;
		uint8_t rParam;	
	} hRequest_t;

	/*!
	 * @brief Enum defining paramters for response generation
	 *
	 * REQ_V - Return Voltage Data
	 * REQ_I - Return Current Data
	 * REQ_P - Return Power Data
	 *
	 */
	enum reqParameters {REQ_V = 0x8, REQ_I = 0x4, REQ_A = 0x2};

	/*!
	 * @brief Enum defining paramters for HOUND commands
	 *
	 * CMD_ON - Turn socket on
	 * CMD_OFF - Turn socket off
	 * CMD_DATA - Query socket state
	 * 
	 */
	enum cmdOperations {CMD_ON = 0x0, CMD_OFF = 0x01, CMD_DATA = 0x02}

	/*!
	 * @brief Parse incoming HOUND Command
	 * 
	 * General Structure
	 *  8 bits
	 *	8      4      1
	 *	| ---- | ---- | 
	 *	socket   op
	 * 
	 * Socket:
	 * References a socket defined in the global socket map, defined in 
	 * 		socket_map.h
	 *
	 * Op:
	 * References an operation to execute on the selected socket.  Possible options
	 * are defined in enum @cmdOperations
	 *
	 * @param [in]		ptr 	arrCommands			Array of input commands to execute
	 * @param [in] 		int 	count 				Count of commands pointed to by @arrCommands
	 * @param [in/out]	ptr 	strResponse			Pointer to block of memory to which output JSON string should be written
	 * @param [in] 		int 	responseBuffSize 	Maximum size of response string
	 * @param [in] 		int 	reference 			Command reference used to track input/output on server end, reference
	 *													is written back to server as first parameter of strResponse JSON string
	 *
	 * @returns	 Size of generated response buffer 
	 */
	int parseCommand(uint8_t * arrCommands, int count, char * strResponse, int responseBuffSize, uint8_t reference);

	/*!
	 * @brief Parse incoming hound request
	 * 
	 * General Structure
	 * 16        12        8          1
	 * | ------- | ------- | -------- |
	 *   socket    count    paramters
	 * 
	 * Socket:
	 * References a socket defined in the global socket map, defined in 
	 * 		socket_map.h
	 *
	 * Count:
	 * Number of data points to return.  Currently supports two options, 
	 * 		0x0 - One Sample
	 * 		0xF - Every Sample
	 *
	 * Parameters
	 * Data points to return.  Possible options defined in reqParameters enum.
	 *
	 * @param [in]		ptr 	arrRequest			Array of input commands to execute
	 * @param [in] 		int 	count 				Count of commands pointed to by @arrCommands
	 * @param [in/out]	ptr 	strResponse			Pointer to block of memory to which output JSON string should be written
	 * @param [in] 		int 	responseBuffSize 	Maximum size of response string
	 * @param [in] 		ptr 	identity 			All data requests must include a parameter for tracking individual cores
	 *
	 * @returns	 Size of generated response buffer 
	 */
	int parseRequest(hRequest_t * arrRequest, int count, char * strResponse, int responseBuffSize, HoundIdentity * identity);
}

#endif // __HOUND_COMCMD_H