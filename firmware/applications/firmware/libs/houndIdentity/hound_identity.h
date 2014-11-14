#ifndef __HOUND_IDENTITY_H
#define __HOUND_IDENTITY_H

#include "com_proto.h"

class HoundIdentity
{
public:
	HoundIdentity();
	~HoundIdentity();

	void retrieveIdentity(void);
	void broadcast(Communication::ipAddr_t * serverLocation, char * broadcastBuffer, uint16_t broadcastBufferSize);
	const char * get(void);

private:
	char hound_id[25];
};

#endif //__HOUND_IDENTITY_H