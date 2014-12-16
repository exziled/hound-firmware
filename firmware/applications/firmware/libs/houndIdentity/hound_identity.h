/*!
 * @file hound_identity.h
 * 
 * @brief HOUND Identification Library
 * 
 * @author Benjamin Carlson
 * @author Blake Bourque
 * 
 * @date November 20, 2014
 * 
 * Functional defines for HOUND Identity class, allowing for identification between
 * HOUND Node devices.
 */

#ifndef __HOUND_IDENTITY_H
#define __HOUND_IDENTITY_H

#include "com_proto.h"

class HoundIdentity
{
public:
	HoundIdentity();
	~HoundIdentity();

	/*!
	* @brief Retrieve HOUND Identity
	* 
	* Retrieve HOUND Identity from some specified location.  Generic so this can be
	* changed in the future.
	*
	* @returns	 Nothing 
	*/
	void retrieveIdentity(void);

	/*!
	* @brief Broadcast via Multicast
	* 
	* Let servers known about this Node
	*
	* @param [in] 	ptr		serverLocation 		Multicast address to broadcast to
	* @param [in]	ptr 	broadcastBuffer		Data to send
	* @param [in]	int 	broadcastBufferSize	Amount of data to send
	*
	* @returns	 Nothing 
	*/
	void broadcast(Communication::ipAddr_t * serverLocation, char * broadcastBuffer, uint16_t broadcastBufferSize);
	const char * get(void);

private:
	char hound_id[25];
};

#endif //__HOUND_IDENTITY_H