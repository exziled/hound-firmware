#ifndef __HOUND_COMSUB_H
#define __HOUND_COMSUB_H

#define MAX_SOCKETS 2

#include "com_proto.h"
#include "hound_identity.h"

class Subscription
{
public:
	Subscription(Communication::ipAddr_t * requestAddress, uint8_t * comBuff, uint16_t bufferSize, HoundIdentity * identity);
	~Subscription();

	// 0, added sucessfully
	// -1, no more room
	int addSocket(uint8_t rNode, uint8_t rParam);
	// 0, socket removed
	// -1, socket not found
	int removeSocket(uint8_t socket_id);

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