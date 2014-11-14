#include "hound_identity.h"
#include <string.h>
#include <stdio.h>

#include "com_proto.h"


HoundIdentity::HoundIdentity(void)
{
	//strcpy(hound_id, "53ff6d065067544847310187");
	strcpy(hound_id, "48ff6c065067555026311387");
}

HoundIdentity::~HoundIdentity(void)
{


}

// Retrieve From EEPROM
void HoundIdentity::retrieveIdentity(void)
{


}

void HoundIdentity::broadcast(Communication::ipAddr_t * serverLocation, char * broadcastBuffer, uint16_t broadcastBufferSize)
{
	int ret;
	
	ret = snprintf(broadcastBuffer, broadcastBufferSize, "{\"e\":\"broadcast\", \"id\":\"%s\"}", hound_id);

	Communication::HoundProto::sendData((uint8_t *)broadcastBuffer, ret, serverLocation);
}

const char * HoundIdentity::get(void)
{
	return hound_id;
}