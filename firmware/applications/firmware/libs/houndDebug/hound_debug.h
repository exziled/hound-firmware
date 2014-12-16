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
	   /*!
		* @brief Log an Error
		* 
		* Alert the user that an error occured
		*
		* @param [in] 	int		location 	 		Specify the location (defines above) to log the error to
		* @param [in]	int 	error_number		Some arbitrary identification number
		* @param [in]	ptr 	log_message			Small message to describe error
		*
		* @returns	 Nothing 
		*/
		static void logError(int location, int error_number, char * log_message);

	   /*!
		* @brief Log an Error
		* 
		* Alert the user that an error occured, output at the DEFAULT_LOG_LOCATION
		*
		* @param [in]	int 	error_number		Some arbitrary identification number
		* @param [in]	ptr 	log_message			Small message to describe error
		*
		* @returns	 Nothing 
		*/
		static void logError(int error_number, char * log_message);

	   /*!
		* @brief Log a Message
		* 
		* Alert the user of something, output at the DEFAULT_LOG_LOCATION
		*
		* @param [in]	int 	message_number		Some arbitrary identification number
		* @param [in]	ptr 	log_message			Small message to describe error
		*
		* @returns	 Nothing 
		*/
		static void logMessage(int message_number, char * log_message);

	   /*!
		* @brief Log Anything
		* 
		* Alert the user that something happened
		*
		* @param [in] 	int		location 	 		Specify the location (defines above) to log the error to
		* @param [in]	int 	error_number		Some arbitrary identification number
		* @param [in]	ptr 	log_message			Small message to describe error
		*
		* @returns	 Nothing 
		*/
		static void log(int location, int error_number, char * log_message);
};

#endif //__HOUND_DEBUG_H