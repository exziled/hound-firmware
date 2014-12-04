/*!
 * @file com_strings.h
 * 
 * @brief HOUND Communication String Generation Defintions
 * 
 * @author Benjamin Carlson
 * @author Blake Bourque
 * 
 * @date October 15, 2014
 * 
 * Functional defintions of lightweight string generation libraries, commonly used with CC3000
 * communication functions defined in @com_command.h
 * 
 */

#ifndef __HOUND_COMSTRINGS_H
#define __HOUND_COMSTRINGS_H

namespace Communcation
{
	/*!
	 * @brief Append one string to the end of another
	 * 
	 * @param [in/out]	ptr 	strDest				Pointer to memory where data output should be written
	 * @param [in] 		int 	buffSize			Maxmium size of output string
	 * @param [in/out]	ptr 	strSrc				Input string to append
	 *
	 * @returns	 Size of generated response buffer 
	 */
	int strcat(char * strDest, uint16_t buffSize, char * strSrc);

	/*!
	 * @brief Append fixed_t format fixed point number to the end of a string
	 * 
	 * @param [in/out]	ptr 	strDest				Pointer to memory where data output should be written
	 * @param [in] 		int 	buffSize			Maxmium size of output string
	 * @param [in/out]	int 	concat				Number in fixed_t format to append
	 * @param [in] 		char 	delim				Delmiter to add to the end of every string, null
	 * 													character default results in no value delimiter
	 *
	 * @returns	 Size of generated response buffer 
	 */
	int strcat(char * strDest, uint16_t buffSize, fixed_t concat, char delim = '\0');

	/*!
	 * @brief Append int32_t format integer to the end of a string
	 * 
	 * @param [in/out]	ptr 	strDest				Pointer to memory where data output should be written
	 * @param [in] 		int 	buffSize			Maxmium size of output string
	 * @param [in/out]	int 	concat				Unsigned integer in int32_t format to append
	 * @param [in] 		char 	delim				Delmiter to add to the end of every string, null
	 * 													character default results in no value delimiter
	 *
	 * @returns	 Size of generated response buffer 
	 */
	int strcat(char * strDest, uint16_t buffSize, uint32_t concat, char delim = '\0');

	/*!
	 * @brief Determine the power of an intege
	 * 
	 * @param [in/out]	ptr 	strDest				Pointer to memory where data output should be written
	 * @param [in] 		int 	buffSize			Maxmium size of output string
	 * @param [in/out]	int 	concat				Unsigned integer in int32_t format to append
	 * @param [in] 		char 	delim				Delmiter to add to the end of every string, null
	 * 													character default results in no value delimiter
	 *
	 * @returns	 Size of generated response buffer 
	 */
	int integerPower(int base, int exp);
	int integerPlaces(int n);
}