/*!
 * @file hound_debug.h
 * 
 * @brief HOUND Debug Library
 * 
 * @author Benjamin Carlson
 * @author Blake Bourque
 * 
 * @date November 20, 2014
 * 
 * Functional defines for HOUND Debug class.  Allows for remote and local debugging
 * using HD44780 library, remote socket library, and LED.
 */

#ifndef __HOUND_DEBUG_H
#define __HOUND_DEBUG_H

#define LOG_LOCATION_LCD (1 << 0)
#define LOG_LOCATION_LED (1 << 1)
#define LOG_LOCATION_NET (1 << 2)
#define LOG_NET_PORT 8088

#define DEBUG_BUFF_SIZE 32

#define DEFAULT_LOG_LOCATION LOG_LOCATION_NET
#define DISABLE_LCD_LOG
// #define DISABLE_LED_LOG
// #define DISABLE_NET_LOG

class HoundDebug
{
	public:
		static void logError(int location, int error_number, char * log_message);
		static void logError(int error_number, char * log_message);
		static void logMessage(int message_number, char * log_message);

		static void log(int location, int error_number, char * log_message);
};

#endif //__HOUND_DEBUG_H