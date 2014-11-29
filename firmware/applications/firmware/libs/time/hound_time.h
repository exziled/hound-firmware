/*!
 * @file hound_time.h
 * 
 * @brief HOUND NTP Sync Functions
 * 
 * @author Benjamin Carlson
 * @author Blake Bourque
 * 
 * @date November 13, 2014
 * 
 * NTP Time sync library to manage initial request, return values, and updating
 * real time clock with updated time data.
 * 
 */

#ifndef __HOUND_NTP_H
#define __HOUND_NTP_H

// Standard Libraries
#include <stdint.h>
// HOUND Libraries
// ST Libraries

/*!
 * @brief Generate NTP Request
 * 
 * Fill a communcation buffer with data required to execute an NTP request
 * 
 * @param[in|out] 	ptr		Pointer to a communcation buffer
 * @param[in]		int 	Size of communication buffer
 * 
 * @returns	 		None
 */
int generateNTPRequest(uint8_t * sendBuffer, int sendBufferSize);

/*!
 * @brief Send NTP Request/Recieve Response
 * 
 * Given a communication buffer, make a request to a remote NTP Server and
 * wait for the response data
 * 
 * @param[in|out] 	ptr		Pointer to a communcation buffer
 * @param[in]		int 	Size of communication buffer
 * 
 * @returns	 		None
 */
int sendNTPRequest(uint8_t * sendBuffer, int sendBufferSize);

/*!
 * @brief Parse Recieved NTP Data
 * 
 * Given a communication buffer with an NTP response, parse data and extract
 * current time string.  Finally, update RTC with timestamp.
 * 
 * @param[in|out] 	ptr		Pointer to a communcation buffer
 * @param[in]		int 	Size of communication buffer
 * 
 * @returns	 		None
 */
void parseNTPResponse(uint8_t * recieveBuffer, int recieveBufferSize);

#endif //__HOUND_NTP_H