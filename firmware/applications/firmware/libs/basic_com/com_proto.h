/*!
 * @file com_proto.h
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

#ifndef __HOUND_COM_H
#define __HOUND_COM_H

// Standard Libraries
#include <stdint.h>
// TI Libraries
#include "data_types.h"	// type defines for socket.h/c
#include "socket.h"		// CC3000 std driver library

namespace Communication
{

	typedef struct
	{
		uint8_t oct[4];
	} ipAddr_t;


	class HoundProto
	{
	public:
		/*!
		 * @brief HoundProto Ctor
		 * 
		 * @param [in]			int 	listenPort			Port on which getData will isten
		 *
		 * @returns	 0 on success, less than 0 otherwise
		 */
		HoundProto(int listenPort);
		~HoundProto();

		/*!
		 * @brief Get data from HOUND Node
		 * 
		 * @param [in|out]		ptr 	recvBuffer			Buffer to fill with recieved data
		 * @param [in]	 		int 	buffSize			Maximum size of recieve buffer
		 * @param [in|out]		ptr 	ipAddr 				IP address struct to fill with ip of recieved data
		 *
		 * @returns	 0 on success, less than 0 otherwise
		 */
		int getData(uint8_t * recvBuffer, int buffSize, ipAddr_t * ipAddr);

		/*!
		 * @brief Send data to HOUND Server
		 * 
		 * @param [in|out]		ptr 	sendBuffer			Buffer to send to server
		 * @param [in]	 		int 	buffSize			Maximum size of recieve buffer
		 * @param [in|out]		ptr 	ipAddr 				IP Address of remote server
		 *
		 * @returns	 0 on success, less than 0 otherwise
		 */
		static int sendData(uint8_t * sendBuffer, int bufferSize, ipAddr_t * ipAddr);

	private:
		int m_sockHandle;
		sockaddr * m_sockAddress;
		socklen_t m_sockAddressSize;
	};

}


#endif // __HOUND_COM_H