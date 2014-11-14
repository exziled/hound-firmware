#include "hound_debug.h"

#include "hd44780.h"

// G++ doesn't like constant strings
#pragma GCC diagnostic ignored "-Wwrite-strings"

void HoundDebug::logError(int location, int error_number, char * log_message)
{
	HoundDebug::log(location, error_number, log_message);
}

void HoundDebug::logError(int error_number, char * log_message)
{
	HoundDebug::log(LOG_LOCATION_LCD, error_number, log_message);
}

void HoundDebug::logMessage(int message_number, char * log_message)
{
	HoundDebug::log(LOG_LOCATION_LCD, message_number, log_message);
}

void HoundDebug::log(int location, int error_number, char * log_message)
{
	if (location & LOG_LOCATION_LCD)
	{
		HD44780 * lcd = HD44780::getInstance();
		lcd->clear();
		lcd->printf("Error: %d", error_number);
		lcd->setPosition(1, 0);
		lcd->printf("%s", log_message);
	}

	if (location & LOG_LOCATION_LED)
	{

	}
}