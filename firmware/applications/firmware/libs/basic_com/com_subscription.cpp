#include "com_subscription.h"
#include "hound_debug.h"

#include <stdint.h>

#include <cstdlib>

#include "com_command.h"


Subscription::Subscription(Communication::ipAddr_t * requestAddress, uint8_t * comBuff, uint16_t bufferSize, HoundIdentity * identity)
{
	socket_count = 0; 
	subscription_map = (uint16_t *)malloc(sizeof(uint16_t) * MAX_SOCKETS);

	mComBuff = comBuff;
	mIdentity = identity;
	mComBuffSize = bufferSize;

	subscription_address.oct[0] = requestAddress->oct[0];
	subscription_address.oct[1] = requestAddress->oct[1];
	subscription_address.oct[2] = requestAddress->oct[2];
	subscription_address.oct[3] = requestAddress->oct[3];
}

Subscription::~Subscription()
{
	free(subscription_map);

	subscription_map = NULL;

	return;
}

bool Subscription::isValid(void)
{
	if (subscription_map == NULL)
	{
		return false;
	}

	return true;
}

int Subscription::addSocket(uint8_t rNode, uint8_t rParam)
{
	for(int i = 0; i < socket_count; i++)
	{
		if (((subscription_map[i] >> 12) & 0x0F) == ((rNode >> 4) & 0x0F))
		{
			return -2;
		}
	}

	if (socket_count < MAX_SOCKETS);
	{
		subscription_map[socket_count++] = ((uint16_t)rNode) << 8 | rParam;
		return 0;
	}

	return -1;
}

// Super inefficient sequential search.  I don't see us ever having more than 2-4 sockets mapped to a single spark core
// so speed shouldn't be a huge concern here
int Subscription::removeSocket(uint8_t socket_id)
{
	for(int i = 0; i < socket_count; i++)
	{
		// check to see if socket is mapped into subscription
		if (subscription_map[i] >> 12 == socket_id)
		{
			socket_count--;	// We found it, decrement socket count

			for (int j = i; j < socket_count; j++)
			{
				// Move array over
				subscription_map[j] = subscription_map[j+1];
			}

			return 0;
		}
	}

	return -1;
}

void Subscription::sendSubscription(void)
{
	int buffSendSize = 0;
	Communication::hRequest_t subscriptionRequest;

	HoundDebug::logMessage(0, "Sending Sub");

	for (int i = 0; i < socket_count; i++)
	{	
		subscriptionRequest.rNode = (subscription_map[i] >> 8) & 0xFF;
		subscriptionRequest.rParam = (subscription_map[i]) & 0xFF;

		buffSendSize = Communication::parseRequest(&subscriptionRequest, 1, (char *)mComBuff, mComBuffSize, mIdentity);
		Communication::HoundProto::sendData(mComBuff, buffSendSize, &subscription_address);
	}
}