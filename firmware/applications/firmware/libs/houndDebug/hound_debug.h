#ifndef __HOUND_DEBUG_H
#define __HOUND_DEBUG_H

#define LOG_LOCATION_LCD (1 << 0)
#define LOG_LOCATION_LED (1 << 1)

class HoundDebug
{
	public:
		static void logError(int location, int error_number, char * log_message);
		static void logError(int error_number, char * log_message);
		static void logMessage(int message_number, char * log_message);

		static void log(int location, int error_number, char * log_message);
};

#endif //__HOUND_DEBUG_H