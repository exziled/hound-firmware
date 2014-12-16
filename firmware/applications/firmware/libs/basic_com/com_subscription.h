/*!
 * @file com_subscription.h
 * 
 * @brief HOUND Communication Subscription Functions
 * 
 * @author Benjamin Carlson
 * @author Blake Bourque
 * 
 * @date December 5, 2014
 * 
 * Functional definition of subscription class managing subscribed clients and
 * requested data sources.
 * 
 */

#ifndef __HOUND_COMSUB_H
#define __HOUND_COMSUB_H

#define MAX_SOCKETS 2

// Hound Libraries
#include "com_proto.h"
#include "hound_identity.h"

class Subscription
{
public:
	Subscription(Communication::ipAddr_t * requestAddress, uint8_t * comBuff, uint16_t bufferSize, HoundIdentity * identity);
	~Subscription();

	/*!
	 * @brief Ensure class initialization succeeded
	 * 
	 * Ensures any data allocated during class instantiation succeeded.
	 *
	 * @returns	 ture if valid, false otherwise
	 */
	bool isValid(void);

	/*!
	 * @brief Add socket to subscription
	 * 
	 * @param [in]		int 	rNode				Socket node (as defined in com_command.h) to add
	 * @param [in] 		int 	rParam				Socket paramters (as defined in com_command.h) to add
	 *
	 * @returns	 0 on success, less than 0 otherwise
	 */
	int addSocket(uint8_t rNode, uint8_t rParam);

	/*!
	 * @brief Remove socket from subscription
	 * 
	 * @param [in]		int 	socket_id			Socket node (as defined in com_command.h) to add
	 *
	 * @returns	 0 on success, -1 otherwise
	 */
	int removeSocket(uint8_t socket_id);

	/*!
	 * @brief Send all subscribed data to client
	 * 
	 *
	 * @returns	Nothing
	 */
	void sendSubscription(void);
private:
	uint8_t socket_count;
	uint16_t * subscription_map;
	uint8_t * mComBuff;
	uint16_t mComBuffSize;
	HoundIdentity * mIdentity;
	Communication::ipAddr_t subscription_address;
};

#endif //__HOUND_COMSUB_H