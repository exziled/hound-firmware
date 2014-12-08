/*!
 * @file socket_control.h
 * 
 * @brief HOUND Socket Control Abstraction
 * 
 * @author Benjamin Carlson
 * @author Blake Bourque
 * 
 * @date November 20, 2014
 * 
 * Function abstractions for socket control and data requests
 * 
 */

#ifndef __HOUND_SOCKET_CONTROL
#define __HOUND_SOCKET_CONTROL

// Standard Libraries
// HOUND Libraries
#include "socket_map.h"
// ST Libraries

/*!
 * @brief Initialize all hardware related to a socket
 * 
 * Initializes chip select pins for SPI communcation, control pins, and
 * buffers of data storage.
 * 
 * @param[in] int   Socket index, from socketMap, of socket to initialize
 * 
 * @returns	 		None
 */
void initializeSocket(uint16_t socket);

/*!
 * @brief Turn a socket on or off
 * 
 * Given an initiaized socket index, turn the socket on or off.
 * 
 * @param[in] int   Socket index, from socketMap, of socket to control
 * @param[in] bool 	Socket state, 1 is On, 0 is Off
 * 
 * @returns	 		None
 */
void socketSetState(uint16_t socket, bool socketState);

/*!
 * @brief Toggle a socket 
 * 
 * Given an initialized socket, toggle the current output state
 * 
 * @param[in] int   Socket index, from socketMap, of socket to control
 * 
 * @returns	 		None
 */
void socketToggleState(uint16_t socket);

/*!
 * @brief Determine current state of a socket
 * 
 * Given an initilized socket, read the current output state of control
 * 
 * @param[in] int   Socket index, from socketMap, of socket to read
 * 
 * @returns	 		1 for socket active, 0 for inactive.
 */
uint8_t socketGetState(uint16_t socket);

/*!
 * @brief Retrun number of sockets currently mapped
 * 
 * @returns	 		Number of sockets stored in socket map
 */
uint8_t socketGetCount(void);

/*!
 * @brief Return socketMap struct from array
 * 
 * @param[in] int   Socket index, from socketMap, of socket to modify
 * 
 * @returns	 		Volatile pointer to socket struct
 */
volatile socketMap_t * socketGetStruct(uint16_t socket);

#endif //__HOUND_SOCKET_CONTROL
