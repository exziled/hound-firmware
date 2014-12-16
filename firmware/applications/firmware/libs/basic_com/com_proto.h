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
		HoundProto(int listenPort);
		~HoundProto();

		int getData(uint8_t * recvBuffer, int buffSize, ipAddr_t * ipAddr);
		static int sendData(uint8_t * sendBuffer, int bufferSize, ipAddr_t * ipAddr);

	private:
		int m_sockHandle;
		sockaddr * m_sockAddress;
		socklen_t m_sockAddressSize;
	};

}


#endif // __HOUND_COM_H